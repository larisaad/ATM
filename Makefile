build: server.c client.c
	gcc -Wall server.c -o server
	gcc -Wall client.c -o client
clean:
	rm server client *.log
