#ifndef _RECORD_H_
#define _RECORD_H_

#ifdef _WINDOWS
void RecordStart();
void RecordBuffer(s16* pSound,long lBytes);
void RecordStop();
BOOL CALLBACK RecordDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern char szRecFileName[MAX_PATH];
#endif

#endif