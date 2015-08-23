#ifndef NODE_H
#define NODE_H
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
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
#include <vector>

const int m=64;
using namespace std;

class Node
{
public:	
	Node(int ID,char *p);
	unsigned long long int getID();
	unsigned long long int getsucID();
	unsigned long long int getpreID();
	struct sockaddr_in getIP();
	struct sockaddr_in getsucIP();
	struct sockaddr_in getpreIP();
	char* getipstr();
	int getsocket();
	struct sockaddr_in lookup(unsigned long long int id);
	bool join(struct sockaddr_in ip);
	void leave();
	int find_succesor(unsigned long long int id,struct sockaddr_in addr);
	void upload_file();
	void  join_node(unsigned long long int id,struct sockaddr_in addr);
	void  stablize(unsigned long long int id,struct sockaddr_in addr);
	void  hash_update(unsigned long long int id,struct sockaddr_in addr);
	void  send_files(unsigned long long int id,struct sockaddr_in addr);
	struct sockaddr_in  search(unsigned long long int id,struct sockaddr_in addr);
	struct sockaddr_in localsearch(string);
	void print_hash();
	void download(struct sockaddr_in peer_server_addr,const char* file);
	void startsever();
	void init_finger();
	void update_finger();
	void print_finger();
	void leave_request(unsigned long long int id,struct sockaddr_in addr);
	struct sockaddr_in local_find_succesor(unsigned long long int id);

private:
	int sockfd;
	// int sockfd_tcp;
	int ID;
	char ipstr[INET_ADDRSTRLEN];
	unsigned long long int Hash_ID;
	unsigned long long int succesor_id;
	unsigned long long int predesessor_id;
	struct sockaddr_in my_ip;
	struct sockaddr_in succesor_ip;
	struct sockaddr_in predesessor_ip;
	vector< pair<unsigned long long int,struct sockaddr_in> > hashed_file;
	vector< pair<unsigned long long int,struct sockaddr_in> > finger_table;
	// vector< string> > hashed;

};

#include "Node.cpp"
#endif