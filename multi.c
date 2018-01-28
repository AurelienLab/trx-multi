/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <alsa/asoundlib.h>

#include "defaults.h"
#include "multistructure.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

/* FONCTIONS SERVEUR */

SOCKET commInitServ(int nbClient) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin;
    
    if(sock == INVALID_SOCKET)
    {
	perror("socket()");
	exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY); /* nous sommes un serveur, nous acceptons n'importe quelle adresse */
    sin.sin_family = AF_INET; //Protocole ipV4
    sin.sin_port = htons(DEFAULT_COM_PORT); //port de communication
    
    if(bind (sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
	perror("bind()");
	exit(errno);
    }
    
    if(listen(sock, nbClient) == SOCKET_ERROR)
    {
       perror("listen()");
       exit(errno);
    }
    
    return sock;
}
static int read_client(SOCKET sock, char *buff) {
   int n = 0;

   if((n = recv(sock, buff, DEFAULT_COM_BUFFSIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   //comBuff[n] = 0;

   return n;
}

static void send_client(SOCKET sock, const char data) {
    if(send(sock, &data, strlen(&data), 0) < 0)
    {
	perror("send()");
	exit(errno);
    }
}

static int check_slot(Slot* slotList, int nbSlot) {
    int i = 0;
    
    for(i=0;i<nbSlot;i++) {
	if(slotList[i].isFree) return slotList[i].portNumber;
    }
    return 0;
}

static int give_slot(Slot* slots, SOCKET newSock, int portNumber, int nbSlot) {
    int i =0;
    for(i=0;i<nbSlot;i++) {
	if(slots[i].portNumber == portNumber) {
	    slots[i].isFree = 0 ;
	    slots[i].sock = newSock;
	    return 1;
	}
    }
    
    return 0;
}

static int back_slot(Slot* slot, int i) {
    close(slot[i].sock);
    slot[i].isFree = 1;
    return 1;
}

int close_all_sockets(SOCKET sock, Slot* clients, int nbSlots) {
    int i = 0;
    for(i=0;i<nbSlots; i++) {
	close(clients[i].sock);
    }
    close(sock);
    
    return 1;
}

void commListenServ(SOCKET sock, int nb_slot, Slot* clients) {
    char comBuff[DEFAULT_COM_BUFFSIZE] = {0}; //Buffer de lecture
    //int actual = 0;
    struct timeval timeout = {(long)0,(long)1000};
    int max = nb_slot;
    fd_set rdfs;
    
    while(1) {
	int i = 0;
	FD_ZERO(&rdfs);//on vide le fd
	
	FD_SET(sock, &rdfs); //on écoute le socket
	
	for(i=0; i < nb_slot; i++) { //On parcoure le tableau de clients
	    if(clients[i].isFree == 0) { //Si un slot est pris (on a un client)
		FD_SET(clients[i].sock, &rdfs); //on ajoute son socket au fd
	    }
	}
	fprintf(stdout, "Attente d'une connexion client.\n");
	if(select(max, &rdfs, NULL, NULL, NULL) == -1)
	{
	   perror("select()"); //Gestion d'erreur
	   exit(errno);
	}
	fprintf(stdout, "Evenement enregistré.\n");
	
	
	if(FD_ISSET(sock, &rdfs)) { //Connexion d'un client sur socket principal
	    fprintf(stdout, "Un client tente de se connecter.\n");
	    SOCKADDR_IN csin = { 0 };
	    socklen_t sinsize = sizeof(csin);
	    SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
	    if(csock == SOCKET_ERROR)
	    {
	       perror("accept()");
	       continue;
	    }
	    if(read_client(csock, comBuff) == -1) //Si erreur, on stoppe la boucle
	    {
	       /* Erreur, Deconnexion du client */
	       continue;
	    }
	    if(strcmp(comBuff, "check_slot")) { //Si demande de slot et place disponbile
		int dispo = check_slot(clients, nb_slot);
		if(dispo > 0) {		    
		    if(give_slot(clients, csock, dispo, nb_slot)) {			
			send_client(csock, (char)dispo);//envoyer dispo au client
		    }
		}
		else if(dispo == 0) {
		    send_client(csock, (char)dispo);
		}
	    }
	    else {
		continue;
	    }
	    
	    max = csock > max ? csock : max; //Si csock > max, max devient csock, sinon il ne change pas
	    
	    FD_SET(csock, &rdfs); //on ajoute le socket actuel à la lecture
	}
	else { //Si pas de changement sur socket principal
	    
	    int i = 0;
	    for(i=0;i<nb_slot;i++) {
		if(!clients[i].isFree) {
		    if(FD_ISSET(clients[i].sock, &rdfs)) { //info d'un client
			fprintf(stdout, "DEBUG3\n");
			int command = read_client(clients[i].sock, comBuff);
			if( command == 0) { //client deconnecté
			    back_slot(clients, i);
			}
			else if (strcmp(comBuff, "check_slot")){
			    int dispo = check_slot(clients, nb_slot);
			    if(dispo > 0) {				
				if(give_slot(clients, clients[i].sock, dispo, nb_slot)) {			
				    send_client(clients[i].sock, (char)dispo);//envoyer dispo au client
				}
			    }
			    else if(dispo == 0) {
				send_client(clients[i].sock, (char)dispo);
			    }
			}
			else if (strcmp(comBuff, "disconnect")) {
			    back_slot(clients, i);
			}
		    }
		}
	    }
	}
    }
}

/* FONCTIONS CLIENT */

SOCKET commInitClient(const char *ipaddress) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin;
    struct hostent *hostinfo;

    if(sock == INVALID_SOCKET)
    {
       perror("socket()");
       exit(errno);
    }
    
    fprintf(stdout, "Connexion à %s ... \n", ipaddress);
    
    hostinfo = gethostbyname(ipaddress);
    if (hostinfo == NULL)
    {
       fprintf (stderr, "Serveur inconnu %s.\n", ipaddress);
       exit(EXIT_FAILURE);
    }
    
    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(DEFAULT_COM_PORT);
    sin.sin_family = AF_INET;
    
    
    if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
       perror("connect()");
       exit(errno);
    }

    fprintf(stdout, "Connexion initialisée.\n");
    return sock;
}

static void send_server(SOCKET sock, const char *buff) {
    if(send(sock, buff, strlen(buff), 0) < 0)
    {
       perror("send()");
       exit(errno);
    }
    fprintf(stdout, "Demande %s envoyée au serveur. \n", buff);
}

static void end_connection(SOCKET sock) {
    close(sock);
}

static int read_server(SOCKET sock, char *buff) {
   int n = 0;

   if((n = recv(sock, buff, DEFAULT_COM_BUFFSIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buff[n] = 0;

   return n;
}

int ask_slot(SOCKET sock, int wait) {
    fprintf(stdout, "socket2: %d\n", sock);
    char buff[DEFAULT_COM_BUFFSIZE];
    struct timeval timeout;
    timeout.tv_sec = (long)3;
    timeout.tv_usec = (long)0;
    
    fd_set rdfs;
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    fprintf(stdout, "Envoi de la demande de slot au serveur.\n");
    send_server(sock, "check_slot");
    
   // fprintf(stdout, "DEBUG1 select : %d\n", select(*sock + 1, &rdfs, NULL, NULL, &timeout));
    if(select(sock + 1, &rdfs, NULL, NULL, &timeout) == -1)
    {
        
	perror("select()");
	exit(errno);
    }

    while(1) {
	if(FD_ISSET(sock, &rdfs)) {
		fprintf(stdout, "DEBUG3\n");
	    int n = read_server(sock, buff);
	    if(n == 0) {
		printf("Le serveur s'est arrêté\n");
		return -1;
	    }
	    fprintf(stdout, "Retour du serveur: %s", buff);
	    int givenPort = atoi(buff);
	    if(givenPort == 0) { //Pas de place
		/*if(wait) {
		    fprintf(stdout, "Serveur plein. En attente d'un slot libre...");
		    //On rentre dans une boucle d'attente du serveur
		    while(givenPort == 0) {

		    }
		}
		else if(!wait) {
		    return 0;
		}
		 */
		fprintf(stderr, "Tous les slots de duplex sont pris.\n");
		return 0; //En attendant de programmer la file d'attente...
	    }
	    else if(givenPort > 0) {
		return givenPort;
	    }
	}
    }
    return -1;
}

int commListenClient(SOCKET sock) {
    char buff[DEFAULT_COM_BUFFSIZE];
    
    fd_set rdfs;
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    
    if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
    {
       perror("select()");
       exit(errno);
    }
    while(1) {
	if(FD_ISSET(sock, &rdfs)) {
	    int n = read_server(sock, buff);
	    if(n == 0) {
		printf("Le serveur s'est arrêté\n");
		return -1;
	    }
	    
	    if(strcmp(buff, "disconnect")) {
		fprintf(stdout, "Le serveur a coupé la connexion");
		return 0; //on sort de la fonction bloquante, qui stoppe alors le serveur
	    }
	}
    }
}

