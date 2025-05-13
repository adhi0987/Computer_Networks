all: server client

client: mysmtp_client.c
	gcc -o client mysmtp_client.c

server: mysmtp_server.c
	gcc -o server mysmtp_server.c

clean:
	rm client server 