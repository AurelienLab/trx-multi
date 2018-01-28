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
#include <sys/select.h>
#include <sys/types.h>
#include <alsa/asoundlib.h>

#include "defaults.h"
#include "multistructure.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

/* FONCTIONS GENERIQUES */
void end_connection(SOCKET sock) {
    fprintf(stdout, "Socket %d fermé.\n", sock);
    close(sock);
}

static char* get_value(char* param, char* string) {
    int paramSize = 0;
    char *paramPosition = NULL;
    
    paramPosition = strstr(string, param);
    if(paramPosition == NULL) {
	return NULL;
    }
    
    paramSize = strlen(param);
    paramPosition += paramSize;
    return paramPosition;
}
/* FONCTIONS SERVEUR */

SOCKET commInitServ(int nbClient) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    
    if(sock == INVALID_SOCKET)
    {
	perror("socket()");
	exit(errno);
	return -1;
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY); /* nous sommes un serveur, nous acceptons n'importe quelle adresse */
    sin.sin_family = AF_INET; //Protocole ipV4
    sin.sin_port = htons(DEFAULT_COM_PORT); //port de communication
    
    if(bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
	perror("bind()");
	exit(errno);
	return -1;
    }
    fprintf(stdout, "nbClient : %d\n", nbClient);
    if(listen(sock, nbClient) == SOCKET_ERROR)
    {
       perror("listen()");
       exit(errno);
       return -1;
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

static void send_client(SOCKET sock, const char* data) {
    fprintf(stdout, "APPEL DE SEND_CLIENT() POUR: %s\n", data);
    if(send(sock, data, strlen(data), 0) < 0)
    {
	perror("send()");
	exit(errno);
    }
    fprintf(stdout, "Message: %s envoyé sur le socket %d\n", data, sock);
}

static int check_slot(Slot* slotList, int nbSlot) {
    int i = 0;
    
    for(i=0;i<nbSlot;i++) {
	if(slotList[i].isFree){
	    return slotList[i].portNumber;
	}
    }
    return 0;
}

static int give_slot(Slot* slots, SOCKET newSock, int freePort, int nbSlot) {
    fprintf(stdout, "APPEL DE GIVE_SLOT().\n");
    int i=0;
    for(i=0 ; i<nbSlot ; i++) {
	if(slots[i].portNumber == freePort) {
	    slots[i].isFree = 0 ;
	    slots[i].sock = newSock;
	    return 1;
	}
    }
    
    return 0;
}

static int back_slot(Slot* slot, SOCKET sock, int nbSlots) {
    int i = 0;
    for(i=0; i < nbSlots; i++) {
	if(slot[i].sock == sock) {
	    slot[i].sock = 0;
	    slot[i].isFree = 1;
	    return 1;
	}
    }
    return 0;
}

static int put_onwait(SOCKET* waitlist, SOCKET csock) {
    int i = 0;
    for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	if(waitlist[i] == 0) {
	    waitlist[i] = csock;
	    return i;
	}
    }
    return -1;
}

static int count_waiting(SOCKET* waitlist) {
    int i = 0;
    for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	if(waitlist[i] == 0) return i;
    }
    return -1;
}

static int revise_waiting(SOCKET* waitlist) {
    int i =0;
    char message[DEFAULT_COM_BUFFSIZE] = {0};
    
    for(i=0; i < DEFAULT_WAIT_LIST - 1; i++) {
	if(waitlist[i] == 0) {
	    waitlist[i] = waitlist[i+1];
	    if(waitlist[i] != 0) {
		snprintf(message, DEFAULT_COM_BUFFSIZE, "wait%d", i);
		send_client(waitlist[i], message);
	    }
	}
    }
    return 1;
}

static int waiting_to_slot(SOCKET* waitlist, Slot* slots, int nbSlots) {
    int i = 0, changes = 0;
    char message[DEFAULT_COM_BUFFSIZE] = {0};
    
    for(i=0;i<nbSlots;i++) {
	if(slots[i].isFree && waitlist[0] != 0) {
	    slots[i].isFree = 0;
	    slots[i].sock = waitlist[0];
	    waitlist[0] = 0;
	    snprintf(message, DEFAULT_COM_BUFFSIZE, "port%d", slots[i].portNumber);
	    send_client(slots[i].sock, message);
	    
	    changes++;
	    
	    revise_waiting(waitlist);
	    continue;
	}
    }
    return changes;
}

int close_all_sockets(SOCKET sock, Slot* clients, int nbSlots) {
    int i = 0;
    fprintf(stdout, "Connexion close\n");
    for(i=0;i<nbSlots; i++) {
	close(clients[i].sock);
    }
    close(sock);
    
    return 1;
}


static int get_param(char* param, char* string) {
    fprintf(stdout, "APPEL DE GET_PARAM pour %s dans %s\n", param, string);
    char *paramPosition = NULL;
    
    paramPosition = strstr(string, param);
    
    if(paramPosition != NULL) {
	return 1;
    }
    return 0;
}

void commListenServ(SOCKET sock, int nb_slot, Slot* clients) {
    char comBuff[DEFAULT_COM_BUFFSIZE] = {0}; //Buffer de lecture
    int max = sock;
    char message[DEFAULT_COM_BUFFSIZE] = {0};
    SOCKET waitlist[DEFAULT_WAIT_LIST] = { 0 };
    int waitAmt = 0;
    int connectedAmt = 0;
    fd_set rdfs;
    
    while(1) {
	int i = 0;
	
	FD_ZERO(&rdfs);//on vide le fd
	
	FD_SET(sock, &rdfs); //on écoute le socket principal
	
	for(i=0;i<nb_slot;i++) {
	    if(!clients[i].isFree) { //Si un slot est occupé
		FD_SET(clients[i].sock, &rdfs); //On ajoute son socket a la lectru d'events
	    }
	}
	
	for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	    if(waitlist[i] != 0) { //Si il y a du monde dans la file d'attente
		FD_SET(waitlist[i], &rdfs); //On ajoute son socket a la lectru d'events
	    }
	}
	
	fprintf(stdout, "Attente d'un évènement sur le sockets %d.\n", sock);
	if(select(max+1, &rdfs, NULL, NULL, NULL) == -1)
	{
	   perror("select()"); //Gestion d'erreur
	   exit(errno);
	}
	
	
	if(FD_ISSET(sock, &rdfs)) { //Changement sur socket principal
	    fprintf(stdout, "Un nouveau client tente de se connecter.\n");
	    SOCKADDR_IN csin = { 0 };
	    socklen_t sinsize = sizeof(csin);
	    
	    int dispo = check_slot(clients, nb_slot);
	    
	    if(dispo > 0) {
		SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
		if(csock == SOCKET_ERROR)
		{
		   perror("accept()");
		   continue;
		}
		
		if(read_client(csock, comBuff) == -1)
		{    
		   //Le client s'est deconnecté
		   continue;
		}
		
		if(give_slot(clients, csock, dispo, nb_slot)) {
		    
		    snprintf(message, DEFAULT_COM_BUFFSIZE, "port%d", dispo);
		    send_client(csock, message);
		    fprintf(stdout, "Connexion acceptée sur le socket %d, port %d\n", csock, dispo);
		    connectedAmt++;
		    max = csock > max ? csock : max;
		}
	    }
	    else if(dispo == 0) {
		if(count_waiting(waitlist) < DEFAULT_WAIT_LIST) { //Si place sur la file d'attente
		    SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
		    if(csock == SOCKET_ERROR)
		    {
		       perror("accept()");
		       continue;
		    }
		    int position = put_onwait(waitlist, csock);
		    snprintf(message, DEFAULT_COM_BUFFSIZE, "wait%d", position);
		    send_client(csock, message);
		    waitAmt++;
		    max = csock > max ? csock : max;
		}
		else {
		    SOCKET tmpsock = accept(sock, (SOCKADDR *)&csin, &sinsize);
		    if(tmpsock == SOCKET_ERROR)
		    {
		       perror("accept()");
		       continue;
		    }
		    send_client(tmpsock, "disconnect");
		    close(tmpsock);
		}
	    }
	}
	else {
	    for(i = 0; i < nb_slot; i++ ) {
		if(FD_ISSET(clients[i].sock, &rdfs)) {
		    Slot client = clients[i];
		    int c = read_client(client.sock, comBuff);
		    if(c == 0 || c == -1) { //Le client s'est deconnecté
			fprintf(stdout, "Un client s'est deconnecté du socket %d\n", client.sock);
			fprintf(stdout, "Liberation du port %d.\n", client.portNumber);
			back_slot(clients, client.sock, nb_slot);
			revise_waiting(waitlist);
			waiting_to_slot(waitlist, clients, nb_slot);
			revise_waiting(waitlist);
		    }
		    else {
			if(get_param("disconnect", comBuff)) {
			    fprintf(stdout, "Un client s'est deconnecté du socket %d\n", client.sock);
			}
		    }
		}
	    }
	    for(i = 0; i < DEFAULT_WAIT_LIST; i++ ) {
		if(FD_ISSET(waitlist[i], &rdfs)) {
		    int c = read_client(waitlist[i], comBuff);
		    
		    if(c == 0 || c == -1) { //Le client s'est deconnecté
			SOCKET oldSock = waitlist[i];
			end_connection(waitlist[i]);
			waitlist[i] = 0;
			revise_waiting(waitlist);
			fprintf(stdout, "Un client s'est deconnecté de la liste d'attente (socket %d).\n", oldSock);
			fprintf(stdout, "Encore %d client(s) en file d'attente.\n", count_waiting(waitlist));
		    }
		    else {
			/* Gestion des messages client */
		    }
		}
	    }
	} 	 
    }
}

/* FONCTIONS CLIENT */

SOCKET commInitClient(const char *ipaddress) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
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

int ask_slot(SOCKET sock) {
    fprintf(stdout, "socket: %d\n", sock);
    char buff[DEFAULT_COM_BUFFSIZE] = {0};
    
    fprintf(stdout, "Envoi de la demande de slot au serveur.\n");
    send_server(sock, "coucou"); //CHANGER POUR TRANSMETTRE IP et autre infos
    
    fd_set rdfs;
    
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    fprintf(stdout, "Attente de la réponse du serveur.\n");
    if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
    {

	perror("select()");
	exit(errno);
    }

    if(FD_ISSET(sock, &rdfs)) {
	int n = read_server(sock, buff);
	if(n == 0 || n == -1) {
	    fprintf(stdout, "Le serveur s'est arrêté\n");
	    return -1;
	}
	fprintf(stdout, "Retour du serveur: %s\n", buff);
	
	if(get_param("port", buff)) {
	    //lancement d'une instance au port indiqué
	    return(strtol(get_value("port", buff), NULL, 0));
	}
	if(get_param("wait", buff)) {
	    
	    int waitpos = strtol(get_value("wait", buff), NULL, 0);
	    if(waitpos < 0) {
		//Le serveur est plein
		fprintf(stdout, "Le serveur indique que tous les slots sont occupés et la file d'attente pleine.\n");
		return -1;
	    }
	    else if(waitpos >= 0) {
		//File d'attente en position  waitpos +1
		fprintf(stdout, "Les slots sont occupé, %d autre(s) personne(s) en file d'attente.\n", waitpos);
		return 0;
	    }
	}
    }  
    return -1;
}

int commListenClient(SOCKET sock) {
    char buff[DEFAULT_COM_BUFFSIZE];
    while(1) {
	fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);

	if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
	{
	   perror("select()");
	   exit(errno);
	}
	int c = read_server(sock, buff);
	
	if(c == 0 || c == -1) {
	    //Perte du serveur
	    return -1;
	}
	if(get_param("port", buff)) {
	    //lancement d'une instance au port indiqué
	    return(strtol(get_value("port", buff), NULL, 0));
	}
	
	if(get_param("wait", buff)) {
	    
	    int waitpos = strtol(get_value("wait", buff), NULL, 0);
	    if(waitpos < 0) {
		//Le serveur est plein
		fprintf(stdout, "Le serveur indique que tous les slots sont occupés et la file d'attente pleine.\n");
		return -1;
	    }
	    else if(waitpos >= 0) {
		fprintf(stdout, "Vous êtes en file d'attente en position n°%d", waitpos + 1);
		return 0;
	    }
	}
	if(get_param("disconnect", buff)) {
	    fprintf(stdout, "Le serveur a coupé la connexion.");
	    return -1;
	}
    }
    return 0;
}

