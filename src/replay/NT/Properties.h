#if !defined(AFX_PROPERTIES_H__0AA19980_85E7_11D5_A796_0000B48FE580__INCLUDED_)
#define AFX_PROPERTIES_H__0AA19980_85E7_11D5_A796_0000B48FE580__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProperties dialog

class CProperties : public CDialog
{
// Construction
public:
	CProperties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProperties)
	enum { IDD = IDD_PROPERTIES };
	CString	m_nickname;
	CString	m_recorddate;
	CString	m_serveraddress;
	CString	m_userhost;
	CString	m_fps;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProperties)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPERTIES_H__0AA19980_85E7_11D5_A796_0000B48FE580__INCLUDED_)
