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
#include "util.cpp"

#define DIR_SIZE 50
#define BUFFER_SIZE 1000
#define MAXFILES_PER_CLIENT 50

using namespace std;
//implement the constructor


inline Node::Node(int ID,char *p):ID(ID)
{
    my_ip.sin_family=AF_INET;
    succesor_ip.sin_family=AF_INET;
    predesessor_ip.sin_family=AF_INET;
	my_ip.sin_port=ntohs(10000+ID);
	getip_system(ipstr,p,&my_ip);
    char *ip_port_str=new char[INET_ADDRSTRLEN+10];
    sprintf(ip_port_str,"%s:%d",ipstr,10000+ID);
    Hash_ID=oat_hash(ip_port_str);

    // create the udp socket to connect to the other client
    if((sockfd =socket(AF_INET, SOCK_DGRAM,0)) == -1) {  perror("socket creation"); exit(0);}
    if (bind(sockfd,(struct sockaddr*)&my_ip,sizeof(struct sockaddr_in)) == -1) {perror("bind socket");exit(0);} 
}

	
inline bool Node::join(struct sockaddr_in ip)
{
   if(ID==0)          
    {
        succesor_id=Hash_ID;
        predesessor_id=Hash_ID;
        succesor_ip= my_ip;
        predesessor_ip= my_ip;
    }
    else
    {
        
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"JOIN<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&ip,sizeof(struct sockaddr))<0) perror("message send");
        

    //  char buffer[BUFFER_SIZE]; 
      memset(buffer,'\0',BUFFER_SIZE);

    // used for storing client address while receiving
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
    if (n_ < 0) perror("ERROR reading from socket");

    unsigned long long int id1,id2;
    int address,port;
    sscanf(buffer,"JOINRSLT<%llu,%llu><%d:%d>",&id1,&id2,&address,&port);
    
    succesor_id=id2;
    succesor_ip.sin_family=AF_INET;
    succesor_ip.sin_port=port;
    succesor_ip.sin_addr.s_addr=address;
    
    // get predessor from client addr
    predesessor_id=id1;
    predesessor_ip.sin_port=cli_addr.sin_port;
    predesessor_ip.sin_addr=cli_addr.sin_addr;

    // notify the succssor about join to network and will retunrn the 
    memset(buffer,'\0',BUFFER_SIZE);
    sprintf(buffer,"NOTIFY<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
    if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("message send");
        
    delete[] buffer;
    }
}

inline void  Node::stablize(unsigned long long int id,struct sockaddr_in addr)
{
    predesessor_id=id;
    predesessor_ip.sin_port=addr.sin_port;
    predesessor_ip.sin_addr=addr.sin_addr;

    char pre[INET_ADDRSTRLEN],suc[INET_ADDRSTRLEN];
    struct in_addr preadd=predesessor_ip.sin_addr,sucadd=succesor_ip.sin_addr;
    inet_ntop(AF_INET,&(preadd),pre,INET_ADDRSTRLEN);
    inet_ntop(AF_INET,&(sucadd),suc,INET_ADDRSTRLEN);       
    printf("ring updated me=%s::%d(%llu) predeccesor=%s::%d(%llu) successor=%s::%d(%llu)\n",ipstr,htons(my_ip.sin_port),Hash_ID,pre,htons(predesessor_ip.sin_port),succesor_id,suc,htons(succesor_ip.sin_port),predesessor_id);
  
}
inline int Node::find_succesor(unsigned long long int id,struct sockaddr_in addr)
{   
    if(Hash_ID==succesor_id ) // only one node
    {   
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
     }
    else  if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // return ip of succsor
    {
        char *buffer=new char[BUFFER_SIZE]; 

        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",succesor_id,succesor_ip.sin_addr.s_addr,succesor_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else  // forward query along the circle
    {
        // ask for
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,addr.sin_addr.s_addr,htons(addr.sin_port));
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
        delete[] buffer;
        return 1;
    }
          
}
inline void Node::join_node(unsigned long long int id,struct sockaddr_in addr)
{
    if(Hash_ID==succesor_id ) // only one node
    {   
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"JOINRSLT<%llu,%llu><%d:%d>",Hash_ID,Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);

        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        
        // make the requesting node your succesor
        succesor_id=id;
        succesor_ip=addr;

        char pre[INET_ADDRSTRLEN],suc[INET_ADDRSTRLEN];
        struct in_addr preadd=predesessor_ip.sin_addr,sucadd=succesor_ip.sin_addr;
        inet_ntop(AF_INET,&(preadd),pre,INET_ADDRSTRLEN);
        inet_ntop(AF_INET,&(sucadd),suc,INET_ADDRSTRLEN);       
        printf("ring updated me=%s::%d(%llu) predeccesor=%s::%d(%llu) successor=%s::%d(%llu)\n",ipstr,htons(my_ip.sin_port),Hash_ID,pre,htons(predesessor_ip.sin_port),succesor_id,suc,htons(succesor_ip.sin_port),predesessor_id);
        return ;
    }
        
    else  if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // return ip of succsor
    {
        char *buffer=new char[BUFFER_SIZE]; 

        sprintf(buffer,"JOINRSLT<%llu,%llu><%d:%d>",Hash_ID,succesor_id,succesor_ip.sin_addr.s_addr,succesor_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;

        // make the requesting node your succesor
        succesor_id=id;
        succesor_ip=addr;

        char pre[INET_ADDRSTRLEN],suc[INET_ADDRSTRLEN];
        struct in_addr preadd=predesessor_ip.sin_addr,sucadd=succesor_ip.sin_addr;
        inet_ntop(AF_INET,&(preadd),pre,INET_ADDRSTRLEN);
        inet_ntop(AF_INET,&(sucadd),suc,INET_ADDRSTRLEN);       
        printf("ring updated me=%s::%d(%llu) predeccesor=%s::%d(%llu) successor=%s::%d(%llu)\n",ipstr,htons(my_ip.sin_port),Hash_ID,pre,htons(predesessor_ip.sin_port),succesor_id,suc,htons(succesor_ip.sin_port),predesessor_id);
        return ;
    }
    else  // forward query along the circle
    {
        
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"JOIN<%llu><%d:%d>",id,addr.sin_addr.s_addr,addr.sin_port);
        
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
        delete[] buffer;
                
        // // read result
        // struct sockaddr_in cli_addr;
        // socklen_t clilen = sizeof(cli_addr);
        // memset(buffer,'\0',BUFFER_SIZE);
        // if(recvfrom(sockfd,buffer,BUFFER_SIZE-1,0,(struct sockaddr*)&cli_addr,&clilen)<0) perror("recvfrom");
        return ;
    }
}
inline void  Node::upload_file()
{   
     // // polulate file list from directory
      char* filelist[MAXFILES_PER_CLIENT];    
      unsigned long long int file_hash[MAXFILES_PER_CLIENT]; 
      int filelist_size=0;

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
            filelist[filelist_size]=strdup(pDirent->d_name);
            unsigned long long int id=oat_hash(pDirent->d_name);   
            file_hash[filelist_size++]=id;
        cout<<"trying to psuh gj hgfhjkghkgnhk"<<id<<" "<<Hash_ID<<" "<<succesor_id<<endl;    
            // ask for succesor
        if(Hash_ID==succesor_id) // only one node
        {   
            cout<<"push in current"<<endl;
            hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
        }
        else  if((id>predesessor_id &&  id<=Hash_ID) || (Hash_ID<=predesessor_id && (id<=Hash_ID || id>predesessor_id))) // return ip of succsor
        {
            cout<<"push in own"<<endl;
            // char *buffer=new char[BUFFER_SIZE]; 
            // memset(buffer,'\0',BUFFER_SIZE);
            // sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
            // if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("send");
            // delete[] buffer;
            hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
        }else
        {
            cout<<"ask current"<<endl;
            char *buffer=new char[BUFFER_SIZE]; 
            sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
            if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
           
            //  char buffer[BUFFER_SIZE]; 
            memset(buffer,'\0',BUFFER_SIZE);

            // used for storing client address while receiving
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);

            int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
            if (n_ < 0) perror("ERROR reading from socket");

            unsigned long long int id;
            int address,port;
            sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id,&address,&port);

            struct sockaddr_in send_to;
            send_to.sin_family=AF_INET;
            send_to.sin_addr.s_addr=address;
            send_to.sin_port=port;

            memset(buffer,'\0',BUFFER_SIZE);
            sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
            if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&send_to,sizeof(struct sockaddr))<0) perror("send");
     
            delete[] buffer;
        } 
          }
      }
        closedir (pDir);
 }
inline void  Node::hash_update(unsigned long long int id,struct sockaddr_in addr)
{
    hashed_file.push_back(make_pair(id,addr));
}

inline void Node::print_hash()
{   cout<<"current filelist of node "<<ipstr<<":"<<htons(my_ip.sin_port)<<" is"<<endl;
    for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=hashed_file.begin();it!=hashed_file.end();it++)
        {
            char ip_file[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&(it->second.sin_addr),ip_file,INET_ADDRSTRLEN);
            cout<<it->first<<"\t"<<ip_file<<":"<<htons(it->second.sin_port)<<endl;
        }
}

inline unsigned long long int  Node::getID()
{
    return Hash_ID;
} 

inline unsigned long long int  Node::getsucID()
{
    return succesor_id;
} 

inline unsigned long long int  Node::getpreID()
{
    return predesessor_id;
} 

inline struct sockaddr_in Node::getIP()
{
    return my_ip;
}
inline struct sockaddr_in Node::getsucIP()
{
    return  succesor_ip;
}
inline struct sockaddr_in Node::getpreIP()
{
    return predesessor_ip;
    
}
inline char* Node::getipstr()
{
    char *dup=strdup(ipstr);
    return dup;
}
inline int Node::getsocket()
{
    return sockfd;
}