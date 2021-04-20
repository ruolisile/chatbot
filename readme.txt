files:
	chat_client.cpp
	chat_server.cpp
	common.h
	file.cpp
	file.h
	Makefile
	readme.txt
	server.conf
	client.conf
compile:
	make
run:
	./chat_server server.conf
	./chat_client client.conf

Mandatory Implementaion:
	Pthread is used in the chat_client to create a thread for receving message from chat_server and a thread for receiving message to other users. 
	Select is used in the chat_server to detect any of the descriptors in the read set that are ready for reading. 
	
Extra Points:
	Second option.
	This is implemented by adding an additional listen socket in chat_client. 
	The listen socket information is forward to chat_server when a suer logs in to the chat_server. 
	To send message directly to another user B, the user A will first get corresponding ip and port number from the chat_server. 
	Then the user A will connect to user B directly and send the message to user B. 

notes:
	Logout first before typing Ctrl-C
	Sending messages to oneself is not supported
	All successful commands will be prompted, unsuccessful commands will be ignored without any prompt.
	The name length is limited to 20 characters, and the message length is limited to 4096. These are defined in header file common.h.
	All commands are case insensitive. 
