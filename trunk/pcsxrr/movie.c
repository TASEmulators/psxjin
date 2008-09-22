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

static int StartRecord()
{
	int movieFlags=0; int empty=0;

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
	currentMovie.currentPosition = ftell(fpRecordingMovie);
	return 1;
}

void PCSX_MOV_WriteHeader()
{
	fseek(fpRecordingMovie, 8, SEEK_SET);
	fwrite(&currentMovie.frameCounter, 1, 4, fpRecordingMovie);             //total frames
	fwrite(&currentMovie.rerecordCount, 1, 4, fpRecordingMovie);            //rerecord count
	fseek(fpRecordingMovie, currentMovie.currentPosition, SEEK_SET);
	currentMovie.totalFrames=currentMovie.frameCounter;
}

void PCSX_MOV_StopMovie()
{
	if (currentMovie.mode == 1)
		PCSX_MOV_WriteHeader();
	currentMovie.mode = 0;
	fclose(fpRecordingMovie);
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

void PCSX_MOV_WriteJoy(PadDataS pad,int port)
{
	int type;

	if (port == 1)
		type = currentMovie.padType1;
	else
		type = currentMovie.padType2;

	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			fwrite(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
			fwrite(&pad.moveX,          1, sizeof(pad.moveX),          fpRecordingMovie);
			fwrite(&pad.moveY,          1, sizeof(pad.moveY),          fpRecordingMovie);
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			fwrite(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
			fwrite(&pad.leftJoyX,       1, sizeof(pad.leftJoyX),       fpRecordingMovie);
			fwrite(&pad.leftJoyY,       1, sizeof(pad.leftJoyY),       fpRecordingMovie);
			fwrite(&pad.rightJoyX,      1, sizeof(pad.rightJoyX),      fpRecordingMovie);
			fwrite(&pad.rightJoyY,      1, sizeof(pad.rightJoyY),      fpRecordingMovie);
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			fwrite(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
			fwrite(&pad.leftJoyX,       1, sizeof(pad.leftJoyX),       fpRecordingMovie);
			fwrite(&pad.leftJoyY,       1, sizeof(pad.leftJoyY),       fpRecordingMovie);
			fwrite(&pad.rightJoyX,      1, sizeof(pad.rightJoyX),      fpRecordingMovie);
			fwrite(&pad.rightJoyY,      1, sizeof(pad.rightJoyY),      fpRecordingMovie);
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			fwrite(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
	}
	currentMovie.currentPosition = ftell(fpRecordingMovie);
}

static PadDataS MakeEmptyJoypad(int port)
{
	PadDataS pad; int type;

	unsigned short emptyButtonStatus = 0xFFFF;
	unsigned char emptyRest = 0xFF;
	
	if (port == 1)
		type = currentMovie.padType1;
	else
		type = currentMovie.padType2;

	switch (type) {
		case PSE_PAD_TYPE_MOUSE:
			pad.buttonStatus = emptyButtonStatus;
			pad.moveX = emptyRest;
			pad.moveY = emptyRest;
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			pad.buttonStatus = emptyButtonStatus;
			pad.leftJoyX = emptyRest;
			pad.leftJoyY = emptyRest;
			pad.rightJoyX = emptyRest;
			pad.rightJoyY = emptyRest;
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			pad.buttonStatus = emptyButtonStatus;
			pad.leftJoyX = emptyRest;
			pad.leftJoyY = emptyRest;
			pad.rightJoyX = emptyRest;
			pad.rightJoyY = emptyRest;
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			pad.buttonStatus = emptyButtonStatus;
	}

	return(pad);
}

PadDataS PCSX_MOV_ReadJoy(int port)
{
	PadDataS pad; int type;

	if (port == 1)
		type = currentMovie.padType1;
	else
		type = currentMovie.padType2;

	if(!feof(fpRecordingMovie)) {
		switch (type) {
			case PSE_PAD_TYPE_MOUSE:
				fread(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
				fread(&pad.moveX,          1, sizeof(pad.moveX),          fpRecordingMovie);
				fread(&pad.moveY,          1, sizeof(pad.moveY),          fpRecordingMovie);
				break;
			case PSE_PAD_TYPE_ANALOGPAD: // scph1150
				fread(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
				fread(&pad.leftJoyX,       1, sizeof(pad.leftJoyX),       fpRecordingMovie);
				fread(&pad.leftJoyY,       1, sizeof(pad.leftJoyY),       fpRecordingMovie);
				fread(&pad.rightJoyX,      1, sizeof(pad.rightJoyX),      fpRecordingMovie);
				fread(&pad.rightJoyY,      1, sizeof(pad.rightJoyY),      fpRecordingMovie);
				break;
			case PSE_PAD_TYPE_ANALOGJOY: // scph1110
				fread(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
				fread(&pad.leftJoyX,       1, sizeof(pad.leftJoyX),       fpRecordingMovie);
				fread(&pad.leftJoyY,       1, sizeof(pad.leftJoyY),       fpRecordingMovie);
				fread(&pad.rightJoyX,      1, sizeof(pad.rightJoyX),      fpRecordingMovie);
				fread(&pad.rightJoyY,      1, sizeof(pad.rightJoyY),      fpRecordingMovie);
				break;
			case PSE_PAD_TYPE_STANDARD:
			default:
				fread(&pad.buttonStatus,   1, sizeof(pad.buttonStatus),   fpRecordingMovie);
		}
		currentMovie.currentPosition = ftell(fpRecordingMovie);
		if( (!feof(fpRecordingMovie)) && (currentMovie.frameCounter<currentMovie.totalFrames) )
			return(pad);
		else {
			GPU_displayText("*PCSX*: Movie End");
			currentMovie.mode = 0;
			return(MakeEmptyJoypad(port));
		}
	}
	else {
		GPU_displayText("*PCSX*: Movie End");
		currentMovie.mode = 0;
		return(MakeEmptyJoypad(port));
	}
}

void PCSX_MOV_FlushMovieFile()
{
	if (fpRecordingMovie)
		fflush(fpRecordingMovie);
}

int movieFreeze(gzFile f, int Mode) {
	unsigned long bufSize = 0;

	PCSX_MOV_FlushMovieFile();
	if ((Mode != 1) || (currentMovie.mode != 2))
		currentMovie.currentPosition = ftell(fpRecordingMovie); // <- break movies when saving state while replaying

	if (Mode == 1) { //  if saving state
		char movieBuf[currentMovie.currentPosition-currentMovie.inputOffset];
		bufSize = sizeof(movieBuf);
		fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
		fread(&movieBuf,1,bufSize,fpRecordingMovie);
		gzfreezel(&currentMovie.frameCounter);
		gzfreezel(&bufSize);
		gzfreezel(movieBuf);
		fclose(fpRecordingMovie);
		fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
		fseek(fpRecordingMovie, currentMovie.currentPosition, SEEK_SET);
	}

	if (Mode == 0) { // if loading state
		if (currentMovie.mode == 1) { // if recording movie
			gzfreezel(&currentMovie.frameCounter);
			gzfreezel(&bufSize);
			char movieLoadBuf[bufSize];
			gzfreezel(movieLoadBuf);
			fclose(fpRecordingMovie);
			fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
			fseek(fpRecordingMovie, currentMovie.inputOffset, SEEK_SET);
			fwrite(&movieLoadBuf,1,sizeof(movieLoadBuf),fpRecordingMovie);
			currentMovie.rerecordCount++;
		}
		else if (currentMovie.mode == 2) { // if replaying movie
			gzfreezel(&currentMovie.frameCounter);
			gzfreezel(&bufSize);
			fclose(fpRecordingMovie);
			fpRecordingMovie = fopen(currentMovie.movieFilename,"r+b");
			fseek(fpRecordingMovie, currentMovie.inputOffset+bufSize, SEEK_SET);
		}
		GPU_setframecounter(currentMovie.frameCounter,currentMovie.totalFrames);
	}

	PCSX_MOV_FlushMovieFile();
	currentMovie.currentPosition = ftell(fpRecordingMovie);

	return 0;
}
