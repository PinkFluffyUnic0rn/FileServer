all:
	gcc fserver.c connection.c handlerequest.c -o server -Wall
