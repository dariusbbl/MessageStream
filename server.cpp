#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <bits/stdc++.h>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <queue>
#include <string>
#include <sstream>
#include "helpers.h"

#define MAX_CONNECTIONS 10000
#define LEN 10000
#define ID_LEN 50
#define TOPIC_LEN 51
#define TYPE_LEN 50

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

using namespace std;

// enum pentru statusul clientului
enum STATUS {
	OFFLINE = 0,
	ONLINE = 1
};

// structura pentru informatiile clientului
struct info_client {
	STATUS status;
	int sock;

	operator bool() {
		return status == ONLINE;
	}
};

// mapare intre id-ul clientului si informatiile acestuia
unordered_map<string, info_client> clients;

// mapare intre id-ul clientului si topicurile la care este abonat
unordered_map<string, vector<string>> subscriptions;

inline void set_max_fd(fd_set fds, int &fd_max) {
	while (fd_max-- >= 0 && !FD_ISSET(fd_max, &fds));
}

// functie care verifica daca clientul este deja conectat
bool handle_client(const char* id_client, int newsockfd) {
	if (clients.find(id_client) == clients.end()) {
		clients[id_client] = {ONLINE, newsockfd};
		return false;
	} else {
		if (clients[id_client]) {
			close(newsockfd);
			cout << "Client " << id_client << " already connected.\n";
			return true;
		} else {
			clients[id_client] = {ONLINE, newsockfd};
			return false;
		}
	}
}


// functie care extrage informatiile din mesajul UDP
// si le pune in structura tcp_message
// tratand fiecare tip de date in parte
void extract_udp_message(udp_message udp_message, tcp_message &tcp_message){
	switch (udp_message.type) {
        case 0: {// INT type
            strcpy(tcp_message.type, "INT");
            int64_t int_value = *(uint32_t *)(udp_message.content + 1);
            int_value = ntohl(int_value);

            if (udp_message.content[0] != 0)
                int_value = -int_value;

            sprintf(tcp_message.content, "%ld", int_value);
            break;
}
        case 1: {// SHORT_REAL type
            strcpy(tcp_message.type, "SHORT_REAL");
            double short_real_value = *(u_int16_t *)(udp_message.content);
            short_real_value = ntohs(short_real_value);
            short_real_value /= 100.0;
            sprintf(tcp_message.content, "%.2f", short_real_value);
            break;
}
        case 2: {// FLOAT type
            strcpy(tcp_message.type, "FLOAT");
             uint32_t raw_value = *(uint32_t *)(udp_message.content + 1);
    		 double float_value = ntohl(raw_value) / pow(10, udp_message.content[5]);

            if (udp_message.content[0] != 0)
                float_value = -float_value;

            sprintf(tcp_message.content, "%lf", float_value);
            break;
}
        case 3: {// STRING type
            strcpy(tcp_message.type, "STRING");
            strcpy(tcp_message.content, udp_message.content);
            break;
}
        default: {
            cerr << "Unknown data type encountered\n";
            break;
}
    }
}

// functie care verifica daca un topic este compatibil cu un altul
// folosindu-se de vectorii de stringuri
// si de regula de abonare la topicuri
bool find(const vector<string>& vec, const vector<string>& target) {
	for (const auto& e : vec) {
		stringstream ss(e);
		string str;
		vector<string> path;
		while(getline(ss, str, '/')) {
			path.push_back(str);
		}
		if (path.size() > target.size()) {
			continue;
		}
		bool ok = true;
		for (size_t i = 0, j = 0; j < target.size(); i++, j++) {
			if (i >= path.size()) {
				ok = false;
				break;
			}

			if (path[i] == "+") {
				continue;
			}

			if (path[i] == "*") {
				
				if (i == path.size() - 1) {
					return true;
				}

				i++;
				j++;
				while (j < target.size() && target[j] != path[i]) {
					j++;
				}

				if (j == target.size()) {
					ok = false;
					break;
				}
			}

			if (path[i] != target[j]) {
				ok = false;
				break;
			}
		}
		if (ok) {
			return true;
		}
	}
	return false;
}

// functie care trimite mesajul catre clientii abonati
// la topicul respectiv folosindu-se de maparea dintre 
// id-ul clientului si topicurile la care este abonat
void send_to_subscribers(const tcp_message& tcp_message) {

	stringstream ss(tcp_message.topic);
	string str;
	vector<string> target;
	while(getline(ss, str, '/')) {
		target.push_back(str);
	}

	for (const auto& e : subscriptions) {
		if (clients[e.first] && find(e.second, target)) {
			int rc = send(clients[e.first].sock, &tcp_message, sizeof(tcp_message), 0);
			DIE(rc < 0, "send");
		}
	}
}

// functie care primeste mesajul UDP si il trimite catre
// clientii abonati la topicul respectiv
// folosindu-se de functia send_to_subscribers
void handle_udp_data(char* buf, sockaddr_in serv_addr) {
	udp_message* udp_data = ((udp_message *)buf);
	tcp_message tcp_data;
	strcpy(tcp_data.source_ip, inet_ntoa(serv_addr.sin_addr));
	tcp_data.source_port = ntohs(serv_addr.sin_port);
	strncpy(tcp_data.topic, udp_data->topic, 51);

	extract_udp_message(*udp_data, tcp_data);
	send_to_subscribers(tcp_data);
}

// functie care verifica daca comanda introdusa de la tastatura
// este valida sau nu
// in cazul in care nu este valida, se afiseaza un mesaj de eroare
inline void handle_stdin(char* command) {
	if (!strcmp(command, "exit"))
		exit(0);
	else 
		cout << "You must introduce a valid command. Expected exit.\n";
}

// functie care adauga un client la lista de abonati
inline void create_subscriber(const char* id_client, const char* topic) {
	if (find(subscriptions[id_client].begin(), subscriptions[id_client].end(), topic) == subscriptions[id_client].end()) {
		subscriptions[id_client].push_back(topic);
	}
}

// functie care sterge un client de la lista de abonati
void unsubscribe(const char* id_client, const char* topic) {

	auto client = subscriptions.find(id_client);
	auto erase = find(client->second.begin(), client->second.end(), topic);
	if (erase != client->second.end()) {
		client->second.erase(erase);
	} else {
		cerr << "Client not subscribed to this topic.\n";
	}

}

// functie care primeste comanda de la client si o executa
// in functie de tipul acesteia
void handle_commands(char *command, const char* id_client) {

	if (!strcmp(command, "subscribe")) { // comanda de subscribe primeste un topic

		command = strtok(NULL, " \n");
		char topic[TOPIC_LEN] = { 0 };
		strncpy(topic, command, sizeof(topic));
		create_subscriber(id_client, topic);
	
	} else if (!strcmp(command, "unsubscribe")) { // comanda de unsubscribe primeste un topic

		command = strtok(NULL, " \n");
		char topic[TOPIC_LEN] = { 0 };
		strncpy(topic, command, sizeof(topic));
		unsubscribe(id_client, topic);

	} else {

		cerr << "Not expected command to handle.\n"; // comanda invalida

	}
}

void set_flags(int sockfd) {
	int enable = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	cerr << "setsockopt(SO_REUSEADDR) failed";

	int rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
	DIE(rc < 0, "disable Nagle");
}

// functie care ruleaza serverul si primeste datele de la clienti si de la UDP
// in functie de tipul de date primit, se trimit datele catre clientii
void run(int sock_tcp, int sock_udp, fd_set fds) {

	// retin cel mai mare descriptor
	int fd_max = MAX(sock_tcp, sock_udp); 

	// iau un set auxiliar pentru a retine descriptorii
	fd_set aux_set;
	FD_ZERO(&aux_set);

	char id_client[50];

	// bucla infinita pentru a primi si procesa datele
	while (true) {
		aux_set = fds;

		// asteapta pana cand datele sunt disponibile
		int rc = select(fd_max + 1, &aux_set, NULL, NULL, NULL);
		DIE(rc < 0, "select");

		// parcurgerea socketilor pentru a vedea de unde se primesc datele
		for (int i = 0; i < fd_max + 1; i++) {
			if (FD_ISSET(i, &aux_set)) {

				if (i == STDIN_FILENO) {

					char command[LEN] = { 0 };
					cin.getline(command, LEN);
					handle_stdin(command);

				} else if (i == sock_tcp) {

					sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);

					int newsockfd = accept(sock_tcp, (sockaddr*)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "TCP accept");
					set_flags(newsockfd);

					char buf[LEN] = { 0 };
					rc = recv(newsockfd, buf, LEN, 0);
					memset(id_client, 0, sizeof(id_client));
					strncpy(id_client, buf, sizeof(id_client));

					if (handle_client(id_client, newsockfd))
						continue;

					FD_SET(newsockfd, &fds);

					fd_max = MAX(fd_max, newsockfd);
					cout << "New client " << id_client << " connected from "
						<< inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";
				} else if (i == sock_udp) {

					sockaddr_in udp_addr_sock;
					char buf[LEN] = { 0 };

					socklen_t udp_sock_len = sizeof(sockaddr);
					rc = recvfrom(i, buf, sizeof(buf), 0, (sockaddr*)&udp_addr_sock, &udp_sock_len);
					DIE(rc < 0, "recvfrom");
					
					handle_udp_data(buf, udp_addr_sock);
				}  else {
					char buf[LEN] = { 0 };
					rc = recv(i, buf, sizeof(buf), 0);
					DIE(rc < 0, "rc recv");

					if (rc == 0 || !strcmp("exit\n", buf)) {
						auto client = find_if(clients.begin(), clients.end(), [i](const auto& e){
							return e.second.sock == i;
						});

						clients[client->first].status = OFFLINE;
						cout << "Client " << client->first << " disconnected.\n";
						close(i);
						FD_CLR(i, &fds);
						set_max_fd(fds, fd_max);
						continue;
					}

					char* id = strtok(buf, " \n");
					char* command = strtok(NULL, " \n");
					
					handle_commands(command, id);
				}
			}
		}
	}
	// inchidere socketi
	for (int i = 0; i <= fd_max; i++) {
		if (FD_ISSET(i, &fds)) {
			close(i);
		}
	}
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		cerr << "\n Usage: " << argv[0]<< " <ip> <port>\n";
		exit(0);
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	// setare port
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// initializare socketi
	fd_set fds;
	FD_ZERO(&fds);

	int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "TCP socket");
	set_flags(sock_tcp);

	
	int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "UDP socket");
	
	int enable = 1;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	fprintf(stderr,"setsockopt(SO_REUSEADDR) failed");

	sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = { .s_addr = INADDR_ANY },
		.sin_zero = {0}
	};

	rc = bind(sock_tcp, (sockaddr *)&serv_addr, sizeof(sockaddr));
	DIE(rc < 0, "bind tcp");

	rc = bind(sock_udp, (sockaddr *)&serv_addr, sizeof(sockaddr));
	DIE(rc < 0, "bind udp");

	rc = listen(sock_tcp, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	// adaugare socketi in setul de descriptori
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(sock_tcp, &fds);
	FD_SET(sock_udp, &fds);
	
	// rulare server
	run(sock_tcp, sock_udp, fds);

	return 0;
}

