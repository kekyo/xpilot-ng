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

// bsstring definition file


class CBSString:public CString {
  public:
    CBSString();
    CBSString(const char *psz);
     CBSString(const CBSString & stringSrc);
     CBSString(const CString & stringSrc);
     CBSString(const CBSString * cbs);

    CBSString GetToken();
    CBSString SkipToken();
    CBSString SkipSpace();

    int MatchToken(const char *list[]);
    COLORREF ParseColor();
    POINT ParsePoint();
    CRect ParseRect();
};
