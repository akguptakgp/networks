/*******                   Networks Laboratory       ***************/
/******                    File Transfer using Sockets  ************/
/******               Ankit Kumar Gutpa 12CS10006  **********/
/******                   Gaurav Kumar  12CS10020  **********/

/* A simple server in the internet domain using UDP DATAGRAM SOCKETS       **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>


#define FIS_SERVER_PORT 10001
#define MAXFILES 500
#define MAX_WAIT_LEN 5
#define INVALID -1
#define BUFFER_SIZE 1000

int startswith(char *s,char *p)	 //  
{
	int rslt=1;
	int i=0;
	while(s[i]!='\0' && p[i]!='\0')
	{
		if(s[i]!=p[i]) rslt=0;
		i++;
	}
	return rslt;	
}

typedef struct file_inf   // 
{
	char *fname; 					 // file name
	struct sockaddr_in peer_Addr;	 // peer address having file
}FILE_LIST;

int sockfd ;
void exithandler(int param)
{
	printf("FIS server on port %d Going Down......\n",FIS_SERVER_PORT);
	close(sockfd);
	exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT,exithandler);
 	char ipstr[INET_ADDRSTRLEN];
     

 	struct ifaddrs * ifAddrStruct=NULL;
  struct ifaddrs * ifa=NULL;
  struct sockaddr_in * tmpAddrPtr=NULL;

  getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
            ///char addressBuffer[INET_ADDRSTRLEN];
            //inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(startswith(ifa->ifa_name,"wlan"))
               {
         			if ((sockfd =socket(tmpAddrPtr->sin_family, SOCK_DGRAM,0)) == -1) {
             			perror("listener: socket");
             			continue;
         				}

         				struct sockaddr_in server;
         				server.sin_family=AF_INET;
         				server.sin_port=htons(FIS_SERVER_PORT);
         				server.sin_addr.s_addr=tmpAddrPtr->sin_addr.s_addr;

         			if (bind(sockfd,(struct sockaddr*)&server,sizeof(struct sockaddr_in)) == -1) {
          			   	close(sockfd);
             			perror("listener: bind");
             			continue;
         				}	

         		//int optval = 1;
  			// 	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

  				//char ipstr[INET_ADDRSTRLEN];
  				memset(ipstr,'\0',INET_ADDRSTRLEN);
 		      	inet_ntop(AF_INET,&(tmpAddrPtr->sin_addr),ipstr,INET_ADDRSTRLEN);
   //    	printf("%s %d\n",ipstr,ntohs((((struct sockaddr_in*)(p->ai_addr))->sin_port)));
       //  printf("%d %d %d\n",p->ai_family==AF_INET,p->ai_socktype==SOCK_STREAM,p->ai_protocol==IPPROTO_UDP);
       	 printf("FIS SERVER up on %s::%d and waiting for clients to join...\n",ipstr,ntohs(server.sin_port));
         break;	

               }// printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        }
    }
    if(ifa==NULL)
    {
    	printf("error in binding socket\n");
    }	

    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);



  //   may maintian maximum 500 files at a time
	FILE_LIST filelist[MAXFILES];    
	int filelist_size=0;
    int n;

    while(1)
    {
	    char buffer[BUFFER_SIZE]; 
	    memset(buffer,'\0',BUFFER_SIZE);

	    // used for storing client address while receiving
	  	struct sockaddr_in cli_addr;
    	socklen_t clilen = sizeof(cli_addr);

	    n= recvfrom(sockfd,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&cli_addr,&clilen);
	    
	    
	    inet_ntop(AF_INET,&cli_addr.sin_addr,ipstr,INET_ADDRSTRLEN);
	    if (n < 0) perror("ERROR reading from socket");

	 	if(startswith(buffer,"LIST")) //client want to infrom about file
	 	{
		 	printf("file list received from client %s::%d\n",ipstr,ntohs(cli_addr.sin_port));

			int no_file=filelist_size;

			char *p=&buffer[5];
			int i=0;
			while(p[i]!='\0')
			{
				int j=0;
				char temp[50];
				while(p[i]!='\0' && p[i]!='>')
				{
					temp[j++]=p[i++];
				}
				temp[j]='\0';
				
				// temp contains file name add it to the filelist
				filelist[filelist_size].fname=strdup(temp);
				filelist[filelist_size++].peer_Addr=cli_addr;
				i++;
				if(p[i]=='\0') break;
				i++;
			}
		 		
		 	// write acknowledgement no. of file received	
		 	char *ack = new char[10];
		 	sprintf(ack,"%d",filelist_size-no_file);

		 	n=sendto(sockfd,ack,strlen(ack),0,(struct sockaddr*)&cli_addr,clilen);
		 	if (n < 0) perror("ERROR writing to socket");
		 	delete[] ack;
	 	}

	 	if(startswith(buffer,"INQ")) //client want to inquire about file
	 	{

	 		struct sockaddr_in peer_addr;
	 		char *to_find=new char[strlen(&(buffer[4]))];
	 		strncpy(to_find,&(buffer[4]),strlen(&(buffer[4]))-1);
	 		printf("search request for file \"%s\" received from from client %s::%d\n",to_find,ipstr,ntohs(cli_addr.sin_port));
	 		int i=0;
	 		for(i=0;i<filelist_size;i++)
	 		{
	 			if(!strcmp(to_find,filelist[i].fname)) // file found
	 				{
	 					peer_addr=filelist[i].peer_Addr;
	 					break;
	 				}
	 		}
			if(i==filelist_size) peer_addr.sin_family=INVALID; 	
	 		// // write result
	 		char *ack = new char[50];
	 		sprintf(ack,"%hd %lu %hu ",peer_addr.sin_family,peer_addr.sin_addr.s_addr,peer_addr.sin_port);
	 		printf("sending result <%s> to client %s::%d\n",ack,ipstr,ntohs(cli_addr.sin_port));
	 		n=sendto(sockfd,ack,strlen(ack),0,(struct sockaddr*)&cli_addr,clilen);
	 		if (n < 0) perror("ERROR writing to socket");
	 		else;
	 			//printf("%d data writetn\n",n);

	 		delete[] ack;
	 		delete[] to_find;
	 	}
	  	printf("current file size is %d and files are\n",filelist_size);
    	for(int j=0;j<filelist_size;j++)
    	{
    		memset(ipstr,'\0',INET_ADDRSTRLEN);
	    	inet_ntop(AF_INET,&filelist[j].peer_Addr.sin_addr,ipstr,INET_ADDRSTRLEN);
    		printf("%s::%d\t>>\t%s\n",ipstr,ntohs(filelist[j].peer_Addr.sin_port),filelist[j].fname);
    	}

	}
	close(sockfd);

    return 0; 
}	
