// XPreplayView.cpp : implementation of the CXPreplayView class
//

#include "stdafx.h"
#include "XPreplay.h"

#include "XPreplayDoc.h"
#include "XPreplayView.h"
#include "MainFrm.h"
#include <math.h>
#include "../../client/recordfmt.h"
#include "../../client/items/itemRocketPack.xbm"
#include "../../client/items/itemCloakingDevice.xbm"
#include "../../client/items/itemEnergyPack.xbm"
#include "../../client/items/itemWideangleShot.xbm"
#include "../../client/items/itemRearShot.xbm"
#include "../../client/items/itemMinePack.xbm"
#include "../../client/items/itemSensorPack.xbm"
#include "../../client/items/itemTank.xbm"
#include "../../client/items/itemEcm.xbm"
#include "../../client/items/itemArmor.xbm"
#include "../../client/items/itemAfterburner.xbm"
#include "../../client/items/itemTransporter.xbm"
#include "../../client/items/itemDeflector.xbm"
#include "../../client/items/itemHyperJump.xbm"
#include "../../client/items/itemPhasingDevice.xbm"
#include "../../client/items/itemMirror.xbm"
#include "../../client/items/itemLaser.xbm"
#include "../../client/items/itemEmergencyThrust.xbm"
#include "../../client/items/itemTractorBeam.xbm"
#include "../../client/items/itemAutopilot.xbm"
#include "../../client/items/itemEmergencyShield.xbm"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXPreplayView

IMPLEMENT_DYNCREATE(CXPreplayView, CScrollView)

BEGIN_MESSAGE_MAP(CXPreplayView, CScrollView)
	//{{AFX_MSG_MAP(CXPreplayView)
	ON_WM_TIMER()
	ON_COMMAND(IDT_PLAY, OnPlay)
	ON_COMMAND(IDT_STOP, OnStop)
	ON_COMMAND(IDT_PAUSE, OnPause)
	ON_COMMAND(IDT_REVERSE, OnReverse)
	ON_COMMAND(IDT_REWIND, OnRewind)
	ON_COMMAND(IDT_FORWARD, OnForward)
	ON_COMMAND(IDT_TOEND, OnToend)
	ON_COMMAND(IDT_TOSTART, OnTostart)
	ON_UPDATE_COMMAND_UI(IDT_FORWARD, OnUpdateForward)
	ON_UPDATE_COMMAND_UI(IDT_PAUSE, OnUpdatePause)
	ON_UPDATE_COMMAND_UI(IDT_PLAY, OnUpdatePlay)
	ON_UPDATE_COMMAND_UI(IDT_REVERSE, OnUpdateReverse)
	ON_UPDATE_COMMAND_UI(IDT_REWIND, OnUpdateRewind)
	ON_UPDATE_COMMAND_UI(IDT_STOP, OnUpdateStop)
	ON_UPDATE_COMMAND_UI(IDT_TOEND, OnUpdateToend)
	ON_UPDATE_COMMAND_UI(IDT_TOSTART, OnUpdateTostart)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_OPTIONS_TRUSTHEADER, OnOptionsTrustheader)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_TRUSTHEADER, OnUpdateOptionsTrustheader)
	ON_COMMAND(IDT_SLOWBACK, OnSlowback)
	ON_UPDATE_COMMAND_UI(IDT_SLOWBACK, OnUpdateSlowback)
	ON_COMMAND(IDT_SLOWPLAY, OnSlowplay)
	ON_UPDATE_COMMAND_UI(IDT_SLOWPLAY, OnUpdateSlowplay)
	ON_COMMAND(IDT_STARTFRAME, OnStartframe)
	ON_COMMAND(IDT_ENDFRAME, OnEndframe)
	ON_UPDATE_COMMAND_UI(IDT_STARTFRAME, OnUpdateStartframe)
	ON_UPDATE_COMMAND_UI(IDT_ENDFRAME, OnUpdateEndframe)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXPreplayView construction/destruction

CXPreplayView::CXPreplayView()
{
	short	*temp;

	status = s_stopped;
	Trustheader = true;
	pencolor = NULL;
	pencolordashed = NULL;
	brushcolor = NULL;
	bitmapcolor = NULL;

	temp = ConvertItem(itemEnergyPack_bits);
	items[0].CreateBitmap(itemEnergyPack_width, itemEnergyPack_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemWideangleShot_bits);
	items[1].CreateBitmap(itemWideangleShot_width, itemWideangleShot_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemRearShot_bits);
	items[2].CreateBitmap(itemRearShot_width, itemRearShot_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemAfterburner_bits);
	items[3].CreateBitmap(itemAfterburner_width, itemAfterburner_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemCloakingDevice_bits);
	items[4].CreateBitmap(itemCloakingDevice_width, itemCloakingDevice_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemSensorPack_bits);
	items[5].CreateBitmap(itemSensorPack_width, itemSensorPack_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemTransporter_bits);
	items[6].CreateBitmap(itemTransporter_width, itemTransporter_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemTank_bits);
	items[7].CreateBitmap(itemTank_width, itemTank_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemMinePack_bits);
	items[8].CreateBitmap(itemMinePack_width, itemMinePack_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemRocketPack_bits);
	items[9].CreateBitmap(itemRocketPack_width, itemRocketPack_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemEcm_bits);
	items[10].CreateBitmap(itemEcm_width, itemEcm_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemLaser_bits);
	items[11].CreateBitmap(itemLaser_width, itemLaser_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemEmergencyThrust_bits);
	items[12].CreateBitmap(itemEmergencyThrust_width, itemEmergencyThrust_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemTractorBeam_bits);
	items[13].CreateBitmap(itemTractorBeam_width, itemTractorBeam_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemAutopilot_bits);
	items[14].CreateBitmap(itemAutopilot_width, itemAutopilot_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemEmergencyShield_bits);
	items[15].CreateBitmap(itemEmergencyShield_width, itemEmergencyShield_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemDeflector_bits);
	items[16].CreateBitmap(itemDeflector_width, itemDeflector_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemHyperJump_bits);
	items[17].CreateBitmap(itemHyperJump_width, itemHyperJump_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemPhasingDevice_bits);
	items[18].CreateBitmap(itemPhasingDevice_width, itemPhasingDevice_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemMirror_bits);
	items[19].CreateBitmap(itemMirror_width, itemMirror_height, 1, 1, temp);
	delete[] temp;

	temp = ConvertItem(itemArmor_bits);
	items[20].CreateBitmap(itemArmor_width, itemArmor_height, 1, 1, temp);
	delete[] temp;
}

CXPreplayView::~CXPreplayView()
{
	for(int i = 0; i < 21; i++)
	{
		items[i].DeleteObject();
	}

	if(pencolor)
		delete[] pencolor;
	if(pencolordashed)
		delete[] pencolordashed;
	if(brushcolor)
		delete[] brushcolor;
	if(bitmapcolor)
		delete[] bitmapcolor;
}

BOOL CXPreplayView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CXPreplayView drawing

void CXPreplayView::OnDraw(CDC* pDC)
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->docOpened)
	{
		DrawFrame(pDoc->rc.cur);
	}
}

void CXPreplayView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CSize sizeTotal;
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pencolor)
		delete[] pencolor;
	if(pencolordashed)
		delete[] pencolordashed;
	if(brushcolor)
		delete[] brushcolor;
	if(bitmapcolor)
		delete[] bitmapcolor;

	if(pDoc->docOpened)
	{
		if(Trustheader)
		{
			sizeTotal.cx = pDoc->rc.view_width;
			sizeTotal.cy = pDoc->rc.view_height;
		}
		else
		{
			sizeTotal.cx = pDoc->max.x;
			sizeTotal.cy = pDoc->max.y;
		}

		pencolor = new CPen[pDoc->rc.maxColors];
		pencolordashed = new CPen[pDoc->rc.maxColors];
		brushcolor = new CBrush[pDoc->rc.maxColors];
		bitmapcolor = new CBitmap[pDoc->rc.maxColors];
		CDC	tempDC;
		tempDC.CreateCompatibleDC(GetDC());

		for(int i = 0; i < pDoc->rc.maxColors; i++)
		{
			pencolor[i].CreatePen(PS_SOLID, 1, pDoc->rc.colors[i].color);
			pencolordashed[i].CreatePen(PS_DOT, 1, pDoc->rc.colors[i].color);
			brushcolor[i].CreateSolidBrush(pDoc->rc.colors[i].color);

			bitmapcolor[i].CreateCompatibleBitmap(GetDC(), 16, 16);
			tempDC.SelectObject(&bitmapcolor[i]);
			tempDC.FillSolidRect(0, 0, 16, 16, pDoc->rc.colors[i].color);
		}
		tempDC.DeleteDC();

		CSliderCtrl	*slider = &(((CMainFrame *)GetParent())->m_wndSliderBar.m_slider);
		slider->SetRange(0, pDoc->frame_count - 1);
		slider->SetSelection(0, pDoc->frame_count - 1);
		slider->SetTicFreq(pDoc->frame_count / 50 + 1);
		slider->SetPos(0);
		slider->EnableWindow(TRUE);
		pDoc->minSelection = 0;
		pDoc->maxSelection = pDoc->frame_count - 1;

		bgDC.DeleteDC();
		bgDC.CreateCompatibleDC(GetDC());

		bgBitmap.DeleteObject();
		if(Trustheader)
			bgBitmap.CreateCompatibleBitmap(GetDC(), pDoc->rc.view_width, pDoc->rc.view_height);
		else
			bgBitmap.CreateCompatibleBitmap(GetDC(), pDoc->max.x, pDoc->max.y);

		bgDC.SelectObject(&bgBitmap);

		gameFont.DeleteObject();
		msgFont.DeleteObject();

		gameFont.CreatePointFont(8*pDoc->rc.gameFontSize, pDoc->rc.gameFont, &bgDC);
		msgFont.CreatePointFont(8*pDoc->rc.msgFontSize, pDoc->rc.gameFont, &bgDC);

		DrawFrame(pDoc->rc.head);		// this line gets rid of small drawing-
										// errors with dashed lines.
	}
	else
		sizeTotal.cx = sizeTotal.cy = 10;

	SetScrollSizes(MM_TEXT, sizeTotal);
	status = s_stopped;
	KillTimer(nTimer);
}

/////////////////////////////////////////////////////////////////////////////
// CXPreplayView diagnostics

#ifdef _DEBUG
void CXPreplayView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CXPreplayView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CXPreplayDoc* CXPreplayView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CXPreplayDoc)));
	return (CXPreplayDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CXPreplayView message handlers

void CXPreplayView::OnTimer(UINT nIDEvent) 
{
	CScrollView::OnTimer(nIDEvent);
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch(status)
	{
	case s_toStart:
		break;
	case s_rewind:
	case s_reverse:
	case s_slowback:
		if(pDoc->rc.cur->prev == NULL)
		{
			KillTimer(nTimer);
			status = s_stopped;
		}
		else
		{
			pDoc->rc.cur = pDoc->rc.cur->prev;
		}
		break;
	case s_playing:
	case s_forward:
	case s_slowplay:
		if(pDoc->rc.cur->next == NULL)
		{
			KillTimer(nTimer);
			status = s_stopped;
		}
		else
		{
			pDoc->rc.cur = pDoc->rc.cur->next;
		}
		break;
	case s_toEnd:
	case s_stopped:
	case s_paused:
	default:
		break;
	}

	Invalidate(FALSE);
}

void CXPreplayView::DrawFrame(CXPreplayDoc::frame *frame)
{
	if(!frame)
		return;

	CXPreplayDoc *pDoc = GetDocument();
	CXPreplayDoc::rShape	*shapes;
	int i;
	CBrush	*originalBrush;
	CPen	*originalPen;
	CFont	*originalFont;
	CPoint	ScrollOffset;
	CPoint	Origin;
	int currentcolor = 1;
	int	height, width;
	CPen	otherpen;

	if(Trustheader)
	{
		height = pDoc->rc.view_height;
		width = pDoc->rc.view_width;
	}
	else
	{
		width = pDoc->max.x;
		height = pDoc->max.y;
	}

	bgDC.FillSolidRect(0, 0, width, height, pDoc->rc.colors[0].color);

	ScrollOffset = GetScrollPosition();
	Origin = bgDC.SetViewportOrg(-ScrollOffset);
	shapes = frame->shapes;
	if(shapes)
		currentcolor = shapes->gc->color;
	else
		return;

	originalPen = bgDC.SelectObject(&pencolor[currentcolor]);

	while(shapes)
	{
		if(shapes->gc->color >= 0)
		{
			currentcolor = shapes->gc->color;
		}

		if(shapes->gc->dashes)
		{
			bgDC.SelectObject(&pencolordashed[currentcolor]);
		}
		else
		{
			bgDC.SelectObject(&pencolor[currentcolor]);
		}

		if(shapes->gc->width > 1)
		{
			bgDC.SelectObject(originalPen);
			otherpen.DeleteObject();
			otherpen.CreatePen(PS_SOLID, shapes->gc->width - 1, pDoc->rc.colors[currentcolor].color);
			bgDC.SelectObject(&otherpen);
		}

		switch(shapes->type)
		{
		case RC_DRAWARC:
			bgDC.Arc(	shapes->shape.arc.x1,
						shapes->shape.arc.y1,
						shapes->shape.arc.x2,
						shapes->shape.arc.y2,
						shapes->shape.arc.x3,
						shapes->shape.arc.y3,
						shapes->shape.arc.x4,
						shapes->shape.arc.y4);
			break;

		case RC_DRAWLINES:
			bgDC.Polyline(	shapes->shape.lines.points,
							shapes->shape.lines.npoints);
			break;

		case RC_DRAWLINE:
			bgDC.MoveTo(shapes->shape.line.x1, shapes->shape.line.y1);
			bgDC.LineTo(shapes->shape.line.x2, shapes->shape.line.y2);
			break;

		case RC_DRAWRECTANGLE:
			originalBrush = (CBrush *)bgDC.SelectStockObject(NULL_BRUSH);
			bgDC.Rectangle(	shapes->shape.rectangle.x,
							shapes->shape.rectangle.y,
							shapes->shape.rectangle.x + shapes->shape.rectangle.width,
							shapes->shape.rectangle.y + shapes->shape.rectangle.height);
			bgDC.SelectObject(originalBrush);
			break;

		case RC_DRAWSTRING:
			bgDC.SetBkMode(TRANSPARENT);
			bgDC.SetTextColor(pDoc->rc.colors[currentcolor].color);

			if(shapes->shape.string.font)
			{
				originalFont = bgDC.SelectObject(&msgFont);
				bgDC.TextOut(	shapes->shape.string.x,
								shapes->shape.string.y - (int)(0.8*pDoc->rc.msgFontSize),
								shapes->shape.string.string);
			}
			else
			{
				originalFont = bgDC.SelectObject(&gameFont);
				bgDC.TextOut(	shapes->shape.string.x,
								shapes->shape.string.y - (int)(0.8*pDoc->rc.gameFontSize),
								shapes->shape.string.string);
			}

			bgDC.SelectObject(originalFont);
			break;

		case RC_FILLARC:
			{
				originalBrush = bgDC.SelectObject(&brushcolor[currentcolor]);

				bgDC.Pie(	shapes->shape.arc.x1,
							shapes->shape.arc.y1,
							shapes->shape.arc.x2,
							shapes->shape.arc.y2,
							shapes->shape.arc.x3,
							shapes->shape.arc.y3,
							shapes->shape.arc.x4,
							shapes->shape.arc.y4);

				bgDC.SelectObject(originalBrush);
			}
			break;

		case RC_FILLPOLYGON:
			{
				originalBrush = bgDC.SelectObject(&brushcolor[currentcolor]);
				bgDC.Polygon(	shapes->shape.polygon.points,
								shapes->shape.polygon.npoints);
				bgDC.SelectObject(originalBrush);
			}
			break;

		case RC_FILLRECTANGLE:
			bgDC.FillSolidRect(	shapes->shape.rectangle.x,
								shapes->shape.rectangle.y,
								shapes->shape.rectangle.width,
								shapes->shape.rectangle.height,
								pDoc->rc.colors[currentcolor].color);
			break;

		case RC_PAINTITEMSYMBOL:
			{
				CImageList	list;
				CPoint		here(shapes->shape.symbol.x+1, shapes->shape.symbol.y);

				list.Create(16, 16, ILC_COLOR8 | ILC_MASK, 1, 1);
				list.Add(&bitmapcolor[currentcolor], &items[shapes->shape.symbol.type]);
				list.Draw(&bgDC, 0, here, ILD_TRANSPARENT);
				list.DeleteImageList();
			}
			break;

		case RC_FILLRECTANGLES:
			for(i = 0; i < shapes->shape.rectangles.nrectangles; i++)
			{
				bgDC.FillSolidRect(shapes->shape.rectangles.rectangles[i], pDoc->rc.colors[currentcolor].color);
			}
			break;

		case RC_DRAWARCS:
			for(i = 0; i < shapes->shape.arcs.narcs; i++)
			{
				bgDC.Arc(	shapes->shape.arcs.arcs[i].x1,
							shapes->shape.arcs.arcs[i].y1,
							shapes->shape.arcs.arcs[i].x2,
							shapes->shape.arcs.arcs[i].y2,
							shapes->shape.arcs.arcs[i].x3,
							shapes->shape.arcs.arcs[i].y3,
							shapes->shape.arcs.arcs[i].x4,
							shapes->shape.arcs.arcs[i].y4);
			}
			break;

		case RC_DRAWSEGMENTS:
			for(i = 0; i < shapes->shape.segments.nsegments; i++)
			{
				bgDC.MoveTo(shapes->shape.segments.segments[i].x1, shapes->shape.segments.segments[i].y1);
				bgDC.LineTo(shapes->shape.segments.segments[i].x2, shapes->shape.segments.segments[i].y2);
			}
			break;

		case RC_DAMAGED:
			bgDC.FillSolidRect(0, 0, width, height, pDoc->rc.colors[currentcolor].color);
			break;

		default:
			{
				CString errormessage;
				errormessage.Format("Unknown shape (%d)", shapes->type);
				MessageBox((LPCTSTR)errormessage, "Draw error", MB_OK | MB_ICONHAND);
			}
			break;
		}
		shapes = shapes->next;
	}

	bgDC.SelectObject(originalPen);
	bgDC.SetViewportOrg(Origin);

	GetDC()->BitBlt(0, 0, width, height, &bgDC, 0, 0, SRCCOPY);

	((CMainFrame *)GetParent())->m_wndSliderBar.m_slider.SetPos(frame->number);
}

void CXPreplayView::OnPlay() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur == pDoc->rc.tail)
	{
		pDoc->rc.cur = pDoc->rc.head;
	}
	nTimer = SetTimer(1, 1000/pDoc->rc.fps, NULL);
	status = s_playing;
}

void CXPreplayView::OnStop() 
{
	KillTimer(nTimer);
	status = s_stopped;
	GetDocument()->rc.cur = GetDocument()->rc.head;
	Invalidate(FALSE);
}

void CXPreplayView::OnPause() 
{
	KillTimer(nTimer);
	status = s_paused;
}

void CXPreplayView::OnReverse() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur == pDoc->rc.head)
	{
		pDoc->rc.cur = pDoc->rc.tail;
	}
	nTimer = SetTimer(1, 1000/pDoc->rc.fps, NULL);
	status = s_reverse;
}

void CXPreplayView::OnRewind() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur != pDoc->rc.head)
	{
		nTimer = SetTimer(1, 1000/(4*pDoc->rc.fps), NULL);
		status = s_rewind;
	}
	else
		status = s_stopped;
}

void CXPreplayView::OnForward() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur != pDoc->rc.tail)
	{
		nTimer = SetTimer(1, 1000/(4*pDoc->rc.fps), NULL);
		status = s_forward;
	}
	else
		status = s_stopped;
}

void CXPreplayView::OnToend() 
{
	GetDocument()->rc.cur = GetDocument()->rc.tail;
	Invalidate(FALSE);
}

void CXPreplayView::OnTostart() 
{
	GetDocument()->rc.cur = GetDocument()->rc.head;
	Invalidate(FALSE);
}

void CXPreplayView::OnSlowplay() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur == pDoc->rc.tail)
	{
		pDoc->rc.cur = pDoc->rc.head;
	}
	nTimer = SetTimer(1, (4*1000)/pDoc->rc.fps, NULL);
	status = s_slowplay;
}

void CXPreplayView::OnSlowback() 
{
	CXPreplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->rc.cur == pDoc->rc.head)
	{
		pDoc->rc.cur = pDoc->rc.tail;
	}
	nTimer = SetTimer(1, (4*1000)/pDoc->rc.fps, NULL);
	status = s_slowback;
}

void CXPreplayView::OnStartframe() 
{
	int	start, end;
	CSliderCtrl	*slider = &(((CMainFrame *)GetParent())->m_wndSliderBar.m_slider);

	slider->GetSelection(start, end);
	start = GetDocument()->rc.cur->number;
	slider->SetSelection(start, end);
	slider->RedrawWindow();
	GetDocument()->minSelection = start;
}

void CXPreplayView::OnEndframe() 
{
	int	start, end;
	CSliderCtrl	*slider = &(((CMainFrame *)GetParent())->m_wndSliderBar.m_slider);

	slider->GetSelection(start, end);
	end = GetDocument()->rc.cur->number;
	slider->SetSelection(start, end);
	slider->RedrawWindow();
	GetDocument()->maxSelection = end;
}

void CXPreplayView::OnUpdateForward(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_forward);
}

void CXPreplayView::OnUpdatePause(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_paused);
}

void CXPreplayView::OnUpdatePlay(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_playing);
}

void CXPreplayView::OnUpdateReverse(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_reverse);
}

void CXPreplayView::OnUpdateRewind(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_rewind);
}

void CXPreplayView::OnUpdateStop(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_stopped);
}

void CXPreplayView::OnUpdateToend(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_toEnd);
}

void CXPreplayView::OnUpdateTostart(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_toStart);
}

void CXPreplayView::OnUpdateSlowback(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_slowback);
}

void CXPreplayView::OnUpdateSlowplay(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
	pCmdUI->SetCheck(status == s_slowplay);
}

void CXPreplayView::OnUpdateStartframe(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
}

void CXPreplayView::OnUpdateEndframe(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocument()->docOpened);
}

BOOL CXPreplayView::OnEraseBkgnd(CDC* pDC) 
{
	CRect rect;
	CXPreplayDoc	*pDoc = GetDocument();
	GetClientRect(&rect);

	if(GetDocument()->docOpened)
	{
		if(Trustheader)
		{
			pDC->FillSolidRect(0, 0, pDoc->rc.view_width, pDoc->rc.view_height, pDoc->rc.colors[0].color);
			pDC->FillSolidRect(pDoc->rc.view_width, rect.top, rect.right, rect.bottom, RGB(128, 128, 128));
			pDC->FillSolidRect(rect.left, pDoc->rc.view_height, rect.right, rect.bottom, RGB(128, 128, 128));
		}
		else
		{
			pDC->FillSolidRect(0, 0, pDoc->max.x, pDoc->max.y, pDoc->rc.colors[0].color);
			pDC->FillSolidRect(pDoc->max.x, rect.top, rect.right, rect.bottom, RGB(128, 128, 128));
			pDC->FillSolidRect(rect.left, pDoc->max.y, rect.right, rect.bottom, RGB(128, 128, 128));
		}
	}
	else
		pDC->FillSolidRect(&rect, RGB(128, 128, 128));


	return TRUE;
}

void CXPreplayView::OnOptionsTrustheader() 
{
	CXPreplayDoc	*pDoc = GetDocument();

	Trustheader = !Trustheader;

	CSize sizeTotal;
	if(Trustheader)
	{
		sizeTotal.cx = pDoc->rc.view_width;
		sizeTotal.cy = pDoc->rc.view_height;
	}
	else
	{
		sizeTotal.cx = pDoc->max.x;
		sizeTotal.cy = pDoc->max.y;
	}
	SetScrollSizes(MM_TEXT, sizeTotal);

	bgBitmap.DeleteObject();
	if(Trustheader)
		bgBitmap.CreateCompatibleBitmap(GetDC(), pDoc->rc.view_width, pDoc->rc.view_height);
	else
		bgBitmap.CreateCompatibleBitmap(GetDC(), pDoc->max.x, pDoc->max.y);

	bgDC.SelectObject(&bgBitmap);
	Invalidate(TRUE);
}

void CXPreplayView::OnUpdateOptionsTrustheader(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(Trustheader);
}

short *CXPreplayView::ConvertItem(unsigned char *c_bits)
{
	short	*s_bits;

	s_bits = new short[itemAutopilot_width * itemAutopilot_width / 16];

	for(int i = 0; i < itemAutopilot_width * itemAutopilot_width / 16; i++)
	{
		s_bits[i] = c_bits[2*i];
		s_bits[i] <<= 8;
		s_bits[i] += c_bits[2*i+1];
		s_bits[i] = ~s_bits[i];

		for(int j = 0; j < 8; j++)
		{
			if(s_bits[i] & (1U << j))	// mirror
			{
				if(s_bits[i] & (1U << (15 - j)))
				{
					// no need to swap 1 with 1
				}
				else
				{
					s_bits[i] &= ~(1U << j);
					s_bits[i] |= 1U << (15 - j);
				}
			}
			else
			{
				if(s_bits[i] & 1U << (15 - j))
				{
					s_bits[i] |= 1U << j;
					s_bits[i] &= ~(1U << (15 - j));
				}
				else
				{
					// no need to swap 0 with 0
				}
			}
		}
	}

	return s_bits;
}

void CXPreplayView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CXPreplayDoc	*pDoc = GetDocument();
	CSliderCtrl	*slider = &(((CMainFrame *)GetParent())->m_wndSliderBar.m_slider);

	slider->SetRange(0, pDoc->frame_count - 1);
	slider->SetSelection(0, pDoc->frame_count - 1);
	slider->SetTicFreq(pDoc->frame_count / 50 + 1);
	slider->SetPos(0);
}
