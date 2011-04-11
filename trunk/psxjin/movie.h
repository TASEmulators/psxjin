#ifndef __MOVIE_H__
#define __MOVIE_H__

#define STANDARD_PAD_SIZE 15
#define ANALOG_PAD_SIZE 33
#define MOUSE_PAD_SIZE 12

extern int iPause;
extern int iDoPauseAtVSync;
extern int iGpuHasUpdated;
extern int iVSyncFlag;
extern int iFrameAdvance;
extern int iJoysToPoll;
extern struct MovieType Movie;
extern struct MovieControlType MovieControl;

void MOV_StartMovie(int mode);
void MOV_StopMovie();
void MOV_Convert();
void MOV_WriteJoy(PadDataS *pad,unsigned char type);
void MOV_ReadJoy(PadDataS *pad,unsigned char type);
void MOV_WriteControl();
void MOV_ReadControl();
void MOV_ProcessControlFlags();
void MOV_WriteMovieFile();
int MOV_ReadMovieFile(char* filename, struct MovieType *tempMovie);
bool IsMovieLoaded();
int MovieFreeze(EMUFILE *f, int Mode);
void ChangeAuthor(const char* author);
void ChangeRerecordCount(int rerecords);
#endif /* __MOVIE_H__ */
