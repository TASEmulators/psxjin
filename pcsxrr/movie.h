#ifndef __MOVIE_H__
#define __MOVIE_H__

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
void MOV_WriteJoy(PadDataS *pad,unsigned char type);
void MOV_ReadJoy(PadDataS *pad,unsigned char type);
void MOV_WriteControl();
void MOV_ReadControl();
void MOV_ProcessControlFlags();
void MOV_WriteMovieFile();
int MOV_ReadMovieFile(char* filename, struct MovieType *tempMovie);
bool IsMovieLoaded();
int MovieFreeze(gzFile f, int Mode);

#endif /* __MOVIE_H__ */
