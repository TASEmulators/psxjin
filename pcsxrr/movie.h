extern int flagDontPause;
extern int flagFakePause;
extern int flagGPUchain;
extern int flagVSync;
extern int HasEmulatedFrame;
extern struct Movie_Type currentMovie;

void PCSX_MOV_WriteJoy(PadDataS pad,int port);
void PCSX_MOV_StopMovie();
void PCSX_MOV_StartMovie(int mode);
PadDataS PCSX_MOV_ReadJoy(int port);
void PCSX_MOV_CopyMovie(int mode, char *file);
void PCSX_MOV_FlushMovieFile();
int movieFreeze(gzFile f, int Mode);
int loadMovieFile(char* filename, struct Movie_Type *tempMovie);
