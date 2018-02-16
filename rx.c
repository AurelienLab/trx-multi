/*
 * Copyright (C) 2012 Mark Hills <mark@xwax.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <netdb.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <opus/opus.h>
#include <ortp/ortp.h>
#include <stdlib.h>
#include <signal.h> //Recupération et envoi de signaux entre processus

#include "defaults.h"
#include "device.h"
#include "notice.h"
#include "sched.h"
#include "multi.h"
#include "rx_start.h"
#include "admin.h"

extern Server server;
static void usage(FILE *fd) {
	fprintf(fd, "Usage: rx [<parameters>]\n"
		"Real-time audio receiver over IP\n");

	fprintf(fd, "\nAudio device (ALSA) parameters:\n");
	fprintf(fd, "  -d <dev>    Device name (default '%s')\n",
		DEFAULT_DEVICE);
	fprintf(fd, "  -m <ms>     Buffer time (default %d milliseconds)\n",
		DEFAULT_BUFFER);

	fprintf(fd, "\nNetwork parameters:\n");
	fprintf(fd, "  -h <addr>   IP address to listen on (default %s)\n",
		DEFAULT_ADDR);
	fprintf(fd, "  -p <port>   UDP port number (default %d)\n",
		DEFAULT_PORT);
	fprintf(fd, "  -j <ms>     Jitter buffer (default %d milliseconds)\n",
		DEFAULT_JITTER);
        fprintf(fd, "  -i <n>      Amount of receiver instances launched (default % instances)\n",
                DEFAULT_INSTANCES);

	fprintf(fd, "\nEncoding parameters (must match sender):\n");
	fprintf(fd, "  -r <rate>   Sample rate (default %dHz)\n",
		DEFAULT_RATE);
	fprintf(fd, "  -c <n>      Number of channels (default %d)\n",
		DEFAULT_CHANNELS);

	fprintf(fd, "\nProgram parameters:\n");
	fprintf(fd, "  -v <n>      Verbosity level (default %d)\n",
		DEFAULT_VERBOSE);
	fprintf(fd, "  -D <file>   Run as a daemon, writing process ID to the given file\n");
}

extern unsigned int verbose;

int main(int argc, char *argv[]) {
	char log_message[1024];
	//int r, i, error;
        int i, error;
	//snd_pcm_t *snd;
	OpusDecoder *decoder;
	//RtpSession *session;
	//pid_t rxpid;
	
	/* command-line options */
	const char *device = DEFAULT_DEVICE,
		*addr = DEFAULT_ADDR,
		*pid = NULL;
	unsigned int buffer = DEFAULT_BUFFER,
		rate = DEFAULT_RATE,
		jitter = DEFAULT_JITTER,
		channels = DEFAULT_CHANNELS,
		port = DEFAULT_PORT,
                instances = DEFAULT_INSTANCES;

	fputs(COPYRIGHT "\n", stderr);

	for (;;) {
		int c;

		c = getopt(argc, argv, "c:d:h:j:m:p:r:v:i:");
		if (c == -1)
			break;
		switch (c) {
		case 'c':
			channels = atoi(optarg);
			break;
		case 'd':
			device = optarg;
			break;
		case 'h':
			addr = optarg;
			break;
		case 'j':
			jitter = atoi(optarg);
			break;
		case 'm':
			buffer = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'r':
			rate = atoi(optarg);
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		case 'D':
			pid = optarg;
			break;
                case 'i': //nombre d'instances demandées par l'utilisateur au lancement du serveur
                        instances = atoi(optarg);
                        break;
		default:
			usage(stderr);
			return -1;
		}
	}
	
	server.start_time = time(NULL);
	server.pid = getpid();
	
	decoder = opus_decoder_create(rate, channels, &error);
	if (decoder == NULL) {
		fprintf(stderr, "opus_decoder_create: %s\n",
			opus_strerror(error));
		return -1;
	}
        Slot *slots = malloc(instances * sizeof(Slot)); //initialisation du tableau de ports
   
        int currentPort = port; //premier port demandé par l'utilisateur, on l'incrementera à chaque nouveau slot créé
        
        
	if(slots != NULL) {
	    for(i=0;i<instances;i++) { //création de la plage de ports selon le nombre d'instances demandé
		slots[i].pid = 0;
                slots[i].client = 0;
		
		slots[i].param.addr = addr;
		slots[i].param.buffer = buffer;
		slots[i].param.channels = channels;
		slots[i].param.decoder = decoder;
		slots[i].param.device = device;
		slots[i].param.jitter = jitter;
		slots[i].param.pid = pid;
		slots[i].param.port = currentPort;
		slots[i].param.rate = rate;
		
                
                currentPort++;
		
		
		sprintf(log_message, "Slot %d created on port %d", i+1, slots[i].param.port);
		log_add(log_message, stdout);
		
	    }
        }
	else {
	    log_add("Not enough memory", stderr);
            exit(0);
	}
        
        Client *clients = malloc((instances + DEFAULT_WAIT_LIST) * sizeof(Client)); //Tableau de clients
	
        if(clients != NULL) { 
            for(i=0; i < (instances + DEFAULT_WAIT_LIST) ; i++) {
                clients[i].sock =  0;
                snprintf(clients[i].ip, 16, " ");
                snprintf(clients[i].name, 100, " ");
		clients[i].rate = 0;
		clients[i].connex_time = 0;
            }
	}
        
        
	ortp_init(); //Initialisation oRTPS
	ortp_scheduler_init();
	SOCKET adminSock = admin_init_socket();
	SOCKET mainSock = server_connection_init(instances); //Creation du socket principal
	
	if(mainSock > 0) {
	    server_listen(mainSock, instances, slots, clients, adminSock);
	}
	
	socket_close_all(mainSock, clients, instances);
	
	for(i=0;i<instances;i++) { //fermeture de toutes les sessions
	    kill(slots[i].pid, SIGTERM);
	}
	
	ortp_exit();
	
	opus_decoder_destroy(decoder);
        
        free(slots);
	sprintf(log_message, "-- FERMETURE DU SERVEUR --");
		log_add(log_message, stdout);
	return 0;
}
