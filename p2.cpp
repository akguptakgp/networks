#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <ifaddrs.h> 
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>

#define length 256

using namespace std;

class Node{
public:
	char ip[length],port[length];
	long long id,ids;
	char ips[length],ports[length];
	Node* pre(long long int index){
		while(1){
			//if(id1 < node.id && successor(id1) > node.id) break;
			if(id1 < node.id &&  > node.id)
				break;


		}


	}
	Node* predecessor(){

	}
};

void distribute(int index){
	char buff[length],wd[length];
    getcwd(wd,length);
	strcat(wd,"/client_data");      strcat(wd,index);
    DIR* dirp = opendir(wd);
    struct dirent* sd;
    while((sd=readdir(dirp))!=NULL){
        sprintf(buff,"%s ",sd->d_name);
        if(buff[0] == '.')
            continue;
        //strcat(sendbuff,buff);
        printf("%s\n",buff);
    }
    closedir(dirp);
}


void getip(char buff2[]){
	struct ifaddrs *addrs,*tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp){
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && !strcmp(tmp->ifa_name,"wlan0")){
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            sprintf(buff2,"%s",inet_ntoa(pAddr->sin_addr));
            break;
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
}

unsigned long long hash(const char *p){
    int len = strlen(p);
    unsigned long long h = 0;
    for (int i = 0; i < len; i++){
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    return h;
}

// argv[1] = id
// argv[2] = ip of any node
// argv[3] = port of that node

int main(int args,char* argv[]){
	if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL){
		printf("Enter command line parameters\n");
		return 0;
	}
	
	/* store the ip , port and id of the node */
	Node node;
	getip(node.ip);			
	sprintf(node.port,"%d",10000+atoi(argv[1]));
	char ip1[length];
	sprintf(ip1,"%s",node.ip);
	strcat(ip1,":");
	strcat(ip1,node.port);
	node.id = hash(ip1);

	/* set up a datagram socket */
	struct sockaddr_in serv_addr,cl_addr;
	int sockfd = socket(AF_INET,SOCK_DGRAM, 0);
	
	cl_addr.sin_family = AF_INET;
	inet_pton(AF_INET,node.ip,&(cl_addr.sin_addr));
    cl_addr.sin_port = atoi(node.port);
    bind(sockfd, (struct sockaddr*)&cl_addr, sizeof(cl_addr)); 
	
	/* ask the given node in ring to find my predessecor in the form REQFORSUCOF id ip port */
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET,node.ip,&(serv_addr.sin_addr));
	serv_addr.sin_port = atoi(node.port);
	char rbuf[length],sendbuff[length];
	sprintf(sendbuff,"REQFORSUCOF ");
	sprintf(rbuf,"%d ",node.id);
	strcat(sendbuff,rbuf);
	strcat(sendbuff," ");
	strcat(sendbuff,node.ip);
	strcat(sendbuff," ");
	sprintf(sendbuff,"%d",node.port);
	
	sendto(sockfd,sendbuff,sizeof(sendbuff),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

	
	/* get the successor from the node (which is already existing)	*/
	char recvbuff[length];
	socklen_t srn;
    recvfrom(sockfd,recvbuff,length,0,(struct sockaddr*)&serv_addr,&srn);
    sscanf(recvbuff,"%s",ips);
    int i =0;
    long long index;
    /* the recvbuff is of the form SUCINFO ip port */
    if(!strcmp(ips,"SUCINFO")){
    	i+= (strlen(ips) + 1);
    	sscanf(&recvbuff[i],"%s",ips);
	    i+= (strlen(ips) + 1);
	    sscanf(&recvbuff[i],"%s",ports);	
    }
    /* the recvbuff is of the form REQFORSUCOF id ip port*/
    else if(!strcmp(ips,"REQFORSUCOF")){
    	i+= (strlen(ips) + 1);
    	sscanf(&recvbuff[i],"%lld",rbuf);
    	index = atoi(rbuf);
    	i+= (strlen(rbuf) + 1);
    	if(index > node.ip){
    		// send the same REQFORSUCOF to the forward node
	    	sendto(sockfd,recvbuff,sizeof(recvbuff),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    	}
    	else{
    		// send my ip to index in the format ANS ip port
    		sprintf(recvbuff,"ANS ");
    		strcat(recvbuff,node.ip);
    		strcat(recvbuff," ");
    		strcat(recvbuff,node.port);
    		sscanf(&recvbuff[i],"%s",rbuf);
    		i+= (strlen(rbuf) + 1);
    		inet_pton(AF_INET,rbuf,&(cl_addr.sin_addr));
    		sscanf(&recvbuff[i],"%s",rbuf);
    		i+= (strlen(rbuf) + 1);
			cl_addr.sin_port = atoi(rbuf);
    		sendto(sockfd,recvbuff,sizeof(recvbuff),0,(struct sockaddr*)&cl_addr,sizeof(cl_addr));	
    	}

    }

    /**/

	distribute(x1);
    


    //client_addr.sin_port = 10000+x1;
    //if(bind(sockfd, (struct sockaddr*)&client_addr,sizeof(client_addr)) < 0) perror("Ankit\n");

}