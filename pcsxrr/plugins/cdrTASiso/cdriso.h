
#ifdef __LINUX__
typedef void *HWND;
#define CALLBACK
#endif

#define VERBOSE 1

char IsoFile[256];
#define DEV_DEF		""
char CdDev[256];
#define CDDEV_DEF	"/dev/cdrom"
extern FILE *cdHandle;

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f))

unsigned char cdbuffer[CD_FRAMESIZE_RAW * 10];
unsigned char *pbuffer;

int Zmode; // 1 Z - 2 bz2
int fmode;						// 0 - file / 1 - Zfile
char *Ztable;

extern char *methods[];

void UpdateZmode();
void CfgOpenFile();
void SysMessage(char *fmt, ...);

long CALLBACK CDRinit(void);
long CALLBACK CDRshutdown(void);
long CALLBACK CDRopen(void);
long CALLBACK CDRclose(void);
long CALLBACK CDRgetTN(unsigned char *);
long CALLBACK CDRgetTD(unsigned char , unsigned char *);
long CALLBACK CDRreadTrack(unsigned char *);
unsigned char * CALLBACK CDRgetBuffer(void);
long CALLBACK CDRconfigure(void);
long CALLBACK CDRtest(void);
void CALLBACK CDRabout(void);
long CALLBACK CDRplay(unsigned char *);
long CALLBACK CDRstop(void);
struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];
};
long CALLBACK CDRgetStatus(struct CdrStat *);
struct SubQ {
	char res0[11];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	char res1[72];
};
unsigned char* CALLBACK CDRgetBufferSub(void);
