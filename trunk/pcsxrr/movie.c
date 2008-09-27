#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "PsxCommon.h"

#ifdef WIN32
#include <windows.h>
#endif

FILE* fpRecordingMovie = 0;
FILE* fpTempMovie = 0;
int flagDontPause = 1;
int flagFakePause = 0;
int flagGPUchain = 0;
int flagVSync = 0;
int HasEmulatedFrame = 0;
extern struct Movie_Type currentMovie;

static const char szFileHeader[] = "PXM ";				// File identifier

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

static int StartRecord()
{
	int movieFlags=0; int empty=0;

	SetBytesPerFrame();

	currentMovie.rerecordCount = 0;
	if (!currentMovie.saveStateIncluded)
		movieFlags |= MOVIE_FLAG_FROM_POWERON;
	if (Config.PsxType)
		movieFlags |= MOVIE_FLAG_PAL_TIMING;

	fpRecordingMovie = fopen(currentMovie.movieFilename,"w+b");
	currentMovie.readOnly = 0;

	fwrite(&szFileHeader, 1, 4, fpRecordingMovie);          //header
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
	fseek (fpRecordingMovie, 16, SEEK_SET);
	fwrite(&currentMovie.savestateOffset, 1, 4, fpRecordingMovie); //write savestate offset
	fwrite(&currentMovie.inputOffset, 1, 4, fpRecordingMovie);     //write input offset
	fseek (fpRecordingMovie, 0, SEEK_END);
	currentMovie.currentPosition = ftell(fpRecordingMovie);
	return 1;
}

static int StartReplay()
{
	SetBytesPerFrame();

	if (currentMovie.saveStateIncluded) {
		fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
		fseek(fpRecordingMovie, 16, SEEK_SET);
		fread(&currentMovie.savestateOffset, 1, 4, fpRecordingMovie); //get savestate offset
		fread(&currentMovie.inputOffset, 1, 4, fpRecordingMovie); //get input offset
		fclose(fpRecordingMovie);
		LoadStateEmbed(currentMovie.movieFilename);
	}

	fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
	fseek(fpRecordingMovie, 20, SEEK_SET);
	fread(&currentMovie.inputOffset, 1, 4, fpRecordingMovie); //get input offset
	fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
//	currentMovie.currentPosition = ftell(fpRecordingMovie);

	// fill input buffer
	currentMovie.inputBufferPtr = currentMovie.inputBuffer;
	uint32 to_read = currentMovie.bytesPerFrame * (currentMovie.totalFrames+1);
	ReserveBufferSpace(to_read);
	fread(currentMovie.inputBufferPtr, 1, to_read, fpRecordingMovie);

	return 1;
}

void PCSX_MOV_FlushMovieFile()
{
	fflush(fpRecordingMovie); // probably not needed...
	fseek(fpRecordingMovie, 8, SEEK_SET);
	fwrite(&currentMovie.currentFrame, 1, 4, fpRecordingMovie);  //total frames
	fwrite(&currentMovie.rerecordCount, 1, 4, fpRecordingMovie); //rerecord count
	currentMovie.totalFrames=currentMovie.currentFrame; //used when toggling read-only mode
	fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
	fwrite(currentMovie.inputBuffer, 1, currentMovie.bytesPerFrame*(currentMovie.totalFrames+1), fpRecordingMovie);
	fflush(fpRecordingMovie); // probably not needed...
}

static void truncateMovie()
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

void PCSX_MOV_StopMovie()
{
	if (currentMovie.mode == 1)
		PCSX_MOV_FlushMovieFile();
	currentMovie.mode = 0;
	fclose(fpRecordingMovie);
	truncateMovie();
	fpRecordingMovie = NULL;
}

void PCSX_MOV_StartMovie(int mode)
{
	currentMovie.mode = mode;
	if (currentMovie.mode == 1)
		StartRecord();
	else if (currentMovie.mode == 2)
		StartReplay();
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
			JoyWrite16(pad.buttonStatus);
			JoyWrite8(pad.moveX);
			JoyWrite8(pad.moveY);
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+6)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus);
			JoyWrite8(pad.leftJoyX);
			JoyWrite8(pad.leftJoyY);
			JoyWrite8(pad.rightJoyX);
			JoyWrite8(pad.rightJoyY);
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+6)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus);
			JoyWrite8(pad.leftJoyX);
			JoyWrite8(pad.leftJoyY);
			JoyWrite8(pad.rightJoyX);
			JoyWrite8(pad.rightJoyY);
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			ReserveBufferSpace((uint32)((currentMovie.inputBufferPtr+2)-currentMovie.inputBuffer));
			JoyWrite16(pad.buttonStatus);
	}
}

static inline uint8 JoyRead8() /* const version */
{
	uint8 v=currentMovie.inputBufferPtr[0];
	currentMovie.inputBufferPtr++;
	return v;
}
static inline uint16 JoyRead16() /* const version */
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
	return(pad);
}

int movieFreeze(gzFile f, int Mode) {
	unsigned long bufSize = 0;

	if (Mode == 1) { //  if saving state
		bufSize = currentMovie.bytesPerFrame * (currentMovie.currentFrame+1);
		gzfreezel(&currentMovie.currentFrame);
		gzfreezel(&bufSize);
		gzfreeze(currentMovie.inputBuffer, bufSize);
	}

	if (Mode == 0) { // if loading state
		if (currentMovie.mode == 1) { // if recording movie
			gzfreezel(&currentMovie.currentFrame);
			gzfreezel(&bufSize);
			gzfreeze(currentMovie.inputBuffer, bufSize);
			currentMovie.rerecordCount++;
		}
		else if (currentMovie.mode == 2) { // if replaying movie
			gzfreezel(&currentMovie.currentFrame);
		}
		currentMovie.inputBufferPtr = currentMovie.inputBuffer+(currentMovie.bytesPerFrame * currentMovie.currentFrame);
		GPU_setframecounter(currentMovie.currentFrame,currentMovie.totalFrames);
	}

	return 0;
}
