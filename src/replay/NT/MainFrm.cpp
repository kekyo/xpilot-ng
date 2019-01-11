// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "XPreplay.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZING()
	ON_COMMAND(ID_VIEW_SLIDERBAR, OnViewSliderbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SLIDERBAR, OnUpdateViewSliderbar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_FRAMECOUNTER,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	showslider = true;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	if (!m_wndSliderBar.Create(this, IDD_SLIDERBAR, CBRS_BOTTOM, IDD_SLIDERBAR))
	{
		TRACE0("Failed to create sliderbar\n");
		return -1;      // fail to create
	}
	m_wndSliderBar.m_slider.EnableWindow(FALSE);

	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_FRAMECOUNTER, SBPS_NORMAL, 110);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSizing(UINT fwSide, LPRECT pRect) 
{
	CFrameWnd::OnSizing(fwSide, pRect);

	bool	adjust = false;

	if(pRect->right - pRect->left < 388)
	{
		switch(fwSide)
		{
		case 1:		// left
		case 4:		// top left
		case 7:		// bottom left
			pRect->left = pRect->right - 388;
			break;
		case 2:		// right
		case 5:		// top right
		case 8:		// bottom right
			pRect->right = pRect->left + 388;
			break;
		}

		adjust = true;
	}

	if(pRect->bottom - pRect->top < 247)
	{
		switch(fwSide)
		{
		case 3:		// top
		case 4:		// top left
		case 5:		// top right
			pRect->top = pRect->bottom - 247;
			break;
		case 6:		// bottom
		case 7:		// bottom left
		case 8:		// bottom right
			pRect->bottom = pRect->top + 247;
			break;
		}

		adjust = true;
	}

	if(adjust)
		MoveWindow(pRect);
}

void CMainFrame::OnViewSliderbar() 
{
	showslider = !showslider;
	m_wndSliderBar.ShowWindow(showslider);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewSliderbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(showslider);
}
