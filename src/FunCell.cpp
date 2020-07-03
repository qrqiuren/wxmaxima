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
  This file defines the class FunCell

  FunCell is the Cell type that represents functions that don't require special handling.
 */

#include "FunCell.h"

FunCell::FunCell(GroupCell *parent, Configuration **config, InitCells init)
    : Cell(parent, config)
{
  if (init.init)
  {
    SetName(nullptr);
    SetArg(nullptr);
  }
}

FunCell::FunCell(const FunCell &cell)
    : FunCell(cell.m_group, cell.m_configuration, { false })
{
  CopyCommonData(cell);
  SetName(CopyList(cell.m_nameCell.get()));
  SetArg(CopyList(cell.m_argCell.get()));
}

void FunCell::SetName(Cell *name)
{
  m_nameCell.reset(InvalidCellOr(name));
  m_nameCell->SetStyle(TS_FUNCTION);
}

void FunCell::SetArg(Cell *arg)
{
  m_argCell.reset(InvalidCellOr(arg));
}

void FunCell::RecalculateWidths(int fontsize)
{
  if(!NeedsRecalculation(fontsize))
    return;

  m_argCell->RecalculateWidthsList(fontsize);
  m_nameCell->RecalculateWidthsList(fontsize);
  m_width = m_nameCell->GetFullWidth() + m_argCell->GetFullWidth() - Scale_Px(1);

  if(m_isBrokenIntoLines)
    m_width = 0;
  Cell::RecalculateWidths(fontsize);
}

void FunCell::RecalculateHeight(int fontsize)
{
  if(!NeedsRecalculation(fontsize))
    return;

  m_nameCell->RecalculateHeightList(fontsize);
  m_argCell->RecalculateHeightList(fontsize);
  if(!m_isBrokenIntoLines)
  {
    m_center = wxMax(m_nameCell->GetCenterList(), m_argCell->GetCenterList());
    m_height = m_center + wxMax(m_nameCell->GetMaxDrop(), m_argCell->GetMaxDrop());
  }
  else
    m_height = 0;
  Cell::RecalculateHeight(fontsize);
}

void FunCell::Draw(wxPoint point)
{
  Cell::Draw(point);
  if (DrawThisCell(point))
  {

    wxPoint name(point), arg(point);
    m_nameCell->DrawList(name);

    arg.x += m_nameCell->GetFullWidth();
    m_argCell->DrawList(arg);
  }
}

wxString FunCell::ToString()
{
  if (m_isBrokenIntoLines)
    return wxEmptyString;
  if (m_altCopyText != wxEmptyString)
    return m_altCopyText;
  return m_nameCell->ListToString() + m_argCell->ListToString();
}

wxString FunCell::ToMatlab()
{
  if (m_isBrokenIntoLines)
	return wxEmptyString;
  if (m_altCopyText != wxEmptyString)
	return m_altCopyText + Cell::ListToMatlab();
  wxString s = m_nameCell->ListToMatlab() + m_argCell->ListToMatlab();
  return s;
}

wxString FunCell::ToTeX()
{
  if (m_isBrokenIntoLines)
    return wxEmptyString;

  wxString s;

  if (
    (m_nameCell->ToString() == wxT("sin")) ||
    (m_nameCell->ToString() == wxT("cos")) ||
    (m_nameCell->ToString() == wxT("cosh")) ||
    (m_nameCell->ToString() == wxT("sinh")) ||
    (m_nameCell->ToString() == wxT("log")) ||
    (m_nameCell->ToString() == wxT("cot")) ||
    (m_nameCell->ToString() == wxT("sec")) ||
    (m_nameCell->ToString() == wxT("csc")) ||
    (m_nameCell->ToString() == wxT("tan"))
    )
    s = wxT("\\") + m_nameCell->ToString() + wxT("{") + m_argCell->ListToTeX() + wxT("}");
  else
    s = m_nameCell->ListToTeX() + m_argCell->ListToTeX();
  
  return s;
}

wxString FunCell::ToXML()
{
//  if (m_isBrokenIntoLines)
//    return wxEmptyString;
  wxString flags;
  if (m_forceBreakLine)
    flags += wxT(" breakline=\"true\"");
  return wxT("<fn") + flags + wxT("><r>") + m_nameCell->ListToXML() + wxT("</r>") +
         m_argCell->ListToXML() + wxT("</fn>");
}

wxString FunCell::ToMathML()
{
//  if (m_isBrokenIntoLines)
//    return wxEmptyString;
  return wxT("<mrow>") + m_nameCell->ListToMathML() +
         wxT("<mo>&#x2061;</mo>") + m_argCell->ListToMathML() + wxT("</mrow>\n");
}

wxString FunCell::ToOMML()
{
  return m_nameCell->ListToOMML() +
         m_argCell->ListToOMML();
}

bool FunCell::BreakUp()
{
  if (m_isBrokenIntoLines)
    return false;

  m_isBrokenIntoLines = true;
  m_nameCell->last()->SetNextToDraw(m_argCell);
  m_argCell->last()->SetNextToDraw(Cell::GetNextToDrawImpl());
  m_width = 0;
  ResetData();
  return true;
}

void FunCell::SetNextToDrawImpl(Cell *next) {
  m_argCell->last()->SetNextToDraw(next);
}

Cell *FunCell::GetNextToDrawImpl() const { return m_nameCell.get(); }
