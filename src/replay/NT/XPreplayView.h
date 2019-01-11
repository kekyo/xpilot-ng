// XPreplayView.h : interface of the CXPreplayView class
//
/////////////////////////////////////////////////////////////////////////////

//#include "XPreplayDoc.h"
#if !defined(AFX_XPREPLAYVIEW_H__909E260C_85C6_11D5_A796_0000B48FE580__INCLUDED_)
#define AFX_XPREPLAYVIEW_H__909E260C_85C6_11D5_A796_0000B48FE580__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CXPreplayView : public CScrollView
{
protected: // create from serialization only
	CXPreplayView();
	DECLARE_DYNCREATE(CXPreplayView)

public:
	CXPreplayDoc* GetDocument();

private:
	CPen	*pencolor;
	CPen	*pencolordashed;
	CBrush	*brushcolor;
	CBitmap	*bitmapcolor;
	CBitmap	items[21];
	bool	Trustheader;
	CDC		bgDC;	// for flickerfree drawing
	CBitmap	bgBitmap;	// for flickerfree drawing
	CFont	gameFont;
	CFont	msgFont;

	short	*ConvertItem(unsigned char *c_bits);
	void DrawFrame(CXPreplayDoc::frame *frame);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXPreplayView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CXPreplayView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	enum {
		s_toStart,
		s_rewind,
		s_reverse,
		s_slowback,
		s_slowplay,
		s_playing,
		s_forward,
		s_toEnd,
		s_stopped,
		s_paused
	} status;
	int nTimer;

// Generated message map functions
protected:
	//{{AFX_MSG(CXPreplayView)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPlay();
	afx_msg void OnStop();
	afx_msg void OnPause();
	afx_msg void OnReverse();
	afx_msg void OnRewind();
	afx_msg void OnForward();
	afx_msg void OnToend();
	afx_msg void OnTostart();
	afx_msg void OnUpdateForward(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePause(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlay(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReverse(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRewind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToend(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTostart(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnOptionsTrustheader();
	afx_msg void OnUpdateOptionsTrustheader(CCmdUI* pCmdUI);
	afx_msg void OnSlowback();
	afx_msg void OnUpdateSlowback(CCmdUI* pCmdUI);
	afx_msg void OnSlowplay();
	afx_msg void OnUpdateSlowplay(CCmdUI* pCmdUI);
	afx_msg void OnStartframe();
	afx_msg void OnEndframe();
	afx_msg void OnUpdateStartframe(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEndframe(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in XPreplayView.cpp
inline CXPreplayDoc* CXPreplayView::GetDocument()
   { return (CXPreplayDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPREPLAYVIEW_H__909E260C_85C6_11D5_A796_0000B48FE580__INCLUDED_)
