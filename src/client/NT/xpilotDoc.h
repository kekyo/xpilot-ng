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

// xpilotDoc.h : interface of the CXpilotDoc class
//
/////////////////////////////////////////////////////////////////////////////

/*
 * Windows MFC deals with a Document/View model.
 * XPilot does not.  This module is required but completely ignored.
 */
class CXpilotDoc:public CDocument {
  protected:			// create from serialization only
    CXpilotDoc();
    DECLARE_DYNCREATE(CXpilotDoc)
// Attributes
  public:

// Operations
  public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CXpilotDoc)
  public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive & ar);
    //}}AFX_VIRTUAL

// Implementation
  public:
     virtual ~ CXpilotDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext & dc) const;
#endif

  protected:

// Generated message map functions
  protected:
    //{{AFX_MSG(CXpilotDoc)
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
     DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
