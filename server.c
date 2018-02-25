#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define MAX_CLIENTS 20
#define BUFLEN 256
#define true 1
#define false 0

typedef struct {
  char nume[12];
  char prenume[12];
  long int nr_card;
  int pin;
  char parola[16];
  float sold;
  int unlocked;
  int count;
  int in_use;

} User;

User *users; //vector cu userii din baza de date
int N;
User empty_user; //o variabila ajutatoare
int count; //nr. 3 introduceri de pin consecutive

void error(char* msg) {
  perror(msg);
  exit(1);
}

/*Functia creeaza un user empty, il folosesc pentru a semnala
ca intr-un terminal nu este logat niciun client*/
void create_empty_user() {
  strcpy(empty_user.nume, "");
  strcpy(empty_user.prenume, "");
  empty_user.nr_card = 0;
  empty_user.pin = 0;
  strcpy(empty_user.parola, "");
  empty_user.sold = 0;
  empty_user.unlocked = true;
  empty_user.count = 0;
  empty_user.in_use = false;

}

int check_credentials(long int nr_card, int pin, User *u) {
  int i;
  for (i = 0; i < N; i++) {
    if (users[i].nr_card == nr_card) {
      if(users[i].in_use == true) return -2; //avem deja userul pornit in alt terminal
      if(users[i].unlocked == true) {
        if (users[i].pin == pin) { //login corect
          users[i].count = 0;
          users[i].in_use = true;
          *u = users[i]; //o sa modifice u, pentru a indica userul curent
          return 1; //login corect
        }  else if(users[i].count == 2) { //daca e a 3-oara cand pinul e gresit
          users[i].count = 0;
          users[i].unlocked = false; //se blocheaza cardul
          return -5;
        } else {
          users[i].count++; //altfel se numara greselile
          return -3;
        }
      } else { //este deja blocat
        users[i].count = 0;
        return -5;
      }
    }
    users[i].count = 0;
  }
  return -4; //nr. cardului nu exista in baza noastra de date
}

//Verifica loginul.
int check_login(char *data, User *user) {
  char *token = strtok(data, " ");
  long int nr_card = atol(token);
  int pin = atoi(strtok(NULL, ""));
  //check if credentials are correct
  return check_credentials(nr_card, pin, user);
}

//Verifica si efectueaza logoutul.

int check_logout(User *user) {
  int i;
  for(i = 0 ; i < N; i++) {
    if(users[i].nr_card == (*user).nr_card) {
      users[i].in_use = false;
      *user = empty_user;
    }
  }
  return 2;
}


//Returneaza un cod pentru listarea soldului.
int check_listsold(User *user) {
  return 3;
}

//Se modifica soldul in vectorul users.
void update_sold_user(User user) {
  int i;
  for (i = 0; i < N; i++) {
    if(users[i].nr_card == user.nr_card) {
      users[i].sold = user.sold;
    }
  }
}

int check_getmoney(char *data, User *user) {

  float money = atof(strtok(data, "\n"));
  if ((int) money % 10 != 0)
  return -9;
  if ((*user).sold < money)
  return -8;
  (*user).sold -= money;
  update_sold_user(*user);
  return 4; //cod pentru getmoney
}

int check_putmoney(char *data, User *user) {
  (*user).sold += atof(strtok(data, " \n"));
  update_sold_user(*user);
  return 5; //cod pentru putmoney
}

int check_command(char *buffer, User *user) {
  //verifica ce comanda a trimis clientul prin conexiunea tcp
  char *token = strtok(buffer, " \n");
  if(!strcmp(token, "login"))
  return check_login(strtok(NULL, ""), user);
  if(!strcmp(token, "logout"))
  return check_logout(user);
  if(!strcmp(token, "listsold"))
  return check_listsold(user);
  if(!strcmp(token, "getmoney"))
  return check_getmoney(strtok(NULL, ""), user);
  if(!strcmp(token, "putmoney"))
  return check_putmoney(strtok(NULL, ""), user);
  if(!strcmp(token, "quit"))
  return 6;
  return 6;
}

//Efectueaza unlock.
int unlock(long int id, char *pass) {
  int i;
  for (i = 0; i < N; i++) {
    if (users[i].nr_card == id && !strcmp(pass, users[i].parola)) {
        users[i].unlocked = true; //setez ca e deblocat
        return 1; //deblocare efectuata

    }
  }
  return -7; //deblocare esuata
}

//Verific daca un nr. de card e in baza mea de date users.
int verify_id_card(long int id) {
  int i;
  for (i = 0; i < N; i++) {
    if(users[i].nr_card == id) {
      if(users[i].unlocked == false) return 1;
     else return -6;
    }
  }
  return -4;
}

void print_user(User user) {
  printf("---- INFO CLIENT ----\n");
  printf("NUME %s\n", user.nume);
  printf("PRENUME %s\n", user.prenume);
  printf("NR CARD %ld\n", user.nr_card);
  printf("PIN %d\n", user.pin);
  printf("PAROLA %s\n", user.parola);
  printf("SOLD %.2f\n", user.sold);
  printf("\n");

}
void read_users_data_file(FILE *users_data_file) {
  char linie[256];
  int i;
  fgets(linie, 250, users_data_file);

  N = atoi(linie);

  users = (User *)(malloc(N * sizeof(User)));
  for (i = 0; i < N; i++) {
    fgets(linie, 256, users_data_file);
    User tmp;
    strcpy(tmp.nume, strtok(linie, " \n"));
    strcpy(tmp.prenume, strtok(NULL, " \n"));
    tmp.nr_card = atol(strtok(NULL, " \n"));
    tmp.pin = atoi(strtok(NULL, " \n"));
    strcpy(tmp.parola, strtok(NULL, " \n"));
    char *token = strtok(NULL, " \n");
    tmp.sold = atof(token);
    tmp.unlocked = true;
    tmp.in_use = false;
    tmp.count = 0;
    users[i] = tmp;
  }
}

int main(int argc, char *argv[]) {

  int sockfd, portno, newsockfd, udp_sockfd;
  struct sockaddr_in serv_addr, cli_addr;
  struct sockaddr_in udp_station;
  socklen_t len = sizeof(struct sockaddr_in), clilen;
  char buffer[BUFLEN], send_buffer[BUFLEN];
  int i, n;

  if (argc < 3) {
    exit(1);
  }

  FILE *users_data_file = fopen(argv[2], "r");
  read_users_data_file(users_data_file);

  fd_set read_fds; //multimea de citire folosita de select
  fd_set tmp_fds;
  int fdmax;
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  //deschidere socket conexiune tcp
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket.");
  }
  portno = atoi(argv[1]);

  //deschidere socket conexiune udp
  udp_sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_sockfd < 0) {
    error("ERROR opening socket.");
  }

  //setare adresa serverului
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY; //folosesc adresa ip a mastii;
  serv_addr.sin_port = htons(portno);

  //binding conexiune tcp, asociaza socketului tcp adresa serverului
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
    error("ERROR on binding.");
  }

  //asteapta MAX_CLIENTS cereri de conectare la socket
  listen(sockfd, MAX_CLIENTS);

  //adaugam 0 pentru citirea de la tastatura in file descriptori
  FD_SET(0, &read_fds);

  //adaugam noul file descriptor(socketul pe care se asculta conexiuni) in multimea read_fds
  FD_SET(sockfd, &read_fds);
  fdmax = sockfd;

  //binding conexiune udp, asociaza socketului udp adresa serverului
  if (bind(udp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
    error("ERROR in binding");
  }

  FD_SET(udp_sockfd, &read_fds); //se adauga socketul udp in multimea de file descriptori
  if(fdmax < udp_sockfd)
  fdmax = udp_sockfd;

  create_empty_user(); //initializare empty_user cu niste campuri goale

  /*Vector cu useri curenti care activeaza simultan din mai multe terminale.
  Este realocat dinamic cu fiecare cerere de conectare. */
  User *curr_users = malloc(sizeof(User));
  int k = fdmax ; //variabila pentru a face referire la clienti

  while (1) {
    tmp_fds = read_fds;
    if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
    error("ERROR in select");
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == sockfd) {
          //---- TCP -----
          //a venit ceva pe socketul tcp inactiv(cel cu listen) => avem o noua conexiune
          //actiunea serverului: accept()
          clilen = sizeof(cli_addr);
          if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
            error("ERROR in accept");
          } else {
            //adaug noul socket intors de accept() in multimea descriptorilor de citire
            FD_SET(newsockfd, &read_fds);
            if(newsockfd > fdmax) {
              fdmax = newsockfd;
            }
            //ii atasez terminalului curent i un empty_user
            curr_users[newsockfd-k-1] = empty_user;
            //aloc memorie pt urmatorul client
            curr_users = realloc(curr_users, (newsockfd-k+1)*sizeof(User));
          }
          printf("Noua conexiune de la %s, port %d, socket_client %d\n",
          inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
        } else if (i == udp_sockfd) {
          //---- UDP ----
          //a venit ceva pe socketul udp => actiunea serverului: recv();
          long int id_to_unlock;
          if ((n = recvfrom(udp_sockfd, buffer, BUFLEN, 0, (struct sockaddr *)& udp_station, &len)) <= 0) {
            if (n == 0) {
              //conexiunea s-a inchis
              printf("server: socket %d hung up\n", i);
            } else {
              error("ERROR in recv");
            }
          } else {
            //recv intoarce o valoare pozitiva de bytes primiti
            printf("Am primit pe udp mesajul %s\n", buffer);
            //prelucrez comanda pentru a vedea daca e unlock sau secret password

            char *char1 = strtok(buffer, " \n");
            if (strcmp(char1, "unlock") == 0) {
              id_to_unlock = atol(strtok(NULL, " \n"));
              switch(verify_id_card(id_to_unlock)) {
                case -4:
                sprintf(send_buffer, "UNLOCK> Cod eroare -4");
                break;
                case -6:
                sprintf(send_buffer, "UNLOCK> -6 : Deblocare esuata");
                break;
                default:
                sprintf(send_buffer, "UNLOCK> Trimite parola secreta");
                break;
              }
              //trimite raspuns
              sendto(udp_sockfd, send_buffer, BUFLEN, 0, (struct sockaddr *)&udp_station, len);
            } else {
              //Se primeste parola secreta.
              switch(unlock(id_to_unlock, char1)) {
                case -7:
                sprintf(send_buffer, "UNLOCK> -7 : Deblocare esuata");
                break;
                default:
                sprintf(send_buffer, "UNLOCK> Client deblocat");
                break;
              }
              //trimite raspuns
              sendto(udp_sockfd, send_buffer, BUFLEN, 0, (struct sockaddr *)&udp_station, len);
            }
          }

        } else if (i == 0){
          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN-1, stdin);

          if(!strcmp(buffer, "quit\n")) {
            for (i = 1; i <= fdmax; i++) {
              if(i != udp_sockfd && i != sockfd ) {
                sprintf(send_buffer,"quit");
                send(i, send_buffer, sizeof(send_buffer), 0);
                close(i);
                FD_CLR(i, &read_fds);
              }
            }
            close(sockfd);
            close(udp_sockfd);
            return 0;
          }
        } else {
          // ---- TCP ---
          //am primit date pe unul din socketii cu care vorbesc cu clientii
          //actiunea serverului: recv()
          memset(buffer, 0 , BUFLEN);
          if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0){
            if (n == 0) {
              //conexiunea s-a inchis
              printf("server: socket %d hung up\n", i);
            } else {
              error("ERROR in recv");
            }
            close(i);
            FD_CLR(i, &read_fds); //scoatem din multimea de citire socketul i
          } else { //recv intoarce > 0
            printf("Am primit de la clientul de pe socketul %d, mesajul %s\n", i, buffer);
            char *tmp_buffer;
            tmp_buffer = malloc(strlen(buffer));
            strcpy(tmp_buffer, buffer);
            memset(send_buffer, 0, BUFLEN);
            User curr_user = curr_users[i-k-1]; //userul curent
            switch(check_command(buffer, &curr_user)) {
              case 1: //login corect
              //in cazul loginului corect se actualizeaza userul curent din terminal
              memcpy(&curr_users[i-k-1], &curr_user, sizeof(User));
              sprintf(send_buffer,"ATM> Welcome %s %s\n", curr_users[i-k-1].nume,
               curr_users[i-k-1].prenume);
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -2:
              sprintf(send_buffer,"ATM> -2 : Sesiune deja deschisa\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -3:
              sprintf(send_buffer,"ATM> -3 : Pin gresit\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -4:
              sprintf(send_buffer, "ATM> -4 : Numar card inexistent\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -5:
              sprintf(send_buffer, "ATM> -5 : Card blocat\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case 2: //logout
              //in cazul logout-ului se actualizeaza userul curent din terminal
              memcpy(&curr_users[i-k-1], &curr_user, sizeof(User));
              sprintf(send_buffer,"ATM> Deconectare de la bancomat\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -1:
              sprintf(send_buffer,"-1 : Clientul nu este autentificat\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case 3: //listsold
              sprintf(send_buffer,"ATM> %.2f\n", curr_users[i-k-1].sold);
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -9:
              sprintf(send_buffer,"ATM> -9 : Suma nu este multiplu de 10\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case -8:
              sprintf(send_buffer,"ATM> -8 : Fonduri insuficiente\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case 4: //getmoney
              strtok(tmp_buffer, " \nNULL");
              //in caz de retragere suma, se actualizeaza sold-ul userului
              memcpy(&curr_users[i-k-1], &curr_user, sizeof(User));
              sprintf(send_buffer,"ATM> Suma %s retrasa cu succes\n", strtok(NULL, " \n"));
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case 5: //putmoney
              //se actualizeaza userul curent
              memcpy(&curr_users[i-k-1], &curr_user, sizeof(User));
              sprintf(send_buffer,"ATM> Suma depusa cu succes\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              break;
              case 6: //quit
              memcpy(&curr_users[i-k-1], &empty_user, sizeof(User));
              sprintf(send_buffer,"ATM> ACK quit\n");
              send(i, send_buffer, sizeof(send_buffer), 0);
              close(i);
              FD_CLR(i, &read_fds); //scoatem din multimea de citire socketul i
              break;
            }
          }
        }
      }
    }
  }
  close(udp_sockfd);
  close(sockfd);
  return 0;
}
