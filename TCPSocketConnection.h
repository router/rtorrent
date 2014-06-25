/*
 * TCPSocketConnection.h
 *
 *  Created on: Sep 27, 2013
 *      Author: subhranil
 */


/* An object of this class is created in the following two cases
 * 1) An incoming request on the listening port
 * 2)while connecting to another client
 *
 *
 */

#include<sys/socket.h>
#include "FileDetails.h"
#ifndef TCPSOCKETCONNECTION_H_
#define TCPSOCKETCONNECTION_H_

class TCPSocketConnection {

private:

	char sourceIP[32];
	char destinationIP[32];
	int sourcePort;
	int destinationPort;
	int socketId;
	char destinationHostName[1024];
	bool connectionStatus;
	FileChunkDetails *chunkToBeTransferred; //ideally this should not be a member of this class. but this seems pretty tightly coupled.



public:
	TCPSocketConnection();
	TCPSocketConnection(char destIP[32],int destPort,int mode);
	TCPSocketConnection(struct sockaddr_in *peerAddressInfo,int connectionSocketId);
	int sendData(char *dataBuffer,int maxSendWindow=-1);
	char *getDestinationIP();
	void setDestinationIP(char * str);
	int getDestinationPort();
	void setDestinationPort(int port);
	int getSocketId();
	char *getDestinationHostName();
	void setDestinationHostName(char * str);
	void closeConnection();
	void validateConnection();
	bool isConnectionValid();
	FileChunkDetails *assignedChunk();
	void assignFileChunk(FileChunkDetails* det);
	virtual ~TCPSocketConnection();
};

#endif /* TCPSOCKETCONNECTION_H_ */
