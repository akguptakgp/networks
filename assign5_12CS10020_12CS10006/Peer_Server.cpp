#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <netdb.h>
#include <ifaddrs.h>

//#define PEERSERVERPORT 10005
#define MAX_WAIT_LEN 5
#define BUFFER_SIZE 500
#define FILE_BUFFER_SIZE 100*1000*1000

int socketfd;
int connection;

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

void exithandler(int param)
{
	close(socketfd);
	close(connection);
	exit(0);
}

int main(int argc,char *argv[])
{

    signal(SIGINT,exithandler);

	int ID;
	if(argc==2)
	{
		ID=atoi(argv[1]);
	}
	else
	{
		printf("please specify client ID\n");
		exit(0);
	}

int server_port_no=10020+ID;	

// create socket
socketfd=socket(AF_INET,SOCK_STREAM,0);
if(socketfd<0) perror("Error opening socket");

// prepare server address
struct sockaddr_in serveraddr;
serveraddr.sin_family=AF_INET;
serveraddr.sin_port=htons(server_port_no);


char ipstr[INET_ADDRSTRLEN];
struct ifaddrs * ifAddrStruct=NULL;
struct ifaddrs * ifa=NULL;
struct sockaddr_in * tmpAddrPtr=NULL;

getifaddrs(&ifAddrStruct);

for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
{
	if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
            if(startswith(ifa->ifa_name,"wlan"))
               {
         		 serveraddr.sin_addr.s_addr=tmpAddrPtr->sin_addr.s_addr;
                 break;	
               }
        }
}
if(ifa==NULL)
{
	printf("error in binding socket\n");
}	

if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);


int opt = 1;
 //set master socket to allow multiple connections , this is just a good habit, it will work without this
if( setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
{
    perror("setsockopt");
    exit(EXIT_FAILURE);
}

   /***********************************************************///////////////
    int  addrlen , connection , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
     int max_sd;
//    struct sockaddr_in address;
      
  //set of socket descriptors
    fd_set readfds;
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }
      


if(bind(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0)
		perror("bind Error");

if(listen(socketfd,MAX_WAIT_LEN)<0) perror("listen");	 	

//char ipstr[INET_ADDRSTRLEN];
memset(ipstr,'\0',INET_ADDRSTRLEN);
inet_ntop(serveraddr.sin_family,&serveraddr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);
printf("peer server up on %s::%d waiting for incoming connection.......\n",ipstr,ntohs(serveraddr.sin_port));

struct sockaddr_in clientaddr;

socklen_t clilen=sizeof(clientaddr);

char *buffer=new char[BUFFER_SIZE];
 while(1)
 {
      FD_ZERO(&readfds);

     //add master socket to set
        FD_SET(socketfd, &readfds);
        max_sd = socketfd;
         
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) )
        {
            printf("select error");
        }


        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(socketfd, &readfds))
        {
            if ((connection=accept(socketfd,(struct sockaddr *) &clientaddr,&clilen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            
            } 
            else
            {
                char str[INET_ADDRSTRLEN];
                printf("client with IP=%s port=%d connected\n",inet_ntop(AF_INET,&(clientaddr.sin_addr),str,INET_ADDRSTRLEN),ntohs(clientaddr.sin_port));
            }
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = connection;
                    printf("Adding to list of sockets as %d\n" , i);
                     
                    break;
                }
            }
        }


            //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds))
            {
                    bzero(buffer,BUFFER_SIZE);
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, BUFFER_SIZE-1)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&clientaddr , (socklen_t*)&clilen);
                    printf("client with IP %s , port=%d disconnected\n" , inet_ntoa(clientaddr.sin_addr) , ntohs(clientaddr.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
                  
                
                else
                {

                	if(fork()==0)
                	{


	                    printf("filename received=%s\n",buffer);
	                    char *temp=strdup(buffer);
	                    sprintf(buffer,"./client_Data_%d/%s",ID,temp); 

	                    long int size;
	                    FILE *fp=fopen(buffer,"rb");
	                    if(fp==NULL) { perror("Fileopen"); size=-1;}
	                    else
	                    {
	                        if(fseek (fp,0,SEEK_END)<0) perror("fseek");   // non-portable
	                        size=ftell(fp);
	                        if(size<0) perror("ftell");
	                        rewind(fp);
	                        printf("file size=%ldBytes\n",size);
	                    }

	                    // send filesize to the client
	                    memset(buffer,'\0',BUFFER_SIZE);
	                    sprintf(buffer,"%ld",size);
	                    write(connection,buffer,strlen(buffer));
	                    printf("file size send\n");


	                    char *file_buffer=new char[FILE_BUFFER_SIZE];
	                    memset(file_buffer,'\0',FILE_BUFFER_SIZE);

	                     // send file until done
	                    printf("sending file now............\n");
	                    long int read_size;
	                    while((read_size = fread(file_buffer,1,FILE_BUFFER_SIZE,fp))!=0)
	                    {
	                        long int send_size=0;   
	                        int i=0;
	                        while(send_size!=read_size && i++!=15)
	                        {
	                            long int data_to_send=read_size-send_size;      
	                            //printf("data to be sent %ld\n",data_to_send);     
	                            send_size+= write(connection,file_buffer+send_size,data_to_send);
	                        }
	                       // printf("%d/% done \n", );
	                        memset(file_buffer,'\0',FILE_BUFFER_SIZE);
	                    }   
	                    printf("file send done................\n");
	                    exit(0); 
	                }
                }
            }
        }

 	}
close(socketfd);
return 0;	
}
