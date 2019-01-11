/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/***************************************************************************\
*  XPilotNT.cpp - The Windows port of the world's greatest video game		*
*																			*
*  This file defines the class behavior for the application XPilotNT.		*
*  See http://www.buckosoft.com/xpilot/xpilotnt/ for details.				*
\***************************************************************************/

#include "stdafx.h"
#include "XPilotNT.h"

#include "MainFrm.h"
#include "xpilotDoc.h"
#include "xpilotView.h"
#include "TalkWindow.h"
#include "Splash.h"
#include "winAbout.h"

#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CXpilotApp

BEGIN_MESSAGE_MAP(CXpilotApp, CWinApp)
    //{{AFX_MSG_MAP(CXpilotApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CXpilotApp construction
    CXpilotApp::CXpilotApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CXpilotApp object

CXpilotApp theApp;

//motd* the_motd = NULL;

extern "C" HINSTANCE hInstance;

WSADATA wsadata;

/////////////////////////////////////////////////////////////////////////////
// CXpilotApp initialization

BOOL CXpilotApp::InitInstance()
{
    // CG: The following block was added by the Splash Screen component.
    {
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
    }
    if (!AfxSocketInit(&wsadata)) {
	AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
	return FALSE;
    }

    CString cs(m_pszHelpFilePath);
    int i = cs.ReverseFind('\\');
    if (i != -1) {
	cs = cs.Left(i);
	_chdir((const char *) cs);
    } else {
	CString e;
	e.Format("Can't determine startup directory from <%s>",
		 (const char *) cs);
	AfxMessageBox(e);
    }

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();		// Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
    hInstance = AfxGetInstanceHandle();

    LoadStdProfileSettings(0);	// Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate *pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CXpilotDoc), RUNTIME_CLASS(CMainFrame),	// main SDI frame window
					  RUNTIME_CLASS(CXpilotView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    // ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
	return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CXpilotApp commands

int CXpilotApp::ExitInstance()
{
    // Add your specialized code here and/or call the base class
//      if (the_motd)
//              delete the_motd;

    int i = CWinApp::ExitInstance();
    return (i);
}

// interface routines to the xpilot "C" code
extern "C"
    void _Trace(char *lpszFormat, long a, long b, long c, long d, long e,
		long f, long g, long h, long i, long j, long k)
{
    AfxTrace(lpszFormat, a, b, c, d, e, f, g, h, i, j, k);
}


#if 0
extern "C" const char *_GetWSockErrText(int error)
{
    return (GetWSockErrText(error));
}

extern "C" void DisplayMOTD(const char *text, int len)
{
    CString cs;

    for (int i = 0; i < len; i++) {
	char c = *text++;
	if (c == '\012')
	    cs += '\r';
	cs += c;
    }
    if (!the_motd) {
	the_motd = new motd;
	the_motd->Create(IDD_MOTD, NULL);
	the_motd->CenterWindow();
	the_motd->isopen = TRUE;
	the_motd->m_edit1.FmtLines(TRUE);
	the_motd->m_edit1.SetWindowText(cs);
    } else {
	if (!the_motd->isopen) {
	    the_motd->Create(IDD_MOTD, NULL);
	    the_motd->CenterWindow();
	    the_motd->isopen = TRUE;
	}
    }
}
#endif

CString talkstring;

extern "C" const char *mfcDoTalkWindow(void)
{
    CTalkWindow tw;
    int ret = tw.DoModal();
    if (ret == IDOK)
	talkstring = tw.m_text;
    else
	talkstring = "";
    return ((const char *) talkstring);
}

BOOL CXpilotApp::PreTranslateMessage(MSG * pMsg)
{
    // CG: The following lines were added by the Splash Screen component.
    if (CSplashWnd::PreTranslateAppMessage(pMsg))
	return TRUE;

    return CWinApp::PreTranslateMessage(pMsg);
}

// App command to run the dialog
void CXpilotApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

extern "C"
    void Progress(const char *s, long a, long b, long c, long d, long e,
		  long f, long g, long h, long i, long j, long k)
{
    CString cs;
    cs.Format(s, a, b, c, d, e, f, g, h, i, j, k);
    CSplashWnd::ShowMessage(cs);


}
