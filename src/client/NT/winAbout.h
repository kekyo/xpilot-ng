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
*  winAbout.h : CAboutDlg dialog used for XPilotNT About					*
*																			*
*  This file contains the Windows about dialog and scrolling credits box.	*
*  						*
\***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// CCredits window

class CCredits:public CStatic {
// Construction
  public:
    CCredits();

// Attributes
  public:
    CFont font;
    BOOL haveFont;
    CRect crRect;
    int scrollofs;
    CBitmap bm;
    BOOL timer;
    BOOL virgin;

// Operations
  public:
    void BuildBitmap(CDC * dc);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCredits)
  public:
     virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
			 DWORD dwStyle, const RECT & rect,
			 CWnd * pParentWnd, UINT nID,
			 CCreateContext * pContext = NULL);
    virtual BOOL DestroyWindow();
    //}}AFX_VIRTUAL

// Implementation
  public:
     virtual ~ CCredits();

    // Generated message map functions
  protected:
    //{{AFX_MSG(CCredits)
     afx_msg void OnPaint();
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG

     DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////

class CAboutDlg:public CDialog {
  public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    CCredits m_credits;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
  protected:
     virtual void DoDataExchange(CDataExchange * pDX);	// DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
  protected:
    //{{AFX_MSG(CAboutDlg)
     virtual BOOL OnInitDialog();
    //}}AFX_MSG
     DECLARE_MESSAGE_MAP()
};
