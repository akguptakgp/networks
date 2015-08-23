#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
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
#include <signal.h>
#include "node.h"
#include <fstream>

#define MAXFILES_PER_CLIENT 50
#define BUFFER_SIZE 1000
using namespace std;

class Node *n;


void leavehandler(int param)
{
  // send leave message to the main node
  cout<<"message sent to predeccesor"<<endl;
  n->leave();
  exit(0);
}
void exithandler(int param)
{
  n->print_hash();
  printf("Enter the file name you want to search for\n");
  // char filename[100];
  // scanf("%s",filename);
  string filename;
  cin>>filename;
   struct sockaddr_in result;
   result=n->localsearch(filename);
   if(result.sin_family!=65535)
   {
        char ip_tmp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&(result.sin_addr),ip_tmp,INET_ADDRSTRLEN);
         printf("File found at %s:%d\n",ip_tmp,htons(result.sin_port));

      printf("do u want to download\n");
      char c;
      cin>>c;
      cout<<"chae"<<c<<endl;
      if(c=='y')
      {
         cout<<"okk"<<endl;
         n->download(result,filename.c_str());
        return;
      }
     else
     {
      cout<<"file source ignored"<<endl;
     }
  }
  else
  {
    cout<<"file not found"<<endl;
  }



  // char filename[100];
}

int main(int argc,char *argv[])
{
  signal(SIGQUIT,exithandler);
  signal(SIGINT,leavehandler);

  int ID;
  char *p;
  if(argc==3)
  {
    ID=atoi(argv[1]);
    p=strdup(argv[2]);
  }
  else
  {
    printf("Please provide the id and ip source\n");
    exit(0);
  }

   n= new Node(ID,p);

  printf("node creation done ID=%llu at %s::%d\n",n->getID(),n->getipstr(),htons(n->getIP().sin_port));
  
  // main_node is already existing node
 struct sockaddr_in main_node;
  main_node.sin_family=AF_INET;
  main_node.sin_port=ntohs(10000);
  char main_ip[INET_ADDRSTRLEN];
  if(startswith(p,"lo"))      //////////////////////////////////////////////should be changed
    strcpy(main_ip,"127.0.0.1");
  if(startswith(p,"wlan"))
    strcpy(main_ip,"10.138.177.192");
  inet_pton(AF_INET,main_ip,&(main_node.sin_addr));


    // join the network
  if(ID!=0)
    printf("joining to %s::%d\n",main_ip,10000);
  else{
    printf("first node in the network\n");
  }
  n->join(main_node);

  char pre[INET_ADDRSTRLEN],suc[INET_ADDRSTRLEN];
  struct in_addr preadd=n->getpreIP().sin_addr,sucadd=n->getsucIP().sin_addr;
  inet_ntop(AF_INET,&(preadd),pre,INET_ADDRSTRLEN);
  inet_ntop(AF_INET,&(sucadd),suc,INET_ADDRSTRLEN);
  
  printf("joining done me=%s::%d(%llu) predeccesor=%s::%d(%llu) successor=%s::%d(%llu)\n",n->getipstr(),htons(n->getIP().sin_port),n->getID(),pre,htons(n->getpreIP().sin_port),n->getpreID(),suc,htons(n->getsucIP().sin_port),n->getsucID()) ;

  
  n->init_finger();  
  // cout<<"init finger done"<<endl;
  // char c;
  // cin>>c;
  // n->upload_file();
  

  int sockfd=n->getsocket();
  char ipstr[INET_ADDRSTRLEN];
  while(1)  // read from TCP socket and write serve the user according to that
  {
   
    
    cout<<"ready to serve"<<endl;
     // n->print_hash();
    
    char buffer[BUFFER_SIZE]; 
    memset(buffer,'\0',BUFFER_SIZE);

    // used for storing client address while receiving
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
    if (n_ < 0) perror("ERROR reading from socket");

    memset(ipstr,'\0',INET_ADDRSTRLEN);
    inet_ntop(AF_INET,&cli_addr.sin_addr,ipstr,INET_ADDRSTRLEN);
    

    if(startswith(buffer,"SUCCINQ")) //client want to inquire about successor
    {
      // find the successor 
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"SUCCINQ<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;
      n->find_succesor(id,to_send); 
    }
    if(startswith(buffer,"JOIN")) //client want to join
    {
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"JOIN<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;
      n->join_node(id,to_send);
    }
    if(startswith(buffer,"NOTIFY")) //client want to join
    {
     // printf("noti\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"NOTIFY<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;
      n->stablize(id,to_send);
    }
    if(startswith(buffer,"HASHUPDATE")) //client want to join
    {
     printf("request for update\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"HASHUPDATE<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;

      n->hash_update(id,to_send);
    }
    if(startswith(buffer,"ASKFILE")) 
    {
      printf("request for files from successor\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"ASKFILE<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;

      n->send_files(id,to_send);
    }
    if(startswith(buffer,"SRCHFILE")) 
    {
      printf("request for search\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"SRCHFILE<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;

      n->search(id,to_send);
    }
    if(startswith(buffer,"INITFINGER")) 
    {
      printf("request for init finger\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"INITFINGER<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;
      n->update_finger(); 
      if(to_send.sin_addr.s_addr !=n->getsucIP().sin_addr.s_addr ||  to_send.sin_port!=n->getsucIP().sin_port)
      {
         // forward the request 
          if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(n->getsucIP()),sizeof(struct sockaddr))<0) perror("senting done");
      } 
    }
    if(startswith(buffer,"LEAVE")) 
    {
      printf("message \"%s\" received from %s::%d\n",buffer,ipstr,ntohs(cli_addr.sin_port));    
      printf("request for leave\n");
      unsigned long long int id;
      struct sockaddr_in to_send;
      to_send.sin_family=AF_INET;
      int port;
      sscanf(buffer,"LEAVE<%llu><%u:%d>",&id,&(to_send.sin_addr.s_addr),&port); 
      to_send.sin_port=port;
      n->leave_request(id,to_send); 
    }
    

  }

  return 0; 
}



 