/*
 * DistributedFileTransfer.cpp
 *
 *  Created on: Sep 27, 2013
 *      Author: subhranil
 */



#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<cstdlib>
#include<netinet/in.h>
#include <ifaddrs.h>
#include<cstring>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<errno.h>
#include <netdb.h>
#include <sys/time.h>
#include"TCPSocketConnection.h"

#define MAXCLIENTS 10 // for the clients
#define MAXConnections 100 // for the server
#define MAXPACKETSIZE 10000


#define SERVERIP "192.168.1.15"
#define SERVERPORT 7788
#define SERVERHOSTNAME "timberlake.cse.buffalo.edu"


#define headerDelim "!@#$%^)(*&"
#define PROMPTVAR "DistributedFileTransfer>>"

long long int totalBytesTransferred=0;
long long int totalTimeInMicroSecs=0;
int totalBytesRecvd=0;


typedef struct PeerDetails
{
	char IP[40];
	int port;
	char hostName[100];
} PeerDetails;


#pragma mark Globals
int localPort;
bool isServer;
char hostIP[32];
char localHostName[1024];
PeerDetails serverIPList[MAXConnections];//={"",0};
TCPSocketConnection* peerConnectionDetails[MAXCLIENTS];
FileChunkDetails* missedChunks[100];
int numberOfMissedChunks=0;
int lastWrittenByte=0;
bool downloadActive=false;

int numberOfOpenConnections=0;
typedef struct sockaddr_in SocketAddressInfo;

// FileChunkDetails class definition

FileChunkDetails::FileChunkDetails(char * filename,int sByte,int chunksize)
{
	if(filename)
		strcpy(fileName,filename);
	startByte=sByte;
	chunkSize=chunksize;
	transferStatus=false;

}
char* FileChunkDetails::getFileName()
{
	return fileName;
}
int FileChunkDetails::getStartByte()
{
	return startByte;
}
int FileChunkDetails::getChunkSize()
{
	return chunkSize;
}
void FileChunkDetails::setActiveTransfer()
{
	transferStatus=true;
}
void FileChunkDetails::resetActiveTransfer()
{
	transferStatus=false;
}



/***************************************************/


 // TCP Socket Class definition
TCPSocketConnection::TCPSocketConnection() {
	// TODO Auto-generated constructor stub


}

TCPSocketConnection::~TCPSocketConnection() {
	// TODO Auto-generated destructor stub
	if(chunkToBeTransferred)
		delete(chunkToBeTransferred);
}


TCPSocketConnection::TCPSocketConnection(char destIP[32],int destPort,int mode)
{

if(mode==0) // create a tcp socket
{
	SocketAddressInfo *socketAddress; // this structure basically stores info about the socket address
	socketAddress=(SocketAddressInfo*)malloc(sizeof(SocketAddressInfo));
	socketAddress->sin_family=AF_INET;

	socketAddress->sin_port=htons(destPort);
	inet_pton(socketAddress->sin_family,destIP,&(socketAddress->sin_addr));

	//create a socket for connecting to server
	socketId=socket(AF_INET,SOCK_STREAM,0);
	if(socketId==-1)
	{
		printf("Error n creating socket");
		return;
	}
	int retConnect=connect(socketId,(struct sockaddr*)socketAddress,sizeof(SocketAddressInfo));
	if(retConnect<0)
	{
		connectionStatus=false;
		printf("Connection with peer failed");
		return;
	}
	strcpy(destinationIP,destIP);
	destinationPort=destPort;
	getnameinfo((struct sockaddr*)socketAddress,sizeof(*socketAddress),destinationHostName,sizeof(destinationHostName),NULL,0,0);
	//getnameinfo((struct sockaddr*)&sa, sizeof(sa), node, sizeof(node), NULL, 0, 0);
	connectionStatus=true;
}
else // create an udp socket
{
	SocketAddressInfo *socketAddress; // this structure basically stores info about the socket address
		socketAddress=(SocketAddressInfo*)malloc(sizeof(SocketAddressInfo));
		socketAddress->sin_family=AF_INET;

		socketAddress->sin_port=htons(destPort);
		inet_pton(socketAddress->sin_family,destIP,&(socketAddress->sin_addr));

		//create a socket for connecting to server
		socketId=socket(AF_INET,SOCK_DGRAM,0);
		if(socketId==-1)
			printf("Error n creating socket");
		int retConnect=connect(socketId,(struct sockaddr*)socketAddress,sizeof(SocketAddressInfo));
		if(retConnect<0)
		{
			connectionStatus=false;
			printf("connection with peer failed with error : %d",retConnect);
		}

		SocketAddressInfo localAddressInfo;
		socklen_t len=sizeof(localAddressInfo);
		getsockname(socketId,(struct sockaddr*)&localAddressInfo,&len);
		char buffer[32];
		inet_ntop(AF_INET,&localAddressInfo.sin_addr,buffer,sizeof(buffer));
		strcpy(hostIP,buffer);
		gethostname(localHostName,sizeof(localHostName));
}

}

// End of class declaration
TCPSocketConnection::TCPSocketConnection(SocketAddressInfo* peerAddressInfo,int connectionSocketId)
{


	//socklen_t len;
	//getsockname(connectionSocketId,peerAddressInfo,&len);
	//char buffer[32];
	//strcpy(destinationIP,inet_ntoa(peerAddressInfo->sin_addr));

	//inet_ntop(AF_INET,peerAddressInfo->sin_addr.s_addr,buffer,sizeof(buffer));
	//strcpy(destinationIP,buffer);
	//destinationPort=peerAddressInfo->sin_port;
	socketId=connectionSocketId;
	connectionStatus=false;
	/*struct sockaddr peerInfo;

	socklen_t len=sizeof(peerInfo);
	getpeername(connectionSocketId,&peerInfo,&len);
	peerAddressInfo=(SocketAddressInfo*)&peerInfo;
	destinationPort=peerAddressInfo->sin_port;*/


	//printf("incoming connection from %s-->%d\n",destinationIP,destinationPort);

}
void TCPSocketConnection::closeConnection()
{
	close(socketId);
}


int TCPSocketConnection::sendData(char *dataBuffer,int maxSendWindow)
{
	int  retV=send(socketId,dataBuffer,(maxSendWindow==-1)?strlen(dataBuffer):maxSendWindow,0);
	//printf("sending:%s-->%d-->%d\n",dataBuffer,socketId,retV);
	return retV;
}

char *TCPSocketConnection::getDestinationIP()
{
	return destinationIP;
}
int TCPSocketConnection::getDestinationPort()
{
	return destinationPort;
}

int TCPSocketConnection::getSocketId()
{
	return socketId;
}

char * TCPSocketConnection::getDestinationHostName()
{
	return destinationHostName;
}

void TCPSocketConnection::setDestinationIP(char * str)
{
	strcpy(destinationIP,str);
}
void TCPSocketConnection::setDestinationPort(int port)
{
	destinationPort=port;
}
void TCPSocketConnection::setDestinationHostName(char * str)
{
	strcpy(destinationHostName,str);
}

void TCPSocketConnection:: validateConnection()
{

	connectionStatus=true;

}
bool TCPSocketConnection:: isConnectionValid()
{
	return connectionStatus;
}

void TCPSocketConnection::assignFileChunk(FileChunkDetails* det)
{
	if(chunkToBeTransferred)
		delete(chunkToBeTransferred);

	chunkToBeTransferred=det;
}

FileChunkDetails * TCPSocketConnection::assignedChunk()
{
	return chunkToBeTransferred;
}
long long int getFileSize(char* fileName)
{
	struct stat fileDetails;
	stat(fileName,&fileDetails);
	return fileDetails.st_size;
}

void getLocalIP()
{
	TCPSocketConnection *udpConn=new TCPSocketConnection("8.8.8.8",53,1);
	udpConn->closeConnection();
	delete(udpConn);
}
// host list handling mthds
bool addConnection(TCPSocketConnection* newConn)
{
	// add conn to the set of peer ids
	//printf("new connection");
	bool isAdded=false;
	for(int i=0;i<MAXCLIENTS;i++)
	{
		if(peerConnectionDetails[i]==0)
		{
			isAdded=true;
			peerConnectionDetails[i]=newConn;
			numberOfOpenConnections++;
			break;
		}
	}
	return isAdded;
}

void removeConnctionWithConnectionId(int id)
{
	if(peerConnectionDetails[id])
	{
		peerConnectionDetails[id]->closeConnection();
		free(peerConnectionDetails[id]);
		peerConnectionDetails[id]=0;
		numberOfOpenConnections--;
	}
}

void removeConnectionWithSocketId(int fd)
{

	for (int i=0;i<MAXCLIENTS;i++)
	{
		peerConnectionDetails[i]->closeConnection();
		free(peerConnectionDetails[i]);
		peerConnectionDetails[i]=0;
		numberOfOpenConnections--;
	}

}
TCPSocketConnection* getConnectionDetailsForId(int id)
{
	return peerConnectionDetails[id];
}


void listConnections()
{
	for(int i=0;i<MAXCLIENTS;i++)
	{
		if(peerConnectionDetails[i])
			printf("%d\t%s\t%s\t%d\n",(i+1),peerConnectionDetails[i]->getDestinationHostName(),peerConnectionDetails[i]->getDestinationIP(),peerConnectionDetails[i]->getDestinationPort());
	}
}

// Internal methods

void displayServerIPList()
{
	for(int i=0;i<MAXCLIENTS;i++)
	{
		printf("%d\t%s\t%d\n",(i+1),serverIPList[i].IP,serverIPList[i].port);
	}
}

bool addToServerList(char *IP,int port,char *hostname)
{
	if(!isServer)
		return false;
	int index=0;
	while(index<MAXCLIENTS)
	{
		if(strcmp(serverIPList[index].IP,"")==0 && serverIPList[index].port==0)
		{
			if(IP)
				strcpy(serverIPList[index].IP,IP);
			if(hostname)
				strcpy(serverIPList[index].hostName,hostname);
			serverIPList[index].port=port;
			return true;
		}
		index++;
	}
	return false;

}

bool removeFromServerList(char*IP,int port)
{
	if(!isServer)
		return false;
	int index=0;
		while(index<MAXCLIENTS)
		{
			if(strcmp(serverIPList[index].IP,IP)==0 && serverIPList[index].port==port)
			{
				strcpy(serverIPList[index].IP,"");
				serverIPList[index].port=0;
				return true;
			}
			index++;
		}
		return false;



}

void broadcastServerList()
{
	if(!isServer)
		return;

	int totalSize=sizeof(PeerDetails)*MAXCLIENTS+22;
	char *serverListMssg=(char*)malloc(totalSize);
	strcpy(serverListMssg,"ServerListUpdate\n");

	//memcpy(serverListMssg+22,&serverIPList,sizeof(serverIPList));
	for(int i=0;i<MAXCLIENTS;i++)
	{
		if(serverIPList[i].port!=0)
			sprintf(serverListMssg,"%s%s:%s:%d:",serverListMssg,serverIPList[i].IP,serverIPList[i].hostName,serverIPList[i].port);
	}
	sprintf(serverListMssg,"%s\n",serverListMssg);

	for(int i=0;i<MAXCLIENTS;i++)
	{
		if(peerConnectionDetails[i])
		{
			peerConnectionDetails[i]->sendData(serverListMssg);
		}
	}
	free(serverListMssg);
}
// Interaction with shell methods
void registerOnServer()
{
	TCPSocketConnection *serverConn=new TCPSocketConnection(SERVERIP,SERVERPORT,0);
	serverConn->sendData("RegisterRequest\n");
}

bool completeRequestAndCheckValidity(char *param1, char* param2 , int *port,char **finalIP,char **errStr)
{
	// param1-->hostname/ip
	// param2--->port
	int tentPort=atoi(param2);
	bool foundMatch=false;
	if(tentPort==localPort&&(!strcmp(param1,localHostName)||!strcmp(param1,hostIP)))
	{
		strcpy(*errStr,"Self Connections are not allowed.\n");
		return false;
	}
	if(numberOfOpenConnections==4)
	{
		strcpy(*errStr,"Maximum number of connections has been reached.\n");
		return false;
	}
	for(int i=0;i<MAXConnections;i++)
	{
		if(serverIPList[i].port==0) //we have iterated over all the entries.
			break;
		if(tentPort==serverIPList[i].port)
		{
			if(strcmp(param1,serverIPList[i].IP)==0)
			{
				strcpy(*finalIP,param1);
				foundMatch=true;
				break;
			}
			else if(strcmp(param1,serverIPList[i].hostName)==0)
			{
				strcpy(*finalIP,serverIPList[i].IP);
				//*finalIP=serverIPList[i].IP;
				foundMatch=true;
				break;
			}
		}
	}
	if(!foundMatch)
	{
		strcpy(*errStr, "Unknown host.\n");
		return false;
	}
	// for duplicating connections

	for(int i=0;i<MAXCLIENTS;i++)
	{
		if(peerConnectionDetails[i])
		{
			if(tentPort==peerConnectionDetails[i]->getDestinationPort()&&strcmp(peerConnectionDetails[i]->getDestinationIP(),*finalIP)==0)
			{
				strcpy(*errStr,"Duplicate connections are not allowed.\n");
				return false;
			}
		}
	}
	*port=tentPort;
	return true;

}

bool terminateConnection(int id,bool force)
{
	if(id==1&&!force)
		return false;
	if(peerConnectionDetails[id-1])
	{
		peerConnectionDetails[id-1]->closeConnection();
		free(peerConnectionDetails[id-1]);
		peerConnectionDetails[id-1]=0;
		numberOfOpenConnections--;

	}
	return true;
}

void terminateAllConnections()
{
	for (int i=1;i<=MAXCLIENTS;i++)
	{
		terminateConnection(i,true);
	}
}

void downloadMissedChunks(TCPSocketConnection *conn)
{
	if(numberOfMissedChunks==0)
	{
		printf("\nDownload complete!!\n");
		return;
	}


	if(conn==NULL) // just been initiated.thus we have to loop
	{
		for(int i=1;i<MAXCLIENTS;i++)
		{

			if(numberOfMissedChunks==0)
				return;
		if(peerConnectionDetails[i]&&peerConnectionDetails[i]->isConnectionValid())
		{
					FileChunkDetails *chunk=missedChunks[--numberOfMissedChunks];
					peerConnectionDetails[i]->assignFileChunk(chunk);
					char fileReqMssg[1024];
					sprintf(fileReqMssg,"FileRequest\n%s\n%d\n%d\n",chunk->getFileName(),chunk->getStartByte(),chunk->getChunkSize());
					peerConnectionDetails[i]->sendData(fileReqMssg);

		}
	}
	}
	else
	{
							FileChunkDetails *chunk=missedChunks[--numberOfMissedChunks];
							conn->assignFileChunk(chunk);
							char fileReqMssg[1024];
							sprintf(fileReqMssg,"FileRequest\n%s\n%d\n%d\n",chunk->getFileName(),chunk->getStartByte(),chunk->getChunkSize());
							conn->sendData(fileReqMssg);

	}


}

//if conn is null  then it is the first case with the iteration
void downloadFile(char *fileName,int chunkSize, int tentativeSize,TCPSocketConnection *conn)
{
	static int sChunkSize=chunkSize;

	if(tentativeSize>0&&lastWrittenByte>=tentativeSize)
	{

		downloadActive=false;
		downloadMissedChunks(NULL);
		return;

	}

	if(!conn)
	{
		for(int i=1;i<MAXCLIENTS;i++)
		{
			if(peerConnectionDetails[i]&&peerConnectionDetails[i]->isConnectionValid())
			{
				FileChunkDetails *chunk=new FileChunkDetails(fileName,lastWrittenByte,sChunkSize);
				peerConnectionDetails[i]->assignFileChunk(chunk);
				char fileReqMssg[1024];
				sprintf(fileReqMssg,"FileRequest\n%s\n%d\n%d\n",fileName,lastWrittenByte,sChunkSize);
				peerConnectionDetails[i]->sendData(fileReqMssg);
				lastWrittenByte+=sChunkSize;
			}

		}
	}
	else
	{
		FileChunkDetails *chunk=new FileChunkDetails(fileName,lastWrittenByte,sChunkSize);
		conn->assignFileChunk(chunk);
		char fileReqMssg[1024];
		sprintf(fileReqMssg,"FileRequest\n%s\n%d\n%d\n",fileName,lastWrittenByte,sChunkSize);
		conn->sendData(fileReqMssg);
		lastWrittenByte+=sChunkSize;
	}

}

void resetToPrompt()
{
	if(downloadActive)
		return;

	printf(PROMPTVAR);
	fflush(stdout);
}


void displayShell()
{
	//printf("displayShell");

	//printf(PROMPTVAR);
	//while(1)
	//{
	char inputLine[1024];

	//fflush(stdout);
	fgets(inputLine,1024,stdin);
	char *command;//[30];
	command=strtok(inputLine,"\n");
	command=strtok(inputLine," ");
	//	strcpy(command,strtok(inputLine,"\n"));
	//strcpy(command,strtok(inputLine," "));
	//printf("command:%s\n",command);
	if(!strcmp(command,"HELP"))
	{
		printf("No help available\n");
		resetToPrompt();


	}
	else if(!strcmp(command,"MYIP"))
	{
		printf("%s\n",hostIP);
		resetToPrompt();
	}
	else if(!strcmp(command,"MYPORT"))
	{
		printf("%d\n",localPort);
		resetToPrompt();
	}
	else if(!strcmp(command,"REGISTER"))
	{
		if(isServer)
		{
			printf("Invalid Command\n");
			resetToPrompt();
			return;
		}
		char *tempServerIP;//[40];
		//strcpy(tempServerIP,strtok(NULL," "));
		tempServerIP=strtok(NULL," ");
		char *portStr;//[5];
		portStr=strtok(NULL," ");
		//strcpy(portStr,strtok(NULL," "));

		if(!tempServerIP||!portStr|| !strlen(tempServerIP)||!strlen(portStr))
		{
			printf("Invalid Command Format.\n");
			resetToPrompt();
		}
		else if (peerConnectionDetails[0])
		{
			printf("You are already registered to a server.\n");
			resetToPrompt();
		}
		else
		{
//			if(strcmp(tempServerIP,SERVERIP)||atoi(portStr)!=SERVERPORT)
//			{
//				printf("Invalid Details\n");
//				resetToPrompt();
//				return;
//			}
			printf("Trying to register....\n");
			TCPSocketConnection *serverRegisterConn=new TCPSocketConnection(tempServerIP,atoi(portStr),0);
			char registerReqMessage[1024];
			sprintf(registerReqMessage,"RegisterRequest\n%s\n%d\n%s\n",hostIP,localPort,localHostName);
			serverRegisterConn->setDestinationHostName(SERVERHOSTNAME);
			serverRegisterConn->sendData(registerReqMessage);
			addConnection(serverRegisterConn);
			//raise(10);
		}
	}
	else if(!strcmp(command,"CONNECT"))
	{
		if(isServer)
		{
			printf("Invalid Command\n");
			resetToPrompt();
			return;
		}
		char *peerIP;//[40];
				//strcpy(tempServerIP,strtok(NULL," "));
		peerIP=strtok(NULL," ");
		char *portStr;//[5];
		portStr=strtok(NULL," ");
		//if(!strlen(peerIP)||strlen(portStr))
		if(!peerIP||!portStr|| !strlen(peerIP)||!strlen(portStr))
			printf("Invalid Command Format.\n");
		else
		{
			// check for the validity of the connection
			char *fIP=(char*)malloc(40);
			int fPort;
			char *errString=(char*)malloc(500);
			bool status=completeRequestAndCheckValidity(peerIP,portStr,&fPort,&fIP,&errString);
			if(!status)
			{
				printf("%s",errString);
				free(errString);
				resetToPrompt();
				return;
			}
			printf("Trying to connect to %s at port %d\n",fIP,fPort);
			TCPSocketConnection *peerConnectConn=new TCPSocketConnection(fIP,fPort,0);
			char connectReqMessage[1024];
			sprintf(connectReqMessage,"ConnectRequest\n%s\n%d\n%s\n",hostIP,localPort,localHostName);
			peerConnectConn->sendData(connectReqMessage);
			addConnection(peerConnectConn);
			free(fIP);
			//resetToPrompt();

		}

	}
	else if(!strcmp(command,"LIST"))
	{
		listConnections();
		resetToPrompt();

	}

	else if(!strcmp(command,"DOWNLOAD"))
	{
		if(isServer)
		{
			printf("Invalid Command\n");
			resetToPrompt();
			return;
		}

		char *fileName=strtok(NULL," ");
		char *chunkSizeStr=strtok(NULL," ");
		if(!fileName||!chunkSizeStr|| !strlen(fileName)||!strlen(chunkSizeStr))
			printf("Invalid Command Format.\n");
		else
		{
			downloadActive=true;
			lastWrittenByte=0;
			downloadFile(fileName,atoi(chunkSizeStr),-1,NULL);
		}
		printf("Downloading %s...\n",fileName);
	}

	else if(!strcmp(command,"TERMINATE"))
	{
		char *idStr;
		idStr=strtok(NULL," ");
		if(strlen(idStr)==0)
			printf("Invalid Usage.\n");
		else
		{
			if(terminateConnection(atoi(idStr),false))
				printf("Connection Terminated.\n");
			else
				printf("Operation not permitted.\n");
		}


		resetToPrompt();
	}
	else if(!strcmp(command,"EXIT"))
	{
		terminateAllConnections();
		printf("\n*** Transferred %lld bytes in %lld us\n",totalBytesTransferred,totalTimeInMicroSecs);
		exit(0);
	}

	else if(!strcmp(command,"SIP"))
	{
		displayServerIPList();
		resetToPrompt();
	}
	else if(!strcmp(command,"CREATOR"))
	{
		printf("Name:Subhranil Banerjee\nUBITName:subhrani\nEmail:subhrani@buffalo.edu\n");
		resetToPrompt();
	}

	else
	{
		printf("Command not found.\n");
		resetToPrompt();
	}

	//}

}


void handleDataOnSocket(char* message,TCPSocketConnection* connObj,int numberOfBytesRecvd, long long int usecs)
{
	//printf("message:%s",message);
	char messageType[30];
	char *tempMessageBuffer=(char*)malloc(numberOfBytesRecvd);
	memcpy(tempMessageBuffer,message,numberOfBytesRecvd);
	//int totalMessageLength=strlen(message);
	strcpy(messageType,strtok(message,"\n"));

	if(strcmp(messageType,"FileRequest")==0)
	{
		char *fileNameToSend=strtok(NULL,"\n");
		long long int startByte=atoll(strtok(NULL,"\n"));
		int chunkSize=atoi(strtok(NULL,"\n"));

		char *dataBuffer=(char*)malloc(chunkSize+1);
		int fileSendDesc=open(fileNameToSend,O_RDONLY);
		lseek(fileSendDesc,startByte,SEEK_SET);
		int numberOfBytesRead=read(fileSendDesc,dataBuffer,chunkSize);
		dataBuffer[numberOfBytesRead]=0;
		long long int fileSize=getFileSize(fileNameToSend);
		int numberOfBytesSent=0;
		char headerMessage[1024];
		sprintf(headerMessage,"FileResponse\n%s\n%lld\n%lld\n%lld%s",fileNameToSend,fileSize,startByte,(startByte+numberOfBytesRead),headerDelim);
		int totalWindowSize=strlen(headerMessage)+numberOfBytesRead;
		char *totalMessage=(char*)malloc(totalWindowSize);
		memcpy(totalMessage,headerMessage,strlen(headerMessage));

		// cannot use sprintf here since the databuffer may not be a string
		memcpy(totalMessage+strlen(headerMessage),dataBuffer,numberOfBytesRead);
		//sprintf(totalMessage,"%s%s%s%s",headerMessage,headerDelim,dataBuffer,headerDelim);
		struct timeval startTime,endTime;
		gettimeofday(&startTime,NULL);
		totalBytesTransferred+=(long long int)connObj->sendData(totalMessage,totalWindowSize);
		gettimeofday(&endTime,NULL);
		totalTimeInMicroSecs+=100000LL*(long long int)(endTime.tv_sec-startTime.tv_sec)+(long long int)(endTime.tv_usec-startTime.tv_usec);
		close(fileSendDesc);
		free(totalMessage);
		free(dataBuffer);

	}
	else if(strcmp(messageType,"FileResponse")==0)
	{

		char header[1200]="";
		strcpy(header,strtok(NULL,headerDelim));
		totalBytesTransferred+=numberOfBytesRecvd;
		totalTimeInMicroSecs+=usecs;
		int contentLength=numberOfBytesRecvd-strlen(messageType)-strlen(header)-11;
		char* indexofDelim=strstr(tempMessageBuffer,headerDelim);
		//char *contents=(char*)malloc(totalMessageLength-strlen(header));
		//strcpy(contents,strtok(NULL,"\n"));
		// parse the header
		//char fileName0[1024]="";
//		strcpy(fileName,strtok(header,headerDelim));
//		int fileSize=atoi(strtok(NULL,headerDelim));
//		int startByte=atoi(strtok(NULL,headerDelim));
//		int endByte=atoi(strtok(NULL,headerDelim));

		char *fileName=strtok(header,"\n");
		long long int fileSize=atoll(strtok(NULL,"\n"));
		long long int startByte=atoll(strtok(NULL,"\n"));
		long long endByte=atoll(header);


	//	printf("MessageType:%s\nFileName:%s\nSize:%d\nStart:%d\nEnd:%d-->%s\n\n\n",messageType,fileName,fileSize,startByte,endByte,contents);

		// open file for writing
		char destPath[1024]="";
		sprintf(destPath,fileName);
		//int writefd=creat(destPath,O_EXCL|S_IRUSR | S_IWUSR);
		int writefd=open(destPath,O_WRONLY|O_CREAT| O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
		if(writefd<0)
		{
			if(errno==EEXIST)
			{
				//printf("file already exists");
				writefd=open(destPath,O_WRONLY);
				//printf("error in open%d",errno);
			}

		}

		int offset,numberofbytes=0;
		//while(numberofbytes<numberOfBytesRecvd)
		//{
			offset=lseek(writefd,(startByte+numberofbytes),SEEK_SET);
			numberofbytes=write(writefd,indexofDelim+10,contentLength);
			//printf("wrote:%d bytes to %d\n",numberofbytes,writefd);
		//}
		//int nuberofbutes=pwrite(writefd,contents,strlen(contents),startByte);
		printf("Downloaded:%d%%\r",((startByte+numberofbytes)*100LL)/fileSize);
		fflush(stdout);
		close(writefd);
		//if(endByte>=fileSize)


		//free(contents);
		if(downloadActive)
			downloadFile(fileName,numberofbytes,fileSize,connObj);
		else
			downloadMissedChunks(connObj);
//		if(endByte>=fileSize)
//		{
//			// check for missing chunks in aray
//			printf("Donwload completed!!\n%s",PROMPTVAR);
//		}
//		else
//		{
//			printf("endbyte :%d filesize:%d ==> so calling downlaod \n",endByte,fileSize);
//
//		}



	}
	else if(strcmp(messageType,"RegisterRequest")==0) // this is applicable only in the case of ther server
	{
		if(isServer)
		{
			char *ip=strtok(NULL,"\n");
			char *port=strtok(NULL,"\n");
			char *hostname=strtok(NULL,"\n");
			if(strlen(ip)>0&&strlen(port)>0&&strlen(hostname)>0)
			{
				connObj->setDestinationIP(ip);
				connObj->setDestinationPort(atoi(port));
				connObj->setDestinationHostName(hostname);
				addToServerList(ip,atoi(port),hostname);
				connObj->sendData("RegisterAcknowledged\n"); // the server will always try to add the client
				//addConnection(connObj);
				sleep(1);
				broadcastServerList();
				connObj->validateConnection();
			}
			else
				connObj->sendData("RegisterRequestIncomplete\n");

		}
		else
			connObj->sendData("RegisterRequestInvalid\n");

	}
	else if (strcmp(messageType,"RegisterRequestIncomplete")==0)
		removeConnectionWithSocketId(connObj->getSocketId());

	else if(strcmp(messageType,"RegisterAcknowledged")==0) // will be received by a client trying to register
	{
		// USE THIS PART TO DO WHATEVER YOU WANT WITH THE SEVER IP LIST

//		char *scanStartPt=strstr(tempMessageBuffer,"\n");
//
//		memcpy(serverIPList,scanStartPt+1,numberOfBytesRecvd-strlen(messageType)-1);
//		displayServerIPList();


		connObj->validateConnection();
		//addConnection(connObj); // assumption: that register will take place b4 any connect
		printf("Registration with server successful.\n");
		resetToPrompt();
	}
	else if(strcmp(messageType,"RegisterRequestInvalid")==0)
	{
		printf("You can only register with the server.\n");
		removeConnectionWithSocketId(connObj->getSocketId());
		resetToPrompt();

	}

	else if(strcmp(messageType,"ConnectRequest")==0)
	{
//		if(addConnection(connObj))
//			connObj->sendData("ConnectAccepted");
//		else
//			connObj->sendData("ConnectRejected");


		if(isServer)
				connObj->sendData("ConnectRequestInvalid\n");

		else
		{

			char *ip=strtok(NULL,"\n");
			char *port=strtok(NULL,"\n");
			char *hostname=strtok(NULL,"\n");
			if(strlen(ip)>0&&strlen(port)>0&&strlen(hostname)>0&&numberOfOpenConnections<=4)
			{


					connObj->setDestinationIP(ip);
					connObj->setDestinationPort(atoi(port));
					connObj->setDestinationHostName(hostname);
					connObj->sendData("ConnectAccepted\n"); // the server will always try to add the client
					connObj->validateConnection();
					//addConnection(connObj);

			}
			else
					connObj->sendData("ConnectRejected\n");

			}

	}

	else if(strcmp(messageType,"ConnectRequestInvalid")==0)
	{
		printf("You cannot send connect request to a server.\n");
		removeConnectionWithSocketId(connObj->getSocketId());

	}

	else if(strcmp(messageType,"ConnectAccepted")==0) // will be received by the sender of the request
	{

		connObj->validateConnection();
		printf("Successfully connected to peer.\n");
		resetToPrompt();
	}
	else if(strcmp(messageType,"ConnectRejected")==0)
	{

		removeConnectionWithSocketId(connObj->getSocketId());
		printf("Connection with host failed.\n");
		resetToPrompt();
	}
	else if(strcmp(messageType,"ServerListUpdate")==0)
	{
		memset(serverIPList,0,MAXCLIENTS*sizeof(PeerDetails));

		char* lineFeed=strtok(NULL,"\n");
		char *tempLine=lineFeed;//(char*)malloc(strlen(lineFeed));
		//strcpy(tempLine,lineFeed);

		int index=0;
		char* firstToken;
		firstToken=strtok(tempLine,":");
		while(firstToken)
		{


			strcpy(serverIPList[index].IP,firstToken);
			strcpy(serverIPList[index].hostName,strtok(NULL,":"));
			serverIPList[index].port=atoi(strtok(NULL,":"));
			firstToken=strtok(NULL,":");
			//sscanf(lineFeed,"%s %s%d",serverIPList[index].IP,serverIPList[index].hostName,&serverIPList[index].port);


			index++;
		}
		//free(tempLine);
		//char *scanStartPt=strstr(tempMessageBuffer,"\n");

		//memcpy(&serverIPList,scanStartPt+1,numberOfBytesRecvd-strlen(messageType)-1);
		//displayServerIPList();
		free(tempMessageBuffer);
		printf("Server List updated\n");
		resetToPrompt();
	}

}


void handleSocketBehaviour(void)
{
	//printf("handleSocketBehaviour");

		int sendFileDesc,recvFileDesc;

	    /* observedSockets file descriptor list and max value */
		fd_set observedSockets;
		int fdmax;

		int mainSocket;

		memset(peerConnectionDetails,0,MAXCLIENTS*sizeof(TCPSocketConnection *));

		// defining the address for the main socket

		SocketAddressInfo *socketAddress; // this structure basically stores info about the socket address
		socketAddress=(SocketAddressInfo*)malloc(sizeof(SocketAddressInfo));
		socketAddress->sin_family=AF_INET;
		inet_pton(socketAddress->sin_family,hostIP,&(socketAddress->sin_addr));
		socketAddress->sin_port=htons(localPort);

		//create a socket for accepting incoming connections

		mainSocket=socket(AF_INET,SOCK_STREAM,0);
		if(mainSocket==-1)
		{
			printf("Error in creating socket");
			exit(0);
		}


		// bind the spcket to the addressinfo

		if(bind(mainSocket,(struct sockaddr*)socketAddress,sizeof(SocketAddressInfo))==-1)
		{
			printf("Error in binding socket IP address");
			exit(0);
		}



		// listen for incoming connections
		if(listen(mainSocket,10)==-1)
		{
			printf("Error in listening");
			exit(0);
		}


		// Begin code for socket listening

		//printf("Server running...");
		/* clear the observedSockets and temp sets */
		FD_ZERO(&observedSockets);

		// run loop for observing sockets

		while(true)
		{
			/* add the listener to the observedSockets set */
			FD_SET(mainSocket, &observedSockets);
			if(!downloadActive)
				FD_SET(0,&observedSockets);
			/* keep track of the biggest file descriptor */
			fdmax = mainSocket ; /* so far, it's this one*/

			// add the client sockets for observation
			for(int i=0;i<MAXCLIENTS;i++)
			{
				if(!peerConnectionDetails[i])
					continue;
				int tempDesc=peerConnectionDetails[i]->getSocketId();
				if(tempDesc>0) // already assigned to a valid connection
				{
					FD_SET(tempDesc,&observedSockets);
					if(tempDesc>fdmax)
						fdmax=tempDesc;
				}

			}

			struct timeval timeout;
			timeout.tv_sec=1;
			timeout.tv_usec=0;
			int activity=select(fdmax+1,&observedSockets,NULL,NULL,&timeout); // blocking all until there is some activity on any of the sockets
			if(activity<0)
			{
				if(errno==EINTR)
					continue;
				printf("error in select");
				return ;
			}
			if(FD_ISSET(mainSocket,&observedSockets)) // .. there has been activity on the mainSocket. thus there s a new connection that needs to be added
			{
				SocketAddressInfo clientSocketInfo;
				int sizeOfSocketData=sizeof(clientSocketInfo);
				int conn=accept(mainSocket,(struct sockaddr*)&clientSocketInfo,(socklen_t*)&sizeOfSocketData);
				//  add the connection temp.
				TCPSocketConnection *newConn=new TCPSocketConnection(&clientSocketInfo,conn);
				addConnection(newConn);

//				printf("%added connection with id:%d",conn);

			}
			else if(FD_ISSET(0,&observedSockets))
			{
				displayShell();
			}
			else // there is some activity on the other open sockets possibly incoming data
			{
				for(int i=0;i<MAXCLIENTS;i++)
				{
					if(!peerConnectionDetails[i]) // the sender message may arrive even before you are done with the registration wrok
						continue;
					int tempDesc=peerConnectionDetails[i]->getSocketId();
					if(tempDesc>0)
					{
						if(FD_ISSET(tempDesc,&observedSockets))
						{
							char incomingMessage[MAXPACKETSIZE];
							int numberOfBytesRecvd=0;

							struct timeval startTime,endTime;
							gettimeofday(&startTime,NULL);
							numberOfBytesRecvd=recv(tempDesc,&incomingMessage,MAXPACKETSIZE-1,0);
							gettimeofday(&endTime,NULL);

							if(numberOfBytesRecvd<=0) //connection terminated by peer
							{

								//peerSocketIds[i]=0;
								FD_CLR(tempDesc,&observedSockets);

								if(isServer)
								{
									removeFromServerList(getConnectionDetailsForId(i)->getDestinationIP(),getConnectionDetailsForId(i)->getDestinationPort());
								}
								missedChunks[numberOfMissedChunks++]=getConnectionDetailsForId(i)->assignedChunk();
								removeConnctionWithConnectionId(i);
								broadcastServerList();

								printf("connection terminated for host#%d-->%d\n",i,numberOfBytesRecvd);
							}
							else
							{
								//dataBuffer[numberOfBytesRecvd]='\0';
								long long int recvTime=100000LL*(long long int)(endTime.tv_sec-startTime.tv_sec)+(long long int)(endTime.tv_usec-startTime.tv_usec);
								handleDataOnSocket(incomingMessage,peerConnectionDetails[i],numberOfBytesRecvd,recvTime);


							}
						}
					}
				}

			}



			}


}




int main(int argc, char*argv[])
{
	if(argc!=3)
	{
		printf("Invalid usage\n");
		exit(1);
	}

	isServer=(strcmp(argv[1],"s")==0);
	localPort=atoi(argv[2]);
	getLocalIP();
	resetToPrompt();
	//pthread_t socketThread;
	handleSocketBehaviour();
	//pthread_create( &socketThread,NULL,handleSocketBehaviour,0);


	//displayShell();


	//pthread_join(socketThread,NULL);
	return 0;

}
