/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides a view of the contents of the ReactOS clipboard.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#include "precomp.h"

static const char szClassName[] = "ClipBookClass";

CLIPBOARD_GLOBALS Globals;
SCROLLSTATE Scrollstate;

static void SaveClipboardToFile(void)
{
    OPENFILENAME sfn;
    LPSTR c;
    char szFileName[MAX_PATH];
    char szFilterMask[MAX_STRING_LEN + 10];

    memset(&szFilterMask, 0, sizeof(szFilterMask));
    c = szFilterMask + LoadString(Globals.hInstance, STRING_FORMAT_NT, szFilterMask, MAX_STRING_LEN) + 1;
    lstrcpy(c, "*.clp");

    memset(&szFileName, 0, sizeof(szFileName));
    memset(&sfn, 0, sizeof(sfn));
    sfn.lStructSize = sizeof(sfn);
    sfn.hwndOwner = Globals.hMainWnd;
    sfn.hInstance = Globals.hInstance;
    sfn.lpstrFilter = szFilterMask;
    sfn.lpstrFile = szFileName;
    sfn.nMaxFile = ARRAYSIZE(szFileName);
    sfn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    sfn.lpstrDefExt = "clp";

    if (!GetSaveFileName(&sfn))
        return;

    if (!OpenClipboard(Globals.hMainWnd))
    {
        //ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    WriteClipboardFile(szFileName, CLIP_FMT_NT /* CLIP_FMT_31 */);

    CloseClipboard();
}

static void LoadClipboardDataFromFile(LPSTR lpszFileName)
{
    if (MessageBoxRes(Globals.hMainWnd, Globals.hInstance,
                      STRING_DELETE_MSG, STRING_DELETE_TITLE,
                      MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
    {
        return;
    }

    if (!OpenClipboard(Globals.hMainWnd))
    {
        //ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    EmptyClipboard();
    ReadClipboardFile(lpszFileName);

    CloseClipboard();
}

static void LoadClipboardFromFile(void)
{
    OPENFILENAME ofn;
    LPSTR c;
    char szFileName[MAX_PATH];
    char szFilterMask[MAX_STRING_LEN + 10];

    memset(&szFilterMask, 0, sizeof(szFilterMask));
    c = szFilterMask + LoadString(Globals.hInstance, STRING_FORMAT_GEN, szFilterMask, MAX_STRING_LEN) + 1;
    lstrcpy(c, "*.clp");

    memset(&szFileName, 0, sizeof(szFileName));
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Globals.hMainWnd;
    ofn.hInstance = Globals.hInstance;
    ofn.lpstrFilter = szFilterMask;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = ARRAYSIZE(szFileName);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn))
        return;

    LoadClipboardDataFromFile(szFileName);
}

static void LoadClipboardFromDrop(HDROP hDrop)
{
    char szFileName[MAX_PATH];

    DragQueryFile(hDrop, 0, szFileName, ARRAYSIZE(szFileName));
    DragFinish(hDrop);

    LoadClipboardDataFromFile(szFileName);
}

static void SetDisplayFormat(UINT uFormat)
{
    RECT rc;

    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_UNCHECKED);
    Globals.uCheckedItem = uFormat + CMD_AUTOMATIC;
    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_CHECKED);

    if (uFormat == 0)
    {
        Globals.uDisplayFormat = GetAutomaticClipboardFormat();
    }
    else
    {
        Globals.uDisplayFormat = uFormat;
    }

    GetClipboardDataDimensions(Globals.uDisplayFormat, &rc);
    Scrollstate.CurrentX = Scrollstate.CurrentY = 0;
    Scrollstate.iWheelCarryoverX = Scrollstate.iWheelCarryoverY = 0;
    UpdateWindowScrollState(Globals.hMainWnd, rc.right, rc.bottom, &Scrollstate);

    InvalidateRect(Globals.hMainWnd, NULL, TRUE);
}

static void InitMenuPopup(HMENU hMenu, LPARAM index)
{
    if ((GetMenuItemID(hMenu, 0) == CMD_DELETE) || (GetMenuItemID(hMenu, 1) == CMD_SAVE_AS))
    {
        if (CountClipboardFormats() == 0)
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_GRAYED);
            EnableMenuItem(hMenu, CMD_SAVE_AS, MF_GRAYED);
        }
        else
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_ENABLED);
            EnableMenuItem(hMenu, CMD_SAVE_AS, MF_ENABLED);
        }
    }

    DrawMenuBar(Globals.hMainWnd);
}

static void UpdateDisplayMenu(void)
{
    UINT uFormat;
    HMENU hMenu;
    char szFormatName[MAX_FMT_NAME_LEN + 1];

    hMenu = GetSubMenu(Globals.hMenu, DISPLAY_MENU_POS);

    while (GetMenuItemCount(hMenu) > 1)
    {
        DeleteMenu(hMenu, 1, MF_BYPOSITION);
    }

    if (CountClipboardFormats() == 0)
        return;

    if (!OpenClipboard(Globals.hMainWnd))
        return;

    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

    /* Display the supported clipboard formats first */
    for (uFormat = EnumClipboardFormats(0); uFormat;
         uFormat = EnumClipboardFormats(uFormat))
    {
        if (IsClipboardFormatSupported(uFormat))
        {
            RetrieveClipboardFormatName(Globals.hInstance, uFormat, TRUE,
                                        szFormatName, ARRAYSIZE(szFormatName));
            AppendMenu(hMenu, MF_STRING, CMD_AUTOMATIC + uFormat, szFormatName);
        }
    }

    /* Now display the unsupported clipboard formats */
    for (uFormat = EnumClipboardFormats(0); uFormat;
         uFormat = EnumClipboardFormats(uFormat))
    {
        if (!IsClipboardFormatSupported(uFormat))
        {
            RetrieveClipboardFormatName(Globals.hInstance, uFormat, TRUE,
                                        szFormatName, ARRAYSIZE(szFormatName));
            AppendMenu(hMenu, MF_STRING | MF_GRAYED, 0, szFormatName);
        }
    }

    CloseClipboard();
}

static int OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case CMD_OPEN:
        {
            LoadClipboardFromFile();
            break;
        }

        case CMD_SAVE_AS:
        {
            SaveClipboardToFile();
            break;
        }

        case CMD_EXIT:
        {
            PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0);
            break;
        }

        case CMD_DELETE:
        {
            if (MessageBoxRes(Globals.hMainWnd, Globals.hInstance,
                              STRING_DELETE_MSG, STRING_DELETE_TITLE,
                              MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
            {
                break;
            }

            DeleteClipboardContent();
            break;
        }

        case CMD_AUTOMATIC:
        {
            SetDisplayFormat(0);
            break;
        }

        case CMD_HELP:
        {
			WinHelp(Globals.hMainWnd, "clipbrd.hlp", HELP_INDEX, 0);
            //HtmlHelpW(Globals.hMainWnd, L"clipbrd.chm", 0, 0);
            break;
        }

        case CMD_ABOUT:
        {
            HICON hIcon;
            char szTitle[MAX_STRING_LEN];

            hIcon = LoadIcon(Globals.hInstance, MAKEINTRESOURCE(CLIPBRD_ICON));
            LoadString(Globals.hInstance, STRING_CLIPBOARD, szTitle, ARRAYSIZE(szTitle));
            ShellAbout(Globals.hMainWnd, szTitle, NULL, hIcon);
            DeleteObject(hIcon);
            break;
        }

        default:
        {
            break;
        }
    }
    return 0;
}

static void OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    COLORREF crOldBkColor, crOldTextColor;
    RECT rc;

    if (!OpenClipboard(Globals.hMainWnd))
        return;

    hdc = BeginPaint(hWnd, &ps);

    /* Erase the background if needed */
    if (ps.fErase)
        FillRect(ps.hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    /* Set the correct background and text colors */
    crOldBkColor   = SetBkColor(ps.hdc, GetSysColor(COLOR_WINDOW));
    crOldTextColor = SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));

    /* Realize the clipboard palette if there is one */
    RealizeClipboardPalette(ps.hdc);

    switch (Globals.uDisplayFormat)
    {
        case CF_NONE:
        {
            /* The clipboard is empty */
            break;
        }

        case CF_DSPTEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
        //case CF_UNICODETEXT:
        {
            DrawTextFromClipboard(Globals.uDisplayFormat, ps, Scrollstate);
            break;
        }

        case CF_DSPBITMAP:
        case CF_BITMAP:
        {
            BitBltFromClipboard(ps, Scrollstate, SRCCOPY);
            break;
        }

        case CF_DIB:
        //case CF_DIBV5:
        {
            SetDIBitsToDeviceFromClipboard(Globals.uDisplayFormat, ps, Scrollstate, DIB_RGB_COLORS);
            break;
        }

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
        {
            GetClientRect(hWnd, &rc);
            PlayMetaFileFromClipboard(hdc, &rc);
            break;
        }

        //case CF_DSPENHMETAFILE:
        //case CF_ENHMETAFILE:
        //{
            //GetClientRect(hWnd, &rc);
            //PlayEnhMetaFileFromClipboard(hdc, &rc);
            //break;
        //}

        // case CF_PALETTE:
            // TODO: Draw a palette with squares filled with colors.
            // break;

        case CF_OWNERDISPLAY:
        {
            HGLOBAL hglb;
            LPPAINTSTRUCT pps;

            hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(ps));
            if (hglb)
            {
                pps = (LPPAINTSTRUCT)GlobalLock(hglb);
                _fmemcpy(pps, &ps, sizeof(ps));
                GlobalUnlock(hglb);

                SendClipboardOwnerMessage(TRUE, WM_PAINTCLIPBOARD,
                                          (WPARAM)hWnd, (LPARAM)hglb);

                GlobalFree(hglb);
            }
            break;
        }

        default:
        {
            GetClientRect(hWnd, &rc);
            DrawTextFromResource(Globals.hInstance, ERROR_UNSUPPORTED_FORMAT,
                                 hdc, &rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
            break;
        }
    }

    /* Restore the original colors */
    SetTextColor(ps.hdc, crOldTextColor);
    SetBkColor(ps.hdc, crOldBkColor);

    EndPaint(hWnd, &ps);

    CloseClipboard();
}

static LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
        {
            TEXTMETRIC tm;
            HDC hDC = GetDC(hWnd);

            /*
             * Note that the method with GetObjectW just returns
             * the original parameters with which the font was created.
             */
            if (GetTextMetrics(hDC, &tm))
            {
                Globals.CharWidth  = tm.tmMaxCharWidth; // tm.tmAveCharWidth;
                Globals.CharHeight = tm.tmHeight + tm.tmExternalLeading;
            }
            ReleaseDC(hWnd, hDC);


            Globals.hMenu = GetMenu(hWnd);
            Globals.hWndNext = SetClipboardViewer(hWnd);

            // For now, the Help dialog item is disabled because of lacking of HTML support
            EnableMenuItem(Globals.hMenu, CMD_HELP, MF_BYCOMMAND | MF_GRAYED);

            UpdateLinesToScroll(&Scrollstate);

            UpdateDisplayMenu();
            SetDisplayFormat(0);

            DragAcceptFiles(hWnd, TRUE);
            break;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hWnd);
            break;
        }

        case WM_DESTROY:
        {
            ChangeClipboardChain(hWnd, Globals.hWndNext);

            if (Globals.uDisplayFormat == CF_OWNERDISPLAY)
            {
                HGLOBAL hglb;
                LPRECT prc;

                hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(*prc));
                if (hglb)
                {
                    prc = (LPRECT)GlobalLock(hglb);
                    SetRectEmpty(prc);
                    GlobalUnlock(hglb);

                    SendClipboardOwnerMessage(TRUE, WM_SIZECLIPBOARD,
                                              (WPARAM)hWnd, (LPARAM)hglb);

                    GlobalFree(hglb);
                }
            }

            PostQuitMessage(0);
            break;
        }

        case WM_PAINT:
        {
            OnPaint(hWnd, wParam, lParam);
            break;
        }

        case WM_KEYDOWN:
        {
            OnKeyScroll(hWnd, wParam, lParam, &Scrollstate);
            break;
        }

        //case WM_MOUSEWHEEL:
        //case WM_MOUSEHWHEEL:
        //{
            //OnMouseScroll(hWnd, uMsg, wParam, lParam, &Scrollstate);
            //break;
        //}

        case WM_HSCROLL:
        {
            // NOTE: Windows uses an offset of 16 pixels
            OnScroll(hWnd, SB_HORZ, wParam, 5, &Scrollstate);
            break;
        }

        case WM_VSCROLL:
        {
            // NOTE: Windows uses an offset of 16 pixels
            OnScroll(hWnd, SB_VERT, wParam, 5, &Scrollstate);
            break;
        }

        case WM_SIZE:
        {
            RECT rc;

            if (Globals.uDisplayFormat == CF_OWNERDISPLAY)
            {
                HGLOBAL hglb;
                LPRECT prc;

                hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(*prc));
                if (hglb)
                {
                    prc = (LPRECT)GlobalLock(hglb);
                    if (wParam == SIZE_MINIMIZED)
                        SetRectEmpty(prc);
                    else
                        GetClientRect(hWnd, prc);
                    GlobalUnlock(hglb);

                    SendClipboardOwnerMessage(TRUE, WM_SIZECLIPBOARD,
                                              (WPARAM)hWnd, (LPARAM)hglb);

                    GlobalFree(hglb);
                }
                break;
            }

            GetClipboardDataDimensions(Globals.uDisplayFormat, &rc);
            UpdateWindowScrollState(hWnd, rc.right, rc.bottom, &Scrollstate);

            // NOTE: There still are little problems drawing
            // the background when displaying clipboard text.
            if (!IsClipboardFormatSupported(Globals.uDisplayFormat) ||
                Globals.uDisplayFormat == CF_DSPTEXT ||
                Globals.uDisplayFormat == CF_TEXT    ||
                Globals.uDisplayFormat == CF_OEMTEXT /*||
                Globals.uDisplayFormat == CF_UNICODETEXT*/)
            {
                InvalidateRect(Globals.hMainWnd, NULL, TRUE);
            }
            else
            {
                InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            }

            break;
        }

        case WM_CHANGECBCHAIN:
        {
            /* Transmit through the clipboard viewer chain */
            if ((HWND)wParam == Globals.hWndNext)
            {
                Globals.hWndNext = (HWND)lParam;
            }
            else if (Globals.hWndNext != (HANDLE)NULL)
            {
                SendMessage(Globals.hWndNext, uMsg, wParam, lParam);
            }

            break;
        }

        case WM_DESTROYCLIPBOARD:
            break;

        case WM_RENDERALLFORMATS:
        {
            /*
             * When the user has cleared the clipboard via the DELETE command,
             * we (clipboard viewer) become the clipboard owner. When we are
             * subsequently closed, this message is then sent to us so that
             * we get a chance to render everything we can. Since we don't have
             * anything to render, just empty the clipboard.
             */
            DeleteClipboardContent();
            break;
        }

        case WM_RENDERFORMAT:
            // TODO!
            break;

        case WM_DRAWCLIPBOARD:
        {
            UpdateDisplayMenu();
            SetDisplayFormat(0);

            /* Pass the message to the next window in clipboard viewer chain */
            SendMessage(Globals.hWndNext, uMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) > CMD_AUTOMATIC))
            {
                SetDisplayFormat(LOWORD(wParam) - CMD_AUTOMATIC);
            }
            else
            {
                OnCommand(hWnd, uMsg, wParam, lParam);
            }
            break;
        }

        case WM_INITMENUPOPUP:
        {
            InitMenuPopup((HMENU)wParam, lParam);
            break;
        }

        case WM_DROPFILES:
        {
            LoadClipboardFromDrop((HDROP)wParam);
            break;
        }

        case WM_PALETTECHANGED:
        {
            /* Ignore if this comes from ourselves */
            if ((HWND)wParam == hWnd)
                break;

            /* Fall back to WM_QUERYNEWPALETTE */
        }

        case WM_QUERYNEWPALETTE:
        {
            BOOL Success;
            HDC hDC;

            if (!OpenClipboard(Globals.hMainWnd))
                return FALSE;

            hDC = GetDC(hWnd);
            if (!hDC)
            {
                CloseClipboard();
                return FALSE;
            }

            Success = RealizeClipboardPalette(hDC);

            ReleaseDC(hWnd, hDC);
            CloseClipboard();

            if (Success)
            {
                InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
                return TRUE;
            }
            return FALSE;
        }

        case WM_SYSCOLORCHANGE:
        {
            SetDisplayFormat(Globals.uDisplayFormat);
            break;
        }

        //case WM_SETTINGCHANGE:
        //{
            //if (wParam == SPI_SETWHEELSCROLLLINES)
            //{
                //UpdateLinesToScroll(&Scrollstate);
            //}
            //break;
        //}
        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

BOOL CLIPBRD_RegisterMainWinClass(void)
{
	WNDCLASS class;
	
	class.style         = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc   = MainWndProc;
	class.hInstance     = Globals.hInstance;
	class.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(CLIPBRD_ICON));
	class.hCursor       = LoadCursor(0, (LPCSTR)IDC_ARROW);
	class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	class.lpszMenuName  = MAKEINTRESOURCE(MAIN_MENU);
	class.lpszClassName = szClassName;

	return RegisterClass(&class);
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccel;
    HWND hPrevWindow;
    char szBuffer[MAX_STRING_LEN];

    hPrevWindow = FindWindow(szClassName, NULL);
    if (hPrevWindow)
    {
//        BringWindowToFront(hPrevWindow);
//        return 0;
    }

    memset(&Globals, 0, sizeof(Globals));
    Globals.hInstance = hInstance;

    if (!CLIPBRD_RegisterMainWinClass()) return 0;

    memset(&Scrollstate, 0, sizeof(Scrollstate));

    LoadString(Globals.hInstance, STRING_CLIPBOARD, szBuffer, sizeof(szBuffer));
    Globals.hMainWnd = CreateWindowEx(/*WS_EX_CLIENTEDGE | */ WS_EX_ACCEPTFILES ,
                                       szClassName,
                                       szBuffer,
                                       WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       0,
                                       0,
                                       Globals.hInstance,
                                       NULL);

    if (!Globals.hMainWnd)
    {
        //ShowLastWin32Error(NULL);
        return 0;
    }

    ShowWindow(Globals.hMainWnd, nCmdShow);
    UpdateWindow(Globals.hMainWnd);

    hAccel = LoadAccelerators(Globals.hInstance, MAKEINTRESOURCE(ID_ACCEL));
    if (!hAccel)
    {
        //ShowLastWin32Error(Globals.hMainWnd);
    }

    /* If the user provided a path to a clipboard data file, try to open it */
    if (__argc >= 2)
        LoadClipboardDataFromFile(__argv[1]);

    while (GetMessage(&msg, 0, 0, 0))
    {
        if (!TranslateAccelerator(Globals.hMainWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
