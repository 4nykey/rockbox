/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include <process.h>
#include "uisw32.h"
#include "resource.h"
#include "button.h"

// extern functions
extern void                 app_main (void *); // mod entry point
extern void					new_key(int key);

// variables
HWND                                hGUIWnd; // the GUI window handle
unsigned int                        uThreadID; // id of mod thread
PBYTE                               lpKeys;
bool                                bActive; // window active?

// GUIWndProc
// window proc for GUI simulator
LRESULT GUIWndProc (
                    HWND hWnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam
                    )
{
    static HBITMAP hBkgnd;
    static lpBmp [UI_WIDTH * UI_HEIGHT * 3];
    static HDC hMemDc;

    switch (uMsg)
    {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
            bActive = true;
        else
            bActive = false;
        return TRUE;
    case WM_CREATE:
        // load background image
        hBkgnd = (HBITMAP)LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_UI),
            IMAGE_BITMAP, 0, 0, LR_VGACOLOR);
        hMemDc = CreateCompatibleDC (GetDC (hWnd));
        SelectObject (hMemDc, hBkgnd);
        return TRUE;
    case WM_SIZING:
        {
            LPRECT r = (LPRECT)lParam;
            RECT r2;
            char s[256];
            int v;

            switch (wParam)
            {
            case WMSZ_BOTTOM:
                v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5;
                r->right = r->left + v * UI_WIDTH / 5;
                r->right += GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->bottom += GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_RIGHT:
                v = (r->right - r->left) / (UI_WIDTH / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5;
                r->right = r->left + v * UI_WIDTH / 5;
                r->right += GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->bottom += GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_TOP:
                v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5;
                r->right = r->left + v * UI_WIDTH / 5;
                r->right += GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->top -= GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_LEFT:
                v = (r->right - r->left) / (UI_WIDTH / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5;
                r->left = r->right - v * UI_WIDTH / 5;
                r->left -= GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->bottom += GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_BOTTOMRIGHT:
                GetWindowRect (hWnd, &r2);
                if (abs(r2.right - r->right) > abs(r2.bottom - r->bottom))
                    v = (r->right - r->left) / (UI_WIDTH / 5);
                else
                    v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5;
                r->right = r->left + v * UI_WIDTH / 5;
                r->right += GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->bottom += GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_BOTTOMLEFT:
                GetWindowRect (hWnd, &r2);
                if (abs(r2.left - r->left) > abs(r2.bottom - r->bottom))
                    v = (r->right - r->left) / (UI_WIDTH / 5);
                else
                    v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->bottom = r->top + v * UI_HEIGHT / 5;
                r->left = r->right - v * UI_WIDTH / 5;
                r->left -= GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->bottom += GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_TOPRIGHT:
                GetWindowRect (hWnd, &r2);
                if (abs(r2.right - r->right) > abs(r2.top - r->top))
                    v = (r->right - r->left) / (UI_WIDTH / 5);
                else
                    v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5;
                r->right = r->left + v * UI_WIDTH / 5;
                r->right += GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->top -= GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            case WMSZ_TOPLEFT:
                GetWindowRect (hWnd, &r2);
                if (abs(r2.left - r->left) > abs(r2.top - r->top))
                    v = (r->right - r->left) / (UI_WIDTH / 5);
                else
                    v = (r->bottom - r->top) / (UI_HEIGHT / 5);
                r->top = r->bottom - v * UI_HEIGHT / 5;
                r->left = r->right - v * UI_WIDTH / 5;
                r->left -= GetSystemMetrics (SM_CXSIZEFRAME) * 2;
                r->top -= GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION);
                break;
            }

            wsprintf (s, "RockBox Simulator @%d%%", 
                (r->right - r->left - GetSystemMetrics (SM_CXSIZEFRAME) * 2)
                * 100 / UI_WIDTH);
            SetWindowText (hWnd, s);

            return TRUE;
        }
    case WM_ERASEBKGND:
        {
            PAINTSTRUCT ps;
            HDC hDc = BeginPaint (hWnd, &ps);
            RECT r;

            GetClientRect (hWnd, &r);
            // blit to screen
            StretchBlt (hDc, 0, 0, r.right, r.bottom,
                hMemDc, 0, 0, UI_WIDTH, UI_HEIGHT, SRCCOPY);
            EndPaint (hWnd, &ps);
            return TRUE;
        }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT r;
            HDC hDc = BeginPaint (hWnd, &ps);

            GetClientRect (hWnd, &r);
            // draw lcd screen
			StretchDIBits (hDc,
                UI_LCD_POSX * r.right / UI_WIDTH, UI_LCD_POSY * r.bottom / UI_HEIGHT,
                LCD_WIDTH * r.right / UI_WIDTH, LCD_HEIGHT * r.bottom / UI_HEIGHT,
				0, 0, LCD_WIDTH, LCD_HEIGHT,
				bitmap, (BITMAPINFO *) &bmi, DIB_RGB_COLORS, SRCCOPY);

            EndPaint (hWnd, &ps);
            return TRUE;
        }
    case WM_CLOSE:
        // close simulator
        hGUIWnd = NULL;
        PostQuitMessage (0);
        break;
    }

    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

// GUIStartup
// register window class, show window, init GUI
BOOL GUIStartup ()
{
    WNDCLASS wc;

    // create window class
    ZeroMemory (&wc, sizeof(wc));
    wc.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hInstance = GetModuleHandle (NULL);
    wc.lpfnWndProc = (WNDPROC)GUIWndProc;
    wc.lpszClassName = "RockBoxUISimulator";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClass (&wc) == 0)
        return FALSE;

    // create window
    hGUIWnd = CreateWindowEx (
        WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW,
        "RockBoxUISimulator", "ARCHOS JukeBox",
        WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        UI_WIDTH + GetSystemMetrics (SM_CXSIZEFRAME) * 2,
        UI_HEIGHT + GetSystemMetrics (SM_CYSIZEFRAME) * 2 + GetSystemMetrics (SM_CYSMCAPTION),
        NULL, NULL, GetModuleHandle (NULL), NULL);

    if (hGUIWnd == NULL)
        return FALSE;

    return TRUE;
}

// GUIDown
// destroy window, unregister window class
int GUIDown ()
{
    DestroyWindow (hGUIWnd);
    _endthreadex (uThreadID);
    return 0;
}

// GUIMessageLoop
// standard message loop for GUI window
void GUIMessageLoop ()
{
    MSG msg;
    while (GetMessage (&msg, hGUIWnd, 0, 0) && hGUIWnd != NULL)
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
}


// WinMain
// program entry point
int WINAPI WinMain (
                    HINSTANCE hInstance, // current instance
                    HINSTANCE hPrevInstance, // previous instance
                    LPSTR lpCmd, // command line
                    int nShowCmd // show command
                    )
{
    if (!GUIStartup ())
        return 0;

    uThreadID = _beginthread (app_main, 0, NULL);
    if (uThreadID == -0L)
        return MessageBox (NULL, "Error creating mod thread!", "Error", MB_OK);

    GUIMessageLoop ();

    return GUIDown ();
}