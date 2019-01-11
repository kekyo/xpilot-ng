// SliderBar.cpp : implementation file
//

#include "stdafx.h"
#include "XPreplay.h"
#include "SliderBar.h"
#include "XPreplayDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSliderBar dialog


CSliderBar::CSliderBar(CWnd* pParent /*=NULL*/)
	: CDialogBar()
//	: CDialog(CSliderBar::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSliderBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSliderBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSliderBar)
	DDX_Control(pDX, IDC_SLIDER1, m_slider);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSliderBar, CDialogBar)
	//{{AFX_MSG_MAP(CSliderBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSliderBar message handlers

int CSliderBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect	rect;
	GetClientRect(&rect);
	m_slider.Create(TBS_HORZ | TBS_AUTOTICKS | TBS_BOTTOM | TBS_ENABLESELRANGE | WS_CHILD | WS_VISIBLE, rect, this, IDC_SLIDER1);
	
	return 0;
}

void CSliderBar::OnSize(UINT nType, int cx, int cy) 
{
	CDialogBar::OnSize(nType, cx, cy);

	CRect	rect(0, 0, cx, cy - 2);
	m_slider.MoveWindow(rect);
}

void CSliderBar::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CXPreplayDoc	*pDoc = ((CXPreplayDoc *)GetParentFrame()->GetActiveDocument());

	if(pDoc->rc.cur->number != m_slider.GetPos())
	{
		if(pDoc->rc.cur->number > m_slider.GetPos())
		{
			while(pDoc->rc.cur->number != m_slider.GetPos())
			{
				pDoc->rc.cur = pDoc->rc.cur->prev;
			}
		}
		else
		{
			while(pDoc->rc.cur->number != m_slider.GetPos())
			{
				pDoc->rc.cur = pDoc->rc.cur->next;
			}
		}
		GetParentFrame()->GetActiveView()->Invalidate(FALSE);
	}

	CDialogBar::OnHScroll(nSBCode, nPos, pScrollBar);
}
