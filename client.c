#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include<arpa/inet.h>


#define BUFLEN 256
#define true 1
#define false 0

void error(char *msg)
{
	perror(msg);
	exit(0);
}

int main (int argc, char* argv[]) {
	int sockfd, n, udp_sockfd;
	struct sockaddr_in serv_addr;
	
	socklen_t len = sizeof(struct sockaddr_in);
	char buffer[BUFLEN];

	if (argc < 3) {
		exit(0);
	}

	fd_set read_fds, tmp_fds;
	FD_ZERO(&read_fds);
	int fdmax;
	FD_SET(0, &read_fds); //adaugam citirea de la stdin in file descriptori
	fdmax = 0;

	//deschidere socket tcp
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	error("ERROR opening socket");
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	//deschidere socket udp
	udp_sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sockfd < 0){
		error("ERROR in opening socket");
	}
	FD_SET(udp_sockfd, &read_fds);
	if(fdmax < udp_sockfd)
	fdmax = udp_sockfd;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &serv_addr.sin_addr);

	//conexiune tcp
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	error("ERROR in connect");

	char recv_buf[BUFLEN], tmp_buf[BUFLEN];
	memset(recv_buf, 0, BUFLEN);
	int curr_log_state = false;

	char name[20];
	//creare si deschidere fisier de log
	sprintf(name, "client-%d.log", getpid());
	int file  =  open(name, O_WRONLY | O_CREAT, 0644);
	long int lastlog;

	while (1) {
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
		error("ERROR in select");
		if (FD_ISSET(sockfd, &tmp_fds)) {
			//primim de la server
			//actiunea clientului: recv();
			if (recv(sockfd, recv_buf, BUFLEN, 0) > 0) {
				if(!strcmp(recv_buf, "ATM> ACK quit\n")) {
					return 0;
				} else if(!strcmp(recv_buf, "quit")) {
					return 0;
				}

				write(file, recv_buf, strlen(recv_buf));
				printf("%s\n", recv_buf);
				memset(tmp_buf, 0, BUFLEN);
				strcpy(tmp_buf, recv_buf);
				strtok(tmp_buf, " \n");
				if(!strcmp(strtok(NULL, " \n"), "Welcome")) {
					curr_log_state = true; //actualizam starea de log
				}
			}
		} else if (FD_ISSET(0, &tmp_fds)) {

			//citire de la stdin
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN-1, stdin);
			//scriere in fisier de log
			write(file, buffer, strlen(buffer));
			//verificari din client
			memset(tmp_buf, 0, BUFLEN);
			strcpy(tmp_buf, buffer);
			char *tmp = strtok(tmp_buf, " \n");
			if (!strcmp(tmp, "login")) { //daca se citeste comada login
				if (curr_log_state == false) { //nu am un user logat
					lastlog = atol(strtok(NULL, " ")); //se retine ultimul log
					//timitere la server
					n = send(sockfd, buffer, strlen(buffer), 0); //trimitem la server comanda
					if (n < 0) {
						error("ERROR in writing socket");
					}
				} else { //curr_log_state = true; exista cineva logat in acest terminal
					sprintf(buffer, "-2 : Sesiune deja deschisa\n");
					printf("%s\n", buffer);
					write(file, buffer, strlen(buffer));
				}
			} else if (strcmp(tmp, "unlock")) { //daca comanda NU e unlock
				if(!strcmp(tmp, "quit")) {
					n = send(sockfd, buffer, strlen(buffer), 0);
					if (n < 0) {
						error("ERROR in writing socket");
					}

				}
				else if (curr_log_state == true) {
					if (!strcmp(tmp, "logout")) curr_log_state = false;
					//timitere la server
					n = send(sockfd, buffer, strlen(buffer), 0);
					if (n < 0) {
						error("ERROR in writing socket");
					}
				} else { //curr_log_state = false; nu e nimeni logat, nu putem aplica comenzi
					sprintf(buffer, "-1 : Clientul nu este autentificat\n");
					printf("%s\n", buffer);
					write(file, buffer, strlen(buffer));
				}
			} else { //se citeste comanda unlock

				//se face unlock pe ultimul id introdus alaturi de comanda login.
				sprintf(buffer, "unlock %ld\n", lastlog);
				sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr, len);

				recvfrom(udp_sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &serv_addr, &len);
				printf("%s\n", buffer);
				if(!strcmp("UNLOCK> Trimite parola secreta", buffer)) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN-1, stdin);
					sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr, len);
					recvfrom(udp_sockfd, buffer, BUFLEN, 0, (struct sockaddr *) &serv_addr, &len);
					printf("%s\n", buffer);
				}
			}
		}

	}
	close(udp_sockfd);
	close(sockfd);
	return 0;
}
