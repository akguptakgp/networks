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
#include <signal.h>

#define length 200
#define m 8


typedef unsigned int ui;

const char* ip_existing = "10.144.44.28";//"10.141.34.199";//10.132.155.1
int port_existing = 10000;
ui id_existing;

char* files[length];
int filecount = 0;

int nodecount = 0;

char* hashedfile[length];
int hashcount = 0;
int iiiii =0;
using namespace std;
/*
	start is (n+2^i)%64 
	succ is the next node in the ring from start ( inclusive )
	ports is the port number for succ
*/
struct finger_table_entry{
	ui start,succ;
	int ports;
	char ips[length];
};

typedef finger_table_entry finger_table;


unsigned int hash(const char *p){
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
    return h % 251;
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


/* true if x lies in [start,end) */
bool belongsto(ui start,ui end,ui x){
	if(end == start) return true;
	if(end > start && start <= x && x < end)
		return true;
	if(end < start && (start <= x || x < end))
		return true;
	return false;
}

class Node{
public:
	char ip[length];
	int port;
	ui id;
	finger_table ft[m];
	int sockfd,sockfd1;
	int listenfd,listenfd1;
	ui closest_preceding_finger(ui given_id,char[],int*);

	ui find_successor(ui given_id,char[],int*);

	void init_fingertable(ui given_id);

	void update_fingertable(ui given_id,char [],int);

	void join();

	void search(char [],char [],int*);

	void download(char []);

	void leave();

	void remove_from_fingertable(ui,ui,char[],int,char[],int);

	void filetransfer_tosucc();
};

/* closest node just greater than given_id complete 
	return the closest_preceding id , ip ,port
*/
ui Node::closest_preceding_finger(ui given_id,char ip1[],int* port1){
	for(int i=m-1;i>=0;i--){
		if(belongsto(id+1,given_id,ft[i].succ)){
			strcpy(ip1,ft[i].ips);
			*port1 = ft[i].ports;
			return ft[i].succ;
		}
	}
	strcpy(ip1,ip);
	*port1 = port;
	return id;
}

/* successor means a id >= given_id complete 
   return the successor id , ip ,port
*/
ui Node::find_successor(ui given_id,char ip1[],int* port1){
	ui n = given_id,nn;
	int port2;
	struct sockaddr_in addr;
	char buff[length],rbuff[length],ip2[length];
	addr.sin_family = AF_INET; 
	socklen_t srn; 
	if(id + 1 == given_id){
		n = id;
	}
	else{
		n = closest_preceding_finger(given_id,ip2,&port2);	
	}
	// printf("closest preceding succ of %u is %u",given_id,n);
	if(n == id){
		strcpy(ip1,ft[0].ips);
		*port1 = ft[0].ports;
		return ft[0].succ;
	}

	/* buff : find_successor given_id */
	inet_pton(AF_INET,ip2,&(addr.sin_addr)); addr.sin_port = port2;
	sprintf(buff,"find_successor %u",given_id);
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	// printf("msg sendt : %s to %d\n",buff,port2);

	srn  = sizeof(addr);
	/* buff : find_successor_return given_id ans_id ans_ip ans_port */
	recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
	// printf("msg got : %s from %d\n",buff,addr.sin_port);
	sscanf(buff,"%s",rbuff);
	if(strcmp(rbuff,"find_successor_return")){
		printf("Error 3\n");
		return 0;
	}
	int i=strlen(rbuff) + 1;
	sscanf(&buff[i],"%u",&nn);
	sscanf(&buff[i],"%s",rbuff);
	if(nn != given_id){
		printf("Error 4\n");
		return 0;
	}

	i+=(strlen(rbuff)+ 1);
	sscanf(&buff[i],"%u",&nn);
	sscanf(&buff[i],"%s",rbuff);

	i+=(strlen(rbuff)+ 1);
	sscanf(&buff[i],"%s",ip1);

	i+=(strlen(ip1)+ 1);
	sscanf(&buff[i],"%d",port1);

	return nn;
}

/* find dist between start (incl) and end (excl) */
int dist(int start,int end){
	if( start <= end) return end - start;
	return (ui)pow(2,m) - (start - end );

}


/* initialise finger table of given_id assuming that it is not the first node complete 
	and index my files to other nodes correctly
*/
void Node::init_fingertable(ui given_id){
	
	struct sockaddr_in addr;
	char buff[length],rbuff[length];
	socklen_t srn;
	addr.sin_family = AF_INET;
	// printf("+++%u\n",given_id);

	for(int i=0;i<m;i++){
		ft[i].start = (given_id + (ui)pow(2,i))%(ui)pow(2,m);
		printf("\nft[%d].start : %u\n",i,ft[i].start);
	}

	char wd[length];
    getcwd(wd,length);
	strcat(wd,"/client_data");   sprintf(buff,"%d",port-10000);   strcat(wd,buff);
	DIR* dirp = opendir(wd);
	if(!dirp){
    	printf("No directory to index from\n");
    	return ;
	}
    struct dirent* sd;
    ui nn;
    int kk;
    while((sd=readdir(dirp))!=NULL){
        sprintf(rbuff,"%s",sd->d_name);
        if(rbuff[0] == '.')
            continue;
        
        printf("hash(%s) : %u\n",rbuff,hash(rbuff));
        /* buff : find_successor given_id */
		inet_pton(AF_INET,ip_existing,&(addr.sin_addr)); addr.sin_port = port_existing;
		sprintf(buff,"find_successor %u",hash(rbuff));
		sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));

		/* buff : find_successor_return given_id ans_id ans_ip ans_port */
		recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
		// printf("In node join\n");
		// printf("got message : %s\n",buff);
		sscanf(buff,"%s",wd);
		if(strcmp(wd,"find_successor_return")){
			printf("Error 6\n");
			return ;
		}
		
		int i=strlen(wd) +1;
		sscanf(&buff[i],"%u",&nn);
		sscanf(&buff[i],"%s",wd);
		if(nn != hash(rbuff)){
			printf("Error 7\n");
			return ;
		}
		i+=(strlen(wd) +1);
		sscanf(&buff[i],"%u",&nn);
		sscanf(&buff[i],"%s",wd);

		i+=(strlen(wd) +1);
		sscanf(&buff[i],"%s",wd);

		i+=(strlen(wd) +1);
		sscanf(&buff[i],"%d",&kk);

		// nn stores the successor id , wd stores the ip , kk stores the port
		
		// if I am the node where the file is indexed
		if(dist(hash(rbuff) , nn) > dist(hash(rbuff) , id)){

			hashedfile[hashcount] = new char[length];
			/* hashedfile[i] : filename ip port */
			sprintf(hashedfile[hashcount],"%s %s %d",rbuff,ip,port);
			hashcount++;
		}
		else{

			/*buff : hash filename ip port */
			inet_pton(AF_INET,wd,&(addr.sin_addr)); addr.sin_port = kk;
			sprintf(buff,"hash %s %s %d",rbuff,ip,port);
			sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
		} 
    }
    closedir(dirp);

	/*buff : find_successor given_id */
	inet_pton(AF_INET,ip_existing,&(addr.sin_addr)); addr.sin_port = port_existing;
	sprintf(buff,"find_successor %u",ft[0].start);
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	
	/* buff : find_successor_return given_id ans_id ans_ip ans_port */
	recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
	// printf("In node join\n");
	// printf("got message : %s\n",buff);
	sscanf(buff,"%s",rbuff);
	if(strcmp(rbuff,"find_successor_return")){
		printf("Error 1\n");
		return ;
	}
	
	int i=strlen(rbuff) +1;
	sscanf(&buff[i],"%u",&nn);
	sscanf(&buff[i],"%s",rbuff);
	if(nn != ft[0].start){
		printf("Error 5\n");
		return ;
	}
	i+=(strlen(rbuff) +1);
	sscanf(&buff[i],"%u",&ft[0].succ);
	sscanf(&buff[i],"%s",rbuff);

	i+=(strlen(rbuff) +1);
	sscanf(&buff[i],"%s",ft[0].ips);

	i+=(strlen(ft[0].ips) +1);
	sscanf(&buff[i],"%d",&ft[0].ports);
	
	char rbuff1[length];
	
	ui n = ft[0].succ; strcpy(rbuff1,ft[0].ips); 
	int ii = ft[0].ports;
	for(int i=1;i<m;i++){
		if(belongsto(id , n+1 , ft[i].start)){
			ft[i].succ = n;
			strcpy(ft[i].ips,rbuff1);
			ft[i].ports = ii;
		}
		else{

			/*buff : find_successor given_id */
			inet_pton(AF_INET,ip_existing,&(addr.sin_addr)); addr.sin_port = port_existing;
			sprintf(buff,"find_successor %u",ft[i].start);
			sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
			// printf("sent msg : %s\n",buff);

			/* buff : find_successor_return given_id ans_id ans_ip ans_port */
			recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
			// printf("rcvd msg: %s\n",buff);
			sscanf(buff,"%s",rbuff);
			if(strcmp(rbuff,"find_successor_return")){
				printf("Error 6\n");
				return ;
			}
			
			kk=strlen(rbuff) +1;
			sscanf(&buff[kk],"%u",&nn);
			sscanf(&buff[kk],"%s",rbuff);
			if(nn != ft[i].start){
				printf("Error 7 nn : %u\n",nn);
				return ;
			}
			kk+=(strlen(rbuff) +1);
			sscanf(&buff[kk],"%u",&ft[i].succ);
			n = ft[i].succ;
			sscanf(&buff[kk],"%s",rbuff);

			kk+=(strlen(rbuff) +1);
			sscanf(&buff[kk],"%s",ft[i].ips);
			strcpy(rbuff1,ft[i].ips);

			kk+=(strlen(ft[i].ips) +1);
			sscanf(&buff[kk],"%d",&ft[i].ports);
			ii = ft[i].ports;
		}
	}

	for(int i=1;i<m;i++){

		if(dist(ft[i].start,id) < dist(ft[i].start,ft[i].succ)){
			ft[i].succ = id; ft[i].ports = port; strcpy(ft[i].ips,ip);
		}

	}
	for(int i =0;i<m;i++){
		printf("ft[%d] : start: %u , succ : %u , ports : %d\n",i,ft[i].start,ft[i].succ,ft[i].ports);
	}
}

/* update ny finger_table using given_id ,ip,port complete */
void Node::update_fingertable(ui given_id,char ip1[],int port1){

	struct sockaddr_in addr;
	char buff[length],rbuff[length];
	addr.sin_family = AF_INET;
	for(int i=0;i<m;i++){
		if(given_id - id + ((given_id>=id)?0:(ui)pow(2,m)) >= (ui)pow(2,i) && belongsto(id,ft[i].succ,given_id)){
			// if( i == 7 && given_id == 32 && id == 184) printf("Hurray\n");
			ft[i].succ = given_id;	strcpy(ft[i].ips,ip1); ft[i].ports = port1;
		}
	}

	// printf("Update finger table :\n");
	for(int iii=0;iii<m;iii++){
		printf("ft[%d] : %u,%u,%d\n",iii,ft[iii].start,ft[iii].succ,ft[iii].ports);
	}
}

/* join the network complete */
void Node::join(){

	struct sockaddr_in addr;
	char buff[length],rbuff[length];char wd[length];
	socklen_t srn = sizeof(addr);
	addr.sin_family = AF_INET; 

	// it is the only node in the network
	if(port == port_existing){
		for(int i=0;i<m;i++){
			ft[i].start = ( id + (ui)pow(2,i) )% (ui)pow(2,m);
			ft[i].succ = id; ft[i].ports = port_existing;
			strcpy(ft[i].ips,ip); 
		}



	    getcwd(wd,length);
		strcat(wd,"/client_data");   sprintf(buff,"%u",port-10000);   strcat(wd,buff);
	    DIR* dirp = opendir(wd);
	    if(!dirp){
	    	printf("No directory to index from\n");
	    	return ;
	    }
	    struct dirent* sd;
	    ui nn;
	    int kk;
	    while((sd=readdir(dirp))!=NULL){
	        sprintf(rbuff,"%s",sd->d_name);
	        if(rbuff[0] == '.')
	            continue;
	        printf("hash(%s) : %u\n",rbuff,hash(rbuff));
	        hashedfile[hashcount] = new char[length];
			/* hashedfile[i] : filename ip port */
			sprintf(hashedfile[hashcount],"%s %s %d",rbuff,ip,port);
			hashcount++;
	    }
	    closedir(dirp);
		return ;
	}

	// if it is not the first node in the network
	init_fingertable(id);
	// printf("Jai radhe\n");
	char ip1[length]; int port1;

	// transfer few files from my successor to me 

	/*buff : update_indexed_file id */
	inet_pton(AF_INET,ft[0].ips,&(addr.sin_addr)); addr.sin_port = ft[0].ports;
	sprintf(buff,"update_indexed_file %u",id);
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));

	recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
	// printf("rcv msg 525 : %s\n",buff);
	sscanf(buff,"%s",rbuff);
	int i;
	/* buff : hash filename ip port filename1 ip1 port1 */
	if(!strcmp(rbuff,"hash")){
		i = 5;
		while(i < strlen(buff)){
			hashedfile[hashcount] = new char[length];
			sscanf(&buff[i],"%s",hashedfile[hashcount]); // get filename
			
			i+=(strlen(hashedfile[hashcount])+1);

			sscanf(&buff[i],"%s",rbuff);
			strcat(hashedfile[hashcount]," ");
			strcat(hashedfile[hashcount],rbuff);	// get ip also
			i+=(strlen(rbuff)+1);

			sscanf(&buff[i],"%s",rbuff);
			strcat(hashedfile[hashcount]," ");	
			strcat(hashedfile[hashcount],rbuff);	// get port also
			i+=(strlen(rbuff)+1);
			hashcount++;
		}
		// printf("In fine hashcount: %d\n",hashcount);
	}

	i=0;
	ui n = ft[0].succ; 
	strcpy(ip1,ft[0].ips); port1 = ft[0].ports;
	// then linearly update the finger table of all other nodes
	while(1){

		/*buff : update_fingertable given_id ip port */
		inet_pton(AF_INET,ip1,&(addr.sin_addr)); addr.sin_port = port1;
		sprintf(buff,"update_fingertable %u %s %d",id,ip,port);
		sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
		// printf("sent msg 507: %s",buff);

		/* buff : find_successor given_id */
		inet_pton(AF_INET,ip1,&(addr.sin_addr)); addr.sin_port = port1;
		sprintf(buff,"find_successor %u",n);
		sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
		// printf("sent msg 513: %s",buff);

		/* buff : find_successor_return given_id ans_id ans_ip ans_port */
		
		recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
		// printf("recv msg 526: %s",buff);
		sscanf(buff,"%s",rbuff);
		if(strcmp(rbuff,"find_successor_return")){
			printf("Error 8\n");
			return ;
		}
		ui nn;
		i=strlen(rbuff) +1;
		sscanf(&buff[i],"%u",&nn);
		sscanf(&buff[i],"%s",rbuff);
		if(nn != n){
			printf("Error 9\n");
			return ;
		}
		i+=(strlen(rbuff) +1);
		sscanf(&buff[i],"%u",&n);
		if(n == id) return ;
		
		sscanf(&buff[i],"%s",rbuff);

		i+=(strlen(rbuff) +1);
		sscanf(&buff[i],"%s",ip1);
		
		i+=(strlen(ip1) +1);
		sscanf(&buff[i],"%d",&port1);

		// break;
	}
}

/* search for filename , 
	return the ip in rbuff1 and port in j from whom this file is shared*/
void Node::search(char filename[],char rbuff1[],int* j){
	int i;
	struct sockaddr_in addr;
	char buff[length],rbuff[length];
	socklen_t srn = sizeof(addr);
	addr.sin_family = AF_INET; 

	/* buff : find_successor given_id */
	// printf("At 643 %s %d\n",ft[0].ips,ft[0].ports);
	inet_pton(AF_INET,ft[0].ips,&(addr.sin_addr)); addr.sin_port = ft[0].ports;
	sprintf(buff,"find_successor %u",hash(filename));
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	// printf("\nmsg send 643 : %s to %d\n",buff,addr.sin_port);

	/* buff : find_successor_return given_id ans_id ans_ip ans_port */
	recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
	// printf("recv msg 642: %s",buff);
	sscanf(buff,"%s",rbuff);

	if(!strcmp(rbuff,"find_successor")){
		sprintf(buff,"find_successor_return %u %u %s %d",hash(filename),ft[0].succ,ft[0].ips,ft[0].ports);
		sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
		recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
		// printf("recv msg 658: %s",buff);
		sscanf(buff,"%s",rbuff);
		// strcpy(rbuff1,ft[0].ips);
		// *j = ft[0].ports;
	}

 	if(strcmp(rbuff,"find_successor_return")){
		printf("Error 10\n");
		return ;
	}
	// else if(!strcmp(rbuff,"find_successor_return")){
		ui nn;
		 i=strlen(rbuff) +1;
		sscanf(&buff[i],"%u",&nn);
		sscanf(&buff[i],"%s",rbuff);

		i+=strlen(rbuff) +1;
		sscanf(&buff[i],"%s",rbuff1);	

		i+=strlen(rbuff1) +1;
		sscanf(&buff[i],"%s",rbuff1);	// get ans_ip	

		i+=strlen(rbuff1) +1;
		sscanf(&buff[i],"%d",j);	// get ans_port
	// }
	

	// search for the ip , port which has shared this file


	if(!strcmp(rbuff1,ip) && *j==port){
		
		for(int ii=0;ii<hashcount;ii++){
			if(hashedfile[ii]){
				sscanf(hashedfile[ii],"%s",rbuff1);
				if(!strcmp(rbuff1,filename)){
					i = strlen(filename) + 1;
					sscanf(hashedfile[ii] + i ,"%s",rbuff1); 
					i += (strlen(rbuff1) + 1);
					sscanf(hashedfile[ii] + i ,"%d",j); 
					return;
				}
			}
		}		
	}

	/* buff : find_file filename */
	inet_pton(AF_INET,rbuff1,&(addr.sin_addr)); addr.sin_port = *j;
	sprintf(buff,"find_file %s",filename);
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	// printf("\nmsg send 673 : %s to %d\n",buff,addr.sin_port);

	/* buff : find_file_return filename ans_ip ans_port */
	recvfrom(sockfd,buff,length,0,(struct sockaddr*)&addr,&srn);
	// printf("recv msg 677: %s",buff);
	sscanf(buff,"%s",rbuff);

	if(!strcmp(rbuff,"find_file_return_nill") || !strcmp(rbuff,"find_file")){
		*j = 0;
		return ;
	}

	if(strcmp(rbuff,"find_file_return")){
		printf("Error 12\n");
		return ;
	}
	

	i=strlen(rbuff) +1;
	sscanf(&buff[i],"%s",rbuff1);	// get filename
	if(strcmp(rbuff1,filename)){
		printf("Error 16\n");
		return ;
	}

	i+=strlen(rbuff1) +1;
	sscanf(&buff[i],"%s",rbuff1);	// get ans_ip

	i+=(strlen(rbuff1) +1);
	sscanf(&buff[i],"%d",j);	// get ans_port
}

/*	set up a TCP*/
void Node::download(char filename[]){

	printf("Download request sent\n");
    write(listenfd,filename,strlen(filename));
    // printf("msg snd 704 : %s\n",filename);
    int rval;
    char buff[length];
    char wd[length];

    getcwd(wd,length);
    strcat(wd,"/client_download");  sprintf(buff,"%d",port-10000); strcat(wd,buff);  strcat(wd,"/"); strcat(wd,filename);
    sprintf(filename,"%s",wd);
    printf("filename : %s\n",filename); 
    FILE* wfp = fopen(filename,"wb");

    if(!wfp) perror("wfp is NULL:");
     
   time_t t;
   time(&t);
    int n,k;
    int ii=1;
   	socklen_t srn=0;
   	printf("Downloading ... \n");
   	while(1)
   	{
   		ii++;
   		n = read(listenfd,buff,sizeof(buff)-1);
        // n = read(listenfd,buff,sizeof(buff)-1);
        // n = recvfrom(listenfd,buff,length-1,0,NULL,&srn);
        
        
        if(n <= 0){
        	perror("read:");
        	break;
        }
        buff[n] = '\0';
        // printf("%d) n : %d\n",ii,n);
        k = fwrite(buff,sizeof(buff[0]),n,wfp);
        if(ii==2) printf("%d) n : %d\n",ii,k);

    }
    // t = clock()-t;
    // printf("Download finished in %lf seconds\n",((float)t)/CLOCKS_PER_SEC);
    fclose(wfp);
    // close(listenfd);
}

/* transfer my indexed files ( which are not shared by me ) to my successor */
void Node::filetransfer_tosucc(){

	if(ft[0].succ == id) return ;

	char buff[length],rbuff[length];
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	int i,n;
	sprintf(rbuff,"hash");
	for(int iii = 0;iii<hashcount;iii++){
		/* filename ip port */

		if(hashedfile[iii]){
			sscanf(hashedfile[iii],"%s",buff);
			i = strlen(buff);
			sscanf(hashedfile[iii]+i,"%s",buff);
			i+= strlen(buff);
			sscanf(hashedfile[iii]+i,"%d",&n);
			if(!strcmp(buff,ip) && n==port){
				delete(hashedfile[iii]);
				hashedfile[iii] = NULL;
				continue;
			}
									
			/*buff : hash filename ip port */
			strcat(rbuff," ");
			strcat(rbuff,hashedfile[iii]);
			printf("Removing %s ",hashedfile[iii]);
							
			delete(hashedfile[iii]);
			hashedfile[iii] = NULL;
			
		}
	}

	inet_pton(AF_INET,ft[0].ips,&(addr.sin_addr)); addr.sin_port = ft[0].ports;
	/* buff : hash filename1 ip1 port1 filename2 ip2 port2 */
	sendto(sockfd,rbuff,length,0,(struct sockaddr*)&addr,sizeof(addr));
}

/* leave the ring topology */
void Node::leave(){

	if(ft[0].succ == id) return ;

	struct sockaddr_in addr;
	char buff[length],rbuff[length];
	socklen_t srn = sizeof(addr);
	addr.sin_family = AF_INET; 

	/* buff : remove_from_fingertable given_id succ_of_given_id ip port ip_of_given_id port_of_gievn_id */
	inet_pton(AF_INET,ft[0].ips,&(addr.sin_addr)); addr.sin_port = ft[0].ports;
	sprintf(buff,"remove_from_fingertable %u %u %s %d %s %d",id,ft[0].succ,ft[0].ips,ft[0].ports,ip,port);
	sendto(sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	// printf("\nmsg send 751 : %s to %d\n",buff,addr.sin_port);


}

/* remove the given_id from my finger_table
	ip2 , port2 are of given_id
	ip1, port1 are of succ_of_given_id
 */
void Node::remove_from_fingertable(ui given_id,ui succ_of_given_id,char ip1[],int port1,char ip2[],int port2){

	for(int i=m-1;i>=0;i--){
		if(ft[i].succ != given_id)
			continue;
		ft[i].succ = succ_of_given_id;
		strcpy(ft[i].ips,ip1);
		ft[i].ports = port1;
	}

	for(int iii=0;iii<m;iii++){
		printf("ft[%d] : %u,%u,%d\n",iii,ft[iii].start,ft[iii].succ,ft[iii].ports);
	}

	char buff[length],rbuff[length],rbuff1[length];
	sprintf(rbuff,"%s %d",ip2,port2);
	int j;
	for(int i=0;i<hashcount;i++){
		if(hashedfile[i]){
			sscanf(hashedfile[i],"%s",buff);	// get filename
			j = strlen(buff) +1;
			sscanf(hashedfile[i] + j,"%s",buff);	// get ip 
			j += (strlen(buff) +1);
			sscanf(hashedfile[i] + j,"%s",rbuff1);	// get port 
			strcat(buff," ");	strcat(buff,rbuff1);
			if(!strcmp(buff,rbuff)){
				delete(hashedfile[i]);
				hashedfile[i] = NULL; 
			}
		}
	}

	
	for(int iii=0;iii<hashcount;iii++){
		printf("hashedfile[%d] : %s\n",iii,hashedfile[iii]);
	}
}


// argv[1] = single digit number for assigning port

int main(int args,char* argv[]){
	if(argv[1] == NULL){
		printf("Enter command line parameters\n");
		return 0;
	}
	Node node;
	struct sockaddr_in addr;
	// struct sockaddr_in addr1;
	addr.sin_family = AF_INET; 
	socklen_t srn = sizeof(addr);

	/* evaluate id existing */
	char buff[length];
	sprintf(buff,"%s:%d",ip_existing,port_existing);
	id_existing = hash(buff);

	/* store the ip , port and id of the node */
	getip(node.ip);
	strcpy(buff,node.ip);
	strcat(buff,":");
	node.port = 10000 + atoi(argv[1]);
	char rbuff[length];
	sprintf(rbuff,"%d",node.port);
	strcat(buff,rbuff);
	node.id = hash(buff);
	

	for(int i=0;i<length;i++)
		hashedfile[i] = NULL;

	node.sockfd = socket(AF_INET,SOCK_DGRAM, 0);
	int opt =1;
	setsockopt(node.sockfd,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));

	inet_pton(AF_INET,node.ip,&(addr.sin_addr)); 	addr.sin_port = node.port;
	if(bind(node.sockfd,(struct sockaddr*)&addr,sizeof(addr)) < 0) perror("bind");
	// printf("\nid : %u , port : %d\n",node.id,addr.sin_port);
	node.join();

	// printf("In main 737\n");
	// node.index();

	// tell others that I have joined
	
	/* buff : nodecount count */
	nodecount++;
	if(node.port != 10000){
		inet_pton(AF_INET,ip_existing,&(addr.sin_addr)); addr.sin_port = port_existing;
		sprintf(buff,"nodecount",nodecount);
		sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
	}
	
	
	


	ui n,nn;
	int i=0,j,jj,jjj;
	
	while(1){
		i = 0;
		// printf("Waiting for a message %u %s %d\n\n",node.id,node.ip,node.port);
		for(int iii=0;iii<m;iii++){
			printf("ft[%d] : %u,%u,%d\n",iii,node.ft[iii].start,node.ft[iii].succ,node.ft[iii].ports);
		}
		for(int iii=0;iii<hashcount;iii++){
			printf("hashedfile[%d] : %s\n",iii,hashedfile[iii]);
		}
		srn = sizeof(addr);
		if(recvfrom(node.sockfd,buff,length,0,(struct sockaddr*)&addr,&srn) < 0) perror("recv");
		// printf("got a message : %s from port : %d\n",buff,addr.sin_port);
		sscanf(&buff[i],"%s",rbuff);
		i+=(strlen(rbuff)+1);

		/* buff: break */
		if(!strcmp(rbuff,"BREAK")){
			if(node.port != port_existing){
				inet_pton(AF_INET,node.ft[0].ips,&(addr.sin_addr)); addr.sin_port = node.ft[0].ports;
				sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
			}
			break;
		}

		/* buff : nodecount */
		else if(!strcmp(rbuff,"nodecount")){
			nodecount++;
			if(nodecount == 5){
				sprintf(buff,"BREAK");
				inet_pton(AF_INET,node.ft[0].ips,&(addr.sin_addr)); addr.sin_port = node.ft[0].ports;
				sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
			}
		}

		/* buff : find_successor given_id */
		else if(!strcmp(rbuff,"find_successor")){
			sscanf(&buff[i],"%u",&n);
			sscanf(&buff[i],"%s",rbuff);
			i+=(strlen(rbuff)+1);
			if( n == node.id ){
				/* buff : find_successor_return given_id ans_id ans_ip ans_port */
				sprintf(buff,"find_successor_return %u %u %s %d",n,node.ft[0].succ,node.ft[0].ips,node.ft[0].ports);
				
			}
			else{
				nn = node.find_successor(n,rbuff,&j);
				/* buff : find_successor_return given_id ans_id ans_ip ans_port */
				sprintf(buff,"find_successor_return %u %u %s %d",n,nn,rbuff,j);
				
			}
			// printf("sending msg : %s to port %d\n",buff,addr.sin_port);			
			// printf("Jai gurudev 441 %u %s %d\n",nn,rbuff,j);
			
			sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));

		}
		/*buff : update_fingertable given_id ip port*/
		else if(!strcmp(rbuff,"update_fingertable")){
			sscanf(&buff[i],"%u",&n);
			sscanf(&buff[i],"%s",rbuff);
			i+=(strlen(rbuff)+1);

			sscanf(&buff[i],"%s",rbuff);
			i+=(strlen(rbuff)+1);
			sscanf(&buff[i],"%d",&j);

			node.update_fingertable(n,rbuff,j);
		}
		/*buff : hash filename ip port */
		else if(!strcmp(rbuff,"hash")){
			hashedfile[hashcount] = new char[length];
			sscanf(&buff[i],"%s",hashedfile[hashcount]);
			sscanf(&buff[i],"%s",rbuff);
			i+=(strlen(rbuff)+1);

			sscanf(&buff[i],"%s",rbuff);
			strcat(hashedfile[hashcount]," ");
			strcat(hashedfile[hashcount],rbuff);
			i+=(strlen(rbuff)+1);

			sscanf(&buff[i],"%s",rbuff);
			strcat(hashedfile[hashcount]," ");
			strcat(hashedfile[hashcount],rbuff);

			hashcount++;
		}
		/*buff : update_indexed_file id */
		else if(!strcmp(rbuff,"update_indexed_file")){
			ui nq;
			sscanf(&buff[i],"%u",&nq);
			
			sprintf(buff,"hash");
			for(int iii = 0;iii<hashcount;iii++){
				if(hashedfile[iii]){
					sscanf(hashedfile[iii],"%s",rbuff);

					if(node.id != hash(rbuff) && belongsto(hash(rbuff),node.id,nq)){
						strcat(buff," ");
											
						/*buff : hash filename ip port */
						strcat(buff,hashedfile[iii]);
						printf("Removing %s ",hashedfile[iii]);
						// printf("as %s %u %u %u\n",rbuff,hash(rbuff),node.id,nq);
						
						delete(hashedfile[iii]);
						hashedfile[iii] = NULL;
					}
				}
			}
			sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
		}
	}

	// printf("line 868\n");
	// this child is for TCP downloading 
	if(fork() == 0){
		
		node.sockfd1 = socket(AF_INET,SOCK_DGRAM,0);
		inet_pton(AF_INET,node.ip,&(addr.sin_addr));	addr.sin_port = node.port+20000;	// datagram for peer client
		if(bind(node.sockfd1, (struct sockaddr*)&addr, sizeof(addr)) < 0) perror("Ankit23\n");

		char rbuff1[length];
		

		int op=opt=1;
		
		setsockopt(node.sockfd1,SOL_SOCKET,SO_REUSEADDR,(char*)&op,sizeof(op));
		
		int i=0;
		printf("Enter the number of download attempts :");
		scanf("%d",&iiiii);


		while(1){
			
			node.listenfd = socket(AF_INET,SOCK_STREAM,0);
			inet_pton(AF_INET,node.ip,&(addr.sin_addr));	addr.sin_port = node.port+10000;	// TCP for peer client
			if(bind(node.listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) perror("Ankit\n");

			setsockopt(node.listenfd,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
			if(!iiiii){

				/* buff : find_successor given_id */
				inet_pton(AF_INET,node.ip,&(addr.sin_addr)); addr.sin_port = node.port;
				sprintf(buff,"kill_child");
				sendto(node.sockfd1,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
				// printf("\nmsg send 963 : %s to %d\n",buff,addr.sin_port);

				exit(0);
			}
			printf("Enter a file to search\n");
			
			scanf("%s",buff);
			sleep(1);
			
			/* buff : search filename */
			inet_pton(AF_INET,node.ip,&(addr.sin_addr)); addr.sin_port = node.port;
			sprintf(rbuff1,"search %s",buff);
			strcpy(buff,rbuff1);
			sendto(node.sockfd1,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
			// printf("sent msg 1067: %s to %d\n",buff,addr.sin_port);

			//node.search(buff,rbuff,&jj);
			/* buff: search_result filename ip port */
			recvfrom(node.sockfd1,buff,length,0,(struct sockaddr*)&addr,&srn);
			i = 0;
			// printf("got a message 1072: %s from port : %d\n",buff,addr.sin_port);
			sscanf(&buff[i],"%s",rbuff);
			i=(strlen(rbuff)+1);

			if(!strcmp(rbuff,"search_result_nill")){
				printf("No such file to download\n");
				close(node.listenfd);
				continue;
			}

			if(strcmp(rbuff,"search_result")){
				printf("error 1077\n");
				return 0;
			}

			sscanf(&buff[i],"%s",rbuff1);	
			i+=(strlen(rbuff1)+1);
			
			sscanf(&buff[i],"%s",rbuff);	// ip 
			i+=(strlen(rbuff)+1);
			
			sscanf(&buff[i],"%d",&jj);	// port 

			strcpy(buff,rbuff1);	// filename
			
			inet_pton(AF_INET,rbuff,&(addr.sin_addr));	addr.sin_port = jj + 30000;
			// printf("\n%s shared by %s:%d\n",buff,rbuff,jj);

			if(connect(node.listenfd,(struct sockaddr*)&addr,sizeof(addr)) < 0) perror("---------------connect:");
			node.download(buff);
			close(node.listenfd);
			iiiii--;
		}
	}
		
	// the parent process will loop in the datagram socket
	char rbuff1[length];
	int pid = fork();
	if(pid != 0){
		
		while(1){

			for(int iii=0;iii<m;iii++){
				printf("ft[%d] : %u,%u,%d\n",iii,node.ft[iii].start,node.ft[iii].succ,node.ft[iii].ports);
			}
			for(int iii=0;iii<hashcount;iii++){
				printf("hashedfile[%d] : %s\n",iii,hashedfile[iii]);
			}
			
			i = 0;
			// printf("Stuck here 890\n");
			if(recvfrom(node.sockfd,buff,length,0,(struct sockaddr*)&addr,&srn) < 0) perror("recv");
			// printf("got a message 901: %s from port : %d\n",buff,addr.sin_port);
			sscanf(&buff[i],"%s",rbuff);
			i+=(strlen(rbuff)+1);

			/* buff : find_successor given_id */
			if(!strcmp(rbuff,"find_successor")){
				sscanf(&buff[i],"%u",&n);
				sscanf(&buff[i],"%s",rbuff);
				
				if( n == node.id ){
					/* buff : find_successor_return given_id ans_id ans_ip ans_port */
					sprintf(buff,"find_successor_return %u %u %s %d",n,node.id,node.ip,node.port);
					
				}
				else{
					nn = node.find_successor(n,rbuff,&j);
					/*buff : find_successor_return given_id ans_id ans_ip ans_port */
					sprintf(buff,"find_successor_return %u %u %s %d",n,nn,rbuff,j);
					
				}
				// printf("sending msg : %s to port %d\n",buff,addr.sin_port);			
				// printf("Jai gurudev 841 %u %s %d\n",nn,rbuff,j);
				
				sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
			}
			/* buff : find_file filename */
			else if(!strcmp(rbuff,"find_file")){

				sscanf(&buff[i],"%s",rbuff);
				for(int ii=0;ii<hashcount;ii++){
					if(hashedfile[ii]){
						sscanf(hashedfile[ii],"%s",rbuff1);
						if(!strcmp(rbuff1,rbuff)){

							/* buff : find_file_return filename ip port */
							sprintf(buff,"find_file_return ");
							strcat(buff,hashedfile[ii]);
							sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
							i = -1;
							break;

						}
					}
				}
				if(i!=-1){
					/* buff : find_file_return_nill */
					sprintf(buff,"find_file_return_nill");
					sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));
					// printf("sending msg 1234: %s \n",buff);
				}
			}
			/* buff : kill_child */
			else if(!strcmp(rbuff,"kill_child")){
				kill(pid,SIGKILL);
				node.filetransfer_tosucc();
				node.leave();
				exit(0);
			}
			/* buff : remove_from_fingertable given_id succ_of_given_id ip port ip_of_given_id port_of_gievn_id */
			else if(!strcmp(rbuff,"remove_from_fingertable")){

				sscanf(&buff[i],"%u",&n);	// given_id
				sscanf(&buff[i],"%s",rbuff1);
				i+=(strlen(rbuff1)+1);

				if(n == node.id){
					continue;
				}

				inet_pton(AF_INET,node.ft[0].ips,&(addr.sin_addr));	addr.sin_port = node.ft[0].ports ;
				sendto(node.sockfd,buff,length,0,(struct sockaddr*)&addr,sizeof(addr));

				sscanf(&buff[i],"%u",&nn);		// succ_of_given_id
				sscanf(&buff[i],"%s",rbuff1);
				i+=(strlen(rbuff1)+1);

				sscanf(&buff[i],"%s",rbuff1);	// ip_of_succ
				i+=(strlen(rbuff1)+1);

				sscanf(&buff[i],"%d",&jj);		// port_of_succ
				sscanf(&buff[i],"%s",rbuff);
				i+=(strlen(rbuff)+1);

				sscanf(&buff[i],"%s",rbuff);	// ip_of_given_id
				i+=(strlen(rbuff)+1);

				sscanf(&buff[i],"%d",&jjj);	// port_of_given_id

				// printf("\n\nJAI GURUDEV %u %u %s %d %s %d\n\n",n,nn,rbuff1,jj,rbuff,jjj);

				node.remove_from_fingertable(n,nn,rbuff1,jj,rbuff,jjj);

				// printf("\n\nJAI RADHE\n\n");
			}
			/* buff : hash filename ip port filename1 ip1 port1 */
			else if(!strcmp(rbuff,"hash")){
				i = 5;
				while(i < strlen(buff)){
					hashedfile[hashcount] = new char[length];
					sscanf(&buff[i],"%s",hashedfile[hashcount]); // get filename
					
					i+=(strlen(hashedfile[hashcount])+1);

					sscanf(&buff[i],"%s",rbuff);
					strcat(hashedfile[hashcount]," ");
					strcat(hashedfile[hashcount],rbuff);	// get ip also
					i+=(strlen(rbuff)+1);

					sscanf(&buff[i],"%s",rbuff);
					strcat(hashedfile[hashcount]," ");	
					strcat(hashedfile[hashcount],rbuff);	// get port also
					i+=(strlen(rbuff)+1);
					hashcount++;
				}
				// printf("In fine hashcount 1143 : %d\n",hashcount);

				for(int iii=0;iii<m;iii++){
					printf("ft[%d] : %u,%u,%d\n",iii,node.ft[iii].start,node.ft[iii].succ,node.ft[iii].ports);
				}
				for(int iii=0;iii<hashcount;iii++){
					printf("hashedfile[%d] : %s\n",iii,hashedfile[iii]);
				}
			}

			/* buff: search filename */
			else if(!strcmp(rbuff,"search")){

				// printf("got a message 1241: %s from port : %d\n",buff,addr.sin_port);
				sscanf(&buff[i],"%s",rbuff);	// filename
				
				strcpy(buff,rbuff);

				node.search(buff,rbuff,&jj);
				// printf("JJ = %d\n",jj);
				/* if no file to download */
				if(jj == 0){
					sprintf(rbuff1,"search_result_nill");
					sendto(node.sockfd,rbuff1,length,0,(struct sockaddr*)&addr,sizeof(addr));
					// printf("snet msg 1324 : %s\n",rbuff1);
					continue;
				}

				/* buff: search_result filename ip port */

				sprintf(rbuff1,"search_result %s %s %d",buff,rbuff,jj);
				sendto(node.sockfd,rbuff1,length,0,(struct sockaddr*)&addr,sizeof(addr));
				// printf("got a message 1241: %s from port : %d\n",buff,addr.sin_port);
				
			}
		}
	}
	

	// child process with pid is used for TCP uploading

	if(pid == 0){
		node.listenfd1 = socket(AF_INET,SOCK_STREAM,0);
		int opt1=1;
		setsockopt(node.listenfd1,SOL_SOCKET,SO_REUSEADDR,(char*)&opt1,sizeof(opt1));
		inet_pton(AF_INET,node.ip,&(addr.sin_addr));	addr.sin_port = node.port + 30000;
		if(bind(node.listenfd1, (struct sockaddr*)&addr, sizeof(addr)) < 0) perror("Ankit\n");
		listen(node.listenfd1,20);
		int fd;
		char filename[length];
		char wd[length];
		char buf[length];
		int rval;
		
		
		int ii=0;
		FILE * fp;
		while(1){
			// printf("--------jai radhe ------++++++-------\n");
			if((fd = accept(node.listenfd1,(struct sockaddr*)&addr,&srn)) < 0) perror("accept");
			ii=i = 0;
			memset(buff,'\0',length);
			read(fd,buff,length-1);
			// printf("got a message : %s from port : %d\n",buff,addr.sin_port);
			// sscanf(&buff[i],"%s",filename);
			getcwd(wd,length);
			strcat(wd,"/client_data");   sprintf(buf,"%d",node.port-10000); strcat(wd,buf); strcat(wd,"/");
		    strcat(wd,buff);

		    fp = fopen(wd,"rb");
		    if(fp == NULL){
		        printf("Unable to open file\n");
		        exit(0);
		    }
		    printf("Uploading file %s %s\n",buff,wd);
		    int n;
		    while(!feof(fp)){
		        ii++;
		        n = fread(buf,sizeof(buf[0]),length,fp);
		        // if(ii == 1) printf("%d) n : %d\n",ii,n);
		        // if(fd)
		        write(fd,buf,n);
		        // sendto(fd,buff,length-1,0,NULL,0);

		        // sleep(.8);
		    }
		    printf("Upload ends\n");
		    if(close(fd) < 0) perror("close:");
		    fclose(fp);
		}
	} 
}