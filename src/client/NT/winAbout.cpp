/* 
 * winAbout.cpp - XPilot.exe credits box
 *
 * This file contains the Windows about dialog and scrolling credits box.
 *
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
/*
 * $Log: winAbout.cpp,v $
 * Revision 1.3  2003/09/05 23:57:17  kps
 * Indented sources with "indent -kr -i4".
 *
 * Revision 1.2  2003/04/10 12:22:10  kps
 * 4.5.4X rc10
 *
 * Revision 5.2  2002/06/15 21:35:22  dik
 * Don't include includes if the symbols were already defined
 * (winAbout may be included from XPwho).
 *
 */

#include "StdAfx.h"
#ifndef	DRAW_H
#include "draw.h"
#endif
#ifndef	_WINX__H_
#include "winX_.h"
#include "resource.h"
#endif
#include "winAbout.h"
#include "../../common/version.h"

/////////////////////////////////////////////////////////////////////////////

#define	CR_WIDTH	crRect.Width()
//#define       CR_HEIGHT       2500
#define	LINEHEIGHT	19

// read in from credits.inc.h
CString credits;
int creditsHeight = 0;
int lineCount = 0;

CAboutDlg::CAboutDlg():CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT

}

void CAboutDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    DDX_Control(pDX, IDC_CREDITS, m_credits);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CCredits
    CCredits::CCredits()
{
    scrollofs = 0;
    timer = FALSE;
    virgin = TRUE;
}

CCredits::~CCredits()
{
#if 0
    CString e;
    e.Format("scrollofs = %d", scrollofs);
    AfxMessageBox(e);
#endif
}


BEGIN_MESSAGE_MAP(CCredits, CStatic)
    //{{AFX_MSG_MAP(CCredits)
    ON_WM_PAINT()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
    END_MESSAGE_MAP()
///////////////////////////////////////////////////////////////////////////////
// CCredits message handlers
void CCredits::OnTimer(UINT nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    if (scrollofs++ > creditsHeight)
	scrollofs = 0;
    TRACE("scrollofs = %d\n", scrollofs);
    CRect rect;
    GetClientRect(&rect);
    InvalidateRect(&rect, FALSE);
    CStatic::OnTimer(nIDEvent);
}

///////////////////////////////////////////////////////////////////////////////
void CCredits::OnPaint()
{
    CPaintDC dc(this);		// device context for painting
    CString out;
//      int             i;
    int line = 0;

    // TODO: Add your message handler code here
    //CFont*        oldFont = dc.SelectObject(&font);
    GetClientRect(&crRect);

    if (virgin) {
	// create the credits bitmap
	CDC bdc;
	//CDC* wdc;

	virgin = FALSE;
	//wdc = GetDC();
	bdc.CreateCompatibleDC(&dc);
	creditsHeight = lineCount * LINEHEIGHT + crRect.Height();
	bm.CreateCompatibleBitmap(&bdc, CR_WIDTH, creditsHeight);

	CBitmap *oldbm = bdc.SelectObject(&bm);
	BuildBitmap(&bdc);
	bdc.SelectObject(oldbm);
	//ReleaseDC(wdc);
    }
    if (!timer)
	SetTimer(32, 30, NULL);
    timer = TRUE;

    CDC bdc;
    bdc.CreateCompatibleDC(&dc);
    CBitmap *obm = bdc.SelectObject(&bm);
    dc.BitBlt(0, 0, crRect.Width(), crRect.Height(), &bdc, 0, scrollofs,
	      SRCCOPY);
    // dc.SelectObject(obm);
    // Do not call CStatic::OnPaint() for painting messages
    bdc.SelectObject(obm);
    //ReleaseDC(&bdc);
    //dc.SelectObject(oldFont);
}


///////////////////////////////////////////////////////////////////////////////
BOOL CCredits::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
		      DWORD dwStyle, const RECT & rect, CWnd * pParentWnd,
		      UINT nID, CCreateContext * pContext)
{
    // TODO: Add your specialized code here and/or call the base class
    BOOL ret;
    ret =
	CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect,
		     pParentWnd, nID, pContext);
    return (ret);
}

///////////////////////////////////////////////////////////////////////////////
BOOL CCredits::DestroyWindow()
{
    // TODO: Add your specialized code here and/or call the base class
    KillTimer(32);
    bm.DeleteObject();


    return CStatic::DestroyWindow();
}

///////////////////////////////////////////////////////////////////////////////
int GetAnInt(int *i)
{
    int x;
    char a;

    (*i)++;			// skip control
    a = credits[*i];		// get char
    (*i)++;
    x = a & 0x0F;
    a = credits[*i];
    (*i)++;
    x = (x * 10) + (a & 0x0F);
    a = credits[*i];
    (*i)++;
    x = (x * 10) + (a & 0x0F);
    return (x);
}

///////////////////////////////////////////////////////////////////////////////
void CCredits::BuildBitmap(CDC * dc)
{
    int row = 0;
    int col = 0;
    int i = 0;
    CString cs;
    int rowinc = LINEHEIGHT;
    int colinc = 10;
    COLORREF myColor;

    CRect rect(0, 0, CR_WIDTH, creditsHeight);
//      dc->FillSolidRect(&rect, objs[RED].color);
    COLORREF oldColor = dc->GetTextColor();
    dc->SetBkColor(RGB(0, 0, 0));
    myColor = RGB(255, 255, 255);

    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = -(CR_WIDTH / 42);
    lf.lfPitchAndFamily = FF_SWISS;
#if 0
    CString e;
    e.Format("Rect w=%d h=%d lfHeight=%d", crRect.Width(), crRect.Height(),
	     lf.lfHeight);
    AfxMessageBox(e);
#endif
    haveFont = font.CreateFontIndirect(&lf);
    CFont *oldfont = dc->SelectObject(&font);
//      myColor = objs[RED].color;

    for (i = 0; i < credits.GetLength();) {
	if (credits[i] == '#') {
	    if (cs.GetLength())	// anything in the buffer?
	    {			// yes, flush it
		TRACE("color=%08x cs=%s\n", myColor, (PCSTR) cs);
		dc->SetTextColor(myColor);
		dc->TextOut(col * 10 + 2,
			    row * LINEHEIGHT + crRect.Height(), cs);
		cs = "";
	    }
	    switch (credits[++i]) {
	    case 'r':
		row = row + GetAnInt(&i);
		col = 0;
//                              myColor = objs[RED].color;
		myColor = RGB(255, 0, 0);
		break;
	    case 'c':
		col = GetAnInt(&i);
//                              myColor = objs[WHITE].color;
		break;
	    default:
		AfxMessageBox("Unknown command in credits");
		break;
	    }
	} else {
	    cs += credits[i++];
	}
    }
    dc->SetTextColor(oldColor);
    dc->SelectObject(oldfont);
    font.DeleteObject();
}

BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SendDlgItemMessage(IDC_VERSION, WM_SETTEXT, 0,
		       (LPARAM) ("XPilot " TITLE));

#include "credits.inc.h"

    return TRUE;		// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
