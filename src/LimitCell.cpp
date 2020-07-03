// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//  SPDX-License-Identifier: GPL-2.0+

/*! \file
  This file defines the class LimitCell

  LimitCell is the Cell type that represents maxima's <code>limit()</code> command.
*/

#include "LimitCell.h"
#include "TextCell.h"

#define MIN_LIMIT_FONT_SIZE 8
#define LIMIT_FONT_SIZE_DECREASE 1

LimitCell::LimitCell(GroupCell *parent, Configuration **config, InitCells init) :
    Cell(parent, config),
    m_open(new TextCell(parent, config, "(")),
    m_comma(new TextCell(parent, config, ",")),
    m_close(new TextCell(parent, config, ")"))
{
  if (init.init)
  {
    SetBase(nullptr);
    SetUnder(nullptr);
    SetName(nullptr);
  }
  m_open->SetStyle(TS_FUNCTION);
  m_close->SetStyle(TS_FUNCTION);
  m_comma->SetStyle(TS_FUNCTION);
}

// cppcheck-suppress uninitMemberVar symbolName=LimitCell::m_open
// cppcheck-suppress uninitMemberVar symbolName=LimitCell::m_comma
// cppcheck-suppress uninitMemberVar symbolName=LimitCell::m_close
LimitCell::LimitCell(const LimitCell &cell)
    : LimitCell(cell.m_group, cell.m_configuration, { false })
{
  CopyCommonData(cell);
  SetBase(CopyList(cell.m_base.get()));
  SetUnder(CopyList(cell.m_under.get()));
  SetName(CopyList(cell.m_name.get()));
}

void LimitCell::SetName(Cell *name) { m_name.reset(InvalidCellOr(name)); }

void LimitCell::SetBase(Cell *base) { m_base.reset(InvalidCellOr(base)); }

void LimitCell::SetUnder(Cell *under) { m_under.reset(InvalidCellOr(under)); }

void LimitCell::RecalculateWidths(int fontsize)
{
  if(!NeedsRecalculation(fontsize))
    return;

  m_base->RecalculateWidthsList(fontsize);
  m_under->RecalculateWidthsList(wxMax(MIN_LIMIT_FONT_SIZE, fontsize - LIMIT_FONT_SIZE_DECREASE));
  m_name->RecalculateWidthsList(fontsize);
  m_open->RecalculateWidthsList(fontsize);
  m_comma->RecalculateWidthsList(fontsize);
  m_close->RecalculateWidthsList(fontsize);\
  if(!m_isBrokenIntoLines)
  {
    m_width = wxMax(m_name->GetFullWidth(), m_under->GetFullWidth())
      + m_base->GetFullWidth();
  }
  else
    m_width = 0;
  
  Cell::RecalculateWidths(fontsize);
}

void LimitCell::RecalculateHeight(int fontsize)
{
  if(!NeedsRecalculation(fontsize))
    return;

  m_under->RecalculateHeightList(wxMax(MIN_LIMIT_FONT_SIZE, fontsize - LIMIT_FONT_SIZE_DECREASE));
  m_name->RecalculateHeightList(fontsize);
  m_base->RecalculateHeightList(fontsize);
  m_open->RecalculateHeightList(fontsize);
  m_comma->RecalculateHeightList(fontsize);
  m_close->RecalculateHeightList(fontsize);
  
  if(!m_isBrokenIntoLines)
  {
    m_center = wxMax(m_base->GetCenterList(), m_name->GetCenterList());
    m_height = m_center + wxMax(m_name->GetMaxDrop() + m_under->GetHeightList(),
                                m_base->GetMaxDrop());
  }
  else
  {
    m_height = m_name->GetHeightList();
    m_center = m_name->GetCenterList();
  }
  Cell::RecalculateHeight(fontsize);
}

void LimitCell::Draw(wxPoint point)
{
  Cell::Draw(point);
  if (DrawThisCell(point))
  {   
    wxPoint base(point), under(point), name(point);

    name.x = point.x + wxMax(m_name->GetFullWidth(),
                           m_under->GetFullWidth()) / 2 -
             m_name->GetFullWidth() / 2;
    m_name->DrawList(name);

    under.x = point.x + wxMax(m_name->GetFullWidth(),
                            m_under->GetFullWidth()) / 2 -
              m_under->GetFullWidth() / 2;
      
    under.y = point.y + m_name->GetMaxDrop() + m_under->GetCenterList();
    m_under->DrawList(under);

    base.x += wxMax(m_name->GetFullWidth(),
                  m_under->GetFullWidth());
    m_base->DrawList(base);
  }
}

wxString LimitCell::ToString()
{
  wxString s(wxT("limit"));
  wxString under = m_under->ListToString();
  wxString base = m_base->ListToString();
  wxString var = under.SubString(0, under.Find(wxT("->")) - 1);
  wxString to = under.SubString(under.Find(wxT("->")) + 2,
								under.Length() - 1);
  if (to.Right(1) == wxT("+"))
	to = to.Left(to.Length() - 1) + wxT(",plus");
  if (to.Right(1) == wxT("-"))
	to = to.Left(to.Length() - 1) + wxT(",minus");

  s += wxT("(") + base + wxT(",") + var + wxT(",") + to + wxT(")");
  return s;
}

wxString LimitCell::ToMatlab()
{
  wxString s(wxT("limit"));
  wxString under = m_under->ListToMatlab();
  wxString base = m_base->ListToMatlab();
  wxString var = under.SubString(0, under.Find(wxT("->")) - 1);
  wxString to = under.SubString(under.Find(wxT("->")) + 2,
                                under.Length() - 1);
  if (to.Right(1) == wxT("+"))
    to = to.Left(to.Length() - 1) + wxT(",plus");
  if (to.Right(1) == wxT("-"))
    to = to.Left(to.Length() - 1) + wxT(",minus");

  s += wxT("(") + base + wxT(",") + var + wxT(",") + to + wxT(")");
  return s;
}

wxString LimitCell::ToTeX()
{
  wxString under = m_under->ListToTeX();
  wxString base = m_base->ListToTeX();
  int varEnd = under.Find(wxT("->"));
  int toStart = 0;
  if(varEnd == wxNOT_FOUND)
  {
    varEnd = under.Find(wxT("\\mbox{\\rightarrow }"));
    if(varEnd != wxNOT_FOUND)
    {
      toStart = varEnd + 19;
      varEnd -= 1;
    }
  }
  else
  {
    toStart = varEnd + 2;
    varEnd -= 1;
  }
                          
  wxString var = under.SubString(0, varEnd);
  wxString to = under.SubString(toStart,
                                under.Length() - 1);
  wxString s = wxT("\\lim_{") + var + wxT("\\to ") + to + wxT("}{") + base + wxT("}");
  return s;
}

wxString LimitCell::ToMathML()
{
  wxString base = m_base->ListToMathML();
  wxString from = m_under->ListToMathML();

  wxString retval;
  if (from.IsEmpty())
    retval = wxT("<mo>lim</mo>") + base;
  else
    retval = wxT("<munder><mo>lim</mo>") + from + wxT("</munder>\n");
  return (retval);
}

wxString LimitCell::ToXML()
{
  wxString flags;
  if (m_forceBreakLine)
    flags += wxT(" breakline=\"true\"");

  return _T("<lm") + flags + wxT("><r>") + m_name->ListToXML() + _T("</r><r>") +
         m_under->ListToXML() + _T("</r><r>") +
         m_base->ListToXML() + _T("</r></lm>");
}

wxString LimitCell::ToOMML()
{
  wxString under = m_under->ListToOMML();
  under.Replace(wxT("->"), wxT("\u2192"));

  return _T("<m:func><m:fName><m:limLow><m:e><m:r>lim</m:r></m:e><m:lim>") +
         under + _T("</m:lim></m:limLow></m:fName><m:e>") +
         m_base->ListToOMML() + _T("</m:e></m:func>");
}


bool LimitCell::BreakUp()
{
  if (m_isBrokenIntoLines)
    return false;

  m_isBrokenIntoLines = true;
  m_name->last()->SetNextToDraw(m_open);
  m_open->SetNextToDraw(m_base);
  m_base->last()->SetNextToDraw(m_comma);
  m_comma->SetNextToDraw(m_under);
  m_under->last()->SetNextToDraw(m_close);
  m_close->SetNextToDraw(Cell::GetNextToDrawImpl());
  ResetData();
  return true;
}

void LimitCell::SetNextToDrawImpl(Cell *next) { m_close->SetNextToDraw(next); }

Cell *LimitCell::GetNextToDrawImpl() const { return m_name.get(); }
