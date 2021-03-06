SHELL      		= /bin/bash
CC         		= gcc
INCLUDES   		= -I./lib/*.h
LIBS       		= -lpthread
SERVER_OUT 		= ./out/server
CLIENT_OUT 		= ./out/client
FIFOTEST_OUT	= ./out/FIFOtest
FLAGS			= -std=c99 -Wall
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
	$(CC) $(INCLUDES) -o $(SERVER_OUT) server.c $(server_lib) $(LIBS) $(FLAGS)

client: client.c
	$(CC) $(INCLUDES) -o $(CLIENT_OUT) client.c $(client_lib) $(LIBS) $(FLAGS)

FIFOtest_algorithm: FIFOtest_algorithm.c
	$(CC) $(INCLUDES) -o $(FIFOTEST_OUT) FIFOtest_algorithm.c $(client_lib) $(LIBS) $(FLAGS)

all: server client FIFOtest_algorithm

test1: server client clean
	clear
	valgrind --leak-check=full $(SERVER_OUT) -c./config/test1.ini &
	./script/test1.sh
	@killall -TERM -w memcheck-amd64-
	@printf "\ntest1 terminato\n"

test2: server client clean
	clear
	$(SERVER_OUT) -c./config/test2.ini &
	./script/test2.sh
	@killall -HUP -w server
	@printf "\ntest2 terminato\n"

test3: server client clean
	clear
	$(SERVER_OUT) -c./config/test3.txt &
	./script/test3.sh
	@killall -INT -w server
	@printf "\nTutti i client hanno scritto e cancellato 5 file\n"
	@printf "Di conseguenza, tutti gli errori dovuti a: file già esistenti e file non trovati, sono aspettati\n\n"
	@printf "test3 terminato\n"

fifo_test: server FIFOtest_algorithm
	clear
	$(SERVER_OUT) -c./config/test2.ini &
	@sleep 2 && \
	$(FIFOTEST_OUT)
	@killall -TERM -w server
	@printf "\nFIFOtest terminato\n"

clean		:
	@rm -f ./TestFolder/backupdir/* ./TestFolder/downloadDir/* ./socket/*

cleanall: clean
	@rm -f $(SERVER_OUT) $(CLIENT_OUT) $(FIFOTEST_OUT)