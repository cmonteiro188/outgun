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
 *  Copyright (C) 2002 - Fabio Reis Cecin
 *  Copyright (C) 2003, 2004 - Niko Ritari
 */

/*

	rudp: UDP with reliable message extensions, for action games

	requires HawkNL from htpp://www.hawksoft.com/hawknl

*/

#include "dlog.h"

#include "../mutex.h"
#include "../nassert.h"
#include "rudp.h"		
#include <nl.h>				// HawkNL
#include <pthread.h>

// buffer size limitations (stupid hardcoded but works)
//

#define EXTRA_RELIABLE_STORAGE
/* if EXTRA_RELIABLE_STORAGE is defined, reliable messages queue is split to two when more than MAXMSG messages are being sent
 * - the extra ones are separately stored in a queue
 * - only the first MAXMSG messages are transmitted until some of them receive acks
 * it should definitely be used if losing reliable messages can't be afforded
 */

#define MAXMSG				  64		// capacity of to-be-acked message buffer
#define ACKBUF				  64		// capacity of acks-to-send list (num of ulongs)
#define SENDS				  16		// size of sends queue
#define BIG_UDPBUF			8192		// a bigger UDP buffer
#define MAX_PACKET_SIZE		 256		// this is not a fixed limit; it just limits the sending of >1 reliable message

#define	MAX_INCOMING_MESSAGES	  64		// size of incoming msg buffer (64 is already overkill)
#define MAX_MESSAGE_SIZE		 256		// maximum size of a single reliable message (using more than this trashes the retransmit scheme anyways)


// 256 x 10 = 2560 = 2,5K/s
// obs: não tem controle se vai ser enviado 10 pacotes por segundo ou mais. ver caso do envio de
//      teclas pelo client

#ifdef EXTRA_RELIABLE_STORAGE
#include <queue>
#endif	


// gets the IP of this machine
//
void get_self_IP(NLaddress *addr) {
	NLsocket s = nlOpen(0, NL_UNRELIABLE);
	nlGetLocalAddr(s, addr);
	nlSetAddrPort(addr, 0);
	nlClose(s);
}


// data class implementation
//

#define DATA_BUF_SIZE 512			//increased from 512

class data_ci : public data_c {
public:
	
	//allocated length, used length
	int alen, ulen;

	//data buffer
	char buf[DATA_BUF_SIZE];

	//extend buffer to fit additional len
	void extend(int len) {
		if (len + ulen > DATA_BUF_SIZE) {
			throw 66677;
		}
		
		// not allocated yet
		/*
		if (alen == 0) {
			alen = 2048;
			buf = new char[alen];
		}
		// not enought allocated space : extend
		if (alen - ulen < len) {
			int nlen = alen + 2048;	// new len	
			char *buf2 = new char[nlen];	// alloc new
			memcpy(buf2, buf, ulen);	// copy old used to new
			if (buf) delete buf;		// delete old
			buf = buf2;		// buf points to new
			alen = nlen;	// new alloc size
		}
		*/
	}

	//add data
	void add(const void* data, int len) {
		if (len == 0) 
			return;
		extend(len);		// extend to fit
		memcpy(buf + ulen, data, len);	// copy data to (buf + alreay used len)
		ulen += len;		// increase used len
	}

	//add long: watch endianess
	void addlong(NLulong data) {
		extend(sizeof(NLulong)); // extend to fit
		writeLong(buf, ulen, data);
		//ulen += sizeof(NLulong);	//previous statemente already incs ulen
	}

	//set data
	void set(const data_c *datac) {
		const data_ci* data = dynamic_cast<const data_ci*>(datac);
		nAssert(data);
		ulen = 0;		// reset my contents
		extend(data->ulen);	// expand to fit other's used count
		ulen = data->ulen;		// my used = other used
		memcpy(buf, data->buf, ulen);		// copy other's data to my buffer
	}

	//set data
	void set(const char *data, NLshort len) {
		ulen = 0;
		extend(len);
		ulen = len;
		memcpy(buf, data, ulen);
	}

	//clear
	void clear() {
		ulen = 0;
		//alen = ulen = 0;
		//if (buf) { delete buf; buf = 0; }
	}

	//get data: use buf/alen/ulen with HawkNL packet reading functions
	char* getbuf() {
		return buf;
	}
	const char* getbuf() const {
		return buf;
	}

	//get length (used)
	int		getlen() const {
		return ulen;
	}

	//ctor
	data_ci() {
		//alen = ulen = 0;
		//buf = 0;
		alen = DATA_BUF_SIZE;
		ulen = 0;
	}

	//dtor
	virtual ~data_ci() {
		//if (buf) {
		//	delete buf; buf = 0;
		//}
	}
};

/*

	THIS GIVES MEMORY LEAKS 

	why?

class data_ci : public data_c {
public:
	
	//allocated length, used length
	int alen, ulen;

	//data buffer
	char* buf;

	//extend buffer to fit additional len
	void extend(int len) {
		// not allocated yet
		if (alen == 0) {
			alen = 2048;
			buf = new char[alen];
		}
		// not enought allocated space : extend
		if (alen - ulen < len) {
			int nlen = alen + 2048;	// new len	
			char *buf2 = new char[nlen];	// alloc new
			memcpy(buf2, buf, ulen);	// copy old used to new
			if (buf) delete buf;		// delete old
			buf = buf2;		// buf points to new
			alen = nlen;	// new alloc size
		}
	}

	//add data
	void add(void* data, int len) {
		if (len == 0) 
			return;
		extend(len);		// extend to fit
		memcpy(buf + ulen, data, len);	// copy data to (buf + alreay used len)
		ulen += len;		// increase used len
	}

	//add long: watch endianess
	void addlong(NLulong data) {
		extend(sizeof(NLulong)); // extend to fit
		writeLong(buf, ulen, data);
		//ulen += sizeof(NLulong);	//previous statemente already incs ulen
	}

	//set data
	void set(data_c *data) {
		ulen = 0;		// reset my contents
		extend(((data_ci*)data)->ulen);	// expand to fit other's used count
		ulen = ((data_ci*)data)->ulen;		// my used = other used
		memcpy(buf, ((data_ci*)data)->buf, ulen);		// copy other's data to my buffer
	}

	//set data
	void set(char *data, NLshort len) {
		ulen = 0;
		extend(len);
		ulen = len;
		memcpy(buf, data, ulen);
	}

	//clear
	void clear() {
		alen = ulen = 0;
		if (buf) { delete buf; buf = 0; }
	}

	//get data: use buf/alen/ulen with HawkNL packet reading functions
	char* getbuf() {
		return buf;
	}

	//get length (used)
	int		getlen() {
		return ulen;
	}

	//ctor
	data_ci() {
		alen = ulen = 0;
		buf = 0;
	}

	//dtor
	virtual ~data_ci() {
		if (buf) {
			delete buf; buf = 0;
		}
	}
};
*/

// a message record for the message buffer
//
class msgrec {
public:
	int			id;				// the message id, -1 = unused

	//data_ci sends;			
	NLulong		sends[SENDS]; // id of all packets that sent this message yet
	int				iter,sh,ss;				// sends circular queue iterator/head/size

	data_ci message;		// the message's contents

	//sends queue add
	void sends_add(NLulong x) {
		// not full yet
		if (ss <= SENDS) {
			sends[ss] = x; // set value
			ss++;	// size inc
		}
		// already full
		else {
			sh++;	// advance head & tail..
			if (sh >= SENDS) sh -= SENDS;	//..wrapping them around
		}
	}

	//sends queue clear
	void sends_clear() {
		sh = 0;
		ss = 0;
		iter = 0;
	}

	//sends queue getfirst FALSE if nothing else
	bool sends_first(NLulong *val) {
		iter = 0;
		return sends_next(val);
	}	

	//sends queue getnext FALSE if nothing else
	bool sends_next(NLulong *val) {
		// no elements
		if (ss == 0) return false;
		// if iter >= size, stop
		if (iter >= ss) return false;
		// index: head + iter
		int i = sh + iter;
		// wrap around
		if (i >= SENDS) i -= SENDS;
		// next iter
		iter++;
		// get
		(*val) = sends[i];
		return true;
	}

	msgrec() {
		id = -1;
		sends_clear();
	}
};

// station class implementation
//
class station_ci : public station_c {
public: 

	char debug[256];

	// send socket -- ALSO the receive socket now. 
	NLsocket sendsock;

	// the address set with set_remote_addr() -- must be set in the socket before all sending
	// cause receiving erases it
	NLaddress netaddr;

	// the packet buffer's unreliable data
	data_ci unreliable;

	// return buffer of read_reliable()
	data_ci reldata;

	// outgoing reliable messages id generator
	NLulong idgen_reliable_send;

	// outgoing packet id generator
	NLulong idgen_packet_send;

	// for process_incoming_packet()
	bool is_packet_set;

	// packets received with reliable messages that need acks to be sent
	NLshort acksinbuf;
	NLulong ackbuf[ACKBUF];

	//the UDP packet set
	char	udp_data[BIG_UDPBUF];
	int		udp_size;

	//the UDP send buffer
	char	sendbuf[BIG_UDPBUF];

	// INCOMING messages queue
	// - fixed size & circular (fastest insert & retrieve, waste memory but memory is cheap)
	// - "sliding window": can only always retrieve the next message in the number sequence ("current")
	// - too many messages: messages start to be ignored until app picks them up
	//   OR: receiving message greater than current+bufcap : discard
	// - old messages (< current) discarded
	// - dup messages : will be discovered when trying to put it into the queue (will already inserted)
	char			message[MAX_INCOMING_MESSAGES][MAX_MESSAGE_SIZE];
	int				message_size[MAX_INCOMING_MESSAGES];	//array of sizes. -1 == UNUSED
	NLulong		msg_current;		// current expected message id

	// the packet buffer's reliable messages (OUTGOING)
	// also works as the "messages yet to be acked" list
	// since all unacked messages are always sent, this includes those that were not sent a single time yet.
	msgrec reliable[MAXMSG];  // FIXME: access to "reliable" must be sinchronized
	NLbyte	reliable_count;		//count of reliable messages in buffer

	#ifdef EXTRA_RELIABLE_STORAGE
	NLulong reliable_size;	// total size of reliable messages in reliable[], plus 6 bytes extra for each
	std::queue<data_ci*> extra_reliables;
	pthread_mutex_t queue_mutex;
	void erase_extra_reliables() {
		while (!extra_reliables.empty()) {
			delete extra_reliables.front();
			extra_reliables.pop();
		}
	}
	bool can_add_reliable(NLulong msgsize) const { return reliable_size==0 || reliable_size + msgsize < MAX_PACKET_SIZE; }
	#endif

	// resets the state of the object. so you don't have to delete and create a new one
	// every time you want to use it for a different client/server.
	virtual void reset_state() {

		//disconnect the socket (if any)
		if (sendsock != NL_INVALID_SOCKET) {
			nlClose(sendsock);	
			sendsock = NL_INVALID_SOCKET;
		}		

		//reset msgrecs!
		for (int m=0;m<MAXMSG;m++) {
			reliable[m].id = -1;
			reliable[m].sends_clear();
		}
		#ifdef EXTRA_RELIABLE_STORAGE
		reliable_size = 0;
		erase_extra_reliables();
		#endif

		//reset all the internal state for object initialization or reuse
		sendbuf[0]=0;
		reldata.clear();
		unreliable.clear();
		udp_size = 0;
		is_packet_set = false;
		idgen_reliable_send = 1;
		idgen_packet_send = 1;
		acksinbuf = 0;
		reliable_count = 0;
		//clear incoming messages
		msg_current = 1;
		for (int i=0;i<MAX_INCOMING_MESSAGES;i++)
			message_size[i] = -1;	// unused
	}

	//ctor
	station_ci() {
		#ifdef EXTRA_RELIABLE_STORAGE
		pthread_mutex_init(&queue_mutex, 0);
		#endif
		sendsock = NL_INVALID_SOCKET;		//to avoid reset_state to close an invalid socket
		reset_state();
	}

	//dtor
	virtual ~station_ci() {
		#ifdef EXTRA_RELIABLE_STORAGE
		erase_extra_reliables();
		pthread_mutex_destroy(&queue_mutex);
		#endif
		
		//FIXME -- what else?

		//disconnect the socket
		if (sendsock != NL_INVALID_SOCKET) {
			nlClose(sendsock);	
			sendsock = NL_INVALID_SOCKET;
		}		
	}

	// process incoming reliable message from network
	// - discard duplicates and old messages
	// - implement "sliding window" for reliable message total ordering
	// messages are internally enqueued for later retrieval
	void process_incoming_message(NLulong msgid, char* msgdata, int msgsize) {
DLOG_Scope s("UPIM");
		//printf("process_incoming_message id=%i cur=%i siz=%i\n", msgid, msg_current, msgsize);

		//check message old discard 
		if (msgid < msg_current)
			return;

		//printf("not old\n");

		//check buffer(window) overflow
		if (msgid - msg_current >= MAX_INCOMING_MESSAGES)
			return;	//just drop it

		//calc index of the message in the array
		// 1st message is "1", arrays start at 0 (so, -1)
		int index = (msgid-1) % MAX_INCOMING_MESSAGES;

		//printf("not overflow index= %i\n", index);

		//check for duplicate discard
		if (message_size[index] != -1)
			return;

		//printf("not duplicate, saved at index.\n");
		//save the message
		message_size[index] = msgsize;
		memcpy(&(message[index][0]), msgdata, msgsize);
	}
	
	// set the station's remote address for sending (IP:PORT)
	virtual int set_remote_address(char* address) {
		//decode the address
		nlStringToAddr(address, &netaddr);

		//set address to socket remoteaddress
		nlOpenMutex.lock();
		nlDisable(NL_BLOCKING_IO);
		sendsock = nlOpen(0, NL_UNRELIABLE);
		nlOpenMutex.unlock();
		if (sendsock == NL_INVALID)
			return 0;		//ERROR
		else
			nlSetRemoteAddr(sendsock, &netaddr);
		//ok
		return 1;
	}

	// set the station's remote address for sending (IP:PORT)
	virtual int set_remote_address(NLaddress *some_addr) {
		//copy address
		memcpy(&netaddr, some_addr, sizeof(NLaddress));
	
		//set address to socket remoteaddress
		nlOpenMutex.lock();
		nlDisable(NL_BLOCKING_IO);
		sendsock = nlOpen(0, NL_UNRELIABLE);
		nlOpenMutex.unlock();
		if (sendsock == NL_INVALID)
			return 0;		//ERROR
		else
			nlSetRemoteAddr(sendsock, &netaddr);

		//ok
		return 1;
	}

	// read a reliable message from the queue
	virtual data_c *read_reliable() {
DLOG_Scope s("URR");

		//calc index of the message in the array
		// 1st message is "1", arrays start at 0 (so, -1)
		int index = (msg_current - 1) % MAX_INCOMING_MESSAGES;

		//printf("read_reliable index=%i\n", index);

		//check if current is loaded yet
		if (message_size[index] == -1)
			return 0;		//not yet

		//printf("msg ready!\n");

		//message present - create return data
		reldata.set(&(message[index][0]), (NLshort)message_size[index]);

		//clear msg (slide window)
		message_size[index] = -1;
		
		//update msg_current (slide window)
		msg_current++;
		
		//return data
		return (data_c*)(&reldata);
	}

	// sets UDP raw packet that arrived from network
	virtual int set_incoming_packet(char *data, int size) {
DLOG_Scope s("USIP");

		// ok!
		is_packet_set = true;

		memcpy(udp_data, data, size);
		udp_size = size;
		return 1; //ok
	}

	// process UDP raw packet that was set in set_incoming_packet
	// returns data block pointer and size of block for the unreliable
	// data block of the packet
	// returns special == true if it's a "connection" packet (id == 0)
	// returns 0/length==-1 if called twice before calling set_incoming_packet again
	virtual char* process_incoming_packet(int *size, bool *special) {
DLOG_Scope s("UPIP");

		int i;

		//error: must set packet first
		//
		if (!is_packet_set) {
			(*size) = -1;
			(*special) = false;
			return 0;
		}

		// must set packet again next time
		//
		is_packet_set = false;


		// (1) parse the packet:
		//
		// NLulong										packet_id
		// NLshort										number of acks
		// NLulong[number of acks]		list of acked just received packet_ids
		// NLbyte											number of reliable messages
		// for each reliable message:
		//    NLulong									message id
		//		NLshort									message size
		//		NLbyte[message size]		the reliable message data
		// (FIX: NLshort										unreliable data size --- inferido do packet size!!!)
		// NLbyte[unreliable data size]					all the unreliable data glued in a big chunk
		//

		// list of acks in the packet. they are used just in this function
		NLulong packet_acks[256];
		int			num_packet_acks;
		NLint count = 0;  //packet parse count
		NLulong packet_id;  //the packet id

//		int debug = 1;

		//parse packet id
		readLong(udp_data, count, packet_id);		

		//if (debug) printf("PROC_PACKET: id=%i ", packet_id);

		//SPECIAL CASE: packet_id == 0 means a special "connection" packet or something like that
		//do not process it, pass unchanged to caller
		if (packet_id == 0) {

			//if (debug) printf(" UDPSIZE=%i\n", udp_size);

			(*size) = udp_size;
			(*special) = true;
			return udp_data;
		}

		readShort(udp_data, count, num_packet_acks);	//number of acks

		//if (debug) printf("acks=%i", num_packet_acks);

		for (i=0; i<num_packet_acks; i++) {
			readLong(udp_data, count, packet_acks[i]);

			//if (debug) printf(":%i", packet_acks[i]);
		}
		NLbyte nreliable;
		readByte(udp_data, count, nreliable);	//number of reliable msgs
		
		//if (debug) printf(" rc=%i", nreliable);

		NLulong msgid;
		NLshort	msgsize;
		for (i=0; i<nreliable; i++) {		// read all reliable msgs
			readLong(udp_data, count, msgid);		//id
			readShort(udp_data, count, msgsize);	//size
			
			//if (debug) printf("(%i,%i)", msgid, msgsize);

			// station will process the incoming reliable message
			process_incoming_message(msgid, (udp_data + count), msgsize);

			//advance count since we didn't "readBlock"
			count += msgsize;
			
			//p->add_reliable(msgid, (udp_data + count), msgsize);	//data
		}
		
		// return this
		NLshort unreliable_size;
		
		
		// FIXED: nao eh mais enviado o unreliable size porque ele pode ser inferido do
		//				tamanho do datagrama UDP
		//readShort(udp_data, count, unreliable_size);			// unreliable msg size

		// tamanho = udp_size(tamanho total do datagrama UDP) - count (quantidade jah parseada ate aqui)
		unreliable_size = ((NLshort)(udp_size - count));

		char *unreliable = (udp_data + count);		// unreliable msg pointer

		//advance count since we didn't "readBlock"
		count += unreliable_size;

		//if (debug) printf(" fr=%i TOT=%i\n", unreliable_size, count);
		
		//(2) if the packet had reliable data, add to must-ack list
		//		FIXED: if it's full... fuck it
		if (nreliable > 0) 
		if (acksinbuf < ACKBUF)	{
			ackbuf[acksinbuf] = packet_id;
			acksinbuf++;
		}		

		//(3) for every reliable message in the buffer, check if it was acked by
		//    this incoming data. if yes, delete it from the buffer (id = -1 and clear buffers)
		//
		
		// for every message in the outgoing buffer...
		//
		for (i=0; i<MAXMSG; i++) 
		if (reliable[i].id != -1)	{

			// check all acks...
			//
			for (int a=0; a<num_packet_acks; a++) {

				//sent packet's id
				NLulong	spid;				

				// ...for all packets-that-sent of the message.
				//
				bool has_sends = reliable[i].sends_first(&spid);	//get one sent id

				while (has_sends) {
					
					//test ack in packet with the sent packet id
					//
					//if (spid == acks_a) {
					if (spid == packet_acks[a]) {
DLOG_Scope s("UPIP_A");
						//acked! remove message from buffer
						//
						#ifdef EXTRA_RELIABLE_STORAGE
						reliable[i].sends_clear();
						nAssert((int)reliable_size >= reliable[i].message.getlen() + 6);
						reliable_size -= reliable[i].message.getlen() + 6;
						// check if there's a message on the extra queue that can be sent now
						pthread_mutex_lock(&queue_mutex);
						if (!extra_reliables.empty() && can_add_reliable(extra_reliables.front()->getlen())) {
							reliable[i].id = idgen_reliable_send++;
							data_ci* msg = extra_reliables.front();
							extra_reliables.pop();
							reliable[i].message.set(msg);
							delete msg;
							reliable_size += reliable[i].message.getlen() + 6;
							if (reliable_count < MAXMSG &&
									!extra_reliables.empty() &&
									can_add_reliable(extra_reliables.front()->getlen()))
							{
								for (int rel = 0; rel < MAXMSG; ++rel)	// fill empty spots from queue while possible
									if (reliable[rel].id == -1) {
										reliable[rel].id = idgen_reliable_send++;
										data_ci* msg = extra_reliables.front();
										extra_reliables.pop();
										reliable[rel].message.set(msg);
										delete msg;
										reliable_size += reliable[rel].message.getlen() + 6;
										if (++reliable_count == MAXMSG ||
												extra_reliables.empty() ||
												!can_add_reliable(extra_reliables.front()->getlen()))
											break;
									}
							}
						}
						else {
							reliable[i].id = -1;
							reliable[i].message.clear();
							reliable_count--;
						}
						pthread_mutex_unlock(&queue_mutex);
						#else
						reliable[i].id = -1;
						reliable[i].message.clear();
						reliable[i].sends_clear();
						reliable_count--;					// less one
						#endif

						//quit the two innermost iterations
						//
						a = num_packet_acks;
						break;
					}

					//next sent id
					//
					has_sends = reliable[i].sends_next(&spid);
				}
			}
		}

		//ok -return stuff
		(*size) = unreliable_size;
		(*special) = false;
		return unreliable;
	}

	// append reliable message to the packet buffer
	virtual int writer(const char *data, int length) {
DLOG_Scope s("UWR");
		nAssert(length <= MAX_MESSAGE_SIZE);

		#ifdef EXTRA_RELIABLE_STORAGE
		pthread_mutex_lock(&queue_mutex);
		if (reliable_count<MAXMSG && can_add_reliable(length) && extra_reliables.empty())
		#endif
		{
			#ifdef EXTRA_RELIABLE_STORAGE
			pthread_mutex_unlock(&queue_mutex);
			#endif
			//find slot in reliable
			//
			for (int i=0; i<MAXMSG; i++) 
			if (reliable[i].id == -1) {
				reliable[i].id = idgen_reliable_send++;  // generate and set new unique id
				reliable[i].message.set(data, (NLshort)length);					// set the data
				reliable_count++;												// another one
				reliable_size += length + 6;
				return 1;						//ok
			}
			#ifdef EXTRA_RELIABLE_STORAGE
			nAssert(0);
			#endif
		}

		// can't add to the standard send buffer
		#ifdef EXTRA_RELIABLE_STORAGE
		data_ci* msg = new data_ci();
		msg->set(data, (NLshort)length);
		extra_reliables.push(msg);
		pthread_mutex_unlock(&queue_mutex);
		return 1;
		#else
		return 0;
		#endif
	}

	// append unreliable data to the packet buffer
	//virtual int write(data_c* data) {
	virtual int write(const char *data, int length) {
DLOG_Scope s("UW");

		//piece o'cake
		//unreliable.add(((data_ci*)data)->buf, ((data_ci*)data)->ulen);

		unreliable.add(data, length);

		return 1;
	}
		
	// flush the pending packet buffer as an UDP packet to the remote address, returns "id"
	// for the assigned packet id
	virtual int send_packet(int& id) {
DLOG_Scope s("USP");

		int i;

		// assign id to the outgoing packet
		id = idgen_packet_send++;

		// build the packet:
		//
		// NLulong										packet_id
		// NLshort										number of acks
		// NLulong[number of acks]		list of acked just received packet_ids
		// NLbyte											number of reliable messages
		// for each reliable message:
		//    NLulong									message id
		//		NLshort									message size
		//		NLbyte[message size]		the reliable message data
		// (FIX: NLshort										unreliable data size  -- inferido do packet size!!)
		// NLbyte[unreliable data size]					all the unreliable data glued in a big chunk
		//														
		
		NLint	count = 0;
		
//		static int debug = 1;

		writeLong(sendbuf, count, id);	//packet id
		
		//if (debug) printf("SEND_PACKET: id=%i ",id);
		
		writeShort(sendbuf, count, acksinbuf);	//number of acks
		
		//if (debug) printf("acks=%i",acksinbuf);

		for (i=0;i<acksinbuf;i++) {
			//if (debug) printf(":%i", ackbuf[i]);

			writeLong(sendbuf, count, ackbuf[i]);		//list of acked packet ids
		}

		//if (debug) printf(" rc=%i", reliable_count);

		writeByte(sendbuf, count, reliable_count);	// number of reliable messages (wiil be overwritten)

		for (i=0;i<MAXMSG;i++)	// reliable messages in queue
			if (reliable[i].id != -1) {
				writeLong(sendbuf, count, reliable[i].id);		//id
				writeShort(sendbuf, count, (NLushort)reliable[i].message.ulen);	//size
				writeBlock(sendbuf, count, reliable[i].message.buf, reliable[i].message.ulen);	//data
				//add this send packet id to the message's outgoing sends
				reliable[i].sends_add((NLulong)id);
			}
		
		// FIXED: o "unreliable size" eh inferido do tamanho do datagrama UDP
		//				ou seja o "resto" do pacote eh o unreliable data! (nao precisa enviar size)
		//writeShort(sendbuf, count, unreliable.ulen);		// unreliable size
		writeBlock(sendbuf, count, unreliable.buf, unreliable.ulen);		// unreliable data

		//if (debug) printf(" fr=%i TOT=%i\n",unreliable.ulen,count);

		// send the packet
		//
		nlSetRemoteAddr(sendsock, &netaddr);
//		NLint result = nlWrite(sendsock, sendbuf, count);
NLint result;
{DLOG_Scope s("USPw");
result = nlWrite(sendsock, sendbuf, count);
}
		if (result == NL_INVALID) {
			//FIXME: deal with error
			//printf("station_ci: send_packet() == NL_INVALID!!\n");
		} else if (result != count) {
			//printf("station_ci: send_packet() == %i != count %i\n", result, count);
		}

		// reset the unreliable buffer (don't delete, just invalidate)
		//
		unreliable.ulen = 0;

		// reset the to-ack id's list
		//
		acksinbuf = 0;

		//ok
		return 1;
	}

	// send a raw UDP packet to the destination. returns 1 if ok, 0 if nlWrite failed
	virtual int send_raw_packet(const data_c *data) {
		
		//fix remote addr (changed by reads)
		nlSetRemoteAddr(sendsock, &netaddr);

//		NLint result = nlWrite(sendsock, data->getbuf(), data->getlen());
NLint result;
{DLOG_Scope s("SRPw");
result = nlWrite(sendsock, data->getbuf(), data->getlen());
}
		if (result == NL_INVALID) {
			// FAILED
			return 0;
		}

		//ok
		return 1;
	}

	
	// non-blocking call: attempt to read data from the socket
	// buffer/bufsize: buffer given to the routine
	// int return: value of nlRead()... :-)
	virtual int receive_packet(char *buffer, int bufsize) {
DLOG_Scope s("URPr");

		int fakek = nlRead(sendsock, buffer, bufsize);
		return fakek;
	}

	// return the socket for get_socket_stat purposes
	virtual NLsocket get_nl_socket() {
		return sendsock;
	}

	// get debug info
	virtual char* debug_info() {

		//address
		char ad[200];
		char ad2[200];
		char ad3[200];
		NLaddress fuk;
		nlAddrToString(&netaddr, ad);
		nlGetLocalAddr(sendsock, &fuk);
		nlAddrToString(&fuk, ad2);
		nlGetRemoteAddr(sendsock, &fuk);
		nlAddrToString(&fuk, ad3);
		//sprintf(debug, "addr = %s  loc = %s  rem = %s", ad,ad2,ad3);

		return debug;
	}


};


// factory functions
//
data_c			*new_data_c() {
	data_c* x = new data_ci();
	return x;
}

station_c		*new_station_c() {
	station_c* x = new station_ci();
	return x;
}
