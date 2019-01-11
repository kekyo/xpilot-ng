#if !defined(AFX_SLIDERBAR_H__337983E0_9951_11D5_A796_0000B48FE580__INCLUDED_)
#define AFX_SLIDERBAR_H__337983E0_9951_11D5_A796_0000B48FE580__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SliderBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSliderBar dialog

class CSliderBar : public CDialogBar
{
// Construction
public:
	CSliderBar(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSliderBar)
	enum { IDD = IDD_SLIDERBAR };
	CSliderCtrl	m_slider;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSliderBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSliderBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SLIDERBAR_H__337983E0_9951_11D5_A796_0000B48FE580__INCLUDED_)
