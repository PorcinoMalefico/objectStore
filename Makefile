# Variabili

CC 		= gcc 
CFLAGS 		= -Wall -pedantic -Wshadow

SERVER 		= ./os_server/server
SERVER_C 	= ./os_server/server.c
SERVER_FUN 	= ./os_server/os_server.c

CLIENT 		= ./os_client/client
CLIENT_C 	= ./os_client/client.c

LIST        = ./lib/libthreadlist.a
LISTO       = ./lib/threadlist.o
LISTC       = ./lib/threadlist.c
LISTH       = ./lib/threadlist.h

LIB 		= ./lib/libos_access.a
LIBO 		= ./lib/os_access.o
LIBC 		= ./lib/os_access.c
LIBH       	= ./lib/os_access.h


.PHONY: clean server server_debug client client_debug


# Istruzioni
all: $(SERVER) $(LIST) $(LIB) $(CLIENT)


server_debug: $(SERVER_C) $(SERVER_FUN) $(LIST)
	$(CC) $(CFLAGS) -Wno-variadic-macros -pthread -g $(SERVER_C) $(SERVER_FUN) -o $(SERVER) -L ./lib/ -lthreadlist


server $(SERVER): $(SERVER_C) $(SERVER_FUN) $(LIST)
	$(CC) $(CFLAGS) -Wno-variadic-macros -pthread $(SERVER_C) $(SERVER_FUN) -o $(SERVER) -L ./lib/ -lthreadlist


client_debug: $(LIB) $(CLIENT_C)
	$(CC) $(CFLAGS) -g $(CLIENT_C) -o $(CLIENT) -L ./lib/ -los_access


client $(CLIENT): $(LIB) $(CLIENT_C)
	$(CC) $(CFLAGS) $(CLIENT_C) -o $(CLIENT) -L ./lib/ -los_access


lib $(LIB): $(LIBC) $(LIBH)
	$(CC) $(CFLAGS) -g -c $(LIBC) -o $(LIBO)
	ar rvs $(LIB) $(LIBO)


$(LIST): $(LISTC) $(LISTH)
	$(CC) $(CFLAGS) -pthread -c $(LISTC) -o $(LISTO)
	ar rvs $(LIST) $(LISTO)


clean:
	-rm -fv $(LIBO) $(LIB)
	-rm -fv $(LISTO) $(LIST)
	-rm -fv ./os_client/*.o $(CLIENT)
	-rm -fv ./os_server/*.o $(SERVER)
	-rm -fv *.log
	-rm -fv objstore.sock


clean_data:
	-rm -rf ./os_server/data/*


test:
	./os_server/server 2>server.log & ./test.sh
	./testsum.sh
	killall -INT server
