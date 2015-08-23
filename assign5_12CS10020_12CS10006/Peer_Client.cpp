/***********                   Networks Laboratory       		***************/
/*********                    File Transfer using Sockets  		************/
/*******                 Ankit Kumar Gutpa 12CS10006  			**********/
/*****                    Gaurav Kumar  12CS10020  			   **********/
/* 		The port number is passed as an argument port no=ID+10000		****/
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <ifaddrs.h>

using namespace std;

#define FIS_SERVER_PORT "10001" 
#define MAXFILES_PER_CLIENT 20
#define MAX_WAIT_LEN 5
#define INVALID -1
#define BUFFER_SIZE 500
#define FILE_SIZE 100
#define DIR_SIZE 200

int startswith(char *s,char *p);
int connect_to_FIS(struct sockaddr_in client_addr,struct sockaddr_in* FIS_addr,socklen_t* FIS_len);
void send_file_list(int sockfd_UDP,int ID,struct sockaddr_in FIS_addr,int FIS_len);
void Download_file(struct sockaddr_in peer_server_addr,int ID,char filename[FILE_SIZE]);

char *FIS_SERVER_ADDR;
int main(int argc, char *argv[])
{

/********* 				Peer UDP part        ************/
	int ID;
	if(argc==3)
	{
		ID=atoi(argv[1]);
		FIS_SERVER_ADDR=strdup(argv[2]);
	}
	else{
		printf("please specify client ID and fis server ID\n");
		exit(0);
	}
	struct sockaddr_in client_addr;


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

               		// populate the client address to pass to connect function
					int clinet_port_no=10010+ID;
					memset(&client_addr,'\0',sizeof(client_addr));	
					client_addr.sin_family = AF_INET;
	//client_addr.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET,"10.117.3.238", &(client_addr.sin_addr));
					client_addr.sin_port = htons(clinet_port_no);

         			client_addr.sin_addr.s_addr=tmpAddrPtr->sin_addr.s_addr;

         break;	

               }// printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        }
    }
    if(ifa==NULL)
    {
    	printf("error in binding socket\n");
    }	

    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);


	



	// declare the fis server struct so that connect can populate this
	struct sockaddr_in FIS_addr;
    socklen_t FIS_len = sizeof(FIS_addr);
 	memset(&FIS_addr,'\0',FIS_len);

	// udp socket file descriptor
	int sockfd_UDP;
	
	// connect to fis server
	if((sockfd_UDP=connect_to_FIS(client_addr,&FIS_addr,&FIS_len))==-1)
	{
		printf("Error in connectint to FIS server\n"); exit(0);
	} 

	// send file list to fis
	send_file_list(sockfd_UDP,ID,FIS_addr,FIS_len);


	int n;
	char *buffer=new char[BUFFER_SIZE]; 
	//char ipstr[INET_ADDRSTRLEN];
    memset(ipstr,'\0',INET_ADDRSTRLEN);
    
    while(1)
    {
    	
    	char response;
    	printf("do you want to Search for file y/n\n");
    	scanf("\n%c",&response);
    	if(response=='y')
    	{
    		
    		char filename[FILE_SIZE];
    		printf("Please enter the filename you want to Search for\n");
    		scanf("\n");
    		gets(filename);
    		printf("filename=%s\n",filename);

			memset(buffer,'\0',BUFFER_SIZE);
			sprintf(buffer,"INQ<%s>",filename);
			printf("sending enquiry %s\n",buffer);
			
    		n=sendto(sockfd_UDP,buffer,strlen(buffer),0,(struct sockaddr*)&FIS_addr,FIS_len);
			
		 	struct sockaddr_in sender_addr;
			socklen_t sender_len = sizeof(sender_addr);
			memset(buffer,'\0',BUFFER_SIZE);
		    n= recvfrom(sockfd_UDP,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&sender_addr,&sender_len);
		   
		   
			if (n < 0) perror("ERROR reading from socket");
			else
			{	
				
				short family,port;
				unsigned long address;
				sscanf(buffer,"%hd %lu %hd",&family,&address,&port);				
				struct sockaddr_in peer_server_addr;
				peer_server_addr.sin_family=family;
				peer_server_addr.sin_port=htons(ntohs(port)+10);
				peer_server_addr.sin_addr.s_addr=address;

				memset(ipstr,'\0',INET_ADDRSTRLEN);
				inet_ntop(peer_server_addr.sin_family,&peer_server_addr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);
				
				
				if(family==INVALID)
				{
					 printf("file %s not found\n",filename); continue;
				}
				else
				{
					printf("server %s::%hd has file %s\n",ipstr,ntohs(peer_server_addr.sin_port),filename);
					printf("do you want to Download file y/n\n");
					char Download;
					scanf("\n%c",&Download);
					if(Download=='y')
					{
						
						Download_file(peer_server_addr,ID,filename);
					}
					else
					{
						printf("Neglecting file source\n");
					}
				}	
			}
		}
	}

close(sockfd_UDP);
return 0; 
}	

int startswith(char *s,char *p)
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


int connect_to_FIS(struct sockaddr_in client_addr,struct sockaddr_in* FIS_addr,socklen_t* FIS_len)
{
	int sockfd_UDP;
	if ((sockfd_UDP = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP)) == -1) 
	{
	    perror("peerclient socket"); exit(0);
	}
	
	if(bind(sockfd_UDP,(struct sockaddr*)&client_addr,sizeof(client_addr)) == -1) 
	{
	    close(sockfd_UDP);
	    perror("clinet bind Error:");
	    exit(0);
	}

        FIS_addr->sin_family=AF_INET;
 		FIS_addr->sin_port=htons(10001);
 		//FIS_addr->sin_addr.s_addr=;
 		*FIS_len=sizeof(struct sockaddr_in);
 		inet_pton(AF_INET,FIS_SERVER_ADDR, &(FIS_addr->sin_addr));

  
    	char ipstr[INET_ADDRSTRLEN];
    	memset(ipstr,'\0',INET_ADDRSTRLEN);
   		inet_ntop(AF_INET,&(client_addr.sin_addr),ipstr,INET_ADDRSTRLEN);
    	printf("Peer client up on %s::%d\n",ipstr,ntohs(client_addr.sin_port));
    	//freeaddrinfo(servinfo);	
    	return sockfd_UDP;
   // }
     
}
void send_file_list(int sockfd_UDP,int ID,struct sockaddr_in FIS_addr,int FIS_len)
{
	char* filelist[MAXFILES_PER_CLIENT];    
	int filelist_size=0;

	// polulate file list from directory
    struct dirent *pDirent;
    DIR *pDir;
    char dirname[DIR_SIZE];
    sprintf(dirname,"client_Data_%d",ID);
    pDir = opendir(dirname);
    if (pDir == NULL) perror("open Directory");

    while ((pDirent = readdir(pDir)) != NULL) 
    {
    	string str(pDirent->d_name);
    	if (str.find(".") != 0)
	    {
	    	filelist[filelist_size++]=strdup(pDirent->d_name);
	    }
	}
    closedir (pDir);
  	
  //  send file list
	char *buffer=new char[BUFFER_SIZE]; 
	strcpy(buffer,"LIST");
	for(int i=0;i<filelist_size;i++)
		{
			strcat(buffer,"<");	
			strcat(buffer,filelist[i]);	
			strcat(buffer,">");
		}

    sendto(sockfd_UDP,buffer,strlen(buffer),0,(struct sockaddr*)&FIS_addr,FIS_len);
	
	// read acknowledgement
	struct sockaddr_in sender;
	socklen_t len;
	memset(buffer,'\0',BUFFER_SIZE);
	recvfrom(sockfd_UDP,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&sender,&len);
	printf("ack received %s\n",buffer);

	if(atoi(buffer)!=filelist_size) { printf("Error in sending filelist\n"); exit(0); }

	for(int j=0;j<filelist_size;j++)
		delete[] filelist[j];

	delete[] buffer;
}

void Download_file(struct sockaddr_in peer_server_addr,int ID,char filename[FILE_SIZE])
{
	//printf("ID received %d\n",ID);
	int sockfd_TCP;
	struct addrinfo hints, *res, *p;
	int status;

	int clinet_port_no=10030+ID;
	struct sockaddr_in client_addr;
	memset(&client_addr,'\0',sizeof(client_addr));	
	client_addr.sin_family = AF_INET;
	//client_addr.sin_addr.s_addr = INADDR_ANY;
	
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
				client_addr.sin_addr.s_addr=tmpAddrPtr->sin_addr.s_addr;

         break;	

               }
        }
    }
    if(ifa==NULL)
    {
    	printf("error in binding socket\n");
    }	

    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);



	// inet_pton(AF_INET,"10.117.3.238", &(client_addr.sin_addr));

	client_addr.sin_port = htons(clinet_port_no);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = peer_server_addr.sin_family; 
	hints.ai_socktype =SOCK_STREAM;

	//char ipstr[INET_ADDRSTRLEN];
	memset(ipstr,'\0',INET_ADDRSTRLEN);
	inet_ntop(peer_server_addr.sin_family,&peer_server_addr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);	

	char str_port[20];
	sprintf(str_port,"%d",peer_server_addr.sin_port);

	if ((sockfd_TCP = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) == -1) 
		{
		    	perror("socket");
		    	exit(0);
		}
	int true_ = 1;
	setsockopt(sockfd_TCP,SOL_SOCKET,SO_REUSEADDR,&true_,sizeof(int));

	if(bind(sockfd_TCP,(struct sockaddr *)&client_addr,sizeof(client_addr))<0)
	{
		perror("bind Error");	
		exit(0);
	}
	if(connect(sockfd_TCP,(struct sockaddr*)&peer_server_addr,sizeof(peer_server_addr)) == -1)
	{
		close(sockfd_TCP);
		perror("connect");
		exit(0);
	}

	printf("connection established\n");


	int n=write(sockfd_TCP,filename,strlen(filename));
	if(n==strlen(filename)) printf("file name sent\n");

	char buffer[BUFFER_SIZE];

	// read file size
	long int size;
	memset(buffer,'\0',BUFFER_SIZE);
    n=read(sockfd_TCP,buffer,BUFFER_SIZE-1);
    if(n!=0)
    {
    	//printf("%s\n",buffer);
    	sscanf(buffer,"%ld",&size);
    	printf("file size received %ld\n",size);
	}


    char *dup_file_name=new char[strlen(filename)+30];
    char * pch=strrchr(filename,'.');

    sprintf(dup_file_name,"./client_Download_%d/",ID);
    strncat(dup_file_name,filename,pch-filename);
    //strcat(dup_file_name,"_dup");
    strcat(dup_file_name,pch);

    printf("dup file name=%s\n",dup_file_name);
	FILE *wp=fopen(dup_file_name,"wb");

    long int data_written=0; 
    // read until whole file is read
    printf("downloading the file now................\n");
    clock_t time_=clock();
    double expcted=0.0;
    while(data_written!=size)
    {
    	// read data from socket
    	int localwrite=0;
    	memset(buffer,'\0',BUFFER_SIZE);
    	long int localn=read(sockfd_TCP,buffer,BUFFER_SIZE-1);
    	while(localn!=localwrite)
    	{
    		localwrite+=fwrite(buffer+localwrite,1,localn-localwrite,wp);
    	}
    	data_written+=localwrite;
    	double t=data_written*100.0/size;
    	if(t>expcted){
    	printf("%lf %% File down Done\n",t);
    	expcted+=10.0;
    }
    }
    printf("download done..........................\n");
    time_=time_-clock();
    double t= -((double)time_)/CLOCKS_PER_SEC;
    printf("Download completed in %lf seconds at speed of %lf MBps....\n",t,size/t/1000/1000);
    
    fclose(wp);
    close(sockfd_TCP);
    delete[] dup_file_name;

}








// /***********                   Networks Laboratory       		***************/
// /*********                    File Transfer using Sockets  		************/
// /*******                 Ankit Kumar Gutpa 12CS10006  			**********/
// /*****                    Gaurav Kumar  12CS10020  			   **********/
// /* 		The port number is passed as an argument port no=ID+10000		****/
// #include <stdio.h>
// #include <stdlib.h>
// #include <cstring>
// #include <unistd.h>
// #include <sys/types.h> 
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <dirent.h>
// #include <string>
// #include <iostream>

// using namespace std;

// #define FIS_SERVER_PORT "10001" 
// #define MAXFILES_PER_CLIENT 20
// #define MAX_WAIT_LEN 5
// #define INVALID -1
// #define BUFFER_SIZE 500
// #define FILE_SIZE 100
// #define DIR_SIZE 200

// int startswith(char *s,char *p);
// int connect_to_FIS(struct sockaddr_in client_addr,struct sockaddr_in* FIS_addr,socklen_t* FIS_len);
// void send_file_list(int sockfd_UDP,int ID,struct sockaddr_in FIS_addr,int FIS_len);
// void Download_file(struct sockaddr_in peer_server_addr,int ID,char filename[FILE_SIZE]);

// int main(int argc, char *argv[])
// {

// /********* 				Peer UDP part        ************/
// 	int ID;
// 	if(argc==2)
// 	{
// 		ID=atoi(argv[1]);
// 	}
// 	else{
// 		printf("please specify client ID\n");
// 		exit(0);
// 	}

// 	// populate the client address to pass to connect function
// 	int clinet_port_no=10010+ID;
// 	struct sockaddr_in client_addr;
// 	memset(&client_addr,'\0',sizeof(client_addr));	
// 	client_addr.sin_family = AF_INET;
// 	client_addr.sin_addr.s_addr = INADDR_ANY;
// 	//inet_pton(AF_INET, "10.117.3.238", &(client_addr.sin_addr));

// 	client_addr.sin_port = htons(clinet_port_no);

// 	// declare the fis server struct so that connect can populate this
// 	struct sockaddr_in FIS_addr;
//     socklen_t FIS_len = sizeof(FIS_addr);
//  	memset(&FIS_addr,'\0',FIS_len);

// 	// udp socket file descriptor
// 	int sockfd_UDP;
	
// 	// connect to fis server
// 	if((sockfd_UDP=connect_to_FIS(client_addr,&FIS_addr,&FIS_len))==-1)
// 	{
// 		printf("Error in connectint to FIS server\n"); exit(0);
// 	} 

// 	// send file list to fis
// 	send_file_list(sockfd_UDP,ID,FIS_addr,FIS_len);


// 	int n;
// 	char *buffer=new char[BUFFER_SIZE]; 
// 	char ipstr[INET_ADDRSTRLEN];
//     memset(ipstr,'\0',INET_ADDRSTRLEN);
    
//     while(1)
//     {
    	
//     	char response;
//     	printf("do you want to Search for file y/n\n");
//     	scanf("\n%c",&response);
//     	if(response=='y')
//     	{
    		
//     		char filename[FILE_SIZE];
//     		printf("Please enter the filename you want to Search for\n");
//     		scanf("\n");
//     		gets(filename);
//     		printf("filename=%s\n",filename);

// 			memset(buffer,'\0',BUFFER_SIZE);
// 			sprintf(buffer,"INQ<%s>",filename);
// 			printf("sending enquiry %s\n",buffer);
			
//     		n=sendto(sockfd_UDP,buffer,strlen(buffer),0,(struct sockaddr*)&FIS_addr,FIS_len);
			
// 		 	struct sockaddr_in sender_addr;
// 			socklen_t sender_len = sizeof(sender_addr);
// 			memset(buffer,'\0',BUFFER_SIZE);
// 		    n= recvfrom(sockfd_UDP,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&sender_addr,&sender_len);
		   
		   
// 			if (n < 0) perror("ERROR reading from socket");
// 			else
// 			{	
				
// 				short family,port;
// 				unsigned long address;
// 				sscanf(buffer,"%hd %lu %hd",&family,&address,&port);				
// 				struct sockaddr_in peer_server_addr;
// 				peer_server_addr.sin_family=family;
// 				peer_server_addr.sin_port=htons(ntohs(port)+10);
// 				peer_server_addr.sin_addr.s_addr=address;

// 				memset(ipstr,'\0',INET_ADDRSTRLEN);
// 				inet_ntop(peer_server_addr.sin_family,&peer_server_addr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);
				
				
// 				if(family==INVALID)
// 				{
// 					 printf("file %s not found\n",filename); continue;
// 				}
// 				else
// 				{
// 					printf("server %s::%hd has file %s\n",ipstr,ntohs(peer_server_addr.sin_port),filename);
// 					printf("do you want to Download file y/n\n");
// 					char Download;
// 					scanf("\n%c",&Download);
// 					if(Download=='y')
// 					{
						
// 						Download_file(peer_server_addr,ID,filename);
// 					}
// 					else
// 					{
// 						printf("Neglecting file source\n");
// 					}
// 				}	
// 			}
// 		}
// 	}

// close(sockfd_UDP);
// return 0; 
// }	

// int startswith(char *s,char *p)
// {
// 	int rslt=1;
// 	int i=0;
// 	while(s[i]!='\0' && p[i]!='\0')
// 	{
// 		if(s[i]!=p[i]) rslt=0;
// 		i++;
// 	}
// 	return rslt;	
// }

// int connect_to_FIS(struct sockaddr_in client_addr,struct sockaddr_in* FIS_addr,socklen_t* FIS_len)
// {
// 	int sockfd_UDP;
// 	if ((sockfd_UDP = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP)) == -1) 
// 	{
// 	    perror("peerclient socket"); exit(0);
// 	}
	
// 	if(bind(sockfd_UDP,(struct sockaddr*)&client_addr,sizeof(client_addr)) == -1) 
// 	{
// 	    close(sockfd_UDP);
// 	    perror("clinet bind Error:");
// 	    exit(0);
// 	}

// 	// find fis server
//     struct addrinfo hints, *servinfo, *p;
//     int rv;

//     memset(&hints,'\0', sizeof(hints));
//     hints.ai_family = AF_INET;
//     hints.ai_socktype = SOCK_DGRAM;

//    //  if ((rv = getaddrinfo("localhost", FIS_SERVER_PORT, &hints, &servinfo)) != 0)
//    //  {
//    //      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
//    //      exit(0);
//    //  }

//    //  // loop through all the results and store the first address
//    //  for(p = servinfo; p != NULL; p = p->ai_next)
//    //   {
     	
//    //      FIS_addr->sin_family=((struct sockaddr_in*)p->ai_addr)->sin_family;
//  		// FIS_addr->sin_port=((struct sockaddr_in*)p->ai_addr)->sin_port;
//  		// FIS_addr->sin_addr.s_addr=((struct sockaddr_in*)p->ai_addr)->sin_addr.s_addr;
//  		// *FIS_len=sizeof(struct sockaddr_in);
//  		// break;
//  		// // char ipstr[INET_ADDRSTRLEN];
//    // 	//  	memset(ipstr,'\0',INET_ADDRSTRLEN);
//    // 	// 		inet_ntop(AF_INET,&(FIS_addr->sin_addr),ipstr,INET_ADDRSTRLEN);
//    // 	//  	printf("fis found up on %s::%d\n",ipstr,ntohs(FIS_addr->sin_port));
//    //  }

//         FIS_addr->sin_family=AF_INET;
//  		FIS_addr->sin_port=htons(10001);
//  		//FIS_addr->sin_addr.s_addr=;
//  		*FIS_len=sizeof(struct sockaddr_in);
//  		inet_pton(AF_INET,"10.117.3.238", &(FIS_addr->sin_addr));

//     if (p == NULL) 
//     {
//         fprintf(stderr, "client: failed to find server\n");
//        // freeaddrinfo(servinfo);	
//         exit(0);
//     }
//     else
//     {
//     	char ipstr[INET_ADDRSTRLEN];
//     	memset(ipstr,'\0',INET_ADDRSTRLEN);
//    		inet_ntop(AF_INET,&(client_addr.sin_addr),ipstr,INET_ADDRSTRLEN);
//     	printf("Peer client up on %s::%d\n",ipstr,ntohs(client_addr.sin_port));
//     	//freeaddrinfo(servinfo);	
//     	return sockfd_UDP;
//     }
     
// }

// void send_file_list(int sockfd_UDP,int ID,struct sockaddr_in FIS_addr,int FIS_len)
// {
// 	char* filelist[MAXFILES_PER_CLIENT];    
// 	int filelist_size=0;

// 	// polulate file list from directory
//     struct dirent *pDirent;
//     DIR *pDir;
//     char dirname[DIR_SIZE];
//     sprintf(dirname,"client_Data_%d",ID);
//     pDir = opendir(dirname);
//     if (pDir == NULL) perror("open Directory");

//     while ((pDirent = readdir(pDir)) != NULL) 
//     {
//     	string str(pDirent->d_name);
//     	if (str.find(".") != 0)
// 	    {
// 	    	filelist[filelist_size++]=strdup(pDirent->d_name);
// 	    }
// 	}
//     closedir (pDir);
  	
//   //  send file list
// 	char *buffer=new char[BUFFER_SIZE]; 
// 	strcpy(buffer,"LIST");
// 	for(int i=0;i<filelist_size;i++)
// 		{
// 			strcat(buffer,"<");	
// 			strcat(buffer,filelist[i]);	
// 			strcat(buffer,">");
// 		}

//     sendto(sockfd_UDP,buffer,strlen(buffer),0,(struct sockaddr*)&FIS_addr,FIS_len);
	
// 	// read acknowledgement
// 	struct sockaddr_in sender;
// 	socklen_t len;
// 	memset(buffer,'\0',BUFFER_SIZE);
// 	recvfrom(sockfd_UDP,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&sender,&len);
// 	printf("ack received %s\n",buffer);

// 	if(atoi(buffer)!=filelist_size) { printf("Error in sending filelist\n"); exit(0); }

// 	for(int j=0;j<filelist_size;j++)
// 		delete[] filelist[j];

// 	delete[] buffer;
// }

// void Download_file(struct sockaddr_in peer_server_addr,int ID,char filename[FILE_SIZE])
// {
// 	//printf("ID received %d\n",ID);
// 	int sockfd_TCP;
// 	struct addrinfo hints, *res, *p;
// 	int status;

// 	int clinet_port_no=10030+ID;
// 	struct sockaddr_in client_addr;
// 	memset(&client_addr,'\0',sizeof(client_addr));	
// 	client_addr.sin_family = AF_INET;
// 	client_addr.sin_addr.s_addr = INADDR_ANY;
// 	client_addr.sin_port = htons(clinet_port_no);

// 	memset(&hints, 0, sizeof(hints));
// 	hints.ai_family = peer_server_addr.sin_family; 
// 	hints.ai_socktype =SOCK_STREAM;

// 	char ipstr[INET_ADDRSTRLEN];
// 	memset(ipstr,'\0',INET_ADDRSTRLEN);
// 	inet_ntop(peer_server_addr.sin_family,&peer_server_addr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);	

// 	char str_port[20];
// 	sprintf(str_port,"%d",peer_server_addr.sin_port);

// 	if ((sockfd_TCP = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) == -1) 
// 		{
// 		    	perror("socket");
// 		    	exit(0);
// 		}
// 	int true_ = 1;
// 	setsockopt(sockfd_TCP,SOL_SOCKET,SO_REUSEADDR,&true_,sizeof(int));

// 	if(bind(sockfd_TCP,(struct sockaddr *)&client_addr,sizeof(client_addr))<0)
// 	{
// 		perror("bind Error");	
// 		exit(0);
// 	}
// 	if(connect(sockfd_TCP,(struct sockaddr*)&peer_server_addr,sizeof(peer_server_addr)) == -1)
// 	{
// 		close(sockfd_TCP);
// 		perror("connect");
// 		exit(0);
// 	}

// 	printf("connection established\n");


// 	int n=write(sockfd_TCP,filename,strlen(filename));
// 	if(n==strlen(filename)) printf("file name sent\n");

// 	char buffer[BUFFER_SIZE];

// 	// read file size
// 	long int size;
// 	memset(buffer,'\0',BUFFER_SIZE);
//     n=read(sockfd_TCP,buffer,BUFFER_SIZE-1);
//     if(n!=0)
//     {
//     	//printf("%s\n",buffer);
//     	sscanf(buffer,"%ld",&size);
//     	printf("file size received %ld\n",size);
// 	}


//     char *dup_file_name=new char[strlen(filename)+30];
//     char * pch=strrchr(filename,'.');

//     sprintf(dup_file_name,"./client_Download_%d/",ID);
//     strncat(dup_file_name,filename,pch-filename);
//     //strcat(dup_file_name,"_dup");
//     strcat(dup_file_name,pch);

//     printf("dup file name=%s\n",dup_file_name);
// 	FILE *wp=fopen(dup_file_name,"wb");

//     long int data_written=0; 
//     // read until whole file is read
//     printf("downloading the file now................\n");
//     clock_t time_=clock();
//     while(data_written!=size)
//     {
//     	// read data from socket
//     	int localwrite=0;
//     	memset(buffer,'\0',BUFFER_SIZE);
//     	long int localn=read(sockfd_TCP,buffer,BUFFER_SIZE-1);
//     	while(localn!=localwrite)
//     	{
//     		localwrite+=fwrite(buffer+localwrite,1,localn-localwrite,wp);
//     	}
//     	data_written+=localwrite;
//     }
//     printf("download done..........................\n");
//     time_=time_-clock();
//     double t= -((double)time_)/CLOCKS_PER_SEC;
//     printf("Download completed in %lf seconds at speed of %lf MBps....\n",t,size/t/1000/1000);
    
//     fclose(wp);
//     close(sockfd_TCP);
//     delete[] dup_file_name;

// }






// 	// if((status = getaddrinfo("localhost",str_port,&hints, &res)) != 0) 
// 	// {
//  //    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
//  //    	exit(0);
// 	// }

//  //    else
//  //    {
// 	// 	for(p=res;p!=NULL;p=p->ai_next)
// 	// 	{
// 	// 		if ((sockfd_TCP = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
// 	// 		{
// 	// 	    	perror("socket");
// 	// 	    	continue;
// 	// 		}
// 	// 		if(bind(sockfd_TCP,(struct sockaddr *)&client_addr,sizeof(client_addr))<0)
// 	// 			perror("bind Error");
// 	// 		if(connect(sockfd_TCP,p->ai_addr,p->ai_addrlen) == -1)
// 	// 	 	{
// 	// 	    	close(sockfd_TCP);
// 	// 	    	perror("connect");
// 	// 	    	continue;
// 	// 		}
// 	// 		p=p->ai_next;
// 	// 		 	break; // if we get here, we must have connected successfully
// 	// 	}
// 	// }
// 	// freeaddrinfo(res);