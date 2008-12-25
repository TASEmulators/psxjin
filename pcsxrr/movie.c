#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "PsxCommon.h"
#include "movie.h"
#include "cheat.h"

#ifdef WIN32
#include <windows.h>
#endif

struct MovieType Movie;

FILE* fpRecordingMovie = 0;
FILE* fpTempMovie = 0;
int flagDontPause = 1;
int flagFakePause = 0;
int flagGPUchain = 0;
int flagVSync = 0;
int HasEmulatedFrame = 0;

static const char szFileHeader[] = "PXM "; // File identifier

static void SetBytesPerFrame()
{
	Movie.bytesPerFrame = 0;
	switch (Movie.padType1) {
		case PSE_PAD_TYPE_STANDARD:
			Movie.bytesPerFrame += 2;
			break;
		case PSE_PAD_TYPE_MOUSE:
			Movie.bytesPerFrame += 4;
			break;
		case PSE_PAD_TYPE_ANALOGPAD:
		case PSE_PAD_TYPE_ANALOGJOY:
			Movie.bytesPerFrame += 6;
			break;
	}
	switch (Movie.padType2) {
		case PSE_PAD_TYPE_STANDARD:
			Movie.bytesPerFrame += 2;
			break;
		case PSE_PAD_TYPE_MOUSE:
			Movie.bytesPerFrame += 4;
			break;
		case PSE_PAD_TYPE_ANALOGPAD:
		case PSE_PAD_TYPE_ANALOGJOY:
			Movie.bytesPerFrame += 6;
			break;
	}
}

#define BUFFER_GROWTH_SIZE (4096)

static void ReserveInputBufferSpace(uint32 spaceNeeded)
{
	if (spaceNeeded > Movie.inputBufferSize) {
		uint32 ptrOffset = Movie.inputBufferPtr - Movie.inputBuffer;
		uint32 allocChunks = spaceNeeded / BUFFER_GROWTH_SIZE;
		Movie.inputBufferSize = BUFFER_GROWTH_SIZE * (allocChunks+1);
		Movie.inputBuffer = (uint8*)realloc(Movie.inputBuffer, Movie.inputBufferSize);
		Movie.inputBufferPtr = Movie.inputBuffer + ptrOffset;
	}
}

static void ReserveCdromIdsSpace(struct MovieType *tempMovie, uint32 spaceNeeded)
{
	if (spaceNeeded > tempMovie->CdromIdsSize) {
		tempMovie->CdromIdsSize = spaceNeeded;
		tempMovie->CdromIds = (char*)realloc(tempMovie->CdromIds, tempMovie->CdromIdsSize);
	}
}


/*-----------------------------------------------------------------------------
-                              FILE OPERATIONS                                -
-----------------------------------------------------------------------------*/

int ReadMovieFile(char* szChoice, struct MovieType *tempMovie) {
	char readHeader[4];
	int movieFlags = 0;
	const char szFileHeader[] = "PXM ";       //file identifier

	tempMovie->movieFilename = szChoice;

	FILE* fd = fopen(tempMovie->movieFilename, "r+b");
	if (!fd)
		return 0;

	fread(readHeader, 1, 4, fd);              //read identifier
	if (memcmp(readHeader,szFileHeader,4)) {  //not the right file type
		fclose(fd);
		return 0;
	}

	fread(&tempMovie->formatVersion,1,4,fd);  //file format version number
	fread(&tempMovie->emuVersion, 1, 4, fd);  //emulator version number
	fread(&movieFlags, 1, 1, fd);             //read flags
	{
		tempMovie->saveStateIncluded = movieFlags&MOVIE_FLAG_FROM_SAVESTATE;
		tempMovie->memoryCardIncluded = movieFlags&MOVIE_FLAG_MEMORY_CARDS;
		tempMovie->cheatListIncluded = movieFlags&MOVIE_FLAG_CHEAT_LIST;
		tempMovie->irqHacksIncluded = movieFlags&MOVIE_FLAG_IRQ_HACKS;
		tempMovie->palTiming = movieFlags&MOVIE_FLAG_PAL_TIMING;
	}
	fread(&movieFlags, 1, 1, fd);             //reserved for more flags

	fread(&tempMovie->padType1, 1, 1, fd);
	fread(&tempMovie->padType2, 1, 1, fd);

	fread(&tempMovie->totalFrames, 1, 4, fd);
	fread(&tempMovie->rerecordCount, 1, 4, fd);
	fread(&tempMovie->saveStateOffset, 1, 4, fd);
	fread(&tempMovie->memoryCard1Offset, 1, 4, fd);
	fread(&tempMovie->memoryCard2Offset, 1, 4, fd);
	fread(&tempMovie->cheatListOffset, 1, 4, fd);
	fread(&tempMovie->inputOffset, 1, 4, fd);

	// read metadata
	int nMetaLen;
	fread(&nMetaLen, 1, 4, fd);

	if (nMetaLen >= MOVIE_MAX_METADATA)
		nMetaLen = MOVIE_MAX_METADATA-1;

//	tempMovie->authorInfo = (char*)malloc((nMetaLen+1)*sizeof(char));
	int i;
	for(i=0; i<nMetaLen; ++i) {
		char c = 0;
		c |= fgetc(fd) & 0xff;
		tempMovie->authorInfo[i] = c;
	}
	tempMovie->authorInfo[i] = '\0';

	// read CDs IDs information
	fread(&tempMovie->CdromCount, 1, 1, fd);                 //total CDs used
	int nCdidsLen = tempMovie->CdromCount*9;                 //CDs IDs
	ReserveCdromIdsSpace(tempMovie,nCdidsLen);
	for(i=0; i<nCdidsLen; ++i) {
		char c = 0;
		c |= fgetc(fd) & 0xff;
		tempMovie->CdromIds[i] = c;
	}

	// done reading file
	fclose(fd);

	return 1;
}

void WriteMovieFile()
{
	fseek(fpRecordingMovie, 16, SEEK_SET);
	fwrite(&Movie.currentFrame, 1, 4, fpRecordingMovie);  //total frames
	fwrite(&Movie.rerecordCount, 1, 4, fpRecordingMovie); //rerecord count
	Movie.totalFrames=Movie.currentFrame; //used when toggling read-only mode
	fseek(fpRecordingMovie, Movie.inputOffset, SEEK_SET);
	fwrite(Movie.inputBuffer, 1, Movie.bytesPerFrame*(Movie.totalFrames+1), fpRecordingMovie);
}

static void WriteMovieHeader()
{
	int movieFlags=0;
	int empty=0;
	unsigned long emuVersion = EMU_VERSION;
	unsigned long movieVersion = MOVIE_VERSION;
	if (Movie.saveStateIncluded)
		movieFlags |= MOVIE_FLAG_FROM_SAVESTATE;
	if (Movie.memoryCardIncluded)
		movieFlags |= MOVIE_FLAG_MEMORY_CARDS;
	if (Movie.cheatListIncluded)
		movieFlags |= MOVIE_FLAG_CHEAT_LIST;
	if (Movie.irqHacksIncluded)
		movieFlags |= MOVIE_FLAG_IRQ_HACKS;
	if (Config.PsxType)
		movieFlags |= MOVIE_FLAG_PAL_TIMING;

	fwrite(&szFileHeader, 1, 4, fpRecordingMovie);          //header
	fwrite(&movieVersion, 1, 4, fpRecordingMovie);          //movie version
	fwrite(&emuVersion, 1, 4, fpRecordingMovie);            //emu version
	fwrite(&movieFlags, 1, 1, fpRecordingMovie);            //flags
	fwrite(&empty, 1, 1, fpRecordingMovie);                 //reserved for more flags
	fwrite(&Movie.padType1, 1, 1, fpRecordingMovie);        //padType1
	fwrite(&Movie.padType2, 1, 1, fpRecordingMovie);        //padType2
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //total frames
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //rerecord count
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //savestate offset
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //memory card 1 offset
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //memory card 2 offset
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //cheat list offset
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //input offset

	int authLen = strlen(Movie.authorInfo);
	if (authLen > 0) {
		fwrite(&authLen, 1, 4, fpRecordingMovie);             //author info size
		unsigned char* authbuf = (unsigned char*)malloc(authLen);
		int i;
		for(i=0; i<authLen; ++i) {
			authbuf[i + 0] = Movie.authorInfo[i] & 0xff;
			authbuf[i + 1] = (Movie.authorInfo[i] >> 8) & 0xff;
		}
		fwrite(authbuf, 1, authLen, fpRecordingMovie);        //author info
		free(authbuf);
	}

	fwrite(&Movie.CdromCount, 1, 1, fpRecordingMovie);      //total CDs used
	int cdidsLen = Movie.CdromCount*9;
	if (cdidsLen > 0) {
		unsigned char* cdidsbuf = (unsigned char*)malloc(cdidsLen);
		int i;
		for(i=0; i<cdidsLen; ++i) {
			cdidsbuf[i + 0] = Movie.CdromIds[i] & 0xff;
			cdidsbuf[i + 1] = (Movie.CdromIds[i] >> 8) & 0xff;
		}
		fwrite(cdidsbuf, 1, cdidsLen, fpRecordingMovie);      //CDs IDs
		free(cdidsbuf);
	}

	Movie.saveStateOffset = ftell(fpRecordingMovie);        //get savestate offset
	if (!Movie.saveStateIncluded)
		fwrite(&empty, 1, 4, fpRecordingMovie);               //empty 4-byte savestate
	else {
		fclose(fpRecordingMovie);
		SaveStateEmbed(Movie.movieFilename);
		fpRecordingMovie = fopen(Movie.movieFilename,"r+b");
		fseek(fpRecordingMovie, 0, SEEK_END);
	}
	
	Movie.memoryCard1Offset = ftell(fpRecordingMovie);      //get memory card 1 offset
	if (!Movie.memoryCardIncluded) {
		fwrite(&empty, 1, 4, fpRecordingMovie);               //empty 4-byte memory card
		SIO_ClearMemoryCardsEmbed();
	}
	else {
		fclose(fpRecordingMovie);
		SIO_SaveMemoryCardsEmbed(Movie.movieFilename,1);
		fpRecordingMovie = fopen(Movie.movieFilename,"r+b");
		fseek(fpRecordingMovie, 0, SEEK_END);
	}
	Movie.memoryCard2Offset = ftell(fpRecordingMovie);      //get memory card 2 offset
	if (!Movie.memoryCardIncluded) {
		fwrite(&empty, 1, 4, fpRecordingMovie);               //empty 4-byte memory card
		SIO_ClearMemoryCardsEmbed();
	}
	else {
		fclose(fpRecordingMovie);
		SIO_SaveMemoryCardsEmbed(Movie.movieFilename,2);
		fpRecordingMovie = fopen(Movie.movieFilename,"r+b");
		fseek(fpRecordingMovie, 0, SEEK_END);
	}
	LoadMcds(Config.Mcd1, Config.Mcd2);

	Movie.cheatListOffset = ftell(fpRecordingMovie);        //get cheat list offset
	if (!Movie.cheatListIncluded) {
		fwrite(&empty, 1, 4, fpRecordingMovie);               //empty 4-byte cheat list
		CHT_ClearCheatFileEmbed();
	}
	else {
		fclose(fpRecordingMovie);
		CHT_SaveCheatFileEmbed(Movie.movieFilename);
		fpRecordingMovie = fopen(Movie.movieFilename,"r+b");
		fseek(fpRecordingMovie, 0, SEEK_END);
	}

	Movie.inputOffset = ftell(fpRecordingMovie);            //get input offset

	fseek (fpRecordingMovie, 24, SEEK_SET);
	fwrite(&Movie.saveStateOffset, 1, 4, fpRecordingMovie); //write savestate offset
	fwrite(&Movie.memoryCard1Offset, 1,4,fpRecordingMovie); //write memory card 1 offset
	fwrite(&Movie.memoryCard2Offset, 1,4,fpRecordingMovie); //write memory card 2 offset
	fwrite(&Movie.cheatListOffset, 1, 4, fpRecordingMovie); //write cheat list offset
	fwrite(&Movie.inputOffset, 1, 4, fpRecordingMovie);     //write input offset
	fseek (fpRecordingMovie, 0, SEEK_END);
	Movie.inputBufferPtr = Movie.inputBuffer;
}

static void TruncateMovie()
{
	long truncLen = Movie.inputOffset + Movie.bytesPerFrame*(Movie.totalFrames+1);

	HANDLE fileHandle = CreateFile(Movie.movieFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
	if (fileHandle != NULL) {
		SetFilePointer(fileHandle, truncLen, 0, FILE_BEGIN);
		SetEndOfFile(fileHandle);
		CloseHandle(fileHandle);
	}
}

/*-----------------------------------------------------------------------------
-                            FILE OPERATIONS END                              -
-----------------------------------------------------------------------------*/


static int StartRecord()
{
	fpRecordingMovie = fopen(Movie.movieFilename,"w+b");
	SetBytesPerFrame();

	Movie.rerecordCount = 0;
	Movie.readOnly = 0;
	Movie.CdromCount = 1;
	ReserveCdromIdsSpace(&Movie,9);
	sprintf(Movie.CdromIds, "%9.9s", CdromId);
	Movie.CdromIdsSize = 9;

	WriteMovieHeader();

	return 1;
}

static int StartReplay()
{
	SetBytesPerFrame();

	if (Movie.saveStateIncluded)
		LoadStateEmbed(Movie.movieFilename);

	if (Movie.cheatListIncluded)
		CHT_LoadCheatFileEmbed(Movie.movieFilename);
	else
		CHT_ClearCheatFileEmbed();

	if (Movie.memoryCardIncluded)
		SIO_LoadMemoryCardsEmbed(Movie.movieFilename);
	else
		SIO_ClearMemoryCardsEmbed();

	// fill input buffer
	fpRecordingMovie = fopen(Movie.movieFilename,"r+b");
	{
		fseek(fpRecordingMovie, Movie.inputOffset, SEEK_SET);
		Movie.inputBufferPtr = Movie.inputBuffer;
		uint32 toRead = Movie.bytesPerFrame * (Movie.totalFrames+1);
		ReserveInputBufferSpace(toRead);
		fread(Movie.inputBufferPtr, 1, toRead, fpRecordingMovie);
	}
	fclose(fpRecordingMovie);

	return 1;
}

void PCSX_MOV_StartMovie(int mode)
{
	Movie.mode = mode;
	Movie.currentFrame = 0;
	Movie.lagCounter = 0;
	Movie.CdromIdsSize = 0;
	cdOpenCase = 0;
	cheatsEnabled = 0;
	Config.Sio = 0;
	Config.SpuIrq = 0;
	if (Movie.mode == 1)
		StartRecord();
	else if (Movie.mode == 2)
		StartReplay();
}

void PCSX_MOV_StopMovie()
{
	if (Movie.mode == 1) {
		WriteMovieFile();
		fclose(fpRecordingMovie);
		TruncateMovie();
	}
	Movie.mode = 0;
	fpRecordingMovie = NULL;
}


static void JoyWrite8(uint8 v)
{
	Movie.inputBufferPtr[0]=(uint8)v;
	Movie.inputBufferPtr += 1;
}
static void JoyWrite16(uint16 v)
{
	Movie.inputBufferPtr[0]=(uint8)v;
	Movie.inputBufferPtr[1]=(uint8)(v>>8);
	Movie.inputBufferPtr += 2;
}

void PCSX_MOV_WriteJoy(PadDataS *pad,unsigned char type)
{
	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			ReserveInputBufferSpace((uint32)((Movie.inputBufferPtr+4)-Movie.inputBuffer));
			JoyWrite16(pad->buttonStatus^0xFFFF);
			JoyWrite8(pad->moveX);
			JoyWrite8(pad->moveY);
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			ReserveInputBufferSpace((uint32)((Movie.inputBufferPtr+6)-Movie.inputBuffer));
			JoyWrite16(pad->buttonStatus^0xFFFF);
			JoyWrite8(pad->leftJoyX);
			JoyWrite8(pad->leftJoyY);
			JoyWrite8(pad->rightJoyX);
			JoyWrite8(pad->rightJoyY);
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			ReserveInputBufferSpace((uint32)((Movie.inputBufferPtr+6)-Movie.inputBuffer));
			JoyWrite16(pad->buttonStatus^0xFFFF);
			JoyWrite8(pad->leftJoyX);
			JoyWrite8(pad->leftJoyY);
			JoyWrite8(pad->rightJoyX);
			JoyWrite8(pad->rightJoyY);
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			ReserveInputBufferSpace((uint32)((Movie.inputBufferPtr+2)-Movie.inputBuffer));
			JoyWrite16(pad->buttonStatus^0xFFFF);
	}
}

static inline uint8 JoyRead8()
{
	uint8 v=Movie.inputBufferPtr[0];
	Movie.inputBufferPtr++;
	return v;
}
static inline uint16 JoyRead16()
{
	uint16 v=(Movie.inputBufferPtr[0] | (Movie.inputBufferPtr[1]<<8));
	Movie.inputBufferPtr += 2;
	return v;
}

void PCSX_MOV_ReadJoy(PadDataS *pad,unsigned char type)
{
	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			pad->buttonStatus = JoyRead16();
			pad->moveX = JoyRead8();
			pad->moveY = JoyRead8();
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			pad->buttonStatus = JoyRead16();
			pad->leftJoyX = JoyRead8();
			pad->leftJoyY = JoyRead8();
			pad->rightJoyX = JoyRead8();
			pad->rightJoyY = JoyRead8();
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			pad->buttonStatus = JoyRead16();
			pad->leftJoyX = JoyRead8();
			pad->leftJoyY = JoyRead8();
			pad->rightJoyX = JoyRead8();
			pad->rightJoyY = JoyRead8();
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			pad->buttonStatus = JoyRead16();
	}
	pad->buttonStatus ^= 0xffff;
	pad->controllerType = type;
}

int MovieFreeze(gzFile f, int Mode) {
	unsigned long bufSize = 0;
	unsigned long buttonToSend = 0;
//SysPrintf("Mode %d - %d/%d\n",Mode,Movie.currentFrame,((Movie.inputBufferPtr-Movie.inputBuffer)/Movie.bytesPerFrame));

	//saving state
	if (Mode == 1)
		bufSize = Movie.bytesPerFrame * (Movie.currentFrame+1);

	//saving/loading state
	gzfreezel(&Movie.currentFrame);
	gzfreezel(&Movie.lagCounter);
	gzfreezel(&cdOpenCase);
	gzfreezel(&cheatsEnabled);
	gzfreezel(&Movie.irqHacksIncluded);
	gzfreezel(&Config.Sio);
	gzfreezel(&Config.SpuIrq);
	gzfreezel(&Movie.lastPad1);
	gzfreezel(&Movie.lastPad2);
	gzfreezel(&Movie.CdromCount);
	gzfreeze(Movie.CdromIds,Movie.CdromCount*9);
	gzfreezel(&bufSize);
	if (!(Movie.mode == 2 && Mode == 0))
		gzfreeze(Movie.inputBuffer, bufSize);

	//loading state
	if (Mode == 0) {
		if (Movie.mode == 1)
			Movie.rerecordCount++;
		Movie.inputBufferPtr = Movie.inputBuffer+(Movie.bytesPerFrame * Movie.currentFrame);
		
		//update information GPU OSD after loading a savestate
		GPU_setlagcounter(Movie.lagCounter);
		GPU_setframecounter(Movie.currentFrame,Movie.totalFrames);
		buttonToSend = Movie.lastPad1.buttonStatus;
		buttonToSend = (buttonToSend ^ (Movie.lastPad2.buttonStatus << 16));
		GPU_inputdisplay(buttonToSend);
	}

//SysPrintf("Mode %d - %d/%d\n---\n",Mode,Movie.currentFrame,((Movie.inputBufferPtr-Movie.inputBuffer)/Movie.bytesPerFrame));
	return 0;
}
