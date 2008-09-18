#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "vfw.h"

void (*readScreen)(void **dest, long *width, long *height);

extern HWND gApp.hWnd;

static int avi_opened = 0;
extern int recording;

static int frame;
static BITMAPINFOHEADER infoHeader;
static PAVIFILE avi_file;
static AVISTREAMINFO video_stream_header;
static PAVISTREAM video_stream;
static PAVISTREAM compressed_video_stream;
static AVICOMPRESSOPTIONS video_options;
static AVICOMPRESSOPTIONS *pvideo_options[1];

static int sample;
static WAVEFORMATEX sound_format;
static AVISTREAMINFO sound_stream_header;
static PAVISTREAM sound_stream;
//static PAVISTREAM compressed_sound_stream;
//static AVICOMPRESSOPTIONS sound_options;
//static AVICOMPRESSOPTIONS *psound_options[1];

void win_readScreen(void **dest, long *width, long *height)
{
//	void ShowInfo(char *Str, ...);
//	ShowInfo("win_readScreen()");
	HDC dc = GetDC(gApp.hWnd);
	RECT rect, rectS, rectT;
	
	// retrieve the dimension of the picture
	GetClientRect(gApp.hWnd, &rect);
	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
	
//	if(hTool)
//		GetClientRect(hTool, &rectT);
//	int heightT = hTool ? (rectT.bottom - rectT.top) : 0;
	int heightT = 0;
//	if(hStatus)
//		GetClientRect(hStatus, &rectS);
//	int heightS = hStatus ? (rectS.bottom - rectS.top) : 0;
	int heightS = 0;
	*height -= (heightT + heightS);
	
	const int origWidth = *width;
	const int origHeight = *height;
	
	*width = (origWidth /*+ 3*/) & ~3;
	*height =(origHeight/*+ 3*/) & ~3;
	
	// copy to a context in memory to speed up process
	HDC copy = CreateCompatibleDC(dc);
	HBITMAP bitmap = CreateCompatibleBitmap(dc, *width, *height);
	HBITMAP oldbitmap = SelectObject(copy, bitmap);
	if(copy)
		BitBlt(copy, 0, 0, *width, *height, dc, 0, heightT + (origHeight - *height), SRCCOPY);
	
	
	if (!avi_opened || !copy || !bitmap)
	{
		*dest = NULL;
		if(bitmap)
			DeleteObject(bitmap);
		if(copy)
			DeleteDC(copy);
		if(dc)
			ReleaseDC(gApp.hWnd,dc);
		return;
	}
	
	// read the context
	static unsigned char *buffer = NULL;
	static unsigned int bufferSize = 0;
	if(!buffer || bufferSize < *width * *height * 3 +1)
	{
		if(buffer) free(buffer);
		bufferSize = *width * *height * 3 +1;
		buffer = (unsigned char*)malloc(bufferSize);
	}
	
	if(!buffer)
	{
		*dest = NULL;
		if(bitmap)
			DeleteObject(bitmap);
		if(copy)
			DeleteDC(copy);
		if(dc)
			ReleaseDC(gApp.hWnd,dc);
		return;
	}
	SelectObject(copy, oldbitmap);
	BITMAPINFO bmpinfos;
	memcpy(&bmpinfos.bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));
	GetDIBits(copy, bitmap, 0, *height, buffer, &bmpinfos, DIB_RGB_COLORS);
	
	*dest = buffer;
	if(bitmap)
		DeleteObject(bitmap);
	if(copy)
		DeleteDC(copy);
	if(dc)
		ReleaseDC(gApp.hWnd,dc);
//	ShowInfo("win_readScreen() done");
}

BOOL VCRComp_addVideoFrame( unsigned char *data )
{
	return (0 == AVIStreamWrite(compressed_video_stream, frame++, 1, data, infoHeader.biSizeImage, AVIIF_KEYFRAME, NULL, NULL));
}

BOOL VCRComp_addAudioData( unsigned char *data, int len )
{
	BOOL ok = (0 == AVIStreamWrite(sound_stream, sample, len / sound_format.nBlockAlign, data, len, 0, NULL, NULL));
	sample += len / sound_format.nBlockAlign;
	return ok;
}

void VCRComp_startFile( const char *filename, long width, long height, int fps )
{
   avi_opened = 1;
   frame = 0;
   infoHeader.biSize = sizeof( BITMAPINFOHEADER );
   infoHeader.biWidth = width;
   infoHeader.biHeight = height;
   infoHeader.biPlanes = 1;
   infoHeader.biBitCount = 24;
   infoHeader.biCompression = BI_RGB;
   infoHeader.biSizeImage = width * height * 3;
   infoHeader.biXPelsPerMeter = 0;
   infoHeader.biYPelsPerMeter = 0;
   infoHeader.biClrUsed = 0;
   infoHeader.biClrImportant = 0;

   AVIFileInit();
   AVIFileOpen(&avi_file, filename, OF_WRITE | OF_CREATE, NULL);
   
   ZeroMemory(&video_stream_header, sizeof(AVISTREAMINFO));
   video_stream_header.fccType = streamtypeVIDEO;
   video_stream_header.dwScale = 1;
   video_stream_header.dwRate = fps;
   video_stream_header.dwSuggestedBufferSize = 0;
   AVIFileCreateStream(avi_file, &video_stream, &video_stream_header);
   
   ZeroMemory(&video_options, sizeof(AVICOMPRESSOPTIONS));
   pvideo_options[0] = &video_options;
   AVISaveOptions(gApp.hWnd, 0, 1, &video_stream, pvideo_options);
   AVIMakeCompressedStream(&compressed_video_stream, video_stream, &video_options, NULL);
   AVIStreamSetFormat(compressed_video_stream, 0, &infoHeader, 
                      infoHeader.biSize + infoHeader.biClrUsed * sizeof(RGBQUAD));
                      
   // sound
   sample = 0;
   sound_format.wFormatTag = WAVE_FORMAT_PCM;
   sound_format.nChannels = 2;
   sound_format.nSamplesPerSec = 44100;
   sound_format.nAvgBytesPerSec = 44100 * (2 * 16 / 8);
   sound_format.nBlockAlign = 2 * 16 / 8;
   sound_format.wBitsPerSample = 16;
   sound_format.cbSize = 0;
   
   ZeroMemory(&sound_stream_header, sizeof(AVISTREAMINFO));
   sound_stream_header.fccType = streamtypeAUDIO;
   sound_stream_header.dwQuality = (DWORD)-1;
   sound_stream_header.dwScale = sound_format.nBlockAlign;
   sound_stream_header.dwInitialFrames = 1;
   sound_stream_header.dwRate = sound_format.nAvgBytesPerSec;
   sound_stream_header.dwSampleSize = sound_format.nBlockAlign;
   AVIFileCreateStream(avi_file, &sound_stream, &sound_stream_header);
   AVIStreamSetFormat(sound_stream, 0, &sound_format, sizeof(WAVEFORMATEX));
   
   /*ZeroMemory(&sound_options, sizeof(AVICOMPRESSOPTIONS));
   psound_options[0] = &sound_options;
   AVISaveOptions(gApp.hWnd, 0, 1, &sound_stream, psound_options);
   AVIMakeCompressedStream(&compressed_sound_stream, sound_stream, &sound_options, NULL);
   AVIStreamSetFormat(compressed_sound_stream, 0, &sound_format, sizeof(WAVEFORMATEX));*/
}

void VCRComp_finishFile()
{
   SetWindowPos(gApp.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
   AVIStreamClose(compressed_video_stream);
   AVIStreamClose(video_stream);
   //AVIStreamClose(compressed_sound_stream);
   AVIStreamClose(sound_stream);
   AVIFileClose(avi_file);
   AVIFileExit();
   HMENU hMenu;
   hMenu = GetMenu(gApp.hWnd);
//   EnableMenuItem(hMenu,ID_END_CAPTURE,MF_GRAYED);
//   EnableMenuItem(hMenu,ID_START_CAPTURE,MF_ENABLED);
//   EnableMenuItem(hMenu,FULL_SCREEN,MF_ENABLED);               //Enables fullscreen menu
//   SendMessage( hTool, TB_ENABLEBUTTON, FULL_SCREEN, TRUE );   //Enables fullscreen button
   SetWindowPos(gApp.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Remove the always on top flag
   
   recording = 0;
   avi_opened = 0;
	printf("[VCR]: Finished AVI capture.\n");
}

void init_readScreen()
{
#ifdef __WIN32__
	void ShowInfo(char *Str, ...);
	ShowInfo((readScreen != NULL) ? "ReadScreen is implemented by this graphics plugin." : "ReadScreen not implemented by this graphics plugin - substituting...");
#endif

    if(readScreen == NULL)
        readScreen = win_readScreen;
}

