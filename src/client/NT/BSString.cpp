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

// BSString.cpp - BuckoSoft String Implementation
// Copyright © 1995-1997 BuckoSoft, Inc.
// This class wraps CString to give us some token parsing abilities

#include "stdafx.h"
#include "bsstring.h"
#include <ctype.h>

const char separators[] = " \t\n";

CBSString::CBSString()
{
    this->CString::CString();
}

CBSString::CBSString(const char *psz)
{
    this->CString::CString(psz);
}

CBSString::CBSString(const CBSString & stringSrc)
{
    this->CString::CString((const CString &) stringSrc);
}

CBSString::CBSString(const CString & stringSrc)
{
    this->CString::CString(stringSrc);
}

CBSString::CBSString(const CBSString * cbs)
{
    this->CString::CString((const char *) cbs);
}

CBSString CBSString::GetToken()
{
    CBSString ct;
    ct = SpanIncluding(separators);	// skip white space
    if (ct.GetLength())
	ct = Mid(ct.GetLength());
    else
	ct = (const char *) this[0];
    ct = ct.SpanExcluding(separators);
    return (ct);
}

CBSString CBSString::SkipSpace()
{
    CBSString ct;
    ct = SpanIncluding(separators);	// skip white space
    return (Right(GetLength() - ct.GetLength()));

}

CBSString CBSString::SkipToken()
{
    CBSString cs, ct;
    cs = SkipSpace();
    ct = cs.SpanExcluding(separators);
    cs = cs.Right(cs.GetLength() - ct.GetLength());
    return (cs);
}

int CBSString::MatchToken(const char *list[])
{
    int i = 0;
    while (*list != NULL) {
//              TRACE("MatchToken: <%s> <%s>\n", *list, (const char*)*this);
	if (!strcmp((const char *) *list, (const char *) *this))
//              if (this == *list)
	    return (i);
	list++;
	i++;
    }
    return (-1);
}

COLORREF CBSString::ParseColor()
{
    CBSString cs, ct;
    int r, g, b;

    ct = GetToken();
    r = atoi(ct);
    cs = SkipToken();
    ct = cs.GetToken();
    g = atoi(ct);
    cs = cs.SkipToken();
    ct = cs.GetToken();
    b = atoi(ct);
//    TRACE("ParseColor: r/g/b = %d/%d/%d\n", r, g, b);
    return (RGB(r, g, b));
}

POINT CBSString::ParsePoint()
{
    CBSString cs, ct;

    POINT p;
    ct = GetToken();
    p.x = atoi(ct);
    cs = SkipToken();
    ct = cs.GetToken();
    p.y = atoi(ct);
    return (p);
}

CRect CBSString::ParseRect()
{
    CRect r;
    CBSString cs, ct;
    ct = GetToken();
    r.left = atoi(ct);
    cs = SkipToken();
    ct = cs.GetToken();
    r.top = atoi(ct);
    cs = cs.SkipToken();
    ct = cs.GetToken();
    r.right = atoi(ct);
    cs = cs.SkipToken();
    ct = cs.GetToken();
    r.bottom = atoi(ct);
    return (r);
}
