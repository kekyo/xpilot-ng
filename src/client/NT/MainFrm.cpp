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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "XPilotNT.h"

#include "MainFrm.h"
#include "Splash.h"

#define _XPDOC
#include "winXXPilot.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

    BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    ON_WM_PALETTECHANGED()
    ON_WM_QUERYNEWPALETTE()
    END_MESSAGE_MAP()

static UINT indicators[] = {
    ID_SEPARATOR,		// status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    // TODO: add member initialization code here

}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
	return -1;

#if 0
    if (!m_wndToolBar.Create(this) ||
	!m_wndToolBar.LoadToolBar(IDR_MAINFRAME)) {
	TRACE0("Failed to create toolbar\n");
	return -1;		// fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
	!m_wndStatusBar.SetIndicators(indicators,
				      sizeof(indicators) / sizeof(UINT))) {
	TRACE0("Failed to create status bar\n");
	return -1;		// fail to create
    }
    // TODO: Remove this if you don't want tool tips or a resizeable toolbar
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
			     CBRS_TOOLTIPS | CBRS_FLYBY |
			     CBRS_SIZE_DYNAMIC);

    // TODO: Delete these three lines if you don't want the toolbar to
    //  be dockable
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);
#endif

    // CG: The following line was added by the Splash Screen component.
    CSplashWnd::ShowSplashScreen(this);
    return 0;
}

void CMainFrame::OnPaletteChanged(CWnd * pFocusWnd)
{
    CFrameWnd::OnPaletteChanged(pFocusWnd);

    if (pFocusWnd != this)
	OnQueryNewPalette();
}

BOOL CMainFrame::OnQueryNewPalette()
{
    ChangePalette(HWND(*this));

    //return CFrameWnd::OnQueryNewPalette();
    return (TRUE);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT & cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    if (cs.hMenu != NULL) {
	::DestroyMenu(cs.hMenu);	// delete menu if loaded
	cs.hMenu = NULL;	// no menu for this window
    }

    return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext & dc) const
{
    CFrameWnd::Dump(dc);
}

#endif				//_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::PreTranslateMessage(MSG * pMsg)
{
    // TODO: Add your specialized code here and/or call the base class

    if (pMsg->message == WM_SYSKEYUP)
	return (1);
    return CFrameWnd::PreTranslateMessage(pMsg);
}
