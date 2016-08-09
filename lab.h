#ifndef LAB4_H
#define LAB4_H
#include <sys/types.h>	/* for u_short, u_char, etc. */
#include <netinet/in.h>	/* for struct in_addr */

#define SERVERPORT	3300
#define SERVERADDR	"137.99.11.9" 

#define REQUEST		0	/* values for reqResp */
#define RESPONSE	1

#define NORMAL		0	/* values for Request Type */
#define TESTMYSERVER	1

typedef struct lab1Msg {
  u_short courseEtc;		/* Type,ReqResp,3300 */
  u_char  labNum;		/* =4 for this assignment */
  u_char  version;		/* =8 for this assignment */
  u_int   cookie;		/* client's transaction ID # */
  union {
    u_int uSSN;		/* request Social Sec Number (Type 0) */
    struct in_addr uServerIP;	/* IP addr of server to be tested (Type 1) */
  } u1;


#define	reqSSN		u1.uSSN		/* define some abbreviations */
#define	serverIP	u1.uServerIP.s_addr

  u_short checksum;		
  u_short result;

} LABMSG;

#define serverPort	result

#define TRANSOUTCOME	0x8000
#define MESSAGETYPE	0x8000
#define REQRESP		0x4000

#endif