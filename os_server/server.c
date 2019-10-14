//
// Created by Matteo on 12/06/2019.
//

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ftw.h>
#include "os_server.h"
#include "lib/threadlist.h"


volatile int running = 1;
int usr1rec = 0;
int numclients = 0;
int numobj= 0;
int totsize = 0;

void clean(){
	unlink(SOCKNAME);
}

void cleanup_handler(void* arg){
	free(arg);
}

void* sighandler_thread(){
	int sig;
	sigset_t set;
	sigemptyset(&set);

	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGUSR1);

	pthread_sigmask(SIG_SETMASK, &set, NULL);

	while(running) {
		sigwait(&set, &sig);
		switch(sig) {
			case SIGINT:
				fprintf(stderr,"SIGINT received\n");
				running = 0;
				break;
			case SIGTERM:
				running = 0;
				break;
			case SIGUSR1:
				fprintf(stderr,"SIGUSR1 received\n");
				usr1rec = 1;
				break;
		}
	}

	return NULL;
}

void siginit(){
	pthread_t tid;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	pthread_create(&tid, NULL, &sighandler_thread, NULL);
	pthread_detach(tid);
}

int size_calc(const char *fpath, const struct stat *sb, int typeflag){
	if(typeflag == FTW_F){
		numobj++;
		totsize += sb->st_size;
	}
	return 0;
}

void* worker_fun(void* sockarg) {
	int socket_client = *(int*)sockarg;
	int r;
	char buffer[BUFFLEN];
	char tokbuffer[BUFFLEN];
	char *token, *rest;
	char *usrpath = NULL;
	char *usrname = NULL;
	pthread_cleanup_push(cleanup_handler, usrname);
	pthread_cleanup_push(cleanup_handler, usrpath);
	pthread_cleanup_push(cleanup_handler, sockarg);


	BUFFER_CLEAR(buffer);

	while(running){
		BUFFER_CLEAR(buffer);
		r = read(socket_client, buffer, BUFFLEN);

		if(r <= 0){
			if(r == 0) break;
			if(r == -1){
				DBGprint("read on socket %d from client %s: %s\n",socket_client,usrname,strerror(errno));
				break;
			}
		}

		memcpy(tokbuffer, buffer, BUFFLEN);
		token = strtok_r(tokbuffer, " ", &rest);

		if(strcmp(token,"REGISTER") == 0){
			// REGISTER name \n
			// Estraggo campo name.
			token = strtok_r(NULL, " ", &rest);

			// Controllo eventuali / nel nome.
			if((strchr(token, '/') != NULL) || (token == NULL)){
				WRITEKO(socket_client);
			}else {
				usrname = strdup(token);
				usrpath = register_fun(token, socket_client);
				numclients++;
				// DEBUG
				DBGprint("REGISTER socket: %d   name: %s   usrpath: %s\n",socket_client,token, usrpath);
				//
			}

		}else if(strcmp(token,"STORE") == 0){
			// STORE name len \n data
			int len;
			int headlen = 10;

			// Estraggo campo name. Controllo eventuali / .
			token = strtok_r(NULL, " ", &rest);

			if((strchr(token, '/') != NULL) || (token == NULL)){
				WRITEKO(socket_client);
			}else{
				// Creo percorso.
				// DEBUG
				if(usrpath == NULL){
					DBGprint("usrpath of client %s is null on socket: %d\n",usrname, socket_client);
					break;
				}
				//
				char* path = calloc(strlen(usrpath) + strlen(token) +2, sizeof(char));
				sprintf(path,"%s/%s", usrpath, token);

				headlen += strlen(token);

				//Estraggo len.
				token = strtok_r(NULL, " ", &rest);
				len = atoi(token);
				headlen += strlen(token);

				int res;

				if((len != 0) && (len <= MAXLEN)){
					char block[len+1];
					memset(block, 0, len+1);
					// Se ho letto dati oltre all'header li copio in block.
					// Scorro fino a data.
					token = strtok_r(NULL, " ", &rest);
					if(rest != NULL){
						int lenrest = r-headlen;
						if(len > lenrest){
							memcpy(block, rest, lenrest);
							res = store_fun(socket_client, path, block, len, (len-lenrest));
						}else{
							memcpy(block, rest, len);
							res = store_fun(socket_client, path, block, len, 0);
						}
					}
				}else{
					WRITEKO(socket_client);
				}
				// DEBUG
				DBGprint("STORE socket: %d   name: %s   usrpath: %s   path: %s  len: %d   result: %d\n",socket_client,usrname, usrpath, path, len,res);
				//
				free(path);
			}


		}else if(strcmp(token,"RETRIEVE") == 0){
			// RETRIEVE name \n
			// Estraggo campo name.
			token = strtok_r(NULL, " ", &rest);

			// Controllo eventuali / nel nome.
			if((strchr(token, '/') != NULL) || (token == NULL)){
				WRITEKO(socket_client);
			}else {
				// Creo percorso.
				char* path = calloc(strlen(usrpath) + strlen(token) +2, sizeof(char));
				int res;
				sprintf(path,"%s/%s", usrpath, token);

				res = retrieve_fun(socket_client, path);
				// DEBUG
				DBGprint("RETRIEVE socket: %d   name: %s   usrpath: %s   path: %s   result: %d\n",socket_client,usrname, usrpath, path, res);
				//
				free(path);
			}

		}else if(strcmp(token,"DELETE") == 0){
			// DELETE name \n
			// Estraggo campo name.
			token = strtok_r(NULL, " ", &rest);

			// Controllo eventuali / nel nome.
			if((strchr(token, '/') != NULL) || (token == NULL)){
				WRITEKO(socket_client);
			}else {
				int res;
				// Creo percorso.
				char* path = calloc(strlen(usrpath) + strlen(token) +2, sizeof(char));
				sprintf(path,"%s/%s", usrpath, token);

				res = delete_fun(socket_client, path);
				DBGprint("DELETE socket: %d   name: %s   usrpath: %s   path: %s   result: %d\n",socket_client,usrname, usrpath, path, res);

				free(path);
			}

		}else if(strcmp(token,"LEAVE") == 0){
			disconnect_fun(socket_client);
			numclients--;
			pthread_exit(EXIT_SUCCESS);
		}else{

		}
	}
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	return NULL;
}


int main(void){
	siginit();
	unlink(SOCKNAME);
	atexit(clean);

	int fd_socket_s, fd_socket_cl;
	struct sockaddr_un sa;
	thread_list* threadlist = NULL;

	// Creazione address e socket, binding dei due, listen e controllo errori con macro
	sa = create_sa();
	fd_socket_s = create_socket(AF_UNIX, SOCK_STREAM, 0);
	PERR_M1_EXIT(bind(fd_socket_s, (struct sockaddr *)&sa, sizeof(sa)), "bind")
	PERR_M1_EXIT(listen(fd_socket_s, SOMAXCONN), "listen");

	// Inizializzazione select
	fd_set rfds, rdset;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(fd_socket_s, &rfds);

	mkdir(PATHPRE,0700);

	// while running aspetta su accept e lancia thread worker
	while(running){
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if(usr1rec){
			int res = ftw(PATHPRE, &size_calc, 64);
			if(res == -1){
				fprintf(stderr, "Errore nel recupero statistiche\n");
			} else {
				printf("Objectstore stats:\nClienti correntemente connessi: %d\tOggetti nello store: %d\tDimensione totale store: %d\n",
					   numclients, numobj, totsize);
				usr1rec = 0;
			}
		}

		rdset = rfds;
		if(select(fd_socket_s+1, &rdset, NULL, NULL, &tv) < 0){
			perror("select");
			running = 0;
		}

		if(FD_ISSET(fd_socket_s, &rdset)){
			if((fd_socket_cl = accept(fd_socket_s, NULL, 0)) == -1){
					perror("accept");
			} else {
				pthread_t tid;
				int* new_socket = calloc(1,sizeof(int));
				*new_socket = fd_socket_cl;

				if(pthread_create(&tid, NULL, &worker_fun, (void *) new_socket) != 0){
					perror("pthread_create");
				}
				addThread(&threadlist, tid);
				pthread_detach(tid);
			}
		}
	}

	while(threadlist != NULL){
		pthread_t ttj = removeThread(&threadlist);
		pthread_join(ttj, NULL);
	}

	return 0;
}