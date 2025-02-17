#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include "helpers.h"

#define MAX_CONNECTIONS 10000
#define LEN 10000
#define ID_LEN 50
#define TOPIC_LEN 51
#define TYPE_LEN 50

using namespace std;

// functie care trimite un request la server
void send_request(const char* id_client, char* buf, int sockfd) {

	char result[LEN] = { 0 };
	// construiesc mesajul care va fi trimis la server
	sprintf(result, "%s %s", id_client, buf);
	char *command = strtok(buf, " \n");
	char *topic = strtok(NULL, " \n");
	bool malformed = false;
	
	// afisare mesaj in functie de comanda primita
	// comanda invalida -> mesaj de eroare
	if (!strcmp(command, "subscribe")) {
		cout << "Subscribed to topic " << topic << "\n";
	} else if (!strcmp(command, "unsubscribe")) {
		cout << "Unsubscribed to topic " << topic << "\n";
	} else {
		cerr << "Not a valid command";
		malformed = true;
	}

	// comanda valida -> trimitere mesaj la server
	if (!malformed) {
    int rc = send(sockfd, result, LEN, 0);
		if (rc < 0) {
			perror("send");
		}
	}
}

// functie care afiseaza continutul unui mesaj
void print_contents(tcp_message* message) {
	printf("%s:%hu - %s - %s - %s\n", 
					message->source_ip,
					message->source_port,
                   	message->topic, 
					message->type, 
					message->content);
}

// functie care ruleaza clientul
// se ocupa de citirea de la tastatura si de citirea de la server
// utilizeaza select() pentru a astepta date de la server si de la tastatura
void run(const char* id_client, int sockfd, fd_set fds) {
	
	int rc = send(sockfd, id_client, strlen(id_client) + 1, 0);
	DIE(rc < 0, "id_client");
	int fd_max;

	fd_set aux_set_fs;
	FD_ZERO(&aux_set_fs);
	FD_SET(sockfd, &fds);
	FD_SET(STDIN_FILENO, &fds);

	fd_max = sockfd;
	char buf[LEN];
	
	while (true) {
		aux_set_fs = fds;
		fd_max++;

		rc = select(fd_max, &aux_set_fs, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &aux_set_fs)) {
			memset(buf, 0, LEN);
			cin.getline(buf, LEN - 1);

			if (!strncmp(buf, "exit", 4)) {
				break;
			}

			send_request(id_client, buf, sockfd);

		} else if (FD_ISSET(sockfd, &aux_set_fs)) {

			int data_read = recv(sockfd, buf, sizeof(tcp_message), 0);
			// daca nu mai sunt date de citit, se iese din bucla
			if (data_read == 0) {
				break;
			}
			// afisarea continutului mesajului
			print_contents((tcp_message *) buf);
		}
	}
	// inchidere socket
	close(sockfd);
}


int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s server_port\n", argv[0]);
		exit(0);
	}

	// setare buffer pentru stdout
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	const char *id = argv[1];

	fd_set fds;
	FD_ZERO(&fds);

	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	
	int enable = 1;
  	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	cerr << "setsockopt(SO_REUSEADDR) failed\n";

	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
	DIE(rc < 0, "disable Nagle");

	sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = { .s_addr = INADDR_ANY },
		.sin_zero = { 0 }
	};
	rc = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(rc <= 0, "inet_aton");

	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(sockaddr_in));
	DIE(rc < 0, "connect");

	// se ruleaza clientul
	run(id, sockfd, fds);
	
	return 0;
}
