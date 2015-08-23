
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

unsigned long long int oat_hash(const char *p)
{
	int len=strlen(p);
    
    unsigned long long int h = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

void getip_system(char ipstr[INET_ADDRSTRLEN],char *p,struct sockaddr_in* ip)
{
	struct ifaddrs * ifAddrStruct=NULL;
  	struct ifaddrs * ifa=NULL;
  	struct sockaddr_in * tmpAddrPtr=NULL;

  	getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) // check it is IP4
        { 
       
            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
            if(startswith(ifa->ifa_name,p))
               {		
                    ip->sin_addr=tmpAddrPtr->sin_addr;
       		  		memset(ipstr,'\0',INET_ADDRSTRLEN);
 		      		inet_ntop(AF_INET,&(tmpAddrPtr->sin_addr),ipstr,INET_ADDRSTRLEN);
        	 		break;	
				}
        }	
    }
    if(ifa==NULL)
    {
    	printf("error in finding the IP\n");
    }	
     if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}
