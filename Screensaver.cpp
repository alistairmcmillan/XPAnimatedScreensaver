#include <windows.h>
#include <scrnsave.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string.h>
#include "Screensaver.h"
#include <time.h>
#include <gdiplus.h>
#include "CGdiPlusBitmap.h"

using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

#define DELAYMAX 5
#define IMAGEMAX 4
#define MSGSIZE 80

char str[MSGSIZE+1] = "XP Animated";

long image = 2;
long delay = 3;

unsigned long datatype, datasize;
DWORD result;

HKEY hRegKey;

static RECT scrdim;
static HDC hMemDC;
static int X1 = 0, Y1 = 0, bitmap_width, bitmap_height;
static BOOL headingright = 1, headingdown = 0, collision = 0;
static HBITMAP hBitmap;

VOID ClearDC(HDC hDC) {
	Graphics graphics(hDC);
	graphics.Clear(Color::Black);
}

VOID OnPaint(HDC hDC) {
	Graphics graphics(hDC);
	BitBlt(hDC, 0, 0, scrdim.right, scrdim.bottom, hMemDC, 0, 0, SRCCOPY);
}

VOID OnTimer(HDC hDC) {
	Graphics graphics(hMemDC);

	// Paint black where the flag last was
	SolidBrush* blackBrush = new SolidBrush(Color::Black);
	graphics.FillRectangle(blackBrush, X1, Y1, bitmap_width, bitmap_height);

	// checks image for collission with side of screen
	if ((headingright) && (scrdim.right < (X1 + bitmap_width))) {				// right
		headingright = 0;
		collision = 1;
	}
	if ((!headingright) && (X1 < 0)) {											// left
		headingright = 1;
		collision = 1;
	}
	if ((headingdown) && (scrdim.bottom < (Y1 + bitmap_height)))	{			// bottom
		headingdown = 0;
		collision = 1;
	}
	if ((!headingdown) && (Y1 < 0)) {											// top
		headingdown = 1;
		collision = 1;
	}

	// moves image in whatever direction
	if (headingdown) Y1 += 2; else Y1 -= 2;
	if (headingright) X1 += 2; else X1 -= 2;

	// draws image
	CGdiPlusBitmapResource* pBitmap = new CGdiPlusBitmapResource;
	pBitmap->Load("IDB_XPPRO", "PNG");
	graphics.DrawImage(*pBitmap, X1, Y1, bitmap_width, bitmap_height);
	delete pBitmap;
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hDC;
	static HINSTANCE hInst = GetModuleHandle(NULL);
	static unsigned int timer;
	static SIZE size;
	static PAINTSTRUCT ps;

	ULONG_PTR m_gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	SolidBrush* blackBrush;

	switch (message) {
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
			GetClientRect(hWnd, &scrdim);
			bitmap_width = scrdim.right/(10/image);
			bitmap_height = (int)(bitmap_width/1.41);

			// Sets timer
			timer = SetTimer(hWnd, 1, (1 + (delay - 1)*(delay - 1)*(delay - 1)), NULL);

			// Creates black brush
			blackBrush = new SolidBrush(Color::Black);

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
			hMemDC = CreateCompatibleDC(hDC);
			hBitmap = CreateCompatibleBitmap(hDC, scrdim.right, scrdim.bottom);
			SelectObject(hMemDC, hBitmap);

			// Clear screen
			ClearDC(hMemDC);

			ReleaseDC(hWnd, hDC);
			break;

		case WM_ERASEBKGND:						
			break;

		case WM_TIMER:
			hDC = BeginPaint(hWnd, &ps);
			OnTimer(hDC);
			EndPaint(hWnd, &ps);

			InvalidateRect(hWnd, NULL, 1);
			break;

		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);
			OnPaint(hDC);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			GdiplusShutdown(m_gdiplusToken);
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
