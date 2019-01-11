/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// xpilotDoc.cpp : implementation of the CXpilotDoc class
//

#include "stdafx.h"
#include "XPilotNT.h"

#include "xpilotDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXpilotDoc

/*
 * Windows MFC deals with a Document/View model.
 * XPilot does not.  This module is required but completely ignored.
 * Someday i'd like to completely get rid the MFC poo.
 */

IMPLEMENT_DYNCREATE(CXpilotDoc, CDocument)

    BEGIN_MESSAGE_MAP(CXpilotDoc, CDocument)
    //{{AFX_MSG_MAP(CXpilotDoc)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CXpilotDoc construction/destruction
    CXpilotDoc::CXpilotDoc()
{
    // TODO: add one-time construction code here

}

CXpilotDoc::~CXpilotDoc()
{
}

BOOL CXpilotDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
	return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CXpilotDoc serialization

void CXpilotDoc::Serialize(CArchive & ar)
{
    if (ar.IsStoring()) {
	// TODO: add storing code here
    } else {
	// TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CXpilotDoc diagnostics

#ifdef _DEBUG
void CXpilotDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CXpilotDoc::Dump(CDumpContext & dc) const
{
    CDocument::Dump(dc);
}
#endif				//_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CXpilotDoc commands
