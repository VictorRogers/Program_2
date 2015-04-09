server: server.c
	gcc -Wall -g -o server server.c

clean:
	rm -f *core server client *o
