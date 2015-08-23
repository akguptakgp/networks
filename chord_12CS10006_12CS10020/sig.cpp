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

#define MAXFILES_PER_CLIENT 50
#define BUFFER_SIZE 1000
using namespace std;

void leavehandler(int param)
{
  // send leave message to the main node
  cout<<"message sent to successor"<<endl;
 // n->leave();
  exit(0);
}

int main()
{
  signal(SIGINT,leavehandler);
while(1);

}