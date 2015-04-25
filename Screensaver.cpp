#include <windows.h>
#include <scrnsave.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string.h>
#include "Screensaver.h"
#include <time.h>

#define DELAYMAX 5
#define IMAGEMAX 4
#define MSGSIZE 80

char str[MSGSIZE+1] = "XP Animated";

long image = 2;
long delay = 3;

unsigned long datatype, datasize;
//unsigned long result;
DWORD result;

HKEY hRegKey;

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hDC, hMemDC, hBitDC;
	static HINSTANCE hInst = GetModuleHandle(NULL);
	static unsigned int timer;
	static RECT scrdim;
	static HBRUSH hBlkBrush;
	static HBITMAP hBitmap, hImage1, hImage2;
	static SIZE size;
	static int X1 = 0, Y1 = 0, bitmap_width, bitmap_height;
	static PAINTSTRUCT ps;
	static bool headingright1 = 1, headingdown1 = 0, headingright2 = 0, headingdown2 = 1, collision = 0;

	switch(message) {
		case WM_CREATE:

			// Sets up or creates registry entry as required
			if (RegCreateKeyEx(HKEY_CURRENT_USER, "Control Panel\\Screen Saver.XP Animated", 0, "ScreenSaver", 0, KEY_ALL_ACCESS, NULL, &hRegKey, &result) == ERROR_SUCCESS) {
				if (result == REG_CREATED_NEW_KEY) {					// creates key if doesn't exist
					RegSetValueEx(hRegKey, "image", 0, REG_DWORD, (LPBYTE)&image, sizeof(DWORD));
					RegSetValueEx(hRegKey, "delay", 0, REG_DWORD, (LPBYTE)&delay, sizeof(DWORD));
				}
				else {
					datasize = sizeof(DWORD);						// key was already in registry
					RegQueryValueEx(hRegKey, "image", NULL, &datatype, (LPBYTE)&image, &datasize);
					RegQueryValueEx(hRegKey, "delay", NULL, &datatype, (LPBYTE)&delay, &datasize);
				}
				RegCloseKey(hRegKey);
			}

			// Gets the size of the screen
			// Sets bitmap height and width which is determined from the size of the screen
			// Sets timer
			// Creates black brush
			GetClientRect(hWnd, &scrdim);
			bitmap_width = scrdim.right/(10/image);
			bitmap_height = (int)(bitmap_width/1.58);
			timer = SetTimer(hWnd, 1, (1+(delay-1)*(delay-1)*(delay-1)), NULL);
			hBlkBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);

			// Seeds random number generator with the time
			srand( (unsigned)time( NULL ) );

			// Sets starting position of bitmap1
			do { X1 = rand(); } while ( X1 > (scrdim.right-bitmap_width) );
			do { Y1 = rand(); } while ( Y1 > (scrdim.bottom-bitmap_height) );

			// Get handle to DC for whole screen
			// Creates DC in memory compatible with hDC
			// Creates compatible bitmap in hBitmap
			// Sets hMemDC to be the same as hDC
			hDC = GetDC(hWnd);
			hBitDC = CreateCompatibleDC(hDC);
			hMemDC = CreateCompatibleDC(hDC);
			hBitmap = CreateCompatibleBitmap(hDC, scrdim.right, scrdim.bottom);
			SelectObject(hMemDC, hBitmap);

			// Load the XP bitmaps
			hImage1 = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_XP), IMAGE_BITMAP,bitmap_width,bitmap_height,LR_DEFAULTSIZE);
			
			SelectObject(hMemDC, hBlkBrush);
			PatBlt(hMemDC, 0, 0, scrdim.right, scrdim.bottom, PATCOPY);

			ReleaseDC(hWnd, hDC);
			break;

		case WM_ERASEBKGND:						
			break;

		case WM_TIMER:
			hDC = GetDC(hWnd);

			SelectObject(hMemDC, hBlkBrush);
			PatBlt(hMemDC, X1, Y1, bitmap_width, bitmap_height, PATCOPY);

			// checks image1 for collission with side of screen
			if ((headingright1) && (scrdim.right < (X1 + bitmap_width))) {				// right
				headingright1 = 0;
				collision = 1;
			}
			if ((!headingright1) && (X1 < 0)) {											// left
				headingright1 = 1;
				collision = 1;
			}
			if ((headingdown1) && (scrdim.bottom < (Y1 + bitmap_height)))	{			// bottom
				headingdown1 = 0;
				collision = 1;
			}
			if ((!headingdown1) && (Y1 < 0)) {											// top
				headingdown1 = 1;
				collision = 1;
			}

			// moves image1 in whatever direction
			if (headingdown1) Y1 += 2; else Y1 -= 2;
			if (headingright1) X1 += 2; else X1 -= 2;

			// draws image
			SelectObject(hBitDC, hImage1);
			BitBlt(hMemDC, X1, Y1, bitmap_width, bitmap_height, hBitDC, 0, 0, SRCCOPY);

			InvalidateRect(hWnd, NULL, 1);
			ReleaseDC(hWnd, hDC);
			break;

		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);

			BitBlt(hDC, 0, 0, scrdim.right, scrdim.bottom, hMemDC, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			DeleteObject(hImage1);
			DeleteDC(hMemDC);
			KillTimer(hWnd, timer);
			break;
		default:
			return DefScreenSaverProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// Settings Dialog Function
BOOL WINAPI ScreenSaverConfigureDialog(HWND hdWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hdTitleCntrl, hdSizeSlider, hdSpeedSlider;
	static HFONT hdTitleFont;
	static HBRUSH hbXPBlue = NULL;

	INITCOMMONCONTROLSEX cc;

	LOGFONT lf;

	lf.lfHeight=30;
	lf.lfWidth=15;
	lf.lfEscapement = 0; 
	lf.lfOrientation = 0; 
	lf.lfWeight=FW_BOLD;
	lf.lfItalic = TRUE; 
	lf.lfUnderline = FALSE; 
	lf.lfStrikeOut = FALSE; 
	lf.lfCharSet=OEM_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily=FF_DONTCARE;
	strcpy_s(lf.lfFaceName, sizeof(lf.lfFaceName)-1, "Arial");
	hdTitleFont = CreateFontIndirect(&lf);

	switch(message)
	{
		case WM_INITDIALOG:
			cc.dwSize = sizeof(INITCOMMONCONTROLSEX);
			cc.dwICC = ICC_UPDOWN_CLASS;

			InitCommonControls();

			hbXPBlue = CreateSolidBrush( RGB( 18, 78, 70 ));

			// Sets up or creates registry entry for current user
			if (RegCreateKeyEx(HKEY_CURRENT_USER, "Control Panel\\Screen Saver.XP Animated", 0, "ScreenSaver", 0, KEY_ALL_ACCESS, NULL, &hRegKey, &result) == ERROR_SUCCESS) {
				if (result == REG_CREATED_NEW_KEY) {					// creates key if doesn't exist
					RegSetValueEx(hRegKey, "image", 0, REG_DWORD, (LPBYTE)&image, sizeof(DWORD));
					RegSetValueEx(hRegKey, "delay", 0, REG_DWORD, (LPBYTE)&delay, sizeof(DWORD));
				}
				else {
					datasize = sizeof(DWORD);						// key was already in registry
					RegQueryValueEx(hRegKey, "image", NULL, &datatype, (LPBYTE)&image, &datasize);
					RegQueryValueEx(hRegKey, "delay", NULL, &datatype, (LPBYTE)&delay, &datasize);
				}
				RegCloseKey(hRegKey);
			}

			// Sets font type/size/weight/etc for dialog title
			hdTitleCntrl=GetDlgItem(hdWnd,IDC_DLG_TITLE);
			SendMessage(hdTitleCntrl,WM_SETFONT,(WPARAM) hdTitleFont,(LPARAM) 0);
			hdTitleCntrl=GetDlgItem(hdWnd,IDC_DLG_TITLE2);
			SendMessage(hdTitleCntrl,WM_SETFONT,(WPARAM) hdTitleFont,(LPARAM) 0);

			// Set up sliders
			hdSizeSlider=GetDlgItem(hdWnd,IDC_SIZE_SLIDER);
			SendMessage(hdSizeSlider, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(1, 4));
			SendMessage(hdSizeSlider, TBM_SETPAGESIZE, 0, (LPARAM) 1); 
			SendMessage(hdSizeSlider, TBM_SETSEL, (WPARAM) FALSE, (LPARAM) MAKELONG(1, 4)); 
			SendMessage(hdSizeSlider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) image);

			// Set up sliders
			hdSpeedSlider=GetDlgItem(hdWnd,IDC_SPEED_SLIDER);
			SendMessage(hdSpeedSlider, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(1, 5));
			SendMessage(hdSpeedSlider, TBM_SETPAGESIZE, 0, (LPARAM) 1); 
			SendMessage(hdSpeedSlider, TBM_SETSEL, (WPARAM) FALSE, (LPARAM) MAKELONG(1, 5)); 
			SendMessage(hdSpeedSlider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) delay);

			return 1;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					image = SendMessage(hdSizeSlider, TBM_GETPOS, 0, 0);
					delay = SendMessage(hdSpeedSlider, TBM_GETPOS, 0, 0);

					if (RegCreateKeyEx(HKEY_CURRENT_USER, "Control Panel\\Screen Saver.XP Animated", 0, "ScreenSaver", 0, KEY_ALL_ACCESS, NULL, &hRegKey, &result) == ERROR_SUCCESS) {
						RegSetValueEx(hRegKey, "image", 0, REG_DWORD, (LPBYTE)&image, sizeof(DWORD));
						RegSetValueEx(hRegKey, "delay", 0, REG_DWORD, (LPBYTE)&delay, sizeof(DWORD));
						RegCloseKey(hRegKey);
					}
					/* fall through to next case */

				case IDCANCEL:
					EndDialog(hdWnd, 0);
					return 1;

				case IDVISIT:
					ShellExecute(hdWnd, NULL, "http://www.mcmillan.cx", NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hdWnd, 0);
					return 0;
			}
		break;

		case WM_DESTROY:
			DeleteObject(hbXPBlue);
			return 1;
	}
	return 0;
}

// No classes to register
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return 1;
}
