/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   admin.c
 * Author: VertiDesk
 * 
 * Created on 7 février 2018, 01:49
 */

#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/un.h>
#include <alsa/asoundlib.h>
#include <time.h>

#include "defaults.h"
#include "multi.h"
#include "multistructure.h"
#include "admin.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

extern Server server;

SOCKET admin_init_socket() {
    char log_message[500];
    SOCKADDR_IN sin;
    //const char* sockName = "tmp/trxserver.sock";
    log_add("Administration socket initialisation...", stdout);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock == INVALID_SOCKET)
    {
	snprintf(log_message, 500, "socket() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("socket()");
	exit(errno);
	return -1;
    }
    
    //memset(&name, 0, sizeof(struct sockaddr_un));
    //name.sun_family = AF_UNIX;
    //strncpy(name.sun_path, sockName, sizeof(name.sun_path)-1);
    //unlink(name.sun_path);
    //name.sun_path[sizeof(name.sun_path) -1 ] = '\0';
    
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // nous sommes un serveur, nous acceptons n'importe quelle adresse
    sin.sin_family = AF_INET; //SOCKET LOCAL
    sin.sin_port = htons(DEFAULT_ADMIN_PORT); //port de communication
    
    if(bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
	snprintf(log_message, 500, "bind() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("bind()");
	exit(errno);
	return -1;
    }
    
    if(listen(sock, DEFAULT_MAX_ADMIN) == SOCKET_ERROR)
    {
	snprintf(log_message, 500, "listen() : %s", strerror(errno));
	log_add(log_message, 0);
	
	perror("listen()");
	exit(errno);
	return -1;
    }
    //generation du log

    snprintf(log_message, 500, "Admin socket %d initialized", sock);
    log_add(log_message, stdout);
    
    return sock;
}

static char* admin_serialize_clients(char* serial, Client* clients, int nbSlots) {
    char cstr[DEFAULT_COM_BUFFSIZE];
    int i, ip_len, name_len;
    snprintf(serial, DEFAULT_COM_BUFFSIZE, "a:%d:{", (nbSlots + DEFAULT_WAIT_LIST));
    for(i=0;i<(nbSlots+DEFAULT_WAIT_LIST);i++) {
	ip_len = strlen(clients[i].ip);
	name_len = strlen(clients[i].name);

	snprintf(cstr, DEFAULT_COM_BUFFSIZE, "i:%d;a:5:{s:2:\"ip\";s:%d:\"%s\";s:4:\"name\";s:%d:\"%s\";s:4:\"sock\";i:%d;s:11:\"connex_time\";i:%ld;s:4:\"rate\";i:%d;}",
			i, ip_len, clients[i].ip, name_len, clients[i].name, clients[i].sock, (long)clients[i].connex_time, clients[i].rate);
	strcat(serial, cstr);
    }
    
    strcat(serial, "}\n");
    
    return serial;
}

static char* admin_serialize_waitlist(char* serial, Client* waitlist[], Client* clients, int nb_slots) {
    char cstr[DEFAULT_COM_BUFFSIZE];
    int i;
    snprintf(serial, DEFAULT_COM_BUFFSIZE, "a:%d:{", DEFAULT_WAIT_LIST);
    for(i=0;i<DEFAULT_WAIT_LIST;i++) {
	if(waitlist[i] == NULL) {
	    snprintf(cstr, DEFAULT_COM_BUFFSIZE, "i:%d;N;", i);
	    strcat(serial, cstr);
	    continue;
	}
	int n;
	for(n=0;n<(nb_slots + DEFAULT_WAIT_LIST);n++){
	    if(waitlist[i] == &clients[n]) {
		break;
	    }
	}
	
	snprintf(cstr, DEFAULT_COM_BUFFSIZE, "i:%d;i:%d;",i, n);
	strcat(serial, cstr);
    }
    
    strcat(serial, "}\n");
    
    return serial;
}

static char* admin_serialize_slotlist(char* serial, Slot* slotlist, Client* clientlist, int nbSlots) {
    char cstr[DEFAULT_COM_BUFFSIZE];
    int i, clientID, n;
    snprintf(serial, DEFAULT_COM_BUFFSIZE, "a:%d:{", nbSlots);
    for(i=0;i<nbSlots;i++) {
	if(slotlist[i].client != NULL) {
	    for(n=0;n<(nbSlots + DEFAULT_WAIT_LIST);n++) {
		if(slotlist[i].client == &clientlist[n]) {
		    clientID = n;
		}
	    }
	    snprintf(cstr, DEFAULT_COM_BUFFSIZE, "i:%d;a:7:{s:3:\"pid\";i:%d;s:6:\"client\";i:%d;s:5:\"start\";i:%ld;s:4:\"port\";i:%u;s:6:\"device\";s:%d:\"%s\";s:4:\"buff\";i:%u;s:4:\"jitt\";i:%u;}",
						i, (int)slotlist[i].pid, clientID, (long)slotlist[i].start_time, slotlist[i].param.port, (int)strlen(slotlist[i].param.device), slotlist[i].param.device, slotlist[i].param.buffer, slotlist[i].param.jitter);
	    strcat(serial, cstr);
	}
	else {
	    snprintf(cstr, DEFAULT_COM_BUFFSIZE, "i:%d;a:4:{s:4:\"port\";i:%u;s:6:\"device\";s:%d:\"%s\";s:4:\"buff\";i:%u;s:4:\"jitt\";i:%u;}",
						i, slotlist[i].param.port, (int)strlen(slotlist[i].param.device), slotlist[i].param.device, slotlist[i].param.buffer, slotlist[i].param.jitter);
	    strcat(serial, cstr);
	}
    }
    
    strcat(serial, "}\n");
    
    return serial;
    //a:3:{i:0;a:3{s:4:"port";i:1350;s:6:"device";s:7:"default";s:4:"buff";i:16;}i:1;a:3{s:4:"port";i:1351;s:6:"device";s:7:"default";s:4:"buff";i:16;}i:2;a:3{s:4:"port";i:1352;s:6:"device";s:7:"default";s:4:"buff";i:16;}}
}

static char* admin_serialize_serverinfos(char* serial, Client* clients, int nbSlots) {
    int i,nb_clients = 0;
    for(i=0;i<(nbSlots + DEFAULT_WAIT_LIST);i++) {
	if(clients[i].sock != 0) {
	    nb_clients++;
	}
    }
    
    snprintf(serial, DEFAULT_COM_BUFFSIZE, "a:3:{s:3:\"pid\";i:%d;s:4:\"time\";i:%ld;s:7:\"clients\";i:%d;}\n", server.pid, server.start_time, nb_clients);
    
    return serial;
}

static int admin_kick_client(SOCKET socket) {
    socket_send(socket, "disconnect");
    return 1;
}

int admin_manage(SOCKET adminSock, Client* clients, Client* waitlist[], Slot* slots, int nbSlots) {
    char log_message[500];
    char serial[DEFAULT_COM_BUFFSIZE] = {0};
    
    //char admin_message[DEFAULT_COM_BUFFSIZE] = { 0 };
    int i, c = 0;
    fprintf(stdout, "Reception connexion admin \n");
    SOCKADDR_IN csin = { 0 };
    socklen_t sinsize = sizeof(csin);
    fd_set rdfs;
    SOCKET csock = accept(adminSock, (SOCKADDR *)&csin, &sinsize);
    struct timeval to;
    to.tv_sec = 3;
    to.tv_usec = 0;
    
    if(csock == SOCKET_ERROR) {
	snprintf(log_message, 500, "accept() : %s", strerror(errno));
	log_add(log_message, 0);
	perror("accept()");
	return 1;
    }
    
    do{
	c = 0;
	char comBuff[DEFAULT_COM_BUFFSIZE] = {0};
	FD_ZERO(&rdfs);
	FD_SET(csock, &rdfs);
	int result = select(csock+1, &rdfs, NULL, NULL, &to);
	if(result == -1)
	{
	   snprintf(log_message, 500, "select() : %s", strerror(errno));
	   log_add(log_message, 0);
	   perror("select()"); //Gestion d'erreur
	   exit(errno);
	}
	if(result == 0) {
	    break;
	}
	
	c = recv(csock, comBuff, 1024, 0);
	
	if(c>0) {
	    fprintf(stdout, "Receved: %s\n", comBuff);

	    if(get_param("kick_client", comBuff)) {
		SOCKET kickSock = strtol(get_value("kick_client", comBuff), NULL, 0);
		int kicked = 0;
		for(i=0; i<(nbSlots + DEFAULT_WAIT_LIST);i++) {
		    if(clients[i].sock == kickSock) {
			admin_kick_client(kickSock);
			kicked = 1;
			break;
		    }
		}
		if(kicked) {
		    socket_send(csock, "success\n");
		    fprintf(stdout, "Kick reussi \n");
		}
		else {
		    socket_send(csock, "fail\n");
		    fprintf(stdout, "Kick raté \n");
		}

	    }
	    
	    if(get_param("kick_all_clients", comBuff)) {
		for(i=0; i<(nbSlots + DEFAULT_WAIT_LIST);i++) {
		    if(clients[i].sock != 0) {
			admin_kick_client(clients[i].sock);
		    }
		}
		socket_send(csock, "All clients kicked \n");
	    }

	    if(get_param("get_clients", comBuff)) {
		admin_serialize_clients(serial, clients, nbSlots);
		socket_send(csock, serial);
	    }
	    
	    if(get_param("get_wait_list", comBuff)) {
		admin_serialize_waitlist(serial, waitlist, clients, nbSlots);
		socket_send(csock, serial);
	    }
	    
	    if(get_param("get_slot_list", comBuff)) {
		admin_serialize_slotlist(serial, slots, clients, nbSlots);
		socket_send(csock, serial);
	    }
	    
	    if(get_param("get_server_infos", comBuff)) {
		admin_serialize_serverinfos(serial, clients, nbSlots);
		socket_send(csock, serial);
	    }
	    if(get_param("shutdown", comBuff)) {
		return 2;
	    }
	}
    }while(c>0);

    //admin_serialize_waitlist(serial, waitlist);
    socket_close(csock);
    return 1;
}