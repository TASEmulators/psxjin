#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "PsxCommon.h"
#include "movie.h"

#ifdef WIN32
#include <windows.h>
#endif

struct Movie_Type currentMovie;

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
	currentMovie.bytesPerFrame = 0;
	switch (currentMovie.padType1) {
		case PSE_PAD_TYPE_STANDARD:
			currentMovie.bytesPerFrame += 2;
			break;
		case PSE_PAD_TYPE_MOUSE:
			currentMovie.bytesPerFrame += 4;
			break;
		case PSE_PAD_TYPE_ANALOGPAD:
		case PSE_PAD_TYPE_ANALOGJOY:
			currentMovie.bytesPerFrame += 6;
			break;
	}
	switch (currentMovie.padType2) {
		case PSE_PAD_TYPE_STANDARD:
			currentMovie.bytesPerFrame += 2;
			break;
		case PSE_PAD_TYPE_MOUSE:
			currentMovie.bytesPerFrame += 4;
			break;
		case PSE_PAD_TYPE_ANALOGPAD:
		case PSE_PAD_TYPE_ANALOGJOY:
			currentMovie.bytesPerFrame += 6;
			break;
	}
}

#define BUFFER_GROWTH_SIZE (4096)

static void ReserveBufferSpace(uint32 space_needed)
{
	if(space_needed > currentMovie.inputBufferSize)
	{
		uint32 ptr_offset = currentMovie.inputBufferPtr - currentMovie.inputBuffer;
		uint32 alloc_chunks = space_needed / BUFFER_GROWTH_SIZE;
		currentMovie.inputBufferSize = BUFFER_GROWTH_SIZE * (alloc_chunks+1);
		currentMovie.inputBuffer = (uint8*)realloc(currentMovie.inputBuffer, currentMovie.inputBufferSize);
		currentMovie.inputBufferPtr = currentMovie.inputBuffer + ptr_offset;
	}
}


/*-----------------------------------------------------------------------------
-                              FILE OPERATIONS                                -
-----------------------------------------------------------------------------*/

int ReadMovieFile(char* szChoice, struct Movie_Type *tempMovie) {
	char readHeader[4];
	int movieFlags = 0;
	const char szFileHeader[] = "PXM "; // file identifier

	tempMovie->movieFilename = szChoice;

	FILE* fd = fopen(tempMovie->movieFilename, "r+b");
	if (!fd)
		return 0;

	fread(readHeader, 1, 4, fd);              // read identifier
	if (memcmp(readHeader,szFileHeader,4)) {  // not the right file type
		fclose(fd);
		return 0;
	}

	fread(&tempMovie->formatVersion,1,4,fd);  // file format version number
	fread(&tempMovie->emuVersion, 1, 4, fd);  // emulator version number
	fread(&movieFlags, 1, 1, fd);             // read flags
	{
		if (movieFlags&MOVIE_FLAG_FROM_POWERON) // starts from reset
			tempMovie->saveStateIncluded = 0;
		else
			tempMovie->saveStateIncluded = 1;
	
		if (movieFlags&MOVIE_FLAG_PAL_TIMING)   // get system FPS
			tempMovie->palTiming = 1;
		else
			tempMovie->palTiming = 0;
	}

	fread(&movieFlags, 1, 1, fd);             //reserved for flags
	fread(&tempMovie->padType1, 1, 1, fd);    //padType1
	fread(&tempMovie->padType2, 1, 1, fd);    //padType2

	fread(&tempMovie->totalFrames, 1, 4, fd);
	fread(&tempMovie->rerecordCount, 1, 4, fd);
	fread(&tempMovie->savestateOffset, 1, 4, fd);
	fread(&tempMovie->inputOffset, 1, 4, fd);

	// read metadata
	int nMetaLen;
	fread(&nMetaLen, 1, 4, fd);

	if(nMetaLen >= MOVIE_MAX_METADATA)
		nMetaLen = MOVIE_MAX_METADATA-1;

//	tempMovie->authorInfo = (char*)malloc((nMetaLen+1)*sizeof(char));
	int i;
	for(i=0; i<nMetaLen; ++i) {
		char c = 0;
		c |= fgetc(fd) & 0xff;
		tempMovie->authorInfo[i] = c;
	}
	tempMovie->authorInfo[i] = '\0';

	// done reading file
	fclose(fd);

	return 1;
}

void WriteMovieFile()
{
	fseek(fpRecordingMovie, 16, SEEK_SET);
	fwrite(&currentMovie.currentFrame, 1, 4, fpRecordingMovie);  //total frames
	fwrite(&currentMovie.rerecordCount, 1, 4, fpRecordingMovie); //rerecord count
	currentMovie.totalFrames=currentMovie.currentFrame; //used when toggling read-only mode
	fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
	fwrite(currentMovie.inputBuffer, 1, currentMovie.bytesPerFrame*(currentMovie.totalFrames+1), fpRecordingMovie);
}

static void WriteMovieHeader()
{
	int movieFlags=0;
	int empty=0;
	unsigned long emuVersion = EMU_VERSION;
	unsigned long movieVersion = MOVIE_VERSION;
	if (!currentMovie.saveStateIncluded)
		movieFlags |= MOVIE_FLAG_FROM_POWERON;
	if (Config.PsxType)
		movieFlags |= MOVIE_FLAG_PAL_TIMING;

	fpRecordingMovie = fopen(currentMovie.movieFilename,"w+b");

	fwrite(&szFileHeader, 1, 4, fpRecordingMovie);          //header
	fwrite(&movieVersion, 1, 4, fpRecordingMovie);          //movie version
	fwrite(&emuVersion, 1, 4, fpRecordingMovie);            //emu version
	fwrite(&movieFlags, 1, 1, fpRecordingMovie);            //flags
	fwrite(&empty, 1, 1, fpRecordingMovie);                 //reserved for flags
	fwrite(&currentMovie.padType1, 1, 1, fpRecordingMovie); //padType1
	fwrite(&currentMovie.padType2, 1, 1, fpRecordingMovie); //padType2
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //total frames
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //rerecord count
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //savestate offset
	fwrite(&empty, 1, 4, fpRecordingMovie);                 //input offset

	int authLen = strlen(currentMovie.authorInfo);
	if(authLen > 0) {
		fwrite(&authLen, 1, 4, fpRecordingMovie);         //author info size
		unsigned char* authbuf = (unsigned char*)malloc(authLen);
		int i;
		for(i=0; i<authLen; ++i) {
			authbuf[i + 0] = currentMovie.authorInfo[i] & 0xff;
			authbuf[i + 1] = (currentMovie.authorInfo[i] >> 8) & 0xff;
		}
		fwrite(authbuf, 1, authLen, fpRecordingMovie);    //author info
		free(authbuf);
	}

	currentMovie.savestateOffset = ftell(fpRecordingMovie); //get savestate offset
	if (!currentMovie.saveStateIncluded)
		fwrite(&empty, 1, 4, fpRecordingMovie);               //empty 4-byte savestate
	else {
		fclose(fpRecordingMovie);
		SaveStateEmbed(currentMovie.movieFilename);
		fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
		fseek (fpRecordingMovie, 0, SEEK_END);
	}
	currentMovie.inputOffset = ftell(fpRecordingMovie);     //get input offset
	fseek (fpRecordingMovie, 24, SEEK_SET);
	fwrite(&currentMovie.savestateOffset, 1, 4, fpRecordingMovie); //write savestate offset
	fwrite(&currentMovie.inputOffset, 1, 4, fpRecordingMovie);     //write input offset
	fseek (fpRecordingMovie, 0, SEEK_END);
	currentMovie.inputBufferPtr = currentMovie.inputBuffer;
}

static void TruncateMovie()
{
	long truncLen = currentMovie.inputOffset + currentMovie.bytesPerFrame*(currentMovie.totalFrames+1);

	HANDLE fileHandle = CreateFile(currentMovie.movieFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
	if(fileHandle != NULL)
	{
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
	SetBytesPerFrame();

	currentMovie.rerecordCount = 0;
	currentMovie.readOnly = 0;

	WriteMovieHeader();

	return 1;
}

static int StartReplay()
{
	SetBytesPerFrame();

	if (currentMovie.saveStateIncluded)
		LoadStateEmbed(currentMovie.movieFilename);

	// fill input buffer
	fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
	fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
	{
		currentMovie.inputBufferPtr = currentMovie.inputBuffer;
		uint32 to_read = currentMovie.bytesPerFrame * (currentMovie.totalFrames+1);
		ReserveBufferSpace(to_read);
		fread(currentMovie.inputBufferPtr, 1, to_read, fpRecordingMovie);
	}
	fclose(fpRecordingMovie);

	return 1;
}

void PCSX_MOV_StartMovie(int mode)
{
//	if(currentMovie.inputBuffer)
//	{
//		free(currentMovie.inputBuffer);
//		currentMovie.inputBuffer = NULL;
//	}
	currentMovie.mode = mode;
	currentMovie.currentFrame = 0;
	currentMovie.lagCounter = 0;
	if (currentMovie.mode == 1)
		StartRecord();
	else if (currentMovie.mode == 2)
		StartReplay();
}

void PCSX_MOV_StopMovie()
{
	if (currentMovie.mode == 1)
		WriteMovieFile();
	fclose(fpRecordingMovie);
	if (currentMovie.mode == 1)
		TruncateMovie();
	currentMovie.mode = 0;
	fpRecordingMovie = NULL;
//	if(currentMovie.inputBuffer)
//	{
//		free(currentMovie.inputBuffer);
//		currentMovie.inputBuffer = NULL;
//	}
}


static void JoyWrite8(uint8 v)
{
	currentMovie.inputBufferPtr[0]=(uint8)v;
	currentMovie.inputBufferPtr += 1;
}
static void JoyWrite16(uint16 v)
{
	currentMovie.inputBufferPtr[0]=(uint8)v;
	currentMovie.inputBufferPtr[1]=(uint8)(v>>8);
	currentMovie.inputBufferPtr += 2;
}

void PCSX_MOV_WriteJoy(PadDataS pad,int port)
{
	int type;

	if (port == 1)
		type = currentMovie.padType1;
	else
		type = currentMovie.padType2;

	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+4)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus^0xFFFF);
			JoyWrite8(pad.moveX);
			JoyWrite8(pad.moveY);
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+6)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus^0xFFFF);
			JoyWrite8(pad.leftJoyX);
			JoyWrite8(pad.leftJoyY);
			JoyWrite8(pad.rightJoyX);
			JoyWrite8(pad.rightJoyY);
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+6)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus^0xFFFF);
			JoyWrite8(pad.leftJoyX);
			JoyWrite8(pad.leftJoyY);
			JoyWrite8(pad.rightJoyX);
			JoyWrite8(pad.rightJoyY);
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+2)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus^0xFFFF);
	}
}

static inline uint8 JoyRead8()
{
	uint8 v=currentMovie.inputBufferPtr[0];
	currentMovie.inputBufferPtr++;
	return v;
}
static inline uint16 JoyRead16()
{
	uint16 v=(currentMovie.inputBufferPtr[0] | (currentMovie.inputBufferPtr[1]<<8));
	currentMovie.inputBufferPtr += 2;
	return v;
}

PadDataS PCSX_MOV_ReadJoy(int port)
{
	PadDataS pad; int type;

	if (port == 1)
		type = currentMovie.padType1;
	else
		type = currentMovie.padType2;

	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			pad.buttonStatus = JoyRead16();
			pad.moveX = JoyRead8();
			pad.moveY = JoyRead8();
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			pad.buttonStatus = JoyRead16();
			pad.leftJoyX = JoyRead8();
			pad.leftJoyY = JoyRead8();
			pad.rightJoyX = JoyRead8();
			pad.rightJoyY = JoyRead8();
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			pad.buttonStatus = JoyRead16();
			pad.leftJoyX = JoyRead8();
			pad.leftJoyY = JoyRead8();
			pad.rightJoyX = JoyRead8();
			pad.rightJoyY = JoyRead8();
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			pad.buttonStatus = JoyRead16();
	}
	pad.buttonStatus ^= 0xffff;
	return(pad);
}

int MovieFreeze(gzFile f, int Mode) {
	unsigned long bufSize = 0;
//SysPrintf("Mode %d - %d/%d\n",Mode,currentMovie.currentFrame,((currentMovie.inputBufferPtr-currentMovie.inputBuffer)/currentMovie.bytesPerFrame));

	// saving state
	if (Mode == 1) {
		bufSize = currentMovie.bytesPerFrame * (currentMovie.currentFrame+1);
		gzfreezel(&currentMovie.currentFrame);
		gzfreezel(&bufSize);
		gzfreeze(currentMovie.inputBuffer, bufSize);
	}

	// loading state
	if (Mode == 0) {
		// recording
		if (currentMovie.mode == 1) {
			gzfreezel(&currentMovie.currentFrame);
			gzfreezel(&bufSize);
			gzfreeze(currentMovie.inputBuffer, bufSize);
			currentMovie.rerecordCount++;
		}
		// replaying
		else if (currentMovie.mode == 2) {
			gzfreezel(&currentMovie.currentFrame);
		}
		currentMovie.inputBufferPtr = currentMovie.inputBuffer+(currentMovie.bytesPerFrame * currentMovie.currentFrame);
		GPU_setframecounter(currentMovie.currentFrame,currentMovie.totalFrames);
	}

//SysPrintf("Mode %d - %d/%d\n---\n",Mode,currentMovie.currentFrame,((currentMovie.inputBufferPtr-currentMovie.inputBuffer)/currentMovie.bytesPerFrame));
	return 0;
}
