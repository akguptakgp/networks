#include <iostream>
using namespace std;
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include<time.h>
#include <stdio.h>
int main() 
{
	// struct timeval tv;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	select(1, &readfds, NULL, NULL,NULL);
	if (FD_ISSET(STDIN_FILENO, &readfds)){
		char buff[100];
		int n=read(0,buff,100);
		buff[n-1]='\0';
		printf("%s\n",buff );
	}
	else
	printf("Timed out.");
	return 0;
}