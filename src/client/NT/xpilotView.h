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
*  xpilotView.h : interface of the CXpilotView class						*
*																			*
*  XPilotNT uses the standard MFC doc/view model in an SDI format.			*
*  						*
\***************************************************************************/

#define	WSA_EVENT		WM_USER+300	// from WSAAsyncSelect
#define	WSA_RESOLVEHOST	WM_USER+301
#define	WSA_CONNECT		WM_USER+302
#define	WSA_RECV		WM_USER+303

extern "C" const char *GetWSockErrText(int error);

class CXpilotView:public CView {
  protected:			// create from serialization only
    CXpilotView();
    DECLARE_DYNCREATE(CXpilotView)
// Attributes
  public:
    CXpilotDoc * GetDocument();
    BOOL isVirgin;
    BOOL shuttingdown;

//      CPalette                cpal;
//      LOGPALETTE*             pal;

// Operations
  public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CXpilotView)
  public:
     virtual void OnDraw(CDC * pDC);	// overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT & cs);
    virtual void OnInitialUpdate();
    virtual BOOL DestroyWindow();
    virtual void OnSize(UINT, int, int);
  protected:
     virtual void OnActivateView(BOOL bActivate, CView * pActivateView,
				 CView * pDeactiveView);
    //}}AFX_VIRTUAL

// Implementation
  public:
     virtual ~ CXpilotView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext & dc) const;
#endif

  protected:

// Generated message map functions
  protected:
    //{{AFX_MSG(CXpilotView)
     afx_msg void OnDestroy();
    afx_msg void OnFileNew();
    afx_msg void OnUpdateFileNew(CCmdUI * pCmdUI);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    afx_msg LONG OnWSA_EVENT(UINT, LONG);
    //afx_msg       LONG OnWSA_RESOLVEHOST(UINT, LONG);
    //afx_msg       LONG OnWSA_CONNECT(UINT, LONG);
    //afx_msg       LONG OnWSA_EVENT(UINT, LONG);
     DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG			// debug version in xpilotView.cpp
inline CXpilotDoc *CXpilotView::GetDocument()
{
    return (CXpilotDoc *) m_pDocument;
}
#endif

/////////////////////////////////////////////////////////////////////////////
