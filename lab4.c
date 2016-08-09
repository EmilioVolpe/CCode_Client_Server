/* Lab Assignment 4 CSE330 Spring 2014 */
/* Skeleton Code for ex0 of lab4 */
/* No code will be provided for ex1 of lab4 */

#include <string.h> 
#include <stdlib.h>
#include <stdio.h>	/* for NULL */
#include <ctype.h>	/* for atoi() */
#include <errno.h>	/* for perror() */
#include <signal.h>	/* for sigvec() etc. */
#include <assert.h>	/* for assert() */
#include <netdb.h>      /* for hostnet */
#include <sys/types.h>	/* for <arpa/inet.h> */
#include <sys/socket.h>	/* for PF_INET, etc. */
#include <netinet/in.h>	/* for struct sockaddr_in */
#include <arpa/inet.h>	/* for inet_addr() */
#include <sys/time.h>
#include "lab4.h"

/* this is in netinet/in.h; included here for reference only.
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};
*/

/***************************************************************************/

#define LINESIZE	80
#define SVR_ADDR	"tao.ite.uconn.edu"	/* server name */
//#define SVR_ADDR	"127.0.0.1"	/* server name */
#define SVR_PORT	3300
#define BUFSIZE     256
#define ERROR_CHECKSUM 1
#define ERROR_SYNTAX 2
#define ERROR_UNKSSN 3
#define ERROR_SERVER 4


	struct pair
	{
		int SSN;
		int POBox;
	};
	
	
u_short msgchecksum(u_short *p){
unsigned int check = 0;
unsigned int overflow;

	for(int i = 0; i <= 7; i++){
	check+= (unsigned int) p[i];
	}
	
overflow = check >> 16;
check+= overflow;
check = ~check;	
unsigned mask;
mask = (1 << 16) - 1;
u_short checkReturn = check & mask;
return checkReturn;
//right shift 16 times
//add overflow bit
//calculate ones complement ~
//get the lower 16 bits
}

/* should change it by yourself if needed!!! */
int StringToSockaddr(char *name, struct sockaddr_in *address)
{
	int a,b,c,d,p;
	char string[BUFSIZE];
	register char *cp;

	assert(name!=NULL);
	assert(address!=NULL);
	
/* Copy the name string into a private buffer so we don't crash trying
 * to write into a constant string.
 */
	if (strlen(name) > BUFSIZE-1)
		return -1;
	else
		strcpy(string,name);

	cp = string;

	address->sin_family = AF_INET;

	/* throw away leading blanks, since they make gethostbyname() choke.  */
	while (cp[0]==' ' || cp[0]=='\t') cp++;

	/* is the first character a digit?
	 * If so, we assume "w.x.y.z-port"
	 * If not, we assume "hostname-port" */
	if (isdigit(cp[0])) {
		if (sscanf(cp,"%d.%d.%d.%d-%d",&a,&b,&c,&d,&p) != 5)
			return -2;

		address->sin_addr.s_addr = htonl(a<<24 | b<<16 | c<<8 | d);
		address->sin_port = htons(p);
	} else { 		/* we dont have a digit first */
		char *port;

		/* find the '-' in string: format must be hostname-port*/
		if ((port=strchr(cp,'-')) == NULL)
			return -3;

		/* split string in two... hostname\0port\0 and increment port past \0 */
		*port++ = '\0';

		/* look-up hostentry for the hostname */
		{
			struct hostent *destHostEntry;

			/* find the hostEntry for string */
			if ((destHostEntry=gethostbyname(cp))== NULL)
				return -4;

			/* copy the address from the hostEntry into our address */
			bcopy(destHostEntry->h_addr_list[0],
				&address->sin_addr.s_addr, destHostEntry->h_length);

		} /* look-up the hostentry for hostname */

		address->sin_port = htons(atoi(port));

	} /* else (we have hostname-port) */

	return 0;
}


/*
 * Convert a struct sockaddr_in into dotted.quad-port string notation.
 * String must point to a buffer of at least 22 characters.
 */
int SockaddrToString (char *string, struct sockaddr_in *ss)
{
	int ip = ss->sin_addr.s_addr;
	ip = ntohl(ip);
	if (string==0x0)
		return -1;
	sprintf(string ,"%d.%d.%d.%d-%d", (int)(ip>>24)&0xff,
		(int)(ip>>16)&0xff,
		(int)(ip>>8)&0xff,
		(int)ip&0xff, ntohs(ss->sin_port));
	return 1;
}

void printResponse(LABMSG *mp, int ck)
{
	int type;

	mp->courseEtc = ntohs(mp->courseEtc);
	type = ( (mp->courseEtc & MESSAGETYPE) !=0)?1:0;

	printf("course=%d, Type=%d\n", mp->courseEtc&0x3fff,type);
	if (ntohl(mp->cookie) != ck)
		printf("Cookies don't match: sent %x received %x\n",ck, mp->cookie);

	if (mp->courseEtc & REQRESP)
		printf("response\n ");
	else {
		printf("request??\n");
		return;
	}

	mp->result = ntohs(mp->result);
	/*printf(" result = %x: ",mp->result);*/
	if (mp->result&TRANSOUTCOME) {		/* Check outcome */
		printf("error: \n");
		switch (mp->result & 0x7fff) {
			case ERROR_CHECKSUM: 
				printf("checksum failure\n"); 
				break; 
			case ERROR_SYNTAX: 
				printf("syntax error\n"); 
				break; 
			case ERROR_UNKSSN:
				printf("unknown SSN %d\n", ntohl(mp->reqSSN) ); 
				break; 
			case ERROR_SERVER: 
				printf("Unspecified Server Error\n"); 
			default:
				printf("Unknown Error.\n");

		} /* case switch */
	} else {			/* successful transaction */
		if(type)printf("Test succeeded.\n");
		if (!type)		/* Type 0 -- print SSN and Response */
			printf(": %d -> %d\n", ntohl(mp->reqSSN), mp->result&0x7fff);
		else
			printf("\n");		/* XXX print number of responses */
	}
}

void printPacket(LABMSG *pp, struct sockaddr_in *fromp)
{ 
	printf("==packet");

	pp->checksum=ntohs(msgchecksum((u_short *) &pp));
	/*pp->courseEtc= ntohs(pp->courseEtc);*/

	printf("\n>>>>>>>%x %u %u %u checksum=%x; ",
		pp->courseEtc,pp->labNum,pp->version,
		ntohl(pp->cookie), pp->checksum);

	if (pp->courseEtc&MESSAGETYPE) {/* Type 1 -- print addr and port */
		u_long a;
		a = ntohl(pp->serverIP);
		printf("%d.%d.%d.%d-%d\n",(a>>24)&0xff,
		(a>>16)&0xff,(a>>8)&0xff,
		a&0xff,ntohs(pp->serverPort));
	} else				/* print requested SSN */
		printf("SSN %d\n",ntohl(pp->reqSSN));

}

int main(int argc, char **argv)
{
	
	struct pair pairs[51];
	int serverSocket;
	struct sockaddr_in myAddr, destAddr;
	int mySocket;
	int myPort, reqType;
	char req1[BUFSIZE];
	struct in_addr myServerIP;
	int sizeOfDestAddr;
	LABMSG rpkt,spkt;
	int rv, sv, number, reqtype;
	char linebuf[LINESIZE], addrbuf[LINESIZE];
	int ip1, ip2, ip3, ip4;
	int inputPort=SERVERPORT;
	int myCookie;
	/*
	 * struct sigvec myvec;
	 */

	srandom(1);			/* seed the random generator */

	//create the sock_dgram socket 
	mySocket = socket(AF_INET, SOCK_DGRAM, 0);
	
		
	//fill in the sockaddr_in destAddr for destination
	sprintf(addrbuf, "%s-%d", SVR_ADDR, SVR_PORT);
	StringToSockaddr(addrbuf, &destAddr);
	sizeOfDestAddr = sizeof(destAddr);

	//get request from user and send it
	while (1) {
		printf("Request type [0 or 1]: ");
		fgets(linebuf,LINESIZE,stdin);
		if (linebuf[0]=='\n'){	/* empty line */
			break;
			}
		reqType=atoi(linebuf);
		if (reqType==1) { //REQUEST TYPE 1
			//gets the IP address to send in type 1 message
			printf("Enter SUT IP Address: ");
			fgets(linebuf,LINESIZE,stdin);
			if (sscanf(linebuf, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4 ) != 4){
				printf("Can't parse address %s!\n");
				continue;
			} 
			spkt.serverIP = htonl((ip1<<24 | ip2<<16 | ip3<<8 | ip4));
			printf("Enter SUT port number[%d]: ", SERVERPORT);
			//gets the Port to send in type 1 message
			fgets(linebuf,LINESIZE,stdin);
			inputPort = atoi(linebuf);
			if (inputPort < 0 || inputPort > 35565)
				break;
			if (inputPort==0)
				inputPort = SERVERPORT;

			spkt.result = htons(inputPort);
			spkt.courseEtc = htons(0x8ce4);
		  } 
		  
		  else {	 
		  //REQUEST TYPE 0
			//get SSN from user
			reqType = 0;
			printf("Enter SSN [987654321]: ");
			fgets(linebuf,LINESIZE,stdin);
			number = atoi(linebuf);
			if (number < 0 || number > 999999999){
				break;
				}
			if (number==0){
				number = 987654321;
				}
				
		spkt.reqSSN = htonl(number);
		spkt.result = 0;
		spkt.courseEtc = htons(3300);
		}

		spkt.labNum = 4;
		spkt.version = 8;

		myCookie = random();
		spkt.cookie = htonl(myCookie); /* XXX */
		spkt.checksum = 0;
		spkt.checksum = msgchecksum((u_short *)&spkt);

		
		//send the request
		sv = sendto(mySocket, (void *) &spkt, sizeof(spkt), 0, (struct sockaddr *) &destAddr, sizeOfDestAddr);
		printf("sending packet\n");
			

		//wait to receive the response back
		rv = recvfrom(mySocket, (void *) &rpkt, sizeof(rpkt), 0, (struct sockaddr *) &destAddr, &sizeOfDestAddr);
		
		if (rv > 0) { /* Got a reply --- check for well-formedness */
			if (rpkt.version==8) 
				if (ntohs(msgchecksum((u_short *)&rpkt))==0)
					printf("Received message checksum correct\n");
				else
					printf("Received message failed checksum\n");
			else
				printf("Invalid version number\n");
		
		}
		//print out the response
		printf("Received response:\n");
		printResponse(&rpkt,myCookie);
	};
	
return 0;
}
	/* main */