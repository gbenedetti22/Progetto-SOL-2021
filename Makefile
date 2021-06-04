SHELL      		= /bin/bash
CC         		= gcc
INCLUDES   		= -I./lib/*.h
LIBS       		= -lpthread
SERVER_OUT 		= ./out/server
CLIENT_OUT 		= ./out/client
.DEFAULT_GOAL 	:= all

server_lib = ./lib/src/config_parser.c \
			 ./lib/src/mysocket.c	\
			 ./lib/src/hash_table.c	\
			 ./lib/src/my_string.c	\
			 ./lib/src/queue.c	\
			 ./lib/src/sorted_list.c	\
			 ./lib/src/list.c	\
			 ./lib/src/file_reader.c

client_lib = ./lib/src/fs_client_api.c	\
			 ./lib/src/my_string.c	\
			 ./lib/src/file_reader.c \
			 ./lib/src/mysocket.c

.SUFFIXES: .c .h
.PHONY: all

server: server.c
	$(CC) $(INCLUDES) -o $(SERVER_OUT) server.c $(server_lib) $(LIBS) -w -g

client: client.c
	$(CC) $(INCLUDES) -o $(CLIENT_OUT) client.c $(client_lib) $(LIBS) -w -g

all: server client

test1: server client
	clear
	valgrind --leak-check=full $(SERVER_OUT) -c./config/test1.ini &
	./script/test1.sh
	killall -TERM -w memcheck-amd64-
	@printf "\ntest1 terminato\n"

test2: server client
	clear
	$(SERVER_OUT) -c./config/test2.ini &
	./script/test2.sh
	@killall -HUP -w server
	@printf "\ntest2 terminato\n"

clean		:
	rm -f
cleanall	: clean
	\rm -f *.o *~ *.a ./mysock