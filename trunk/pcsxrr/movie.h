extern int flagDontPause;
extern int flagFakePause;
extern int flagGPUchain;
extern int flagVSync;
extern int HasEmulatedFrame;
extern struct MovieType Movie;

void PCSX_MOV_StartMovie(int mode);
void PCSX_MOV_StopMovie();
void PCSX_MOV_ReadJoy(PadDataS *pad,unsigned char type);
void PCSX_MOV_WriteJoy(PadDataS *pad,unsigned char type);
int ReadMovieFile(char* filename, struct MovieType *tempMovie);
void WriteMovieFile();
int MovieFreeze(gzFile f, int Mode);
