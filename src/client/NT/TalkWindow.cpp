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

// TalkWindow.cpp : implementation file
//

#include "stdafx.h"
#include "XPilotNT.h"
#include "TalkWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTalkWindow dialog


CTalkWindow::CTalkWindow(CWnd * pParent /*=NULL*/ )
:  CDialog(CTalkWindow::IDD, pParent)
{
    //{{AFX_DATA_INIT(CTalkWindow)
    m_text = _T("");
    //}}AFX_DATA_INIT
}


void CTalkWindow::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTalkWindow)
    DDX_Text(pDX, IDC_EDIT1, m_text);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTalkWindow, CDialog)
    //{{AFX_MSG_MAP(CTalkWindow)
    // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CTalkWindow message handlers
