/*
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 *  Modified by Niko Ritari 2003
 */

/*

	a network game client engine

*/


#ifndef _client_h_
#define _client_h_



// runes supplied as parameters by client_c for it's callbacks
struct client_runes_t {

	int 		connect_result;	//connection status (for CFUNC_CONNECTION_UPDATE callback)
													//0 = connected
													//1 = disconnected
													//2 = connection failed (server denied + custom deny data)
													//3 = connection failed (no response from server)
													//4 = connection failed (server is full, no additional information)
	int			length;				// frame/message length (0 == empty/unused)
	char*		data;					// frame/message data		(0 == empty/unused)
};




// type for callback functions. returns an int and brings runes_t* as an argument
typedef int (*client_callback_t)(client_runes_t *);


// the game-client's callback functions
enum {

	CFUNC_CONNECTION_UPDATE,			//connection state has changed
	CFUNC_SERVER_DATA,  					//data arrived from server
	
	NUM_OF_CFUNC
};




//the client class
class client_c {
public:

	//set a callback function.
	virtual void set_callback(int callback_id, client_callback_t callback_function) = 0;	
	
	//set the server's address. call before connect()
	virtual void set_server_address(const char *address) = 0;

	//set the custom data sent with every connection packet
	//gameserver will interpret it by server_c's SFUNC_CLIENT_HELLO callback
	virtual void set_connect_data(char *data, int length) = 0;

	//set connection status. if set to TRUE, engine will try to estabilish connection
	//with the server. if set to FALSE, will stop trying to connect or will disconnect
	//results are returned in the CFUNC_CONNENCTION_UPDATE callback
	virtual void connect(bool yes) = 0;

	//send reliable message
	virtual void send_message(char *data, int length) = 0;

	//dispatches the packet with the given frame (unreliable data) and all the
	//protocol overload (reliable messages, acks...)
	virtual void send_frame(char *data, int length) = 0;

	//function to be called by the CFUNC_SERVER_DATA callback
	//gets the next reliable message avaliable from the server. null if no message pending
	virtual char* receive_message(int *length) = 0;

	//get a statistic from the socket. stat = HawkNL socket-stats id
	virtual int get_socket_stat(int stat) = 0;
};


//factory
client_c		*new_client_c();


#endif // _client_h_
