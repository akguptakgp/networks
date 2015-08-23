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
#include <climits>
#include "util.cpp"
#include <fstream>
#include <iomanip>

#define DIR_SIZE 50
#define BUFFER_SIZE 1000
#define MAXFILES_PER_CLIENT 50
#define FILE_BUFFER_SIZE 10000

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
    delete[] ip_port_str;
    if(fork()==0) 
    {
        startsever();
    }

    // create the udp socket to connect to the other client
    if((sockfd =socket(AF_INET, SOCK_DGRAM,0)) == -1) {  perror("socket creation"); exit(0);}
    if (bind(sockfd,(struct sockaddr*)&my_ip,sizeof(struct sockaddr_in)) == -1) {perror("bind socket");exit(0);} 

    //finger_table.resize(64);
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
        int address;
        int port;
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
            
        
        // find the succesor and update the finger table
        memset(buffer,'\0',BUFFER_SIZE);
        
        delete[] buffer;
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
        return ;
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

inline void Node::init_finger()
{
    // update_finger();
    // char *buffer=new char[BUFFER_SIZE];
    // sprintf(buffer,"INITFINGER<%llu><%u:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
    // if(succesor_ip.sin_addr.s_addr !=my_ip.sin_addr.s_addr ||  succesor_ip.sin_port!=my_ip.sin_port)
    // {
    //      // forward the request 
    //       if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
    // } 
    // delete[] buffer;

   cout<<"called init_finger finger"<<endl;
   finger_table.resize(0); 
   unsigned long long int one=1;
   if(Hash_ID==succesor_id)          
    {
        for(int i=0;i<m;i++)
        {
            unsigned long long int p=(one<<i);
            unsigned long long int start=(Hash_ID+p)%ULLONG_MAX;
            finger_table.push_back(make_pair(start,my_ip));
        }
    }
    else
    {
        char pre[INET_ADDRSTRLEN];
        char *ip_port_str=new char[INET_ADDRSTRLEN+10];
        // update 1st entry
        finger_table.push_back(make_pair(Hash_ID+1,succesor_ip));   

        for(int i=1;i<m;i++)
        {
            unsigned long long int p=(one<<i);
            unsigned long long int start=(Hash_ID+p)%ULLONG_MAX;
            
            memset(pre,'\0',INET_ADDRSTRLEN);
            inet_ntop(AF_INET,&(finger_table[i].second.sin_addr),pre,INET_ADDRSTRLEN);
            
            memset(ip_port_str,'\0',INET_ADDRSTRLEN+10);
            sprintf(ip_port_str,"%s:%d",pre,htons(finger_table[i].second.sin_addr));   

            if(start>=Hash_ID && start< oat_hash(ip_port_str))
            {
                finger_table.push_back(make_pair(start,finger_table[i].second)); 
            }
            else
            {
                struct sockaddr_in result=local_find_succesor(start);
                if(result.sin_family!=65535)
                {
                    //cout<<"succesor res"<<htons(result.sin_port)<<endl;
                    finger_table.push_back(make_pair(start,result));
                }
                else  // read result
                {
                    char *buffer=new char[BUFFER_SIZE]; 
                    memset(buffer,'\0',BUFFER_SIZE);

                    // used for storing client address while receiving
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);

                    int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
                    if (n_ < 0) perror("ERROR reading from socket");

                    unsigned long long int id1;
                    int address;
                    int port;
                    sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id1,&address,&port);

                    struct sockaddr_in push_to;
                    push_to.sin_family=AF_INET;
                    push_to.sin_addr.s_addr=address;
                    push_to.sin_port=port;
                    finger_table.push_back(make_pair(start,push_to));
                    delete[] buffer;
                }
            }   
        }
    }
    delete[] ip_port_str;
    print_finger();
    cout<<"return update finger"<<endl;
}

inline void Node::update_others()
{
    for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=finger_table.begin();it!=finger_table.end();it++)
    {
        char ip_file[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&(it->second.sin_addr),ip_file,INET_ADDRSTRLEN);
        outputfile<<right<<setw(20)<<it->first<<"\t---->\t"<<ip_file<<":"<<htons(it->second.sin_port)<<endl;
    }
}

inline void Node::update_finger()
{   
   
}
inline void Node::print_finger()
{   
    ofstream outputfile;
    char file[100];
    sprintf(file,"result_finger%d.txt",ID);
    outputfile.open(file,ios::out); 
    outputfile<<"finger table of Node\t"<<ipstr<<":"<<htons(my_ip.sin_port)<<" is"<<endl;
   for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=finger_table.begin();it!=finger_table.end();it++)
    {
        char ip_file[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&(it->second.sin_addr),ip_file,INET_ADDRSTRLEN);
        outputfile<<right<<setw(20)<<it->first<<"\t---->\t"<<ip_file<<":"<<htons(it->second.sin_port)<<endl;
    }
}

inline struct sockaddr_in  Node::local_find_succesor(unsigned long long int id)
{
    if(Hash_ID==succesor_id ) // only one node special case
    {   
        return my_ip;
    }
    else if((Hash_ID>= id && id>predesessor_id) || (Hash_ID<predesessor_id && (id>predesessor_id || id<Hash_ID))) // check whether u can be succesor
    {
        // cout<<"one"<<endl;
        return my_ip;
    }
    else  if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // check whether your succesor can be succesor
    {
        // cout<<"second"<<endl;
        return succesor_ip;
    }
    else  // forward query along the circle
    {
        // cout<<"third"<<endl;
        // first check for suitable node to forward the query using finger table
        typedef vector< pair<unsigned long long int,struct sockaddr_in> >::iterator iter_type;
       
        iter_type from (finger_table.begin());                
        iter_type until (finger_table.end());  
        reverse_iterator<iter_type> rev_until (from);
        reverse_iterator<iter_type> rev_from (until);    

        while(rev_from++ != rev_until)
        {
            if(rev_from->first > Hash_ID && rev_from->first< id)
            {
                // cout<<"looping"<<endl;
                char *buffer=new char[BUFFER_SIZE]; 
                sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(rev_from->second),sizeof(struct sockaddr))<0) perror("sending done");
                delete[] buffer;
                break;
            }
        }
        struct sockaddr_in invalid;
        invalid.sin_family=65535; 
        return invalid; 
    }          
}

inline int Node::find_predesecor(unsigned long long int id,struct sockaddr_in addr)
{   
    if(Hash_ID==succesor_id ) // only one node special case
    {   
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else if((Hash_ID>= id && id>predesessor_id) || (Hash_ID<predesessor_id && (id>predesessor_id || id<Hash_ID))) // check whether u can be succesor
    {
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else  if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // check whether your succesor can be succesor
    {
        char *buffer=new char[BUFFER_SIZE]; 

        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",succesor_id,succesor_ip.sin_addr.s_addr,succesor_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else  // forward query along the circle
    {
        // first check for suitable node to forward the query using finger table
        typedef vector< pair<unsigned long long int,struct sockaddr_in> >::iterator iter_type;
       
        iter_type from (finger_table.begin());                
        iter_type until (finger_table.end());  
        reverse_iterator<iter_type> rev_until (from);
        reverse_iterator<iter_type> rev_from (until);    

        while(rev_from++ != rev_until)
        {
            if(rev_from->first > Hash_ID && rev_from->first< id)
            {
                // cout<<"looping"<<id<<endl;
                char *buffer=new char[BUFFER_SIZE]; 
                sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,addr.sin_addr.s_addr,addr.sin_port);
                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(rev_from->second),sizeof(struct sockaddr))<0) perror("senting done");
                delete[] buffer;
                return 1;
            }
        }
    }          
}

inline int Node::find_succesor(unsigned long long int id,struct sockaddr_in addr)
{   
    if(Hash_ID==succesor_id ) // only one node special case
    {   
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else if((Hash_ID>= id && id>predesessor_id) || (Hash_ID<predesessor_id && (id>predesessor_id || id<Hash_ID))) // check whether u can be succesor
    {
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else  if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // check whether your succesor can be succesor
    {
        char *buffer=new char[BUFFER_SIZE]; 

        sprintf(buffer,"SUCCRSLT<%llu><%d:%d>",succesor_id,succesor_ip.sin_addr.s_addr,succesor_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;
        return 1;
    }
    else  // forward query along the circle
    {
        // first check for suitable node to forward the query using finger table
        typedef vector< pair<unsigned long long int,struct sockaddr_in> >::iterator iter_type;
       
        iter_type from (finger_table.begin());                
        iter_type until (finger_table.end());  
        reverse_iterator<iter_type> rev_until (from);
        reverse_iterator<iter_type> rev_from (until);    

        while(rev_from++ != rev_until)
        {
            if(rev_from->first > Hash_ID && rev_from->first< id)
            {
                // cout<<"looping"<<id<<endl;
                char *buffer=new char[BUFFER_SIZE]; 
                sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,addr.sin_addr.s_addr,addr.sin_port);
                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(rev_from->second),sizeof(struct sockaddr))<0) perror("senting done");
                delete[] buffer;
                return 1;
            }
        }
    }          
}

inline  void Node::leave()
{
    char *buffer=new char[BUFFER_SIZE]; 
    sprintf(buffer,"LEAVE<%llu><%d:%d>",succesor_id,succesor_ip.sin_addr.s_addr,succesor_ip.sin_port);
    if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(predesessor_ip),sizeof(struct sockaddr))<0) perror("send");
    delete[] buffer;
    return ;
}

inline void Node::leave_request(unsigned long long int id,struct sockaddr_in addr)
{
    char *buffer=new char[BUFFER_SIZE]; 
    succesor_id=id;
    succesor_ip=addr;
    memset(buffer,'\0',BUFFER_SIZE);
    sprintf(buffer,"NOTIFY<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
    if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("message send");
    // find the succesor and update the finger table
    memset(buffer,'\0',BUFFER_SIZE);
    delete[] buffer;

    char pre[INET_ADDRSTRLEN],suc[INET_ADDRSTRLEN];
    struct in_addr preadd=predesessor_ip.sin_addr,sucadd=succesor_ip.sin_addr;
    inet_ntop(AF_INET,&(preadd),pre,INET_ADDRSTRLEN);
    inet_ntop(AF_INET,&(sucadd),suc,INET_ADDRSTRLEN);   
    printf("ring updated me=%s::%d(%llu) predeccesor=%s::%d(%llu) successor=%s::%d(%llu)\n",ipstr,htons(my_ip.sin_port),Hash_ID,pre,htons(predesessor_ip.sin_port),succesor_id,suc,htons(succesor_ip.sin_port),predesessor_id);
    init_finger();
}

inline void  Node::upload_file()
{   
     // // polulate file list from directory
      // char* filelist[MAXFILES_PER_CLIENT];    
      // unsigned long long int file_hash[MAXFILES_PER_CLIENT]; 
      // int filelist_size=0;
    cout<<"upload_file called"<<endl;
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

        unsigned long long int id=oat_hash(pDirent->d_name); 
        struct sockaddr_in send_to;  
        
        if(Hash_ID==succesor_id ) // only one node special case
        {   
            hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
        }
        else if((Hash_ID>= id && id>predesessor_id) || (Hash_ID<predesessor_id && (id>predesessor_id || id<Hash_ID))) // check whether u can be succesor
        {
            hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
        }
        else  
        {
            char *buffer=new char[BUFFER_SIZE]; 
            if((id>Hash_ID && ( id<=succesor_id || Hash_ID>=succesor_id)) || (Hash_ID>=succesor_id && succesor_id>id)) // check whether your succesor can be succesor
            {
                send_to=succesor_ip;
                char ip_file[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&(send_to.sin_addr),ip_file,INET_ADDRSTRLEN);
                cout<<"sending to1 "<<ip_file<<":"<<htons(send_to.sin_port)<<endl;   
            }
            else  // forward query along the circle
            {
                // first check for suitable node to forward the query using finger table
                typedef vector< pair<unsigned long long int,struct sockaddr_in> >::iterator iter_type;
               
                iter_type from (finger_table.begin());                
                iter_type until (finger_table.end());  
                reverse_iterator<iter_type> rev_until (from);
                reverse_iterator<iter_type> rev_from (until);    

                while(rev_from++ != rev_until)
                {
                    if(rev_from->first > Hash_ID && rev_from->first< id)
                    {
                        cout<<htons(rev_from->second.sin_port)<<endl;
                        sprintf(buffer,"%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
                        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(rev_from->second),sizeof(struct sockaddr))<0) perror("senting done");
                        delete[] buffer;
                        break;
                    }
                }
                memset(buffer,'\0',BUFFER_SIZE);
                struct sockaddr_in cli_addr;
                socklen_t clilen = sizeof(cli_addr);

                int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
                if (n_ < 0) perror("ERROR reading from socket");
                else cout<<"dsgggggggggggggggggggggggggggg=="<<n_<<endl;
                unsigned long long int id1;
                int address;
                int port;
                sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id1,&address,&port);

                printf("%s\n",buffer);

                char ip_file[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&(address),ip_file,INET_ADDRSTRLEN);
                cout<<"sending to2 "<<ip_file<<":"<<htons(port)<<endl;   

                send_to.sin_family=AF_INET;
                send_to.sin_addr.s_addr=address;
                send_to.sin_port=port;

                //char ip_file[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&(send_to.sin_addr.s_addr),ip_file,INET_ADDRSTRLEN);
                cout<<"sending to2 "<<ip_file<<":"<<htons(send_to.sin_port)<<endl;   
            } 

                memset(buffer,'\0',BUFFER_SIZE);
                sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",oat_hash(pDirent->d_name),my_ip.sin_addr.s_addr,my_ip.sin_port);



                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&(send_to),sizeof(struct sockaddr))<0) perror("send dsgdfg");
                delete[] buffer;
            }
        }     


        // if((id>predesessor_id && id<=Hash_ID) || (Hash_ID<predesessor_id && id>predesessor_id) || (Hash_ID==succesor_id)) // return ip of succsor
        // {
        //     // cout<<"push in own"<<endl;
        //     hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
        // }
        // else
        // {
        //      // file_hash[filelist_size++]=id;
        //         // cout<<"trying to push"<<id<<" "<<Hash_ID<<" "<<succesor_id<<endl;    

        //         char *buffer=new char[BUFFER_SIZE]; 
        //         sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
        //         if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
               
        //         //  char buffer[BUFFER_SIZE]; 
        //         memset(buffer,'\0',BUFFER_SIZE);

        //         // used for storing client address while receiving
        //         struct sockaddr_in cli_addr;
        //         socklen_t clilen = sizeof(cli_addr);

        //         int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
        //         if (n_ < 0) perror("ERROR reading from socket");

        //         unsigned long long int id1;
        //        int address;
        //         int port;
        //         sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id1,&address,&port);



        //         if((my_ip.sin_port==port && my_ip.sin_addr.s_addr==address) || Hash_ID==predesessor_id) // saame as mr
        //         {   
        //             // cout<<"push in own"<<endl;
        //             hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));

        //         }
        //         else
        //         {   
                
        //         send_to.sin_family=AF_INET;
        //         send_to.sin_addr.s_addr=address;
        //         send_to.sin_port=port;


        //         char ip_file[INET_ADDRSTRLEN];
        //         inet_ntop(AF_INET,&(send_to.sin_addr),ip_file,INET_ADDRSTRLEN);
        //         // cout<<"sending to "<<ip_file<<":"<<htons(port)<<endl;

               

        //         memset(buffer,'\0',BUFFER_SIZE);
        //         sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",oat_hash(pDirent->d_name),my_ip.sin_addr.s_addr,my_ip.sin_port);
        //         if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&send_to,sizeof(struct sockaddr))<0) perror("send");
         
        //         delete[] buffer;
        //          }
          
               
        // } 
      //}
  }
    closedir (pDir);
    if(Hash_ID!=succesor_id) 
    {  
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"ASKFILE<%llu><%d:%d>",Hash_ID,my_ip.sin_addr.s_addr,my_ip.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
          
    
        struct sockaddr_in cli_addr;
        socklen_t clilen;

        memset(buffer,'\0',BUFFER_SIZE);    
        while(1)
        {
        recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
           if(!strcmp(buffer,"-1")) break; 
          // printf("%s\n",buffer);  
          struct sockaddr_in tmp;
            unsigned long long int id;
        int address;
        int port;
        sscanf(buffer,"ASKRSLT<%llu><%d:%d>",&id,&address,&port);
        tmp.sin_family=AF_INET;
        tmp.sin_addr.s_addr=address;
        tmp.sin_port=port;
        hashed_file.push_back(make_pair(id,tmp)); 
          memset(buffer,'\0',BUFFER_SIZE);    
        }

       delete[] buffer;
    }
    print_hash();
    // read file and update 
    cout<<"upload_file finished"<<endl;
 }

inline void Node::print_hash()
{
    ofstream outputfile;
    char file[100];
    sprintf(file,"result_filehash%d.txt",ID);
    outputfile.open(file,ios::out); 
  
    outputfile<<"current filelist of "<<ipstr<<":"<<htons(my_ip.sin_port)<<"is"<<endl;
   for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=hashed_file.begin();it!=hashed_file.end();it++)
    {
         char ip_file[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&(it->second.sin_addr),ip_file,INET_ADDRSTRLEN);
        outputfile<<it->first<<"\t"<<ip_file<<":"<<htons(it->second.sin_port)<<endl;
    }
    return;
}

inline void  Node::hash_update(unsigned long long int id,struct sockaddr_in addr)
{
    hashed_file.push_back(make_pair(id,addr));
    print_hash();
}

inline void  Node::send_files(unsigned long long int id,struct sockaddr_in addr)
{
    char *buffer=new char[BUFFER_SIZE]; 
    std::vector<vector< pair<unsigned long long int,struct sockaddr_in> >::iterator > temp;
    for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=hashed_file.begin();it!=hashed_file.end();it++)
        {
            unsigned long long int id1=it->first;

            if(Hash_ID==succesor_id ) // only one node special case
            {   
               ;
            }
            else if((Hash_ID>= id1 && id1>predesessor_id) || (Hash_ID<predesessor_id && (id1>predesessor_id || id1<Hash_ID))) // check whether u can be succesor
            {
               ;
            }
            else  // forward query along the circle
            {
                
               

            // // char *buffer=new char[BUFFER_SIZE]; 
            // sprintf(buffer,"SUCCINQ<%llu><%d:%d>",it->first,my_ip.sin_addr.s_addr,my_ip.sin_port);
            // if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
           
            // //  char buffer[BUFFER_SIZE]; 
            // memset(buffer,'\0',BUFFER_SIZE);

            // // used for storing client address while receiving
            // struct sockaddr_in cli_addr;
            // socklen_t clilen = sizeof(cli_addr);

            // int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
            // if (n_ < 0) perror("ERROR reading from socket");

            // unsigned long long int id1;
            // unsigned long address;
            // int port;
            // sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id1,&address,&port);

            // if((my_ip.sin_port==port && my_ip.sin_addr.s_addr==address) || Hash_ID==predesessor_id) // saame as mr
            // {   
            //   cout<<it->first<<" is my file"<<endl;
            // }
            // else
            // {   
                 // cout<<"sending file"<<endl;
                memset(buffer,'\0',BUFFER_SIZE);
                sprintf(buffer,"ASKRSLT<%llu><%d:%d>",it->first,it->second.sin_addr.s_addr,it->second.sin_port);
                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("senting done");
                temp.push_back(it);
            
            } 
          
            // char ip_file[INET_ADDRSTRLEN];
            // inet_ntop(AF_INET,&(it->second.sin_addr),ip_file,INET_ADDRSTRLEN);
            // cout<<it->first<<"\t"<<ip_file<<":"<<htons(it->second.sin_port)<<endl;
        }
        // send
        memset(buffer,'\0',BUFFER_SIZE);
        sprintf(buffer,"-1");
    if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0) perror("senting done");
              

        for (int i = 0; i < temp.size(); ++i)
        {
            hashed_file.erase(temp[i]);
        }
        delete[] buffer;
       print_hash();    
}

inline struct sockaddr_in Node::localsearch(string filename)
{ 
    struct sockaddr_in invalid;
    invalid.sin_family=65535;    
    struct sockaddr_in result;

    unsigned long long int to_srch=oat_hash(filename.c_str());
    for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=hashed_file.begin();it!=hashed_file.end();it++)
    {
         if(to_srch==it->first)
          {

                result.sin_family=AF_INET;
                result.sin_addr.s_addr=it->second.sin_addr.s_addr;
                result.sin_port=ntohs(htons(it->second.sin_port)+30);
        

                char ip_tmp[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&(result.sin_addr),ip_tmp,INET_ADDRSTRLEN);
                // printf("File found at %s:%d\n",ip_tmp,htons(result.sin_port));
              //  it->second=ntohs(htons(it->second.sin_port)+30);
                return result;
                // return it->second;
            }   
    }  
           
    // ask succesor
     // ask succsor to search file
    char *buffer=new char[BUFFER_SIZE]; 
    sprintf(buffer,"SRCHFILE<%llu><%d:%d>",to_srch,my_ip.sin_addr.s_addr,my_ip.sin_port);
    if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
              
    memset(buffer,'\0',BUFFER_SIZE);
    
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
    if (n_ < 0) perror("ERROR reading from socket");

    if(!strcmp(buffer,"-1"))
    {
        printf("file not found\n");
       // printf("dfgfg%d",invalid.sin_family);
        return invalid;
    }
    else
    {
        unsigned long long int id1;
        int address;
        int port;
        sscanf(buffer,"SRCHRSLT<%llu><%d:%d>",&id1,&address,&port);


        result.sin_family=AF_INET;
        result.sin_addr.s_addr=address;
        result.sin_port=ntohs(htons(port)+30);
        return result;

        char ip_tmp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&(address),ip_tmp,INET_ADDRSTRLEN);
        // printf("File found at %s:%d\n",ip_tmp,htons(port));

        
        

    }


}


inline struct sockaddr_in  Node::search(unsigned long long int id,struct sockaddr_in send_to)
{
    struct sockaddr_in invalid;
    invalid.sin_family=65535;   


     for(vector< pair<unsigned long long int,struct sockaddr_in> >::iterator it=hashed_file.begin();it!=hashed_file.end();it++)
    {
         if(id==it->first)
          {
            if(send_to.sin_port!=my_ip.sin_port ) // request was not generate from me
            {
               char *buffer=new char[BUFFER_SIZE]; 

                sprintf(buffer,"SRCHRSLT<%llu><%d:%d>",id,it->second.sin_addr.s_addr,it->second.sin_port);
                if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&send_to,sizeof(struct sockaddr))<0) perror("send");
                delete[] buffer;
                return invalid;
            } 
            else
            {
                return it->second;
            }   
          }  
           
    }
    if(send_to.sin_port==succesor_ip.sin_port && send_to.sin_addr.s_addr==succesor_ip.sin_addr.s_addr) // request was generated from Succsor 
    {

       char *buffer=new char[BUFFER_SIZE]; 

        sprintf(buffer,"-1");
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&send_to,sizeof(struct sockaddr))<0) perror("send");
        delete[] buffer;

        return invalid;
    }
    else
    {
        // ask succsor to search file
        char *buffer=new char[BUFFER_SIZE]; 
        sprintf(buffer,"SRCHFILE<%llu><%d:%d>",id,send_to.sin_addr.s_addr,send_to.sin_port);
        if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
              
       // memset(buffer,'\0',BUFFER_SIZE);
        delete[] buffer;
        return invalid;
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

inline void Node::startsever()
{
    int socketfd=socket(AF_INET,SOCK_STREAM,0);
    if(socketfd<0) perror("Error opening socket");

    int opt = 1;
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    // if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }   

    // prepare server address
    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=ntohs(10030+ID);
    char main_ip[INET_ADDRSTRLEN];
    strcpy(main_ip,"127.0.0.1");

    inet_pton(AF_INET,main_ip,&(serveraddr.sin_addr));


    if(bind(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0)
        perror("bind node server Error");

    if(listen(socketfd,5)<0) perror("listen");       
    
    struct sockaddr_in clientaddr;
    socklen_t clilen=sizeof(clientaddr);
    
    memset(ipstr,'\0',INET_ADDRSTRLEN);
    inet_ntop(serveraddr.sin_family,&serveraddr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN);
   
    printf("Node server up on %s::%d waiting for incoming connection.......\n",ipstr,htons(serveraddr.sin_port));

  char *buffer=new char[BUFFER_SIZE];
    while(1)
     {
        int connection;
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
        memset(buffer,'\0',BUFFER_SIZE);

        read(connection, buffer, BUFFER_SIZE-1);

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
        close(connection);
    }

close(socketfd);
}

inline void Node::download(struct sockaddr_in peer_server_addr,const char* file)
{
    if(fork())
    {
        char *filename=strdup(file);

    int sockfd_TCP;
    // struct addrinfo hints, *res, *p;
    int status;

    int clinet_port_no=10050+ID;
    struct sockaddr_in client_addr;
    memset(&client_addr,'\0',sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(clinet_port_no);  
    
    char main_ip[INET_ADDRSTRLEN];
    strcpy(main_ip,"127.0.0.1");
    inet_pton(AF_INET,main_ip,&(client_addr.sin_addr));
    //client_addr.sin_addr.s_addr = INADDR_ANY;
    
 //    char ipstr[INET_ADDRSTRLEN];
 // struct ifaddrs * ifAddrStruct=NULL;
 //  struct ifaddrs * ifa=NULL;
 //  struct sockaddr_in * tmpAddrPtr=NULL;

 //  getifaddrs(&ifAddrStruct);

 //    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
 //        if (!ifa->ifa_addr) {
 //            continue;
 //        }
 //        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
 //            // is a valid IP4 Address
 //            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
 //            ///char addressBuffer[INET_ADDRSTRLEN];
 //            //inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
 //            if(startswith(ifa->ifa_name,"lo"))
 //               {
 //                client_addr.sin_addr.s_addr=tmpAddrPtr->sin_addr.s_addr;

 //         break; 

 //               }
 //        }
 //    }
 //    if(ifa==NULL)
 //    {
 //        printf("error in binding socket\n");
 //    }   

 //    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);



 //    // inet_pton(AF_INET,"10.117.3.238", &(client_addr.sin_addr));

 //    client_addr.sin_port = htons(clinet_port_no);

 //    memset(&hints, 0, sizeof(hints));
 //    hints.ai_family = peer_server_addr.sin_family; 
 //    hints.ai_socktype =SOCK_STREAM;

 //    //char ipstr[INET_ADDRSTRLEN];
 //    memset(ipstr,'\0',INET_ADDRSTRLEN);
 //    inet_ntop(peer_server_addr.sin_family,&peer_server_addr.sin_addr.s_addr,ipstr,INET_ADDRSTRLEN); 

 //    char str_port[20];
 //    sprintf(str_port,"%d",peer_server_addr.sin_port);

    if ((sockfd_TCP = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) == -1) 
        {
                perror("socket");
                exit(0);
        }
    // int true_ = 1;
    // setsockopt(sockfd_TCP,SOL_SOCKET,SO_REUSEADDR,&true_,sizeof(int));

    // if(bind(sockfd_TCP,(struct sockaddr *)&client_addr,sizeof(client_addr))<0)
    // {
    //     perror("bind Error");   
    //     exit(0);
    // }
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
    // close(sockfd_TCP);
    delete[] dup_file_name;
    }
    
}


// if(Hash_ID==succesor_id) // only one node
//         {   
//             cout<<"push in current"<<endl;
//             hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
//         }
//         else  if((id>predesessor_id &&  id<=Hash_ID) || (Hash_ID<=predesessor_id && (id<=Hash_ID || id>predesessor_id))) // return ip of succsor
//         {
//             cout<<"push in own"<<endl;
//             // char *buffer=new char[BUFFER_SIZE]; 
//             // memset(buffer,'\0',BUFFER_SIZE);
//             // sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
//             // if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("send");
//             // delete[] buffer;
//             hashed_file.push_back(make_pair(oat_hash(pDirent->d_name),my_ip));
//         }else
//         {
//             cout<<"ask current"<<endl;
//             char *buffer=new char[BUFFER_SIZE]; 
//             sprintf(buffer,"SUCCINQ<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
//             if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&succesor_ip,sizeof(struct sockaddr))<0) perror("senting done");
           
//             //  char buffer[BUFFER_SIZE]; 
//             memset(buffer,'\0',BUFFER_SIZE);

//             // used for storing client address while receiving
//             struct sockaddr_in cli_addr;
//             socklen_t clilen = sizeof(cli_addr);

//             int n_= recvfrom(sockfd,buffer,BUFFER_SIZE,0,(struct sockaddr*)&cli_addr,&clilen);
//             if (n_ < 0) perror("ERROR reading from socket");

//             unsigned long long int id;
//             int address,port;
//             sscanf(buffer,"SUCCRSLT<%llu><%d:%d>",&id,&address,&port);

//             struct sockaddr_in send_to;
//             send_to.sin_family=AF_INET;
//             send_to.sin_addr.s_addr=address;
//             send_to.sin_port=port;

//             memset(buffer,'\0',BUFFER_SIZE);
//             sprintf(buffer,"HASHUPDATE<%llu><%d:%d>",id,my_ip.sin_addr.s_addr,my_ip.sin_port);
//             if(sendto(sockfd,buffer,strlen(buffer),0,(struct sockaddr*)&send_to,sizeof(struct sockaddr))<0) perror("send");
     
//             delete[] buffer;
//         } 