﻿// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2007-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
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
  This file defines the class SubSupCell

  SubSubCell is the Cell type that represents a math element with subscript and
  superscript.
 */

#include "SubSupCell.h"
#include "TextCell.h"
#include <wx/config.h>
#include "wx/config.h"
#include "VisiblyInvalidCell.h"

#define SUBSUP_DEC 3

SubSupCell::SubSupCell(GroupCell *parent, Configuration **config) :
  Cell(parent, config),
  m_baseCell(std::make_unique<VisiblyInvalidCell>(parent,config))
{
  InitBitFields();
}

SubSupCell::SubSupCell(const SubSupCell &cell):
    SubSupCell(cell.m_group, cell.m_configuration)
{
  CopyCommonData(cell);
  m_altCopyText = cell.m_altCopyText;
  if(cell.m_baseCell)
    SetBase(cell.m_baseCell->CopyList());
  if(cell.m_postSubCell)
    SetIndex(cell.m_postSubCell->CopyList());
  if(cell.m_postSupCell)
    SetExponent(cell.m_postSupCell->CopyList());
  if(cell.m_preSubCell)
    SetPreSub(cell.m_preSubCell->CopyList());
  if(cell.m_preSupCell)
    SetPreSup(cell.m_preSupCell->CopyList());
}

std::unique_ptr<Cell> SubSupCell::Copy() const
{
  return std::make_unique<SubSupCell>(*this);
}

static void RemoveCell(std::vector<CellPtr<Cell>> &cells, std::unique_ptr<Cell> const &cell)
{
  cells.erase(
    std::remove(cells.begin(), cells.end(), cell.get()), cells.end());
}

void SubSupCell::SetPreSup(std::unique_ptr<Cell> &&index)
{
  if (!index)
    return;
  RemoveCell(m_scriptCells, m_preSupCell);
  m_preSupCell = std::move(index);
  m_scriptCells.emplace_back(m_preSupCell);
}

void SubSupCell::SetPreSub(std::unique_ptr<Cell> &&index)
{
  if (!index)
    return;
  RemoveCell(m_scriptCells, m_preSubCell);
  m_preSubCell = std::move(index);
  m_scriptCells.emplace_back(m_preSubCell);
}

void SubSupCell::SetPostSup(std::unique_ptr<Cell> &&index)
{
  if (!index)
    return;
  RemoveCell(m_scriptCells, m_postSupCell);
  m_postSupCell = std::move(index);
  m_scriptCells.emplace_back(m_postSupCell);
}

void SubSupCell::SetPostSub(std::unique_ptr<Cell> &&index)
{
  if (!index)
    return;
  RemoveCell(m_scriptCells, m_postSubCell);
  m_postSubCell = std::move(index);
  m_scriptCells.emplace_back(m_postSubCell);
}

void SubSupCell::SetIndex(std::unique_ptr<Cell> &&index)
{
  if (!index)
    return;
  RemoveCell(m_scriptCells, m_postSubCell);
  m_postSubCell = std::move(index);
}

void SubSupCell::SetBase(std::unique_ptr<Cell> &&base)
{
  if (!base)
    return;
  m_baseCell = std::move(base);
}

void SubSupCell::SetExponent(std::unique_ptr<Cell> &&expt)
{
  if (!expt)
    return;
  RemoveCell(m_scriptCells, m_postSupCell);
  m_postSupCell = std::move(expt);
}

void SubSupCell::Recalculate(AFontSize const fontsize)
{
  AFontSize const smallerFontSize{ MC_MIN_SIZE, fontsize - SUBSUP_DEC };

  m_baseCell->RecalculateList(fontsize);
  if(m_postSubCell)    
    m_postSubCell->RecalculateList(smallerFontSize);
  if(m_postSupCell)    
    m_postSupCell->RecalculateList(smallerFontSize);
  if(m_preSubCell)    
    m_preSubCell->RecalculateList(smallerFontSize);
  if(m_preSupCell)    
    m_preSupCell->RecalculateList(smallerFontSize);

  int preWidth = 0;
  int postWidth = 0;
  int subHeight = 0;
  int supHeight = 0;
  if(m_postSubCell)
  {
    m_postSubCell->RecalculateList(smallerFontSize);
    postWidth = m_postSubCell->GetFullWidth();
    subHeight = m_postSubCell->GetHeightList();
  }
  if(m_postSupCell)
  {
    m_postSupCell->RecalculateList(smallerFontSize);
    postWidth = wxMax(postWidth, m_postSupCell->GetFullWidth());
    supHeight = m_postSupCell->GetHeightList();
  }
  if(m_preSubCell)
  {
    m_preSubCell->RecalculateList(smallerFontSize);
    preWidth = m_preSubCell->GetFullWidth();
    subHeight = wxMax(subHeight, m_preSubCell->GetHeightList());
  }
  if(m_preSupCell)
  {
    m_preSupCell->RecalculateList(smallerFontSize);
    preWidth = wxMax(preWidth, m_preSupCell->GetFullWidth());
    supHeight = wxMax(subHeight, m_preSupCell->GetHeightList());
  }

  m_width = preWidth + m_baseCell->GetFullWidth() + postWidth;
  
  m_height = m_baseCell->GetHeightList() + subHeight + supHeight -
             2 * Scale_Px(.8 * fontsize + MC_EXP_INDENT);

  m_center = supHeight +
    m_baseCell->GetCenterList() -
    Scale_Px(.8 * fontsize + MC_EXP_INDENT);
  Cell::Recalculate(fontsize);
}

void SubSupCell::Draw(wxPoint point)
{
  Cell::Draw(point);
  if (DrawThisCell(point))
  {
    wxPoint in;

    int preWidth = 0;
    
    if(m_preSubCell)
      preWidth = m_preSubCell->GetFullWidth();
    if(m_preSupCell)
      preWidth = wxMax(preWidth, m_preSupCell->GetFullWidth());

    if(m_preSubCell)
    {
      wxPoint presub = point;
      presub.x += preWidth - m_preSubCell->GetFullWidth();
      presub.y += m_baseCell->GetMaxDrop() +
        m_preSubCell->GetCenterList() -
        Scale_Px(.8 * m_fontSize + MC_EXP_INDENT);
      m_preSubCell->DrawList(presub);
    }

    if(m_preSupCell)
    {
      wxPoint presup = point;
      presup.x += preWidth - m_preSupCell->GetFullWidth();
      presup.y -= m_baseCell->GetCenterList() + m_preSupCell->GetHeightList()
        - m_preSupCell->GetCenterList() -
        Scale_Px(.8 * m_fontSize + MC_EXP_INDENT);
      m_preSupCell->DrawList(presup);
    }

    point.x += preWidth;
    m_baseCell->DrawList(point);

    in.x = point.x + m_baseCell->GetFullWidth() - Scale_Px(2);
    if(m_postSubCell)
    {
      in.y = point.y + m_baseCell->GetMaxDrop() +
        m_postSubCell->GetCenterList() -
        Scale_Px(.8 * m_fontSize + MC_EXP_INDENT);
      m_postSubCell->DrawList(in);
    }
    if(m_postSupCell)
    {
      in.y = point.y - m_baseCell->GetCenterList() - m_postSupCell->GetHeightList()
        + m_postSupCell->GetCenterList() +
        Scale_Px(.8 * m_fontSize + MC_EXP_INDENT);
      m_postSupCell->DrawList(in);
    }
  }
}

wxString SubSupCell::ToString() const
{
  if (m_altCopyText != wxEmptyString)
    return m_altCopyText;

  wxString s;
  if (m_baseCell->IsCompound())
    s += "(" + m_baseCell->ListToString() + ")";
  else
    s += m_baseCell->ListToString();
  if (m_scriptCells.empty())
  {
    s += "[" + m_postSubCell->ListToString() + "]";
    s += "^";
    if (m_postSupCell->IsCompound())
      s += "(";
    s += m_postSupCell->ListToString();
    if (m_postSupCell->IsCompound())
      s += ")";
  }
  else
  {
    for (auto &cell : m_scriptCells)
      s += "[" + cell->ListToString() + "]";
  }
  return s;
}

wxString SubSupCell::ToMatlab() const
{
  wxString s;
  if (m_baseCell->IsCompound())
	s += "(" + m_baseCell->ListToMatlab() + ")";
  else
	s += m_baseCell->ListToMatlab();
  if (m_scriptCells.empty())
  {
    s += "[" + m_postSubCell->ListToMatlab() + "]";
    s += "^";
    if (m_postSupCell->IsCompound())
      s += "(";
    s += m_postSupCell->ListToMatlab();
    if (m_postSupCell->IsCompound())
      s += ")";
  }
  else
  {
    s += "[";
    bool first = false;
    
    for (auto &cell : m_scriptCells)
    {
      if(!first)
        s += ";";
      first = true;
      s += cell->ListToMatlab();
    }
    s += "]";
  }
  return s;
}

wxString SubSupCell::ToTeX() const
{
  wxConfigBase *config = wxConfig::Get();

  bool TeXExponentsAfterSubscript = false;

  config->Read("TeXExponentsAfterSubscript", &TeXExponentsAfterSubscript);

  wxString s;

  if (m_scriptCells.empty())
  {
    if (TeXExponentsAfterSubscript)
    {
      s = "{{{" + m_baseCell->ListToTeX() + "}";
      if(m_postSubCell)
        s += "_{" + m_postSubCell->ListToTeX() + "}";
      s += "}";
      if(m_postSupCell)
        s += "^{" + m_postSupCell->ListToTeX() + "}";
      s += "}";
    }
    else
    {
      s = "{{" + m_baseCell->ListToTeX() + "}";
      if(m_postSubCell)
        s +="_{" + m_postSubCell->ListToTeX() + "}";
      if(m_postSupCell)
        s += "^{" + m_postSupCell->ListToTeX() + "}";
      s += "}";
    }
  }
  else
  {
    if(m_preSupCell || m_preSubCell)
    {
      s = "{}";
      if(m_preSupCell)
        s += "^{" + m_preSupCell->ListToTeX() + "}";
      if(m_preSubCell)
        s += "^{" + m_preSubCell->ListToTeX() + "}";
    }
    s += "{" + m_baseCell->ListToTeX() + "}";
    if(m_postSupCell)
      s += "^{" + m_postSupCell->ListToTeX() + "}";
    if(m_postSubCell)
      s += "^{" + m_postSubCell->ListToTeX() + "}";
  }
  return s;
}

wxString SubSupCell::ToMathML() const
{
  wxString retval;
  if (m_scriptCells.empty())
  {
    retval = "<msubsup>" +
      m_baseCell->ListToMathML();
    if(m_postSubCell)
      retval += m_postSubCell->ListToMathML();
    else
      retval += "<mrow/>";
    if(m_postSupCell)
      m_postSupCell->ListToMathML();
    else
      retval += "<mrow/>";
    retval += "</msubsup>\n";
  }
  else
  {
    retval = "<mmultiscripts>" + m_baseCell->ListToMathML();
    if(m_postSupCell || m_postSubCell)
    {
      if(m_postSubCell)
        retval += "<mrow>" + m_postSubCell->ListToMathML() + "</mrow>";
      else
        retval += "<none/>";
      if(m_postSupCell)
        retval += "<mrow>" + m_postSupCell->ListToMathML() + "</mrow>";
      else
        retval += "<none/>";
    }
    if(m_preSupCell || m_preSubCell)
    {
      retval += "<mprescripts/>";
      if(m_preSubCell)
        retval += "<mrow>" + m_preSubCell->ListToMathML() + "</mrow>";
      else
        retval += "<none/>";
      if(m_preSupCell)
        retval += "<mrow>" + m_preSupCell->ListToMathML() + "</mrow>";
      else
        retval += "<none/>";
    }
    retval += "</mmultiscripts>\n";
  }
  return retval;
}
wxString SubSupCell::ToOMML() const
{
  wxString retval;
  if(m_preSupCell || m_preSubCell)
  {
    retval += "<m:sSubSup><m:e><m:r></m:r></m:e><m:sub>";
    if(m_preSubCell)
      retval += m_preSubCell->ListToOMML();
    else
      retval += "<m:r></m:r>";
    retval += "</m:sub><m:sup>";
    if(m_preSupCell)
      retval += m_preSupCell->ListToOMML();
    else
      retval += "<m:r></m:r>";
    retval += "</m:sup></m:sSubSup>\n";
  }
  retval += "<m:sSubSup><m:e>" + m_baseCell->ListToOMML() + "</m:e><m:sub>";
  if(m_postSubCell)
    retval += m_postSubCell->ListToOMML();
  else
    retval += "<m:r></m:r>";
  retval += "</m:sub><m:sup>";
  if(m_postSupCell)
    retval += m_postSupCell->ListToOMML();
  else
    retval += "<m:r></m:r>";
  retval += "</m:sup></m:sSubSup>\n";
  return retval;
}

wxString SubSupCell::ToXML() const
{
  wxString flags;
  if (m_forceBreakLine)
    flags += " breakline=\"true\"";

  if (m_altCopyText != wxEmptyString)
    flags += " altCopy=\"" + XMLescape(m_altCopyText) + "\"";

  wxString retval;
  if (m_scriptCells.empty())
  {
    retval = "<ie" + flags + "><r>" + m_baseCell->ListToXML()
      + "</r><r>";
    if(m_postSubCell)
      retval += m_postSubCell->ListToXML();
    retval += "</r><r>";
    if(m_postSupCell)
      retval += m_postSupCell->ListToXML();
    retval += "</r></ie>";
  }
  else
  {
    retval = "<ie" + flags + "><r>" + m_baseCell->ListToXML() + "</r>";
    if(m_preSupCell)
      retval += "<r pos=\"presup\">" + m_preSupCell->ListToXML() + "</r>";
    if(m_preSubCell)
      retval += "<r pos=\"presub\">" + m_preSubCell->ListToXML() + "</r>";
    if(m_postSupCell)
      retval += "<r pos=\"postsup\">" + m_postSupCell->ListToXML() + "</r>";
    if(m_postSubCell)
      retval += "<r pos=\"postsub\">" + m_postSubCell->ListToXML() + "</r>";
    retval += "</ie>";
  }
  return retval;
}
