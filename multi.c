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
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <alsa/asoundlib.h>
#include <time.h>

#include "defaults.h"
#include "multistructure.h"
#include "rx_start.h"
#include "admin.h"
#include "multi.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

unsigned int verbose = DEFAULT_VERBOSE;
Client tx_client;
Server server;
/* FONCTIONS GENERIQUES */

char* get_value(char* param, char* string) {
    int paramSize = 0, i = 0;
    char *paramPosition = NULL;
    char *paramValue = NULL;
    
    char stringCpy[50];
    strcpy(stringCpy, string);

    paramPosition = strstr(stringCpy, param);
    
    if(paramPosition == NULL) {
	return NULL;
    }
    
    paramSize = strlen(param);
    paramPosition += paramSize;
    while(paramPosition[0] == ' ') {
	paramPosition++;
    }
    paramValue = paramPosition;
    
    int n = strlen(paramValue);
    for(i=0; i < n; i++) {
	if(paramValue[i] == 32) {
	    break;
	}
    }
    paramValue[(i+1)] = '\0';
    return paramValue;
}

int get_param(char* param, char* string) {
    char *paramPosition = NULL;
    
    paramPosition = strstr(string, param);
    
    if(paramPosition != NULL) {
	return 1;
    }
    return 0;
}

static int self_name(char *progName) {
    
    char namebuff[1024] = {0};
    int i = 0, ret = 0;

    /* Recuperation du chemin vers l'executable */
    ret = readlink("/proc/self/exe", namebuff, sizeof(namebuff)-1);
    /* Repérage du nom du programme */
    i = ret;
    for(;i>=0; i--) {
	if(namebuff[i] == '/') break;	
    }
    
    /* Si on ne trouve rien */
    if(i == 0) { progName = "default" ; return 0; }

    /* Sinon on modifie la variable du pointeur progName avec le nom trouvé */
    i++;
    strncpy(progName, &namebuff[i], 50);
    
    return 1;
}
static int self_path(char *progPath) {
    
    char namebuff[1024] = {0};
    int i = 0, ret = 0;

    /* Recuperation du chemin vers l'executable */
    ret = readlink("/proc/self/exe", namebuff, sizeof(namebuff)-1);

    /* Repérage du nom du programme */
    i = ret;
    for(;i>=0; i--) {
	if(namebuff[i] == '/') {
	    namebuff[(i+1)] = '\0';
	    break;	
	}
    }
    snprintf(progPath, 50, "%s%s", &namebuff, DEFAULT_LOGPATH);
    return 1;
}

void time_string(char* string, int type) {
    // JJ/MM/AAAA HH:MM:SS : = 23 characters
    time_t current_time;
    struct tm *local_time;
    
    current_time = time(NULL);
    local_time = localtime(&current_time);
    char sday[3],
	 smonth[3], 
	 syear[5], 
	 shour[3],  
	 sminuts[3], 
	 sseconds[3];
    
    /* format des numero à deux chiffre (ajouter un 0 si le nombre <10 */
    sprintf(sday, "%02d", local_time->tm_mday) ;
    sprintf(smonth, "%02d", local_time->tm_mon + 1) ;
    sprintf(syear, "%02d", local_time->tm_year+1900) ;
    sprintf(shour, "%02d", local_time->tm_hour) ;
    sprintf(sminuts, "%02d", local_time->tm_min) ;
    sprintf(sseconds, "%02d", local_time->tm_sec) ;
    
    
    if(type == 1) {
    snprintf(string, DEFAULT_TIME_LEN, "%s/%s/%s %s:%s:%s : ",
	    sday, smonth, syear, shour, sminuts, sseconds);
    }
    else if(type == 2) {
	snprintf(string, DEFAULT_TIME_LEN, "%s%s%s",
		syear, smonth, sday);
    }
}

void log_add(char* str, FILE* out) {
    char time_str[DEFAULT_TIME_LEN];
    char filepath[100] = {0};
    char progname[50];
    
    self_name(progname);
    time_string(time_str, 2); //date au format AAAAMMJJ
    //création du chemin du fichier type "logs/prognameAAAAMMJJ.log"
    //snprintf(filepath, 100, "%s%s%s.log", DEFAULT_LOGPATH, progname, time_str);
    strcat(progname, time_str);
    strcat(progname, ".log");
    self_path(filepath);
    strcat(filepath, progname);

    time_string(time_str, 1); //date au format JJ/MM/AAAA HH:MM:SS

    FILE* file = fopen(filepath, "a");
    chmod(filepath,0777);
    fprintf(file, time_str); //Ajout de la date
    
    fprintf(file, str); // 
    fprintf(file, "\n");
    
    /* si on a indiqué une sortie secondaire (stdout, stderr ou un fichier) */
    if(out != 0) {
	fprintf(out, str);
	fprintf(out, "\n");
    }
    fclose(file);
}

/* FONCTIONS SOCKET */

void socket_send(SOCKET sock, const char* data) {
    //char log_message[800];
    if(send(sock, data, strlen(data), 0) < 0)
    {
	perror("send()");
	exit(errno);
    }
    //snprintf(log_message, 800, "message \"%s\" sent on socket %d", data, sock);
    //log_add(log_message, 0);
}

int socket_read(SOCKET sock, char *buff) {
   int n = 0;

   if((n = recv(sock, buff, DEFAULT_COM_BUFFSIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }
   buff[n] = 0;

   return n;
}

void socket_close(SOCKET sock) {
    //char log_message[50];
    //snprintf(log_message, 50, "Socket %d closed", sock);
    //log_add(log_message, 0);
    
    close(sock);
}

int socket_close_all(SOCKET sock, Client* clients, int nbSlots) {
    int i = 0;
    for(i=0;i<nbSlots + DEFAULT_WAIT_LIST; i++) {
	socket_close(clients[i].sock);
    }
    socket_close(sock);
    
    return 1;
}

/* FONCTIONS CLIENT */

static Client* client_add(Client* clients, SOCKADDR_IN* csin, SOCKET sock, int nbSlots, char* buffer) {
    int i;
    Client newClient;
    if(get_param("name", buffer)) {
	snprintf(newClient.name, 100, get_value("name", buffer));
    }
    else {
	snprintf(newClient.name, 100, "Default");
    }
    if(get_param("rate", buffer)) {
	char value[10];
	snprintf(value, 50, get_value("rate", buffer));
	int rate = (int)strtol(value, NULL, 0);
	newClient.rate = rate;
    }
    else {
	newClient.rate = 0;
    }
    newClient.sock = sock;
    snprintf(newClient.ip, 16, inet_ntoa(csin->sin_addr));
    newClient.connex_time = time(NULL);
    
   
    for(i=0;i < (nbSlots + DEFAULT_WAIT_LIST); i++) {
        if(clients[i].sock == 0) {
            clients[i] = newClient;
            
            return &clients[i];
        }
    }
    
    return 0;
}

static int client_delete(Client* clientlist, Client* client, int nbSlots) {
    int i;

    for(i=0; i < (nbSlots + DEFAULT_WAIT_LIST); i++) {
        if(clientlist[i].sock == client->sock) {
            snprintf(clientlist[i].ip, 16, " ");
            snprintf(clientlist[i].name, 100, " ");
            socket_close(clientlist[i].sock);
	    clientlist[i].sock = 0;
            
            return 1;
        }
    }
    return 0;
}


/* FONCTIONS SLOT */

int slot_client_ask(SOCKET sock) {
    char buff[DEFAULT_COM_BUFFSIZE] = {0};
    char infos[DEFAULT_COM_BUFFSIZE] = {0};
    char log_message[200];
    
    snprintf(infos, DEFAULT_COM_BUFFSIZE, "name %s rate %d", tx_client.name, tx_client.rate);
    log_add("Sending slot query to server", stdout);
    socket_send(sock, infos); //CHANGER POUR TRANSMETTRE IP et autre infos
    
    fd_set rdfs;
    
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    
    log_add("Waiting for server response...", stdout);
    
    if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
    {
	snprintf(log_message, 200, "select1(): %s", strerror(errno));
	log_add(log_message, 0);
	exit(errno);
    }
    //socket_send(sock, infos);
    if(FD_ISSET(sock, &rdfs)) {
	int n = socket_read(sock, buff);
	if(n == 0 || n == -1) {
	    log_add("Server stopped", stdout);
	    return -1;
	}
	
	snprintf(log_message, 200, "Server response: %s", buff);
	log_add(log_message, 0);
	
	if(get_param("port", buff)) {
	    //lancement d'une instance au port indiqué
	    snprintf(log_message, 200, "Server info: slot free on port %s.", get_value("port", buff));
	    log_add(log_message, stdout);
	    
	    return(strtol(get_value("port", buff), NULL, 0));
	}
	if(get_param("wait", buff)) {
	    
	    int waitpos = strtol(get_value("wait", buff), NULL, 0);
            if(waitpos < 0) {
		//Le serveur est plein
		log_add("Server info: all slots are busy and waiting list is full.", stdout);
		return -1;
	    }
	    else if(waitpos >= 0) {
		//File d'attente en position  waitpos +1
		snprintf(log_message, 200, "Server info: all slots are busy, %d other clients in waiting list.", waitpos);
		log_add(log_message, stdout);
		return 0;
	    }
	}
    }  
    return -1;
}

static Slot* slot_check(Slot* slotList, int nbSlot) {
    int i = 0;
    
    for(i=0;i<nbSlot;i++) {
	if(slotList[i].client == NULL) {
	    return &slotList[i];
	}
    }
    return NULL;
}

static int slot_give(Slot* slot, Client* newClient) {
    char log_message[200];
    
    slot->client = newClient;
    server_start_rx(slot); //le pid est défini dans cette fonction
	    
    snprintf(log_message, 200, "Slot with port %d associated with socket %d", slot->param.port, slot->client->sock);
    log_add(log_message, 0);
    
    return 1;
}

static int slot_setfree(Slot* slot, Client* client, int nbSlots) {
    int i = 0;
    char log_message[200];    
    for(i=0; i < nbSlots; i++) {
        
	if(slot[i].client == client) {
	    slot[i].client = NULL;
	    
	    snprintf(log_message, 200, "Slot with port %d is now free", slot[i].param.port);
	    log_add(log_message, stdout);
	    
	    return 1;
	}
    }
    return 0;
}

/* FONCTIONS LISTE D'ATTENTE */

static int waitlist_addclient(Client* waitlist[], Client* client) {
    int i = 0;
    char log_message[200];
    for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	if(waitlist[i] == NULL) {
	    waitlist[i] = client;
	    snprintf(log_message, 200, "Client with socket %d was put in waiting list", client->sock);
	    log_add(log_message, stdout);
	    return i;
	}
    }
    return -1;
}

static int waiting_count(Client* waitlist[]) {
    int i = 0;
    for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	if(waitlist[i] == 0) return i;
    }
    return -1;
}

static int waiting_refresh(Client* waitlist[]) {
    int i =0;
    char message[DEFAULT_COM_BUFFSIZE];
    
    
    for(i=0; i < (DEFAULT_WAIT_LIST - 1); i++) {
	if(waitlist[i] == NULL) {
	        
	    waitlist[i] = waitlist[(i+1)];
	    waitlist[(i+1)] = NULL;
	    if(waitlist[i] != NULL) {
		snprintf(message, DEFAULT_COM_BUFFSIZE, "wait%d", i);
		socket_send(waitlist[i]->sock, message);
	    }
	}
    }
    return 1;
}

static int waiting_to_slot(Client* waitlist[], Slot* slots, int nbSlots) {
    int i = 0, changes = 0;
    char message[DEFAULT_COM_BUFFSIZE] = {0};
    char log_message[200];
    for(i=0;i<nbSlots;i++) {
	if(slots[i].client == NULL && waitlist[0] != NULL) {
	    slots[i].client = waitlist[0];
	    waitlist[0] = NULL;
	    snprintf(message, DEFAULT_COM_BUFFSIZE, "port%d", slots[i].param.port);
	    socket_send(slots[i].client->sock, message);
	    
	    snprintf(log_message, 200, "Client on socket %d was transfered from waiting list to slot with port %d", slots[i].client->sock, slots[i].param.port);
	    log_add(log_message, stdout);
	    
	    changes++;
	    
	    waiting_refresh(waitlist);
	    continue;
	}
    }
    return changes;
}


/* FONCTIONS CLIENT */

SOCKET client_connection_init(const char *ipaddress) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    struct hostent *hostinfo;
    char log_message[200];

    if(sock == INVALID_SOCKET)
    {
	snprintf(log_message, 500, "socket() : %s", strerror(errno));
	log_add(log_message, 0);
	perror("socket()");
	exit(errno);
    }
    
    snprintf(log_message, 200, "Connection to %s...", ipaddress);
    log_add(log_message, stdout);
    
    hostinfo = gethostbyname(ipaddress);
    if (hostinfo == NULL)
    {
       snprintf(log_message, 200, "Unknown host %s...", ipaddress);
       log_add(log_message, stdout);
       exit(EXIT_FAILURE);
    }
    
    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(DEFAULT_COM_PORT);
    sin.sin_family = AF_INET;
    
    
    if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
	snprintf(log_message, 500, "connect() : %s", strerror(errno));
	log_add(log_message, 0);
	perror("connect()");
	exit(errno);
    }

    snprintf(log_message, 200, "Connection to %s established", ipaddress);
    log_add(log_message, stdout);
    return sock;
}

int client_listen(SOCKET sock) {
    char buff[DEFAULT_COM_BUFFSIZE];
    char log_message[200];
    while(1) {
	fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);
 
	if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
	{
	    snprintf(log_message, 200, "select(): %s", strerror(errno));
	    log_add(log_message, stdout);
	    exit(errno);
	}
	
	int c = socket_read(sock, buff);
	
	if(c == 0 || c == -1) {
	    //Perte du serveur
	    log_add("Server lost.", stdout);
	    return -1;
	}
	if(get_param("port", buff)) {
	    //lancement d'une instance au port indiqué
	    snprintf(log_message, 200, "Server indicates to stream on port %d", (int)strtol(get_value("port", buff), NULL, 0));
	    log_add(log_message, stdout);
	    return(strtol(get_value("port", buff), NULL, 0));
	}
	
	if(get_param("wait", buff)) {	    
	    int waitpos = strtol(get_value("wait", buff), NULL, 0);
	    if(waitpos < 0) {
		//Le serveur est plein
		log_add("Server: all slots are taken and wait list is full", stdout);
		return -1;
	    }
	    else if(waitpos >= 0) {
		snprintf(log_message, 200, "You're in wait list on position %d", waitpos +1);
	        log_add(log_message, stdout);
		continue;
	    }
	}
	if(get_param("disconnect", buff)) {
	    log_add("Server shutted down your connection", stdout);
	    return -1;
	}
    }
    return 0;
}

/* FONCTIONS SERVEUR */

SOCKET server_connection_init(int nbClient) {
    char log_message[500];
    
    log_add("Socket initialisation...", stdout);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    
    if(sock == INVALID_SOCKET)
    {
	snprintf(log_message, 500, "socket() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("socket()");
	exit(errno);
	return -1;
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY); /* nous sommes un serveur, nous acceptons n'importe quelle adresse */
    sin.sin_family = AF_INET; //Protocole ipV4
    sin.sin_port = htons(DEFAULT_COM_PORT); //port de communication
    
    if(bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
	snprintf(log_message, 500, "bind() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("bind()");
	exit(errno);
	return -1;
    }
    
    if(listen(sock, nbClient + DEFAULT_WAIT_LIST) == SOCKET_ERROR)
    {
	snprintf(log_message, 500, "listen() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("listen()");
	exit(errno);
	return -1;
    }
    //generation du log

    snprintf(log_message, 500, "Socket %d initialized", sock);
    log_add(log_message, stdout);
    
    return sock;
}

void server_listen(SOCKET sock, int nb_slot, Slot* slots, Client* clients, SOCKET adminSock) {
    char comBuff[DEFAULT_COM_BUFFSIZE] = {0}; //Buffer de lecture
    int max = sock > adminSock ? sock : adminSock;
    
    char message[DEFAULT_COM_BUFFSIZE] = {0};
    char log_message[500];
    Client* waitlist[DEFAULT_WAIT_LIST] = { NULL };
    Client* client;
    int connectedAmt = 0;
    fd_set rdfs;
    
    snprintf(log_message, 500, "Started listening events on socket %d", sock);
    log_add(log_message, 0);
    
    while(1) {
	int i = 0, c = 0, x = 0;
	FD_ZERO(&rdfs);//on vide le fd
	FD_SET(adminSock, &rdfs); //on écoute le socket admin
	FD_SET(sock, &rdfs); //on écoute le socket principal
	
	for(i=0;i<nb_slot;i++) {
	    if(slots[i].client != NULL) { //Si un slot est occupé
		FD_SET(slots[i].client->sock, &rdfs); //On ajoute son socket a la lectru d'events
	    }
	}

	for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	    if(waitlist[i] != NULL) { //Si il y a du monde dans la file d'attente
		FD_SET(waitlist[i]->sock, &rdfs); //On ajoute son socket a la lecture d'events
	    }
	}
	
	if(select(max+1, &rdfs, NULL, NULL, NULL) == -1)
	{
	   snprintf(log_message, 500, "select() : %s", strerror(errno));
	   log_add(log_message, 0);
	   perror("select()"); //Gestion d'erreur
	   exit(errno);
	}
	
	
	//snprintf(log_message, 500, "Event detected");
	//log_add(log_message, stdout);
	
	if(FD_ISSET(sock, &rdfs)) { //Changement sur socket principal
	    
	    log_add("A client tries to connect", stdout);
	    
	    SOCKADDR_IN csin = { 0 };
	    socklen_t sinsize = sizeof(csin);
            
            SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            
            if(csock == SOCKET_ERROR) {
                snprintf(log_message, 500, "accept() : %s", strerror(errno));
                log_add(log_message, 0);
                perror("accept()");
                continue;
            }
            
            if(socket_read(csock, comBuff) == -1) {    
                //Le client s'est deconnecté
                log_add("Client disconnected", stdout);
                continue;
            }
            
	    Slot* freeSlot = slot_check(slots, nb_slot);
	    
	    
           // fprintf(stdout, "port: %d, %d \n", freeSlot->param.port, freeSlot->param.rate);
	    
	    if(freeSlot != NULL) { // Si on a de la place
                client = client_add(clients, &csin, csock, nb_slot, comBuff);
		if(slot_give(freeSlot, client)) {
		    
		    snprintf(message, DEFAULT_COM_BUFFSIZE, "port %d", freeSlot->param.port);
		    socket_send(client->sock, message);
		    
		    snprintf(log_message, 500, "Slot with port %d assigned to new client (socket %d) pid= %d", freeSlot->param.port, csock, freeSlot->pid);
		    log_add(log_message, stdout);
		    
		    connectedAmt++;
		    max = csock > max ? csock : max;
		    continue;
		}
	    }
	    else if(freeSlot == NULL) {
                int nb_waiting = waiting_count(waitlist);
		if(nb_waiting < DEFAULT_WAIT_LIST && nb_waiting >= 0 ) { //Si place sur la file d'attente
                    client = client_add(clients, &csin, csock, nb_slot, comBuff);
		    int position = waitlist_addclient(waitlist, client);
		    snprintf(message, DEFAULT_COM_BUFFSIZE, "wait %d", position);
		    socket_send(client->sock, message);
		    
		    
		    int d = socket_read(client->sock, comBuff);
		    if( d == -1 || d == 0 ) { //client deconnecté
			waitlist[position] = NULL;
			client_delete(clients, client, nb_slot);
			log_add("Client disconnected and removed from waiting list", stdout);
			waiting_refresh(waitlist);
			continue;
		    }
		     
		    max = csock > max ? csock : max;
		    
		    continue;
		}
		else {
		    socket_send(csock, "wait -1");
		    continue;
		}
	    }
	}
	if(FD_ISSET(adminSock, &rdfs)) {
	    int adm_rtn = admin_manage(adminSock, clients, waitlist, slots, nb_slot); 
	    
	    if(adm_rtn == 1) {
		continue;
	    }
	    if(adm_rtn == 2) {
		log_add("Server shutdown by admin interface", stdout);
		break;
	    }
	    
	}
	else {
	    for(i = 0; i < nb_slot; i++ ) {
		if(slots[i].client != NULL && FD_ISSET(slots[i].client->sock, &rdfs)) { //changemement sur un ou plusieurs socket des slots
		    client = slots[i].client;
		    c = socket_read(client->sock, comBuff);
		    if(c == 0 || c == -1 || get_param("disconnect", comBuff)) { //Le client s'est deconnecté
			
			snprintf(log_message, 200, "Client disconnected from port %d", slots[i].param.port);
			log_add(log_message, stdout);
                        client_delete(clients, client, nb_slot);
                        
			if(waiting_count(waitlist) == 0) {
                            for(i=0;i<nb_slot;i++) {
                                if(slots[i].client == client) {
                                    break;
                                }
                            }
			    
                            server_stop_rx(&slots[i]);
			    
                        }
                        
                        slot_setfree(slots, client, nb_slot);
                        
                        
			waiting_refresh(waitlist);
			waiting_to_slot(waitlist, slots, nb_slot);
			
			waiting_refresh(waitlist);
			
			
		    }
		    x = 1;
		    break;
		}
	    }
	    
	    if(x) { //Si on a eu changement sur un des slots, on redémarre la boucle à 0
		continue;
	    }
	    
	    for(i = 0; i < DEFAULT_WAIT_LIST; i++ ) {
		if(FD_ISSET(waitlist[i]->sock, &rdfs)) {
		    client = waitlist[i];
		    c = socket_read(client->sock, comBuff);

		    if(c == 0 || c == -1 || get_param("disconnect", comBuff)) { //Le client s'est deconnecté
			waitlist[i] = NULL;
			client_delete(clients, client, nb_slot);
			waiting_refresh(waitlist);

			snprintf(log_message, 200, "Client of waiting list disconnected\nStill %d client(s) in wait list", waiting_count(waitlist));
			log_add(log_message, stdout);

			break;
		    }
		    else {
			/* Gestion des messages client */
		    }
		}
	    }
	}
    } 	 
}
