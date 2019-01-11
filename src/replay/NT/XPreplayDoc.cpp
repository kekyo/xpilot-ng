// XPreplayDoc.cpp : implementation of the CXPreplayDoc class
//

#include "stdafx.h"
#include "XPreplay.h"
#include "MainFrm.h"

#include "XPreplayDoc.h"
#include "../../client/recordfmt.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXPreplayDoc

IMPLEMENT_DYNCREATE(CXPreplayDoc, CDocument)

BEGIN_MESSAGE_MAP(CXPreplayDoc, CDocument)
	//{{AFX_MSG_MAP(CXPreplayDoc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FRAMECOUNTER, OnUpdatePage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXPreplayDoc construction/destruction

CXPreplayDoc::CXPreplayDoc()
{
	docOpened = 0;
	rc.nickname = NULL;
	rc.realname = NULL;
	rc.hostname = NULL;
	rc.servername = NULL;
	rc.recorddate = NULL;
	rc.gameFont = NULL;
	rc.msgFont = NULL;
}

CXPreplayDoc::~CXPreplayDoc()
{
	if(docOpened)
	{
		FreeFrames();
		FreeRC();
	}
}

BOOL CXPreplayDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CXPreplayDoc serialization

void CXPreplayDoc::Serialize(CArchive& ar)
{
	static CString	filename;
	if (ar.IsStoring())
	{							//saving
		BeginWaitCursor();
		struct frame	*frm;
		DWORD	min, max, offset;
		CFile	file;

		if(!file.Open((LPCTSTR)filename, CFile::modeRead))
		{
			MessageBox(NULL, "Save failed, unable to open sourcefile", "Error", MB_OK | MB_ICONSTOP);
			return;
		}

		frm = rc.head;

		char	*headerbuf;
		headerbuf = new char[frm->filepos];
		file.Read(headerbuf, frm->filepos);
		ar.Write(headerbuf, frm->filepos);
		delete[] headerbuf;

		while(minSelection != frm->number)
			frm = frm->next;

		min = frm->filepos;

		while(maxSelection != frm->number)
			frm = frm->next;

		if(frm == rc.tail)
			max = file.GetLength();
		else
			max = frm->next->filepos;

		file.Seek(min, CFile::begin);

		char	buffer[4096];
		for(DWORD i = min; i < max - 4096; i+= 4096)
		{
			file.Read(buffer, 4096);
			ar.Write(buffer, 4096);
		}
		file.Read(buffer, max - i);	// leftovers
		ar.Write(buffer, max - i);

		file.Close();

//		Now that the file is written, show the new file

		frm = rc.head;
		offset = frm->filepos;

		while(frm->number != minSelection)
		{
			FreeShapes(frm);
			frm = frm->next;
			delete frm->prev;
		}
		frm->prev = NULL;
		rc.head = frm;
		rc.cur = frm;
		offset = frm->filepos - offset;

		frm = rc.tail;

		while(frm->number != maxSelection)
		{
			FreeShapes(frm);
			frm = frm->prev;
			delete frm->next;
		}
		frm->next = NULL;
		rc.tail = frm;

		frm = rc.head;
		frame_count = 0;
		while(frm)
		{
			frm->number = frame_count++;
			frm->filepos -= offset;
			frm = frm->next;
		}

		minSelection = 0;
		maxSelection = frame_count - 1;
		UpdateAllViews(NULL);
		filename = ar.GetFile()->GetFilePath();

		EndWaitCursor();
	}
	else
	{							// loading
		BeginWaitCursor();
		if(docOpened)
		{
			docOpened = 0;
			FreeFrames();
			FreeRC();
			rc.head = NULL;
			rc.tail = NULL;
			rc.cur = NULL;
		}

		frame_count = 0;
		rc.tail = NULL;
		max.x = -9999;
		max.y = -9999;
		rc.eof = false;

		ReadHeader(ar);
		while(!rc.eof)
		{
			ReadNextFrame(ar);
		}

		max.x -= 20;
		max.y -= 20;
		filename = ar.GetFile()->GetFilePath();
		docOpened = 1;
		EndWaitCursor();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CXPreplayDoc diagnostics

#ifdef _DEBUG
void CXPreplayDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CXPreplayDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CXPreplayDoc commands

void CXPreplayDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(docOpened);
}

void CXPreplayDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(docOpened);
}

void CXPreplayDoc::ReadHeader(CArchive& ar)
{
	char		magic[5];
	unsigned char	minor;
	unsigned char	major;
	char		dot;
	char		nl;
	char		byte;
	bool		manycolors = false;
	CString	errormessage;

	ar >> magic[0];
	ar >> magic[1];
	ar >> magic[2];
	ar >> magic[3];
	magic[4] = '\0';
	ar >> major;
	ar >> dot;
	ar >> minor;
	ar >> nl;
	if (strcmp(magic, "XPRC") || dot != '.' || nl != '\n')
	{
		MessageBox(NULL, "Not a valid XPilot Recording file.", "Read error", MB_OK | MB_ICONHAND);
		return;
	}
	/*
	 * We are incompatible with newer versions
	 * and with older versions whose major version number
	 * differs from ours.
	 */
	if (major != RC_MAJORVERSION || minor > RC_MINORVERSION)
	{
		errormessage.Format("Incompatible version. (file: %c.%c)(program: %c.%c)",
			major, minor, RC_MAJORVERSION, RC_MINORVERSION);
		MessageBox(NULL, (LPCTSTR)errormessage, "Read error", MB_OK | MB_ICONHAND);
		return;
	}
	rc.majorversion = major;
	rc.minorversion = minor;
	rc.nickname = ReadString(ar);
	rc.realname = ReadString(ar);
	rc.hostname = ReadString(ar);
	rc.servername = ReadString(ar);
	ar >> byte;
	rc.fps = (int)byte;
	rc.recorddate = ReadString(ar);
	ar >> rc.maxColors;

	rc.colors = new struct colors[rc.maxColors];
	for(int i = 0; i < rc.maxColors; i++)
	{
		ar >> rc.colors[i].pixel;
		ar >> rc.colors[i].red;
		ar >> rc.colors[i].green;
		ar >> rc.colors[i].blue;

		if(rc.colors[i].red > 255 || rc.colors[i].green > 255 || rc.colors[i].blue > 255)
			manycolors = true;
	}

	for(i = 0; i < rc.maxColors; i++)
	{
		if(manycolors)
		{
			rc.colors[i].red /= 256;
			rc.colors[i].green /= 256;
			rc.colors[i].blue /= 256;
		}
		rc.colors[i].color = RGB(rc.colors[i].red, rc.colors[i].green, rc.colors[i].blue);
	}

	ReadFont(ar, &rc.gameFont, &rc.gameFontSize);
	ReadFont(ar, &rc.msgFont, &rc.msgFontSize);

	ar >> rc.view_width;
	ar >> rc.view_height;

	PropDlg.m_nickname = rc.nickname;
	PropDlg.m_userhost = rc.realname;
	PropDlg.m_userhost += "@";
	PropDlg.m_userhost += rc.hostname;
	PropDlg.m_serveraddress = rc.servername;
	PropDlg.m_fps.Format("Running at %d fps", rc.fps);
	PropDlg.m_recorddate = rc.recorddate;
}

char *CXPreplayDoc::ReadString(CArchive& ar)
{
	unsigned short len;
	char *string;

	ar >> len;
	string = new char[len + 1];

	string[len] = '\0';
	for(int i = 0; i < len; i++)
	{
		ar >> string[i];
	}

	return string;
}

void CXPreplayDoc::ReadFont(CArchive& ar, char **fontname, int *fontsize)
{
	unsigned short len;
	char *string;
	char *pointer;
	int i;

	ar >> len;
	string = new char[len + 1];

	string[len] = '\0';
	for(i = 0; i < len; i++)
	{
		ar >> string[i];
	}

	for(i = 1; i < len + 1; i++)
	{
		if(string[i] == '-')
			break;
	}
	pointer = string + ++i;

	for(; i < len + 1; i++)
	{
		if(string[i] == '-')
			break;
	}
	string[i] = '\0';

	*fontname = new char[strlen(pointer) + 1];
	strcpy(*fontname, pointer);

	int count = 0;
	for(; i < len + 1; i++)
	{
		if(string[i] == '-')
			count++;

		if(count == 4)
			break;
	}
	i++;

	char	tmp[5];
	int j = 0;
	for(; i < len + 1; i++, j++)
	{
		if(string[i] == '-')
			break;

		tmp[j] = string[i];
	}
	tmp[j] = '\0';
	*fontsize = atoi(tmp);

	delete[] string;
}

void CXPreplayDoc::ReadNextFrame(CArchive& ar)
{
	char			c;
	struct frame	*f = NULL;

	if (rc.eof)
	{
		return;
	}

	try{ar >> c;}
	catch(CArchiveException *e)
	{
		rc.eof = true;
		return;
	}

	if (c != RC_NEWFRAME)
	{
		CString errormessage;

		errormessage.Format("Corrupt record file, next frame expected, not %d.  Truncating.", c);
		MessageBox(NULL, (LPCTSTR)errormessage, "Read error", MB_OK | MB_ICONHAND);

		rc.eof = true;
		return;
	}

	f = new struct frame;

	ar.Flush();
	f->filepos = ar.GetFile()->GetPosition() - 1;	// starting at RC_NEWFRAME
	ar >> f->width;
	ar >> f->height;
	f->shapes = NULL;
	f->next = NULL;
	f->prev = NULL;
	f->number = frame_count;

	ReadFrameData(ar, f);

	if (rc.tail == NULL)
	{
		f->next = NULL;
		f->prev = NULL;
		rc.tail = rc.head = rc.cur = f;
	}
	else
	{
		f->prev = rc.tail;
		f->next = NULL;
		rc.tail->next = f;
		rc.tail = f;
	}

	frame_count++;
}

void CXPreplayDoc::ReadFrameData(CArchive &ar, struct frame *f)
{
	char			c = 0, prev_c;
	struct rShape	*shp = NULL,
					*shphead = NULL,
					*newshp;
	CPoint		*xpp;
	CRect		*xrp;
	rArc		*xap;
	rSegment		*xsp;
	char		*cp;
	bool			done = false;
	unsigned char	byte;
	unsigned short	ushort;
	unsigned char	width, height;	// for the Arcs
	short	angle1, angle2;	// for the Arcs

    while (!done)
	{
		prev_c = c;
		try{ar >> c;}
		catch(CArchiveException *e)
		{
			MessageBox(NULL, "Premature End-Of-File encountered. Truncating.", "Read error", MB_OK | MB_ICONWARNING);
			done = true;
			rc.eof = true;
		    continue;
		}

		switch(c)
		{
		case RC_ENDFRAME:
			done = true;
			continue;

		case RC_DRAWARC:
		case RC_DRAWLINES:
		case RC_DRAWLINE:
		case RC_DRAWRECTANGLE:
		case RC_DRAWSTRING:
		case RC_FILLARC:
		case RC_FILLPOLYGON:
		case RC_PAINTITEMSYMBOL:
		case RC_FILLRECTANGLE:
		case RC_FILLRECTANGLES:
		case RC_DRAWARCS:
		case RC_DRAWSEGMENTS:
		case RC_DAMAGED:
			newshp = new struct rShape;
			newshp->next = NULL;
			newshp->type = 0;

			if (shp == NULL)
			{
				shp = newshp;
				shphead = shp;
			}
			else
			{
				shp->next = newshp;
				shp = shp->next;
			}
			shp->type = c;
			shp->gc = ReadGCValues(ar);

			switch (c)
			{
			case RC_DRAWARC:
			case RC_FILLARC:
				ar >> shp->shape.arc.x1;
				ar >> shp->shape.arc.y1;
				SetMaxX(shp->shape.arc.x1);
				SetMaxY(shp->shape.arc.y1);

				ar >> width;
				ar >> height;
				shp->shape.arc.x2 = shp->shape.arc.x1 + width;
				shp->shape.arc.y2 = shp->shape.arc.y1 + height;
				SetMaxX(shp->shape.arc.x2);
				SetMaxY(shp->shape.arc.y2);

				ar >> angle1;
				ar >> angle2;
				shp->shape.arc.x3 = shp->shape.arc.x1 + width/2 + (int)(100*cos((double)angle1 / 3666.93));
				shp->shape.arc.y3 = shp->shape.arc.y1 + height/2 + (int)(100*sin((double)angle1 / 3666.93));
				shp->shape.arc.x4 = shp->shape.arc.x1 + width/2 + (int)(100*cos((angle1 + angle2 + 64) / 3666.93));
				shp->shape.arc.y4 = shp->shape.arc.y1 + height/2 + (int)(100*sin((angle1 + angle2 + 64) / 3666.93));
				break;

			case RC_DRAWLINES:
				ar >> ushort;
				shp->shape.lines.npoints = ushort;
				xpp = new CPoint[ushort];
				shp->shape.lines.points = xpp;
				while (ushort--)
				{
					short x, y;
					ar >> x;
					ar >> y;

					SetMaxX(x);
					SetMaxY(y);
					xpp->x = x;
					xpp->y = y;
					xpp++;
				}
				ar >> byte;
				shp->shape.lines.mode = byte;
				if(byte)
				{
					for(int i = 1; i < shp->shape.lines.npoints; i++)
					{
						shp->shape.lines.points[i] += shp->shape.lines.points[i-1];
					}
				}
				break;

			case RC_DRAWLINE:
				ar >> shp->shape.line.x1;
				ar >> shp->shape.line.y1;
				ar >> shp->shape.line.x2;
				ar >> shp->shape.line.y2;
				SetMaxX(shp->shape.line.x1);
				SetMaxY(shp->shape.line.y1);
				SetMaxX(shp->shape.line.x2);
				SetMaxY(shp->shape.line.y2);
				break;

			case RC_DRAWRECTANGLE:
			case RC_FILLRECTANGLE:
				ar >> shp->shape.rectangle.x;
				ar >> shp->shape.rectangle.y;
				SetMaxX(shp->shape.rectangle.x);
				SetMaxY(shp->shape.rectangle.y);
				ar >> byte;
				shp->shape.rectangle.width = (unsigned short)byte;
				ar >> byte;
				shp->shape.rectangle.height = (unsigned short)byte;
				SetMaxX(shp->shape.rectangle.x + shp->shape.rectangle.width);
				SetMaxY(shp->shape.rectangle.y + shp->shape.rectangle.height);
				break;

			case RC_DRAWSTRING:
				ar >> shp->shape.string.x;
				ar >> shp->shape.string.y;
				ar >> shp->shape.string.font;

				ar >> ushort;
				shp->shape.string.length = ushort;
				cp = new char[ushort + 1];
				shp->shape.string.string = cp;
				while (ushort--)
				{
					ar >> byte;
					*cp++ = byte;
				}
				*cp = '\0';

				break;

			case RC_FILLPOLYGON:
				ar >> ushort;
				shp->shape.polygon.npoints = ushort;
				xpp = new CPoint[ushort];
				shp->shape.polygon.points = xpp;
				while (ushort--)
				{
					short x, y;
					ar >> x;
					ar >> y;
					SetMaxX(x);
					SetMaxY(y);

					xpp->x = x;
					xpp->y = y;
					xpp++;
				}
				ar >> shp->shape.polygon.shape;
				ar >> shp->shape.polygon.mode;
				break;

			case RC_PAINTITEMSYMBOL:
				ar >> shp->shape.symbol.type;
				ar >> shp->shape.symbol.x;
				ar >> shp->shape.symbol.y;
				SetMaxX(shp->shape.symbol.x);
				SetMaxY(shp->shape.symbol.y);
				break;

			case RC_FILLRECTANGLES:
				ar >> ushort;
				shp->shape.rectangles.nrectangles = ushort;
				xrp = new CRect[ushort+1];
				shp->shape.rectangles.rectangles = xrp;
				while (ushort--)
				{
					short x, y;
					unsigned short width, height;
					ar >> x;
					ar >> y;
					SetMaxX(x);
					SetMaxY(y);
					ar >> byte;
					width = (unsigned short)byte;
					ar >> byte;
					height = (unsigned short)byte;

					xrp->left = x;
					xrp->top = y;
					xrp->right = x + width;
					xrp->bottom = y + height;
					SetMaxX(xrp->right);
					SetMaxY(xrp->bottom);
					xrp++;
				}
				break;

			case RC_DRAWARCS:
				ar >> ushort;
				shp->shape.arcs.narcs = ushort;
				xap = new rArc[ushort];
				shp->shape.arcs.arcs = xap;
				while (ushort--)
				{
					ar >> xap->x1;
					ar >> xap->y1;
					SetMaxX(xap->x1);
					SetMaxY(xap->y1);

					ar >> width;
					ar >> height;
					xap->x2 = xap->x1 + width;
					xap->y2 = xap->y1 + height;
					SetMaxX(xap->x2);
					SetMaxY(xap->y2);

					ar >> angle1;
					ar >> angle2;
					if(angle2 == 19200)
					{
						xap->x3 = xap->x1 + width/2 + (int)(100*cos((angle1 + angle2 + 64) / 3666.93));
						xap->y3 = xap->y1 + height/2 + (int)(100*sin((angle1 + angle2 + 64) / 3666.93));
						xap->x4 = xap->x1 + width/2 + (int)(100*cos((double)angle1 / 3666.93));
						xap->y4 = xap->y1 + height/2 + (int)(100*sin((double)angle1 / 3666.93));
					}
					else
					{
						xap->x3 = xap->x1 + width/2 + (int)(100*cos((double)angle1 / 3666.93));
						xap->y3 = xap->y1 + height/2 + (int)(100*sin((double)angle1 / 3666.93));
						xap->x4 = xap->x1 + width/2 + (int)(100*cos((angle1 + angle2 + 64) / 3666.93));
						xap->y4 = xap->y1 + height/2 + (int)(100*sin((angle1 + angle2 + 64) / 3666.93));
					}
					xap++;
				}
				break;

			case RC_DRAWSEGMENTS:
				ar >> ushort;
				shp->shape.segments.nsegments = ushort;
				xsp = new rSegment[ushort+1];
				shp->shape.segments.segments = xsp;
				while (ushort--)
				{
					ar >> xsp->x1;
					ar >> xsp->y1;
					ar >> xsp->x2;
					ar >> xsp->y2;
					SetMaxX(xsp->x1);
					SetMaxY(xsp->y1);
					SetMaxX(xsp->x2);
					SetMaxY(xsp->y2);
					xsp++;
				}
				break;

			case RC_DAMAGED:
				ar >> shp->shape.damage.damaged;
				break;
			}

			break;

		default:
			CString errormessage;

			errormessage.Format("Unknown shape type %d (previous = %d) when reading frame %d.\nTruncating...", c, prev_c, frame_count); //, frames_in_core);
			MessageBox(NULL, (LPCTSTR)errormessage, "Read error", MB_OK | MB_ICONEXCLAMATION);

			done = true;
			continue;
		}

    }

    if (c != RC_ENDFRAME)
	{
		return;
    }

    f->shapes = shphead;
}

CXPreplayDoc::miniGC *CXPreplayDoc::ReadGCValues(CArchive &ar)
{
	char			c;
	unsigned short	input_mask;
	unsigned char	byte;
	CXPreplayDoc::miniGC	*gc;
	static bool		dashes = false;
	static int		prev_width = 1;
	static int		prev_color = 1;

	ar >> c;

	if (c == RC_NOGC)
	{
		return NULL;
	}
	else if (c != RC_GC)
	{
		CString errormessage;

		errormessage.Format("GC expected, not %d", c);
		MessageBox(NULL, (LPCTSTR)errormessage, "Read error", MB_OK | MB_ICONHAND);
		return NULL;
	}
	else
	{
		gc = new CXPreplayDoc::miniGC;
		gc->color = -1;
		gc->width = prev_width;
		ar >> byte;
		input_mask = (short)byte;

		if (input_mask & RC_GC_B2)
		{
			ar >> byte;
			input_mask |= (byte << 8);
		}

		if (input_mask & RC_GC_FG)
		{
			ar >> byte;
			gc->color = byte;
			prev_color = byte;
		}
		if (input_mask & RC_GC_BG)
		{
			ar >> byte;		// skip
		}
		if (input_mask & RC_GC_LW)
		{
			ar >> byte;
			gc->width = byte;
			prev_width = byte;
		}
		if (input_mask & RC_GC_LS)
		{
			ar >> byte;

			if(byte == 1)
				dashes = true;

			if(byte == 0)
				dashes = false;
		}
		gc->dashes = dashes;

		if (input_mask & RC_GC_DO)
		{
			ar >> byte;		// skip
		}
		if (input_mask & RC_GC_FU)
		{
			ar >> byte;		// skip
		}
		if (input_mask & RC_GC_DA)
		{
			ar >> byte;
			int num_dashes = (int)byte;

			if (num_dashes != 0)
			{
				for (int i = 0; i < num_dashes; i++)
				{
					ar >> byte;		// skip
				}
			}
		}
		if (input_mask & RC_GC_B2)
		{
			if (input_mask & RC_GC_FS)
			{
				ar >> byte;		// skip
			}
			if (input_mask & RC_GC_XO)
			{
				long temp;
				ar >> temp;		// skip
			}
			if (input_mask & RC_GC_YO)
			{
				long temp;
				ar >> temp;		// skip
			}
			if (input_mask & RC_GC_TI)
			{
				SkipTile(ar);	// Maybe for a later version. :-)
			}
		}
	}

	return gc;
}

void CXPreplayDoc::SkipTile(CArchive &ar)
{
	unsigned short			width, height;
	unsigned short			x, y;
	unsigned char		ch;
	unsigned char		tile_id;
	static short	count = 0;

	ar >> ch;
	ar >> tile_id;

	if (ch == RC_TILE)
	{
		return;
	}

	if (ch != RC_NEW_TILE)
	{
		CString errormessage;

		errormessage.Format("Error: New tile expected, not found! (%d) (%d)\n", ch, count);
		MessageBox(NULL, (LPCTSTR)errormessage, "Read error", MB_OK | MB_ICONEXCLAMATION);
		rc.eof = true;
		return;
	}

	ar >> width;
	ar >> height;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			ar >> ch;
		}
	}
}

inline void CXPreplayDoc::SetMaxX(int x)
{
	if(x > max.x)
		max.x = x;
}

inline void CXPreplayDoc::SetMaxY(int y)
{
	if(y > max.y)
		max.y = y;
}

void CXPreplayDoc::OnUpdateFileProperties(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(docOpened);
}

void CXPreplayDoc::OnFileProperties() 
{
	PropDlg.DoModal();
}

void CXPreplayDoc::FreeFrames()
{
	struct frame	*frm, *oldfrm;

	frm = rc.head;

	while(frm)
	{
		FreeShapes(frm);
		oldfrm = frm;
		frm = frm->next;
		delete oldfrm;
	}
}

void CXPreplayDoc::FreeShapes(struct frame *f)
{
	struct rShape	*shp, *oldshp;

	shp = f->shapes;
	while(shp)
	{
			switch(shp->type)
		{
		case RC_DRAWLINES:
			delete[] shp->shape.lines.points;
			break;
		case RC_DRAWSTRING:
			delete[] shp->shape.string.string;
			break;
		case RC_FILLPOLYGON:
			delete[] shp->shape.polygon.points;
			break;
		case RC_FILLRECTANGLES:
			delete[] shp->shape.rectangles.rectangles;
			break;
		case RC_DRAWARCS:
			delete[] shp->shape.arcs.arcs;
			break;
		case RC_DRAWSEGMENTS:
			delete[] shp->shape.segments.segments;
			break;
		}
		delete shp->gc;
		oldshp = shp;
		shp = shp->next;
		delete oldshp;
	}
}
void CXPreplayDoc::FreeRC(void)
{
	delete[] rc.nickname;
	delete[] rc.realname;
	delete[] rc.hostname;
	delete[] rc.servername;
	delete[] rc.recorddate;
	delete[] rc.colors;
	delete[] rc.gameFont;
	delete[] rc.msgFont;
}

void CXPreplayDoc::OnUpdatePage(CCmdUI *pCmdUI)
{
	if(docOpened)
	{
		CString	panetext;
		panetext.Format("frame %d of %d", rc.cur->number + 1, frame_count);
		pCmdUI->SetText(panetext);
	}
}
