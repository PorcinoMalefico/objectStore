//
// Created by Matteo on 29/06/2019.
//
#define UNIX_PATH_MAX 108
#define SOCKNAME "objstore.sock"
#define PATHPRE "os_server/data"
#define LENOK 5
#define BUFFLEN 512
#define MAXLEN 100000

#define DBGprint( fmt , args... )                               \
        do{                                                     \
              fprintf( stderr ,                                 \
                       "[%-12.12s] " fmt , __func__       \
                       , ## args );                             \
        }while(0);

#define PERR_M1(err, wh) \
    do{if( (err) == -1) {perror(wh);}}whle(0);

#define PERR_M1_EXIT(err, wh) \
    do{if( (err) == -1) {perror(wh); exit(EXIT_FAILURE);} }while(0);

#define PERR_M1_RET(err, wh, retval) \
	do{if( (err) == -1) {perror(wh); return retval;} }while(0);

#define PERR_M1_DO(err, wh, action) \
	do{if( (err) == -1) {perror(wh); action} }while(0);

#define EBADF_PEXIT_ELSE_RET(retval)\
	do{if(errno == EBADF){pthread_exit(NULL);} \
		else {return retval;} }while(0);

#define BUFFER_CLEAR(buffer) \
	memset(buffer, 0, BUFFLEN);

#define WRITEOK(sckt) \
	write(sckt,"OK \n", 5);

#define WRITEKO(sckt) \
	write(sckt,"KO \n", 5);

int create_socket();

struct sockaddr_un create_sa();

int readn(int fd_socket, int len, void* buffer);

char* register_fun(char* name, int socket_cl);

void disconnect_fun(int fd_socket);

int store_fun(int fd_socket, char* path, char* block, int len, int left);

int retrieve_fun(int fd_socket, char* path);

int delete_fun(int fd_socket, char* path);