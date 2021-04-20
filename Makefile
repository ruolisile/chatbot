# CC=g++ -march=native -w -O3
# CC=g++ -O3 --std=c++0x
# CC=g++
CC=g++ --std=c++0x
CFLAGS = -Wall -pedantic
file = file.o file.h


all: chat_server chat_client

chat_server: chat_server.o file.o
	$(CC) -o  chat_server chat_server.o file.o

chat_server.o: chat_server.cpp file.h 
	$(CC) -c $(CFLAGS) chat_server.cpp 

chat_client: chat_client.o file.o
	$(CC) -o  chat_client chat_client.o file.o -lpthread

chat_client.o: chat_client.cpp file.h 
	$(CC) -c $(CFLAGS) chat_client.cpp 

# index: index.o $(common) $(file) $(deTruss) $(myG) $(alley)
# 	$(CC)  -rdynamic -o index.out index.o file.o deTruss.o myG.o alley.o
	
file.o: file.h 
	$(CC) -c $(CFLAGS) file.cpp

	
clean:
	rm *.o
	rm chat_server chat_client
