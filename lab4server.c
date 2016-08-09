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
	int myPort, reqType;
	char req1[BUFSIZE];
	struct in_addr myServerIP;
	int sizeOfDestAddr;
	LABMSG rpkt,spkt;
	int rv, sv, number, reqtype;
	char linebuf[LINESIZE], addrbuf[LINESIZE];
	int ip1, ip2, ip3, ip4;
	int inputPort=SERVERPORT;
	
	//hard coded data
	pairs[0].SSN = 111111111;
	pairs[0].POBox = 1234;
	pairs[1].SSN = 987654321;
	pairs[1].POBox = 4321;
	pairs[2].SSN = 325453877;
	pairs[2].POBox = 5113;
	pairs[3].SSN = 874397426;
	pairs[3].POBox = 2423;
	pairs[4].SSN = 544056088;
	pairs[4].POBox = 2343;
	pairs[5].SSN = 412324869;
	pairs[5].POBox = 4292;
	pairs[6].SSN = 402608790;
	pairs[6].POBox = 6290;
	pairs[7].SSN = 670338055;
	pairs[7].POBox = 1126;
	pairs[8].SSN = 453773539;
	pairs[8].POBox = 4213;
	pairs[9].SSN = 991926251;
	pairs[9].POBox = 8155;
	pairs[10].SSN = 976446819;
	pairs[10].POBox = 4076;
	pairs[11].SSN = 453439968;
	pairs[11].POBox = 5853;
	pairs[12].SSN = 184955947;
	pairs[12].POBox = 7620;
	pairs[13].SSN = 371749300;
	pairs[13].POBox = 3091;
	pairs[14].SSN = 372750813;
	pairs[14].POBox = 5516;
	pairs[15].SSN = 725846717;
	pairs[15].POBox = 4082;
	pairs[16].SSN = 954772946;
	pairs[16].POBox = 2594;
	pairs[17].SSN = 749682195;
	pairs[17].POBox = 1372;
	pairs[18].SSN = 293738284;
	pairs[18].POBox = 1067;
	pairs[19].SSN = 360332591;
	pairs[19].POBox = 7928;
	pairs[20].SSN = 718707857;
	pairs[20].POBox = 5881;
	pairs[21].SSN = 118181336;
	pairs[21].POBox = 8421;
	pairs[22].SSN = 233254549;
	pairs[22].POBox = 1939;
	pairs[23].SSN = 238262705;
	pairs[23].POBox = 2759;
	pairs[24].SSN = 958985134;
	pairs[24].POBox = 2672;
	pairs[25].SSN = 864657692;
	pairs[25].POBox = 4987;
	pairs[26].SSN = 110392619;
	pairs[26].POBox = 8312;
	pairs[27].SSN = 243172063;
	pairs[27].POBox = 6125;
	pairs[28].SSN = 584067155;
	pairs[28].POBox = 7780;
	pairs[29].SSN = 509850208;
	pairs[29].POBox = 2101;
	pairs[30].SSN = 872835726;
	pairs[30].POBox = 7755;
	pairs[31].SSN = 220526825;
	pairs[31].POBox = 7447;
	pairs[32].SSN = 405787039;
	pairs[32].POBox = 1893;
	pairs[33].SSN = 181176038;
	pairs[33].POBox = 3968;
	pairs[34].SSN = 107850102;
	pairs[34].POBox = 3920;
	pairs[35].SSN = 779121304;
	pairs[35].POBox = 4790;
	pairs[36].SSN = 180706750;
	pairs[36].POBox = 6841;
	pairs[37].SSN = 139691884;
	pairs[37].POBox = 3581;
	pairs[38].SSN = 697416533;
	pairs[38].POBox = 6224;
	pairs[39].SSN = 225234552;
	pairs[39].POBox = 4198;
	pairs[40].SSN = 945435863;
	pairs[40].POBox = 2615;
	pairs[41].SSN = 529503019;
	pairs[41].POBox = 7747;
	pairs[42].SSN = 405358883;
	pairs[42].POBox = 6472;
	pairs[43].SSN = 517407639;
	pairs[43].POBox = 7298;
	pairs[44].SSN = 836137273;
	pairs[44].POBox = 1030;
	pairs[45].SSN = 530528543;
	pairs[45].POBox = 6663;
	pairs[46].SSN = 459254350;
	pairs[46].POBox = 7997;
	pairs[47].SSN = 704752935;
	pairs[47].POBox = 1006;
	pairs[48].SSN = 569442788;	
	pairs[48].POBox = 8628;
	pairs[49].SSN = 103267859;
	pairs[49].POBox = 1518;
	pairs[50].SSN = 975387379;
	pairs[50].POBox = 2120;
	pairs[51].SSN = 295889711;	
	pairs[51].POBox = 5615;
	
	//set up the sever addr
	myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(inputPort);
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//create the socket
	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//bind the socket
	bind(serverSocket, (struct sockaddr *) &myAddr, sizeof(myAddr));
	sizeOfDestAddr = sizeof(destAddr);
	
	//loop getting requests from the cse 3300 server
	
	while(1){
	
		rv = recvfrom(serverSocket, (void *) &rpkt, sizeof(rpkt), 0, (struct sockaddr *) &destAddr, &sizeOfDestAddr);
		
		if (rv > 0) { /* Got a reply --- check for well-formedness */
			if (rpkt.version==8){
				if (ntohs(msgchecksum((u_short *)&rpkt))==0){
					int i;
					//check if the social security number is in data, if so send a packet back to the client
					for (i = 0; i <= 51; i++)
						if(pairs[i].SSN == ntohl(rpkt.reqSSN))
							break;
					spkt.reqSSN = htonl(pairs[i].SSN);
					spkt.result = pairs[i].POBox;
					spkt.courseEtc = htons(3300);
					spkt.labNum = 4;
					spkt.version = 8;
					spkt.cookie = htonl(random()); /* XXX */
					spkt.checksum = 0;
					spkt.checksum = msgchecksum((u_short *)&spkt);
					sv = sendto(serverSocket, (void *) &spkt, sizeof(spkt), 0, (struct sockaddr *) &destAddr, sizeOfDestAddr);
					//output a log of SSN received and POBox's sent
					printf("Received: %d\n Sent: %d\n", pairs[i].SSN, pairs[i].POBox);
					printf("Received message checksum correct\n");
					continue;

				}else
					printf("Received message failed checksum\n");
			}else
				printf("Invalid version number\n");
		}
		
	};
return 0;
}
	/* main */