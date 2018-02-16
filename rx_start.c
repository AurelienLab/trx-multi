/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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

extern unsigned int verbose;

static void timestamp_jump(RtpSession *session, ...)
{
	if (verbose > 1)
		fputc('|', stderr);
	rtp_session_resync(session);
}

static int play_one_frame(void *packet,
		size_t len,
		OpusDecoder *decoder,
		snd_pcm_t *snd,
		const unsigned int channels)
{
	int r;
	float *pcm;
	snd_pcm_sframes_t f, samples = 1920;

	pcm = alloca(sizeof(float) * samples * channels);

	if (packet == NULL) {
		r = opus_decode_float(decoder, NULL, 0, pcm, samples, 1);
	} else {
		r = opus_decode_float(decoder, packet, len, pcm, samples, 0);
	}
	if (r < 0) {
		fprintf(stderr, "opus_decode: %s\n", opus_strerror(r));
		return -1;
	}

	f = snd_pcm_writei(snd, pcm, r);
	if (f < 0) {
		f = snd_pcm_recover(snd, f, 0);
		if (f < 0) {
			aerror("snd_pcm_writei", f);
			return -1;
		}
		return 0;
	}
	if (f < r)
		fprintf(stderr, "Short write %ld\n", f);

	return r;
}

static int run_rx(RtpSession *session,
		OpusDecoder *decoder,
		snd_pcm_t *snd,
		const unsigned int channels,
		const unsigned int rate)
{
	int ts = 0;

	for (;;) {
		int r, have_more;
		char buf[32768];
		void *packet;

		r = rtp_session_recv_with_ts(session, (uint8_t*)buf,
				sizeof(buf), ts, &have_more);
		assert(r >= 0);
		assert(have_more == 0);
		if (r == 0) {
			packet = NULL;
			if (verbose > 1)
				fputc('#', stderr);
		} else {
			packet = buf;
			if (verbose > 1)
				fputc('.', stderr);
		}

		r = play_one_frame(packet, r, decoder, snd, channels);
		if (r == -1)
			return -1;

		/* Follow the RFC, payload 0 has 8kHz reference rate */

		ts += r * 8000 / rate;
	}
}

static RtpSession* create_rtp_recv(const char *addr_desc, const int port,
		unsigned int jitter)
{
	RtpSession *session;

	session = rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_scheduling_mode(session, FALSE);
	rtp_session_set_blocking_mode(session, FALSE);
	rtp_session_set_local_addr(session, addr_desc, port, -1);
	rtp_session_set_connected_mode(session, FALSE);
	rtp_session_enable_adaptive_jitter_compensation(session, TRUE);
	rtp_session_set_jitter_compensation(session, jitter); /* ms */
	rtp_session_set_time_jump_limit(session, jitter * 16); /* ms */
	if (rtp_session_set_payload_type(session, 0) != 0)
		abort();
	if (rtp_session_signal_connect(session, "timestamp_jump",
					timestamp_jump, 0) != 0)
	{
		abort();
	}

	return session;
}

int server_start_rx(Slot* slot) {
	    pid_t rxpid;
	    RtpSession *session;
	    OpusDecoder *decoder = slot->param.decoder;
	    snd_pcm_t *snd;
	    
	    int r;
	    const char *device = slot->param.device,
		*addr = slot->param.addr,
		*pid = slot->param.pid;
	    unsigned int buffer = slot->param.buffer,
		rate = slot->param.rate,
		jitter = slot->param.jitter,
		channels = slot->param.channels,
		port = slot->param.port;
		    
	    rxpid = fork(); //nouveau processus
	    slot->pid = rxpid;
	    slot->start_time = time(NULL);
	    if(rxpid == -1) {
		/* Erreur */
	    }
	    else if(rxpid == 0) { //On est dans le processus fils
		    //Creation d'une session de lecture
		
		session = create_rtp_recv(addr, port, jitter); //1 Session oRTP créée
		assert(session != NULL);

		r = snd_pcm_open(&snd, device, SND_PCM_STREAM_PLAYBACK, 0); //Ouverture d'une liaison carte son
		if (r < 0) {
			aerror("snd_pcm_open", r);
			return -1;
		}
		if (set_alsa_hw(snd, rate, channels, buffer * 1000) == -1)
			return -1;
		if (set_alsa_sw(snd) == -1)
			return -1;

		if (pid)
			go_daemon(pid); //Mise du recepteur en daemon si presence pid (Pere / fils ?)

		go_realtime();
		r = run_rx(session, decoder, snd, channels, rate); // boucle infinie de lecture



		if (snd_pcm_close(snd) < 0)
			abort(); //Si erreur de liaison carte son, fin de la boucle

		rtp_session_destroy(session); //Fin de la session oRTP

	    //Fin de la session oRTP
	    }
	return 1;
}

int server_stop_rx(Slot* slot) {
    
    kill(slot->pid, SIGTERM);
    slot->pid = 0;
    slot->start_time = 0;
    
    return 1;
}