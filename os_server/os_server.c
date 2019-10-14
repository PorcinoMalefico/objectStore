//
// Created by Matteo on 13/06/2019.
//

#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "os_server.h"

int create_socket(){
	int fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	PERR_M1_EXIT(fd_socket, "socket creation");

	return fd_socket;
}

struct sockaddr_un create_sa(){
	struct sockaddr_un sa;
	snprintf(sa.sun_path, UNIX_PATH_MAX, "%s", SOCKNAME);
	sa.sun_family=AF_UNIX;

	return sa;
}

/*
 * Legge len byte da fd_socket e li appende a buffer.
 * Restituisce il numero di byte restanti (!= 0 equivale a errore).
 */

int readn(int fd_socket, int len, void* buffer){
	int r;
	char tmp[len+1];
	memset(tmp, 0, len+1);

	while(len>0){
		if((r = read(fd_socket, tmp, len)) == -1){
			EBADF_PEXIT_ELSE_RET(-1);
		} else{
			strncat(buffer, tmp, r);
			len -= r;
		}
	}

	return len;
}

/*
 * Crea la cartella dati dell'utente name se non esiste già. Restituisce il path della cartella.
 */
char* register_fun(char* name, int socket_cl){
	char* path = calloc(BUFFLEN, sizeof(char));
	DIR* dirptr;

	//BUFFER_CLEAR(path);
	sprintf(path, "%s/%s", PATHPRE, name);

	// Se non esiste la radice del path, crea la cartella corrispondente.
	if((dirptr = opendir(PATHPRE)) == NULL){
		if(errno == ENOENT)
			mkdir(PATHPRE, 0700);
	}

	// Crea cartella utente. Se ha successo o se la cartella esiste già risponde OK,
	// altrimenti risponde KO.
	if(mkdir(path, 0700) == -1){
		if(errno != EEXIST){
			WRITEKO(socket_cl);
			DBGprint("mkdir: %s\npath: %s\n",strerror(errno),path);
			closedir(dirptr);
			return NULL;
		} else{
			WRITEOK(socket_cl);
		}
	} else{
		WRITEOK(socket_cl);
	}

	closedir(dirptr);
	return path;
}


void disconnect_fun(int fd_socket){
	WRITEOK(fd_socket);
	if(shutdown(fd_socket, SHUT_RDWR) != 0){
		perror("shutdown");
	}
}

/*
 * Legge len bytes da fd_socket e li scrive in path. Se left è >0 leggo altri left byte aggiungendoli poi a block.
 * Restituisce 1 se la scrittura è andata a buon fine.
 */
int store_fun(int fd_socket, char* path, char* block, int len, int left){
	FILE *fp = NULL;
	int written;

	fp = fopen(path, "w+");

	if(fp == NULL){
		DBGprint("fopen: %s\npath: %s\n",strerror(errno),path);
		WRITEKO(fd_socket);
		return -1;
	}

	if(left > 0){
		char tmp[left+1];
		int r;

		memset(tmp, 0, left+1);

		r = read(fd_socket, tmp, left);
		if(r <= 0){
			DBGprint("read: %s\nsocket: %d  trying to read %d bytes", strerror(errno), fd_socket, left);
			fclose(fp);
			return -1;
		}
		strncat(block, tmp, left);
	}

	written = fwrite(block, len, 1, fp);
	if(written != 1){
		DBGprint("fwrite on path: %s : %s\n", strerror(errno),path);
		WRITEKO(fd_socket);
	} else{
		WRITEOK(fd_socket);
	}

	fclose(fp);
	return written;

}

/*
 * Apre il file in path e ne ottiene la dimensione. Trasferisce poi il contenuto su di un buffer e lo invia su fd_socket.
 * Restituisce 1 in caso di successo.
 */
int retrieve_fun(int fd_socket, char* path){
	char buffer[BUFFLEN];
	FILE *fp;
	int len, written;

	fp = fopen(path, "r");

	if(fp == NULL){
		DBGprint("fopen: %s\npath: %s\n",strerror(errno),path);
		WRITEKO(fd_socket);
		return -1;
	}

	// Misura dimensione file
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	// Scrittura blocco su buffer
	char block[len+1];
	memset(block, 0, len+1);
	if(fread(block, len, 1, fp) != 1){
		DBGprint("fread: %s\npath: %s",strerror(errno),path);
		WRITEKO(fd_socket);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	// Scrittura header+blocco su buffer
	BUFFER_CLEAR(buffer);
	written = sprintf(buffer, "DATA %d \n ", len);
	char *writebuffer = calloc(written + len+1, sizeof(char));
	if(writebuffer == NULL){
		DBGprint("calloc: %s\ntrying to calloc %d + %d + 1 byte (len+written+1)\n", strerror(errno),len, written);
		WRITEKO(fd_socket);
		return -1;
	}
	written = snprintf(writebuffer,written + len+1, "%s%s", buffer, block);

	// Se tutto ok invio buffer (header+data)
	if(written < 0){
		DBGprint("sprintf: %s\ntrying to write buffer and block on writebuffer", strerror(errno));
		WRITEKO(fd_socket);
		free(writebuffer);
		return -1;
	}
	if(write(fd_socket, writebuffer, written) == -1){
		DBGprint("write: %s\nsocket: %d\n", strerror(errno),fd_socket);
		WRITEKO(fd_socket);
		free(writebuffer);
		return -1;
	}
	free(writebuffer);
	return 1;
}

/*
 * Rimuove il file in path e invia l'esito dell'operazione su fd_socket.
 */
int delete_fun(int fd_socket, char* path){

	if(remove(path) == -1){
		WRITEKO(fd_socket);
		return -1;
	} else{
		WRITEOK(fd_socket);
		return 1;
	}
}