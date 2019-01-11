// XPreplay.h : main header file for the XPREPLAY application
//

#if !defined(AFX_XPREPLAY_H__909E2604_85C6_11D5_A796_0000B48FE580__INCLUDED_)
#define AFX_XPREPLAY_H__909E2604_85C6_11D5_A796_0000B48FE580__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CXPreplayApp:
// See XPreplay.cpp for the implementation of this class
//

class CXPreplayApp : public CWinApp
{
public:
	CXPreplayApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXPreplayApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CXPreplayApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPREPLAY_H__909E2604_85C6_11D5_A796_0000B48FE580__INCLUDED_)
