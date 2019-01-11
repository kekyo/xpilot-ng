// XPreplayDoc.h : interface of the CXPreplayDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include "Properties.h"

#if !defined(AFX_XPREPLAYDOC_H__909E260A_85C6_11D5_A796_0000B48FE580__INCLUDED_)
#define AFX_XPREPLAYDOC_H__909E260A_85C6_11D5_A796_0000B48FE580__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CXPreplayDoc : public CDocument
{
protected: // create from serialization only
	CXPreplayDoc();
	DECLARE_DYNCREATE(CXPreplayDoc)

// Attributes
public:
	CString test;

	struct miniGC
	{
		int	color;
		bool dashes;
		int	width;
	};

	struct rArc
	{
		short	x1, y1,
				x2, y2,
				x3, y3,
				x4, y4;
	};

	struct rLines
	{
		CPoint		*points;
		unsigned short	npoints;
		short		mode;
	};

	struct rLine
	{
		short		x1;
		short		y1;
		short		x2;
		short		y2;
	};

	struct rString
	{
		short		x;
		short		y;
		unsigned char	font;
		short		length;
		char		*string;
	};

	struct rPolygon
	{
		CPoint		*points;
		unsigned short	npoints;
		unsigned char	shape;
		unsigned char	mode;
	};

	struct rSymbol
	{
		unsigned char	type;
		short		x;
		short		y;
	};

	struct rRectangle
	{
		short		x;
		short		y;
		unsigned short	width;
		unsigned short	height;
	};

	struct rRectangles
	{
		unsigned short	nrectangles;
		CRect		*rectangles;
	};

	struct rArcs
	{
		unsigned short	narcs;
		struct rArc		*arcs;
	};

	struct rSegment
	{
		short x1;
		short y1;
		short x2;
		short y2;
	};

	struct rSegments
	{
		unsigned short	nsegments;
		struct rSegment		*segments;
	};

	struct rDamage
	{
		unsigned char	damaged;
	};


	union shapep
	{
		struct rArc		arc;
		struct rLines	lines;
		struct rLine	line;
		struct rString	string;
		struct rPolygon	polygon;
		struct rSymbol	symbol;
		struct rRectangle	rectangle;
		struct rRectangles	rectangles;
		struct rArcs	arcs;
		struct rSegments	segments;
		struct rDamage	damage;
	};

	struct rShape
	{
		struct rShape	*next;		/* to next shape on frame list */
		char		type;		/* which drawing call */
		union shapep	shape;		/* actual shape data */
		struct miniGC	*gc;
	};

	struct frame
	{
		struct frame	*next;		/* to next on frame list */
		struct frame	*prev;		/* to previous on frame list */
		DWORD		filepos;	/* position in record file */
		unsigned short	width;		/* width of view window */
		unsigned short	height;		/* height of view window */
		struct rShape	*shapes;	/* head of shape list */
		int			number;		/* frame sequence number */
	};

	struct colors
	{
		unsigned long	pixel;	// for XWindows when saving recording
		unsigned short	red;
		unsigned short	green;
		unsigned short	blue;
		COLORREF		color;
	};

	struct xprc
	{
		int			seekable;	/* only seek if file is regular */
		bool			eof;		/* if EOF encountered */
		int			majorversion;	/* major version of protocol */
		int			minorversion;	/* minor version of protocol */
		char		*nickname;	/* XPilot nick name of player */
		char		*realname;	/* login name of player */
		char		*hostname;	/* hostname of player */
		char		*servername;	/* hostname of server */
		int			fps;		/* frames per second of game */
		char		*recorddate;	/* date of game played */
		unsigned char	maxColors;	/* number of colors used */
		struct colors	*colors;	/* pointer to color info */
		unsigned long	*pixels;	/* pointer to my pixel values */
		struct frame	*head;		/* to first frame */
		struct frame	*tail;		/* to last frame read sofar */
		struct frame	*cur;		/* current frame drawn */
		char		*gameFont;	/* Font for game situations */
		char		*msgFont;	/* Font for messages */
		int			gameFontSize;
		int			msgFontSize;
		unsigned short	view_width;	/* initial width of viewing area */
		unsigned short	view_height;	/* initial height of viewing area */
	} rc;


public:
	short	docOpened;
	CPoint	max;
	int	frame_count;
	int	minSelection;
	int	maxSelection;

private:
	CProperties PropDlg;
	bool dashes;

	void ReadNextFrame(CArchive& ar);
	void ReadFrameData(CArchive &ar, struct frame *f);
	struct miniGC *ReadGCValues(CArchive &ar);
	void SkipTile(CArchive &ar);
	inline void SetMaxX(int x);
	inline void SetMaxY(int y);
	void ReadHeader(CArchive& ar);
	char *ReadString(CArchive& ar);
	void ReadFont(CArchive& ar, char **fontname, int *fontsize);
	void FreeFrames(void);
	void FreeShapes(struct frame *f);
	void FreeRC(void);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXPreplayDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CXPreplayDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CXPreplayDoc)
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileProperties(CCmdUI* pCmdUI);
	afx_msg void OnFileProperties();
	//}}AFX_MSG
	afx_msg void OnUpdatePage(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPREPLAYDOC_H__909E260A_85C6_11D5_A796_0000B48FE580__INCLUDED_)
