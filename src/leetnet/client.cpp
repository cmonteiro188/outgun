/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 */

/*

	a network game client engine 

*/

// ***** FORTIFY !!! *****

#include "../fortfy22/fortify.h"

// ***** FORTIFY !!! *****


#include "leetnet.h"

#include "client.h"

#include "nl.h"

#include "pthread.h"
#include "sched.h"

#include "rudp.h"

#include "sleep.h"


#define LOG_NOLOG		//disable logging
#define LOG_EXPR client_c_log
#define LOG_TIMEFUNC get_timeh()
#include "log.h"
FILE *client_c_log;


//connector thread function
void *thread_connect_f(void *arg);
void *thread_disconnect_f(void *arg);

//reader thread function
void *thread_reader_f(void *arg);


//the client class
class client_ci : public client_c {
public:

	//the callbacks
	client_callback_t		gamecfunc[NUM_OF_CFUNC];

	//the server address
	NLaddress		serveraddr;

	//client station
	station_c		*station;

	//connection state
	volatile bool	want_connect;			//yes/no
	volatile int	connect_status;		//0=not connected 1=trying disconnection 2=trying connection 2=connected
	int	tries_left;				//tries left

	bool			started_disconnection;		//if disconnection was started by the client

	//the connecter/disconnecter threads
	pthread_t		thread_connect;
	pthread_t		thread_disconnect;

	//the reader thread
	pthread_t		thread_read;

	//quit reader thread?
	bool				quit_reader_thread;

	//connect data
	char			connect_data[4096];
	int				connect_data_length;

	//------------------------------
	//  GAME CLIENT API
	//------------------------------

	//set a callback function.
	virtual void set_callback(int callback_id, client_callback_t callback_function) {
		if (callback_id < 0) return;
		if (callback_id >= NUM_OF_CFUNC) return;
		gamecfunc[callback_id] = callback_function;
	}
	
	//set the server's address. call before connect()
	virtual void set_server_address(char *address) {
		nlStringToAddr(address, &serveraddr);
	}

	//set the custom data sent with every connection packet
	//gameserver will interpret it by server_c's SFUNC_CLIENT_HELLO callback
	virtual void set_connect_data(char *data, int length) {
		connect_data_length = length;
		memcpy(connect_data, data, length);
	}

	//set connection status. if set to TRUE, engine will try to estabilish connection
	//with the server. if set to FALSE, will stop trying to connect or will disconnect
	//results are returned in the CFUNC_CONNENCTION_UPDATE callback
	virtual void connect(bool yes) {
		
		LOG3("connect now=%i  set to=%i   constatus=%i\n", want_connect, yes, connect_status);

		//noop
		if (want_connect == yes) return;

		//changed
		want_connect = yes;

		//want to connect
		if (want_connect) {
			if (connect_status == 0) {

				LOG("starting connect sequence.\n");

				//start connection sequence
				start_connect();
			}
			else if (connect_status == 1) {

				LOG("wil star connect sequence.\n");
				
				//trying disconnection -- wait until after it's done
				//this is just a hack
				while (connect_status == 1) {	
					MS_SLEEP(500);	// *** NO CPU PROBLEM HERE ***
				}

				LOG("starting connect sequence.\n");

				//now connect normally
				start_connect();
			}
		}
		//want to disconnect
		else {
			if (connect_status == 3) {

				LOG("starting disconnect seq...\n");

				//start disconnecting
				start_disconnect();

				//join with disconnector thread
				LOG("joining disconnect thread...\n");
				pthread_join(thread_disconnect, 0);
				LOG("disconnect thread joined.\n");

				//REVIEW: additional cleanup, must enable
				//        new connections later ? FIXME
			}
			else if (connect_status == 2) {

				LOG("stop_connect() - gave up connecting..\n");
				
				//trying connection - just stop trying. if it gets accepted, we will reply again telling to disconnect
				stop_connect();
			}
		}
	}

	//send reliable message
	virtual void send_message(char *data, int length) {

		//send to the station
		station->writer(data, length);

	}

	//dispatches the packet with the given frame (unreliable data) and all the
	//protocol overload (reliable messages, acks...)
	virtual void send_frame(char *data, int length) {

		//do not send if not connected
		if (connect_status != 3)
			return;

		if (length > 0)
			station->write(data, length);
		int packet_id;
		station->send_packet(packet_id);
	}

	//function to be called by the CFUNC_SERVER_DATA callback
	//gets the next reliable message avaliable from the server. null if no message pending
	virtual char* receive_message(int *length) {

		data_c	*msg = station->read_reliable();

		// no data
		if (msg == 0) {
			(*length) = 0;
			return 0;
		}
		
		//return the message or 0
		(*length) = msg->getlen();
		return msg->getbuf();
	}

	//get a statistic from the socket. stat = HawkNL socket-stats id
	virtual int get_socket_stat(int stat) {

		return nlGetSocketStat(station->get_nl_socket(), stat);
	}

	//------------------------------
	//  internal stuff
	//------------------------------

	//READER thread wants to read from the station
	int read_station(char *buf, int bufsize) {
		if (!station) {
			LOG("WOW REALLY FUCKED UP!!\n");
			return -666;
		}
		else {

			//LOG1("ST=%s\n", station->debug_info());

			return station->receive_packet(buf, bufsize);
		}
	}

	//start disconnection sequence
	void start_disconnect() {

		LOG("start_disconnect()\n");

		//start thread
		started_disconnection = true;		// client started it
		connect_status = 1;	//trying nice disconnection
		tries_left = 10;		//try 10 times
		pthread_create(&thread_disconnect, 0, thread_disconnect_f, (void *)this);
	}

	//disconnect try - return TRUE to stop
	bool disconnect_try() {

		LOG("client disconnect_try()\n");
		
		// check stopped trying to disconnect
		if (connect_status != 1)
			return true;

		// check tries
		if (tries_left-- <= 0)
			return true;

		LOG("client trying (disconnect)...\n");

		// send disconnect packet via station!
		//
		data_c  *dat = new_data_c();
		dat->addlong(0); //special packet
		dat->addlong(2); //disconnect ACK
		station->send_raw_packet(dat);  // FIXME: deal with send erros?
		delete dat;

		//stop now if done
		return (tries_left <= 0);
	}

	//disconnection done (timeout or reply received)
	void nice_disconnect_done() {
		
		LOG("nice_disconnect_done() - delete station - connectstatus = 0\n");

		//connection callback w/ status = 1 (disconnected)
		//FIXME: DISCARDING EXTRA DATA ON THE INCOMING DISCONNECT PACKET (nao tem nada mesmo...)
		//
		client_runes_t args;
		args.connect_result = 1;
		args.data = 0;		//data + 8;	//skip 0,3
		args.length = 0;	//length - 8;	//skip 0,3
		gamecfunc[CFUNC_CONNECTION_UPDATE](&args);

		//quit reader thread
		quit_reader_thread = true;
		pthread_join(thread_read, 0);

		//close the socket/station
		station->reset_state();
		/*
		if (station) {
			delete station;
			station = 0;
		}
		*/

		//set var to not connected anymore (THIS MUST BE THE LAST THING!! ou nao? :-)
		connect_status = 0;
		want_connect = false;		//this var sucks
	}

	//start connection sequence
	void start_connect() {

		//trying. this should be the FIRST THING!
		int old_status = connect_status;
		connect_status = 2;		

		LOG("start_connect()\n");

		//CLEAR station
		//station = new_station_c();
		station->reset_state();

		//set station remote address (opens the client's ONLY socket)
		//char adrstr[NL_MAX_STRING_LENGTH];
		//nlAddrToString(&serveraddr, adrstr);
		//station->set_remote_address(adrstr);
		if (station->set_remote_address(&serveraddr) == 0) {
			LOG("start_connect() ERROR: SET_REMOTE_ADDRESS RETURNED == 0!!!\n");
			connect_status = old_status;	//no idea if this is needed...
			return;
		}

		//create reader thread
		quit_reader_thread = false;
		pthread_create(&thread_read, 0, thread_reader_f, (void *)this);
		
		//start connection tries
		started_disconnection   = false;		//init "started_disconnection" flag for this connection session
		tries_left = 4;					//number of tries
		pthread_create(&thread_connect, 0, thread_connect_f, (void *)this);
	}

	//cleanup connect sequence
	void stop_connect() {

		//disconnected state -- THIS MUST BE THE FIRST THING
		connect_status = 0;	

		LOG("stop_connect() - deletes station - connect status = 0\n");

		//quit reader thread
		quit_reader_thread = true;
		pthread_join(thread_read, 0);

		//delete station -- CLOSES the socket
		station->reset_state();
		/*
		if (station) {
			delete station;
			station = 0;
		}
		*/

		//cancel
		want_connect = false;	//you don't want to connect anymore
	}

	//process datagram read by reader thread
	void process_incoming_datagram(char *udp_data, int udp_length) {

		//DEBUG
		int fubar = 0;
		NLulong l1, l2;
		readLong(udp_data, fubar, l1);		//discard the "0"
		readLong(udp_data, fubar, l2);
		LOG3("CLIENT INCOMING size=%i (%i, %i)\n", udp_length, l1,l2);

		//set datagram
		station->set_incoming_packet(udp_data, udp_length);

		//process data
		int length;
		bool special;
		char *data = station->process_incoming_packet(&length, &special);

		//special packet? (connection accepted/rejected , disconnected ...)
		if (special) {
			
			NLulong code;
			int count = 0;
			readLong(data, count, code);		//discard the "0"
			readLong(data, count, code);
			
			// ping request - send a reply immediately
			if (code == 666) {
				data_c *dat = new_data_c();
				dat->addlong(0);
				dat->addlong(666);		//pong!
				station->send_raw_packet(dat);
				delete dat;
			}
			// connection refused by special motive: engine server doesn't support
			// additional clients. this is similar to a "server full" custom
			// connection denied message from the gameserver ( 0/4/custom ), but in this case the
			// connection fails before the gameserver has right to any opinion on the matter.
			// it's a net-engserver limitation.
			else if (code == 201) {

				//this is similar to  (CODE == 4)  case below
				//------------------------------------------------

				//check if still trying
				if (connect_status == 2) {

					//connection callback w/ status = 2 (failed)
					//also handle the rest of the packet to the gameclient
					client_runes_t args;
					args.connect_result = 4;		// denied-by-engserver-full
					args.data = 0;		// no custom data
					args.length = 0;
					gamecfunc[CFUNC_CONNECTION_UPDATE](&args);
				}

				//stop connect - also quits reader thread
				stop_connect();
				
				//not trying anymore -- done by call above
				//connect_status = 0;	//dead
			}
			// disconnected
			else if (code == 2) {

				LOG2("special packet 0,2 arrived my connect_status == %i started_disc = %i\n", connect_status, started_disconnection);

				// if was not already disconnected
				//if (connect_status != 0) {
				
					//connection callback w/ status = 1 (disconnected)
					//also handle the rest of the packet to the gameclient
					//
					// MOVIDO PARA NICE_DISCONNECT_DONE()

					//disconnected state
					connect_status = 0;
				//}

				//if I didn't start it, send reply
				//send raw by station!!!
				if (!started_disconnection) {

					data_c  *dat = new_data_c();
					dat->addlong(0); //special packet
					dat->addlong(2); //disconnect ACK
					station->send_raw_packet(dat);  // FIXME: deal with send erros?
					delete dat;

					//REMENDÃO: o cliente se suicida assim que recebe noticia que o server
					//				  quer detonar ele
					// TEM QUE CHAMAR NICE_DISCONNECT_DONE porque é esse cara que chama 
					//   o callback "client disconnected!"
					nice_disconnect_done();					
				}
			}
			// connection accepted
			else if (code == 3) {

				// check if callback called already
				if (connect_status != 3) {

					//connection callback w/ status = 0  (connected)
					//also handle the rest of the packet to the gameclient
					client_runes_t args;
					args.connect_result = 0;
					args.data = data + 8;	//skip 0,3
					args.length = length - 8;	//skip 0,3
					gamecfunc[CFUNC_CONNECTION_UPDATE](&args);
					
					//connected!
					connect_status = 3;
				}
			}
			// connection rejected
			else if (code == 4) {

				//check if still trying
				if (connect_status == 2) {

					//connection callback w/ status = 2 (failed)
					//also handle the rest of the packet to the gameclient
					client_runes_t args;
					args.connect_result = 2;
					args.data = data + 8;	//skip 0,4
					args.length = length - 8;	//skip 0,4

					LOG2("INCOMING 0,4 REJECTION length = %i   argslength(game)=%i\n", length, args.length);

					gamecfunc[CFUNC_CONNECTION_UPDATE](&args);
				}

				//stop connect - also quits reader thread
				stop_connect();
				
				//not trying anymore -- done by call above
				//connect_status = 0;	//dead
			}
			//wtf?
			else {
					//FIXME: error
				LOG("WTF!! 777 666 !!!!\n");
			}

		}
		//a regular packet from the server
		else {

			//int count = 0;
			//NLulong a,b,c;
			//readLong(udp_data, count, a);
			//readLong(udp_data, count, b);
			//readLong(udp_data, count, c);

			//LOG("not a special packet. %i : %i %i %i\n",udp_length,a,b,c);

			//if client already disconnecting -- discard
			//else:
			if (want_connect == true) {
			
				//callback gameclient 
				client_runes_t		args;
				args.data = data;
				args.length = length; 
				gamecfunc[CFUNC_SERVER_DATA](&args);
			}
		}
	}

	//called by thread_connect - try to connect. return true if should stop trying
	bool connect_try() {
		
		//send a "want to connect" packet
		//FIXME: game client must specify data to send in the connection packet
		//			 (like client version etc.)

		LOG("connect_try()\n");

		//test if already connected
		//
		if (connect_status != 2)
			return true;		// stop trying, already connected OR disconnected (?)

		//test enough tries
		//
		if (tries_left-- <= 0) {

			//stop connection
			stop_connect();

			//"no response"
			client_runes_t args;
			args.connect_result = 3;
			gamecfunc[CFUNC_CONNECTION_UPDATE](&args);
		
			//stop trying
			return true;	
		}

		char adst[333];
		char remadst[333];
		NLaddress ladr;
		NLaddress radr;
		nlGetLocalAddr( (station->get_nl_socket()), &ladr );
		nlGetRemoteAddr( (station->get_nl_socket()), &radr );
		nlAddrToString( &ladr , adst );
		nlAddrToString( &radr , remadst );

		LOG2("trying... local = '%s' remote = '%s'\n", adst, remadst);
	 
		//send the packet
		data_c  *dat = new_data_c();
		dat->addlong(0); //special packet
		dat->addlong(1); //want to connect
		dat->addlong( ((NLulong)LEETNET_VERSION) );		// LEETNET protocol/build version - must match server's

		//custom data?
		dat->addlong(connect_data_length);		//amound of customdata, goes anyway
		if (connect_data_length > 0)
			dat->add(connect_data, connect_data_length);

		station->send_raw_packet(dat);  // FIXME: deal with send erros?
		delete dat;

		//keep trying
		return false;
	}

	//called by reader thread to check for quit condition
	bool reader_thread_quit() {
		return quit_reader_thread;
	}

	//ctor
	client_ci() {
		station = 0;
		want_connect = false;
		connect_status = 0;
		connect_data_length = 0;

		started_disconnection = false;		//if disconnection was started by the client
		quit_reader_thread = false;
		connect_data_length = 0;

		LOG_OPEN("client_c.log");
		
		//station agora tem reset_state(), entao cria no construtor e deleta no destrutor
		station = new_station_c();
	}

	//dtor
	virtual ~client_ci() {

		//disconnect if connected
		connect(false);

		//delete station
		if (station) {
			delete station;
			station = 0;
		}

		//close logs
		LOG_CLOSE();
	}
};


//connector thread function
void *thread_connect_f(void *arg) {

	LOG("THREAD CONNECT STARTED\n");

	//arg is client
	client_ci *client = (client_ci *)arg;

	//repeat
	bool stop = false;
	while (!stop) {

		//try
		stop = client->connect_try();

		//sleep a bit before sending next try
		MS_SLEEP(1000); // *** NO CPU PROBLEM HERE ***
	}

	LOG("THREAD CONNECT QUITTING\n");

	//exit
	pthread_exit(0);
	return 0;
}

//disconnector thread function
void *thread_disconnect_f(void *arg) {

	LOG("THREAD DISCONNECT STARTED\n");

	//arg is client
	client_ci *client = (client_ci *)arg;

	//repeat
	bool stop = false;
	while (!stop) {

		//try
		stop = client->disconnect_try();

		//sleep a bit before sending next try
		MS_SLEEP(100); // *** NO CPU PROBLEM HERE ***
	}

	//nice disconnect done
	client->nice_disconnect_done();

	LOG("THREAD DISCONNECT QUITTING\n");

	//exit
	pthread_exit(0);
	return 0;
}

//reader thread function
#define THREAD_READER_BUFSIZE 8192
void *thread_reader_f(void *arg) {

	LOG("READER_STARTED\n");

	//arg is client
	client_ci *client = (client_ci *)arg;

	//read buffer
	char	buffer[THREAD_READER_BUFSIZE];
	NLint amount; //amount read
	
	while (1) {

		//read from socket
		amount = client->read_station(buffer, THREAD_READER_BUFSIZE); //nlRead(clsock, buffer, THREAD_READER_BUFSIZE);

		//LOG1("R=%i\n", amount);

		// test quit
		if (client->reader_thread_quit()) 
			break;

		// if no data, keep reading
		while (amount == 0) {

			//sleep a bit
			MS_SLEEP(2);  //alternativa, usar BLOCKING I/O
			   
			//read from socket
			amount = client->read_station(buffer, THREAD_READER_BUFSIZE); //nlRead(clsock, buffer, THREAD_READER_BUFSIZE);

			//LOG1("R=%i\n", amount);

			// test quit
			if (client->reader_thread_quit()) {
				LOG("READER QUITTING!!!!\n");
				//exit reader
				pthread_exit(0);
				return 0;
			}
		}

		// check for error
		if (amount == NL_INVALID) {
			//DEBUG FIXME: error in nlGetError
			static volatile int menosmenos = 0;
			if (menosmenos == 0) {
				LOG("CLIENT READER: NL_INVALID!!\n");
			}
			menosmenos++;
			if (menosmenos > 3000)
				menosmenos = 0;
		}
		// process da packet
		else {
			//SLEEP(50); // lag

			client->process_incoming_datagram(buffer, amount);
		}
	}

	LOG("READER QUITTING!!!!\n");

	//exit
	pthread_exit(0);
	return 0;
}



//factory
client_c		*new_client_c() {
	client_ci	*x = new client_ci();
	return x;
}
