// Properties.cpp : implementation file
//

#include "stdafx.h"
#include "XPreplay.h"
#include "Properties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProperties dialog


CProperties::CProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProperties)
	m_nickname = _T("");
	m_recorddate = _T("");
	m_serveraddress = _T("");
	m_userhost = _T("");
	m_fps = _T("");
	//}}AFX_DATA_INIT
}


void CProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProperties)
	DDX_Text(pDX, IDC_NICK, m_nickname);
	DDX_Text(pDX, IDC_RECDATE, m_recorddate);
	DDX_Text(pDX, IDC_SERVERADDRESS, m_serveraddress);
	DDX_Text(pDX, IDC_USERHOST, m_userhost);
	DDX_Text(pDX, IDC_FPS, m_fps);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProperties, CDialog)
	//{{AFX_MSG_MAP(CProperties)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProperties message handlers
