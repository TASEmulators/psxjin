#ifndef __W32_MOVIE_H__
#define __W32_MOVIE_H__

void CDOpenClose();
int MOV_W32_StartRecordDialog();
int MOV_W32_StartReplayDialog();
int MOV_W32_CdChangeDialog();
void GetMovieFilenameMini(char* filenameMini);
LRESULT CALLBACK PromptAuthorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PromptRerecordProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* __W32_MOVIE_H__ */
