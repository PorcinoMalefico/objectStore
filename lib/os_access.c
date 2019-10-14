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

#define UNIX_PATH_MAX 108
#define SOCKNAME "objstore.sock"
#define PATHPRE "data"
#define LENOK 5
#define BUFFLEN 512
// Lunghezza buffer = massima lunghezza filename UNIX (255) + spazio per nomi operazioni ed eventuali altri parametri.
// Arrotondo a 512.

#define PERR_M1(err, wh) \
    if( (err) == -1) {perror(wh);}

#define PERR_M1_EXIT(err, wh) \
    if( (err) == -1) {perror(wh); exit(EXIT_FAILURE);}

#define PERR_M1_RET(err, wh, retval) \
	if( (err) == -1) {perror(wh); return retval;}

#define PERR_M1_DO(err, wh, action) \
	if( (err) == -1) {perror(wh); action}

#define EBADF_EXIT_ELSE_RET(retval)\
	if(errno == EBADF){exit(EXIT_FAILURE);} \
		else {return retval;}

#define BUFFER_CLEAR(buffer) \
	memset(buffer, 0, BUFFLEN);

#define WRITEOK(sckt) \
	write(sckt,"OK \n", 5);

#define WRITEKO(sckt) \
	write(sckt,"KO \n", 5);

static int fd_socket;

int create_socket(){
	int new_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	PERR_M1_EXIT(fd_socket, "socket creation");

	return new_socket;
}

struct sockaddr_un create_sa(){
	struct sockaddr_un sa;
	snprintf(sa.sun_path, UNIX_PATH_MAX, "%s", SOCKNAME);
	sa.sun_family=AF_UNIX;

	return sa;
}

/*
 * Legge len byte da socketin e li appende a buffer.
 * Restituisce il numero di byte restanti (!= 0 equivale a errore).
 */

int readn(int socketin, int len, void* buffer){
	int r;
	char tmp[len];
	memset(tmp, 0, len);

	while(len>0){
		if((r = read(socketin, tmp, len)) == -1){
			EBADF_EXIT_ELSE_RET(-1);
		} else{
			strncat(buffer, tmp, r);
			len -= r;
		}
	}

	return len;
}

/*
 *  Controlla se le prime 4 posizioni del buffer corrispondo a "OK \n".
 *  Restituisce 1 in caso di uguaglianza, altrimenti -1
 */
int is_ok(char* buffer){


	if(strncmp(buffer, "OK \n", LENOK) == 0){
		return 1;
	} else{

		return -1;
	}

}



/*
 * Inizia la connessione all'object store, registrando il cliente con il name dato.
 * Restituisce true se la connessione ha avuto successo, false altrimenti.
 * Notate che la connessione all'object store è globale per il client.
 */
int os_connect(char *name){
	struct sockaddr_un sa;
	char buffer[BUFFLEN];
	int written;

	sa = create_sa();
	fd_socket = create_socket();

	while(connect(fd_socket, (struct sockaddr*)&sa, sizeof(sa)) == -1){
		if(errno == ECONNREFUSED){
			sleep(1);
		} else{
			perror("connect");
			return -1;
		}
	}

	BUFFER_CLEAR(buffer);
	// Crea e invia messaggio di registrazione
	written = sprintf(buffer, "REGISTER %s \n", name);
	if(write(fd_socket, buffer, written) == -1){
		EBADF_EXIT_ELSE_RET(-1);
	}

	BUFFER_CLEAR(buffer);
	// Attende e legge risposta
	if(read(fd_socket, buffer, 512) == -1){
		EBADF_EXIT_ELSE_RET(-1);
	}

	return is_ok(buffer);
}

/*
 * Richiede all'object store la memorizzazione dell'oggetto puntato da block, per una lunghezza len, con il nome name.
 * Restituisce true se la memorizzazione ha avuto successo, false altrimenti.
 */
int os_store(char *name, void *block, size_t len){
	char buffer[BUFFLEN];
	int written;

	// Invio messaggio e dati.
	BUFFER_CLEAR(buffer);
	written = sprintf(buffer, "STORE %s %d \n ", name, (int)len);
	char *writebuffer = calloc(written + len+1, sizeof(char));
	if(writebuffer == NULL){
		return -1;
	}
	written = snprintf(writebuffer, written+len+1, "%s%s", buffer, (char*)block);

	// Se tutto ok invio buffer (header+data)
	if(written < 0){
		free(writebuffer);
		return -1;
	}
	if(write(fd_socket, writebuffer, written) == -1){
		free(writebuffer);
		return -1;
	}
	free(writebuffer);

	BUFFER_CLEAR(buffer);
	if(read(fd_socket, buffer, LENOK) == -1){
		EBADF_EXIT_ELSE_RET(-1);
	}

	return is_ok(buffer);
}

/*
 * Recupera dall'object store l'oggetto precedentemente memorizzatato sotto il nome name.
 * Se il recupero ha avuto successo, restituisce un puntatore a un blocco di memoria, allocato dalla funzione, contenente i dati precedentemente memorizzati.
 * In caso di errore, restituisce NULL.
 */
void *os_retrieve(char *name){
	char buffer[BUFFLEN];
	int written;

	// Invio comando.
	BUFFER_CLEAR(buffer);
	written = sprintf(buffer, "RETRIEVE %s \n", name);
	PERR_M1_RET(write(fd_socket, buffer, written), "os_retrieve", NULL);

	// Lettura risposta. Se primo token != da DATA esco, altrimenti estraggo la lunghezza.
	BUFFER_CLEAR(buffer);
	PERR_M1_RET(read(fd_socket, buffer, BUFFLEN), "os_retrieve", NULL);
	{
		char *token, *rest;
		rest = buffer;
		token = strtok_r(rest, " ", &rest);
		if(strcmp(token, "DATA") != 0){
			return NULL;
		} else{
			int len;
			int left = BUFFLEN-8;
			// DATA len \n data

			// Estraggo lunghezza
			token = strtok_r(NULL, " ", &rest);
			len = atoi(token);
			left -= strlen(token);

			// Scorro fino al blocco dati
			token = strtok_r(NULL, " ", &rest);

			// Copio eventuali dati già estratti dal socket in block.
			char* block = calloc(len+1, sizeof(char));

			snprintf(block, left+1, "%s", rest);
			left = len-left;

			// Leggo i byte rimasti da socket scrivendoli su block.
			if(left > 0){
				char tmp[left+1];
				int r;

				memset(tmp, 0, left+1);

				r = read(fd_socket, tmp, left);
				if(r <= 0){
					fprintf(stderr,"read in os_retrieve: %s", strerror(errno));
					return NULL;
				}
				strncat(block, tmp, left);
			}

			return block;
		}
	}
}

/*
 * cancella l'oggetto di nome name precedentemente memorizzato.
 * Restituisce true se la cancellazione ha avuto successo, false altrimenti.
 */
int os_delete(char *name){
	char buffer[BUFFLEN];
	int written;

	// Invio messaggio.
	BUFFER_CLEAR(buffer);
	written = sprintf(buffer, "DELETE %s \n", name);
	PERR_M1_RET(write(fd_socket, buffer, written), "os_delete", -1);

	// Attesa e lettura esito.
	BUFFER_CLEAR(buffer);
	PERR_M1_RET(read(fd_socket, buffer, BUFFLEN), "os_retrieve", -1);

	return is_ok(buffer);
}

/*
 * chiude la connessione all'object store.
 * Restituisce true se la disconnessione ha avuto successo, false in caso contrario.
 */
int os_disconnect(){

	int written;
	char buffer[8];

	written = sprintf(buffer,"LEAVE \n");
	write(fd_socket, buffer, written);
	memset(buffer, 0, 8);

	PERR_M1_RET(read(fd_socket, buffer, 8), "os_disconnect", -1);

	shutdown(fd_socket, SHUT_RDWR);

	return is_ok(buffer);
}
