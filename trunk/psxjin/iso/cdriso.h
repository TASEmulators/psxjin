#ifndef _CDRISO_H_
#define _CDRISO_H_

#include <string>

extern std::string CDR_iso_fileToOpen;

#ifdef __LINUX__
typedef void *HWND;
#define CALLBACK
#endif

#define VERBOSE 1

extern char IsoFile[256];
#define DEV_DEF		""
extern char CdDev[256];
#define CDDEV_DEF	"/dev/cdrom"
extern FILE *cdHandle;

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f))

extern unsigned char cdbuffer[CD_FRAMESIZE_RAW * 10];
extern unsigned char *pbuffer;

extern int Zmode; // 1 Z - 2 bz2
extern int fmode;						// 0 - file / 1 - Zfile
extern char *Ztable;

extern char *methods[];

void UpdateZmode();
void CfgOpenFile();
void SysMessage(char *fmt, ...);

long CDRinit(void);
long CDRshutdown(void);
long CDRopen(char* filename);
long CDRclose(void);
long CDRgetTN(unsigned char *);
long CDRgetTD(unsigned char , unsigned char *);
long CDRreadTrack(unsigned char *);
unsigned char * CDRgetBuffer(void);
long CDRtest(void);
void CDRabout(void);
long CDRplay(unsigned char *);
long CDRstop(void);
struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];
};
long CDRgetStatus(struct CdrStat *);
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
unsigned char* CDRgetBufferSub(void);

#endif _CDRISO_H_
