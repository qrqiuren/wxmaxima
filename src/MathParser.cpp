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
  This file defines the class MathParser that reads wxmx data and math from Maxima.
*/

#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/sstream.h>
#include <wx/intl.h>

#include "MathParser.h"

#include "Version.h"
#include "ExptCell.h"
#include "SubCell.h"
#include "SqrtCell.h"
#include "LimitCell.h"
#include "MatrCell.h"
#include "ParenCell.h"
#include "AbsCell.h"
#include "ConjugateCell.h"
#include "AtCell.h"
#include "DiffCell.h"
#include "SumCell.h"
#include "IntCell.h"
#include "FunCell.h"
#include "ImgCell.h"
#include "SubSupCell.h"
#include "SlideShowCell.h"

/*! Calls a member function from a function pointer

  \todo Replace this by a C++17 construct when we switch to C++17
 */
template <class Obj, class Ptr, typename ...Args>
static auto CALL_MEMBER_FN(Obj &object, Ptr ptrToMember, Args &&...args)
{
  return (object.*ptrToMember)(std::forward<Args>(args)...);
}

wxXmlNode *MathParser::SkipWhitespaceNode(wxXmlNode *node)
{
  if (node)
  {
    // If this is a text node there is a chance that this is a whitespace we want to skip
    if (node->GetType() == wxXML_TEXT_NODE)
    {
      // This is a text node => Let's see if it is whitespace-only and skip it if it is.
      wxString contents = node->GetContent();
      contents.Trim();
      if (contents.Length() <= 1)
        node = node->GetNext();
    }
  }
  return node;
}

wxXmlNode *MathParser::GetNextTag(wxXmlNode *node)
{
  if (node)
    node = node->GetNext();
  return SkipWhitespaceNode(node);
}

MathParser::MathParser(Configuration **cfg, const wxString &zipfile)
{
  m_configuration = cfg;
  m_ParserStyle = MC_TYPE_DEFAULT;
  m_FracStyle = FracCell::FC_NORMAL;
  if(m_innerTags.empty())
  {
    m_innerTags[wxT("v")] = &MathParser::ParseVariableNameTag;
    m_innerTags[wxT("mi")] = &MathParser::ParseVariableNameTag;
    m_innerTags[wxT("mo")] = &MathParser::ParseOperatorNameTag;
    m_innerTags[wxT("t")] = &MathParser::ParseMiscTextTag;
    m_innerTags[wxT("n")] = &MathParser::ParseNumberTag;
    m_innerTags[wxT("mn")] = &MathParser::ParseNumberTag;
    m_innerTags[wxT("p")] = &MathParser::ParseParenTag;
    m_innerTags[wxT("f")] = &MathParser::ParseFracTag;
    m_innerTags[wxT("mfrac")] = &MathParser::ParseFracTag;
    m_innerTags[wxT("e")] = &MathParser::ParseSupTag;
    m_innerTags[wxT("msup")] = &MathParser::ParseSupTag;
    m_innerTags[wxT("i")] = &MathParser::ParseSubTag;
    m_innerTags[wxT("munder")] = &MathParser::ParseSubTag;
    m_innerTags[wxT("fn")] = &MathParser::ParseFunTag;
    m_innerTags[wxT("g")] = &MathParser::ParseGreekTag;
    m_innerTags[wxT("s")] = &MathParser::ParseSpecialConstantTag;
    m_innerTags[wxT("fnm")] = &MathParser::ParseFunctionNameTag;
    m_innerTags[wxT("q")] = &MathParser::ParseSqrtTag;
    m_innerTags[wxT("d")] = &MathParser::ParseDiffTag;
    m_innerTags[wxT("sm")] = &MathParser::ParseSumTag;
    m_innerTags[wxT("in")] = &MathParser::ParseIntTag;
    m_innerTags[wxT("mspace")] = &MathParser::ParseSpaceTag;
    m_innerTags[wxT("at")] = &MathParser::ParseAtTag;
    m_innerTags[wxT("a")] = &MathParser::ParseAbsTag;
    m_innerTags[wxT("cj")] = &MathParser::ParseConjugateTag;
    m_innerTags[wxT("ie")] = &MathParser::ParseSubSupTag;
    m_innerTags[wxT("mmultiscripts")] = &MathParser::ParseMmultiscriptsTag;
    m_innerTags[wxT("lm")] = &MathParser::ParseLimitTag;
    m_innerTags[wxT("r")] = &MathParser::ParseTagContents;
    m_innerTags[wxT("mrow")] = &MathParser::ParseTagContents;
    m_innerTags[wxT("tb")] = &MathParser::ParseTableTag;
    m_innerTags[wxT("mth")] = &MathParser::ParseMthTag;
    m_innerTags[wxT("line")] = &MathParser::ParseMthTag;
    m_innerTags[wxT("lbl")] = &MathParser::ParseOutputLabelTag;
    m_innerTags[wxT("st")] = &MathParser::ParseStringTag;
    m_innerTags[wxT("hl")] = &MathParser::ParseHighlightTag;
    m_innerTags[wxT("h")] = &MathParser::ParseHiddenOperatorTag;
    m_innerTags[wxT("img")] = &MathParser::ParseImageTag;
    m_innerTags[wxT("slide")] = &MathParser::ParseSlideshowTag;
    m_innerTags[wxT("editor")] = &MathParser::ParseEditorTag;
    m_innerTags[wxT("cell")] = &MathParser::ParseCellTag;
    m_innerTags[wxT("ascii")] = &MathParser::ParseCharCode;
  }
  if(m_groupTags.empty())
  {
    m_groupTags[wxT("code")] = &MathParser::GroupCellFromCodeTag;
    m_groupTags[wxT("image")] = &MathParser::GroupCellFromImageTag;
    m_groupTags[wxT("pagebreak")] = &MathParser::GroupCellFromPagebreakTag;
    m_groupTags[wxT("text")] = &MathParser::GroupCellFromTextTag;
    m_groupTags[wxT("title")] = &MathParser::GroupCellFromTitleTag;
    m_groupTags[wxT("section")] = &MathParser::GroupCellFromSectionTag;
    m_groupTags[wxT("subsection")] = &MathParser::GroupCellFromSubsectionTag;
    m_groupTags[wxT("subsubsection")] = &MathParser::GroupCellFromSubsubsectionTag;
    m_groupTags[wxT("heading5")] = &MathParser::GroupCellHeading5Tag;
    m_groupTags[wxT("heading6")] = &MathParser::GroupCellHeading6Tag;
  }
  m_highlight = false;
  if (zipfile.Length() > 0)
  {
    m_fileSystem = std::unique_ptr<wxFileSystem>(new wxFileSystem());
    m_fileSystem->ChangePathTo(zipfile + wxT("#zip:/"), true);
  }
}

MathParser::~MathParser()
{}

OwningCellPtr MathParser::ParseHiddenOperatorTag(wxXmlNode *node)
{
  auto retval = ParseText(node->GetChildren());
  retval->m_isHidableMultSign = true;
  return retval;
}

OwningCellPtr MathParser::ParseTagContents(wxXmlNode *node)
{
  if (node && node->GetChildren())
    return ParseTag(node->GetChildren(), true);
  return {};
}

OwningCellPtr MathParser::ParseHighlightTag(wxXmlNode *node)
{
  bool highlight = m_highlight;
  m_highlight = true;
  auto cell = ParseTag(node->GetChildren());
  m_highlight = highlight;
  return cell;
}

OwningCellPtr MathParser::ParseMiscTextTag(wxXmlNode *node)
{
  TextStyle style = TS_DEFAULT;
  if (node->GetAttribute(wxT("type")) == wxT("error"))
    style = TS_ERROR;
  if (node->GetAttribute(wxT("type")) == wxT("warning"))
    style = TS_WARNING;
  return ParseText(node->GetChildren(), style);
}

OwningCellPtr MathParser::ParseSlideshowTag(wxXmlNode *node)
{
  wxString gnuplotSources;
  wxString gnuplotData;
  bool del = node->GetAttribute(wxT("del"), wxT("false")) == wxT("true");
  node->GetAttribute(wxT("gnuplotSources"), &gnuplotSources);
  node->GetAttribute(wxT("gnuplotData"), &gnuplotData);
  auto slideShow = MakeOwned<SlideShow>(nullptr, m_configuration, m_fileSystem);
  wxString str(node->GetChildren()->GetContent());
  wxArrayString images;
  wxString framerate;
  if (node->GetAttribute(wxT("fr"), &framerate))
  {
    long fr;
    if (framerate.ToLong(&fr))
      slideShow->SetFrameRate(fr);
  }
  if (node->GetAttribute(wxT("frame"), &framerate))
  {
    long frame;
    if (framerate.ToLong(&frame))
      slideShow->SetDisplayedIndex(frame);
  }
  if (node->GetAttribute(wxT("running"), wxT("true")) == wxT("false"))
    slideShow->AnimationRunning(false);
  wxStringTokenizer imageFiles(str, wxT(";"));
  int numImgs = 0;
  while (imageFiles.HasMoreTokens())
  {
    wxString imageFile = imageFiles.GetNextToken();
    if (!imageFile.empty())
    {
      images.Add(imageFile);
      numImgs++;
    }
  }
  if (slideShow)
  {
    slideShow->LoadImages(images, del);
    wxStringTokenizer dataFiles(gnuplotData, wxT(";"));
    wxStringTokenizer gnuplotFiles(gnuplotSources, wxT(";"));
    for (int i=0; i<numImgs; i++)
    {
      if((dataFiles.HasMoreTokens()) && (gnuplotFiles.HasMoreTokens()))
      {
        slideShow->GnuplotSource(
          i,
          gnuplotFiles.GetNextToken(),
          dataFiles.GetNextToken(),
          m_fileSystem
          );
      }
    }
  }
  return slideShow;
}

OwningCellPtr MathParser::ParseImageTag(wxXmlNode *node)
{
  OwningPtr<ImgCell> imageCell;
  wxString filename(node->GetChildren()->GetContent());

  if (m_fileSystem) // loading from zip
    imageCell = MakeOwned<ImgCell>(nullptr, m_configuration, filename, m_fileSystem, false);
  else
  {
    if (node->GetAttribute(wxT("del"), wxT("yes")) != wxT("no"))
    {
      std::shared_ptr <wxFileSystem> noFS;
      if (wxImage::GetImageCount(filename) < 2)
        imageCell = MakeOwned<ImgCell>(nullptr, m_configuration, filename, noFS, true);
      else
        return MakeOwned<SlideShow>(nullptr, m_configuration, filename, true);
    }
    else
    {
      // This is the only case show_image() produces ergo this is the only
      // case we might get a local path
      if (
        (!wxFileExists(filename)) &&
        (wxFileExists((*m_configuration)->GetWorkingDirectory() + wxT("/") + filename))
        )
        filename = (*m_configuration)->GetWorkingDirectory() + wxT("/") + filename;
      std::shared_ptr <wxFileSystem> noFS;
      if (wxImage::GetImageCount(filename) < 2)
        imageCell = MakeOwned<ImgCell>(nullptr, m_configuration, filename, noFS, false);
      else
        return MakeOwned<SlideShow>(nullptr, m_configuration, filename, false);
    }
  }
  wxString gnuplotSource = node->GetAttribute(wxT("gnuplotsource"), wxEmptyString);
  wxString gnuplotData = node->GetAttribute(wxT("gnuplotdata"), wxEmptyString);

  if (!gnuplotSource.empty())
    imageCell->GnuplotSource(gnuplotSource, gnuplotData, m_fileSystem);

  if (node->GetAttribute(wxT("rect"), wxT("true")) == wxT("false"))
    imageCell->DrawRectangle(false);
  wxString sizeString;
  if ((sizeString = node->GetAttribute(wxT("maxWidth"), wxT("-1"))) != wxT("-1"))
  {
    double width;
    if (sizeString.ToDouble(&width))
      imageCell->SetMaxWidth(width);
  }
  if ((sizeString = node->GetAttribute(wxT("maxHeight"), wxT("-1"))) != wxT("-1"))
  {
    double height;
    if (sizeString.ToDouble(&height))
      imageCell->SetMaxHeight(height);
  }
  return imageCell;
}

OwningCellPtr MathParser::ParseOutputLabelTag(wxXmlNode *node)
{
  OwningCellPtr cell;
  wxString user_lbl = node->GetAttribute(wxT("userdefinedlabel"), m_userDefinedLabel);
  wxString userdefined = node->GetAttribute(wxT("userdefined"), wxT("no"));
  
  if (userdefined != wxT("yes"))
  {
    cell = ParseText(node->GetChildren(), TS_LABEL);
  }
  else
  {
    cell = ParseText(node->GetChildren(), TS_USERLABEL);
    
    // Backwards compatibility to 17.04/17.12:
    // If we cannot find the user-defined label's text but still know that there
    // is one it's value has been saved as "automatic label" instead.
    if (user_lbl.empty())
    {
      user_lbl = dynamic_cast<TextCell *>(cell.get())->GetValue();
      user_lbl = user_lbl.substr(1,user_lbl.Length() - 2);
    }
  }
  
  dynamic_cast<TextCell *>(cell.get())->SetUserDefinedLabel(user_lbl);
  cell->ForceBreakLine(true);
  return cell;
}


OwningCellPtr MathParser::ParseMthTag(wxXmlNode *node)
{
  OwningCellPtr cell = ParseTag(node->GetChildren());
  if (cell)
  {
    cell->ForceBreakLine(true);
    return cell;
  }
  return MakeOwned<TextCell>(nullptr, m_configuration, wxT(" "));
}

// ParseCellTag
// This function is responsible for creating
// a tree of groupcells when loading XML document.
// Any changes in GroupCell structure or methods
// has to be reflected here in order to ensure proper
// loading of WXMX files.
OwningCellPtr MathParser::ParseCellTag(wxXmlNode *node)
{
  // read hide status
  bool hide = (node->GetAttribute(wxT("hide"), wxT("false")) == wxT("true")) ? true : false;
  // read (group)cell type
  wxString type = node->GetAttribute(wxT("type"), wxT("text"));

  auto function = m_groupTags[type];
  if (!function)
    return {};

  auto group = CALL_MEMBER_FN(*this, function, node);
  
  wxXmlNode *children = node->GetChildren();
  children = SkipWhitespaceNode(children);
  while (children)
  {
    if (children->GetName() == wxT("editor"))
    {
      auto ed = ParseEditorTag(children);
      if (ed)
        group->SetEditableContent(ed->GetValue());
    }
    else if (children->GetName() == wxT("fold"))
    { // This GroupCell contains folded groupcells
      wxXmlNode *xmlcells = children->GetChildren();
      xmlcells = SkipWhitespaceNode(xmlcells);

      std::unique_ptr<GroupCell> tree;
      Cell *last = NULL;
      while (xmlcells)
      {
        std::unique_ptr<Cell> ownedCell(ParseTag(xmlcells, false));
        Cell *cell = ownedCell.get();

        if (!cell)
          continue;
        
        if (!tree) tree = static_unique_ptr_cast<GroupCell>(std::move(ownedCell));
        
        if (!last) last = cell;
        else
        {
          last->m_next = std::move(ownedCell);
          last->SetNextToDraw(cell);
          last->m_next->m_previous = last;
          
          last = last->m_next.get();
        }
        xmlcells = GetNextTag(xmlcells);
      }
      if (tree)
        group->HideTree(std::move(tree));
    }
    else if (children->GetName() == wxT("input"))
    {
      OwningCellPtr editor = ParseTag(children->GetChildren());
      if (!editor)
        editor = MakeOwned<EditorCell>(group.get(), m_configuration, _("Bug: Missing contents"));
      if (editor)
        group->SetEditableContent(editor->GetValue());
    }
    else
    {
      group->AppendOutput(HandleNullPointer(ParseTag(children)));
    }

    children = GetNextTag(children);
  }

  group->SetGroup(group.get());
  group->Hide(hide);
  return group;
}

OwningGroupPtr MathParser::GroupCellFromSubsectionTag(wxXmlNode *node)
{
  wxString sectioning_level = node->GetAttribute(wxT("sectioning_level"), wxT("0"));
  OwningGroupPtr group;  

  // We save subsubsections as subsections with a higher sectioning level:
  // This makes them backwards-compatible in the way that they are displayed
  // as subsections on old wxMaxima installations.
  // A sectioning level of the value 0 means that the file is too old to
  // provide a sectioning level.
  if ((sectioning_level == wxT("0")) || (sectioning_level == wxT("3")))
    group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_SUBSECTION);
  if (sectioning_level == wxT("4"))
    group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_SUBSUBSECTION);
  if (sectioning_level == wxT("5"))
    group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_HEADING5);
  if (!group)
    group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_HEADING6);
  ParseCommonGroupCellAttrs(node, group.get());
  return group;
}

OwningGroupPtr MathParser::GroupCellFromImageTag(wxXmlNode *node)
{
  auto group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_IMAGE);
  ParseCommonGroupCellAttrs(node, group.get());
  return group;
}

OwningGroupPtr MathParser::GroupCellFromCodeTag(wxXmlNode *node)
{
  auto group = MakeOwned<GroupCell>(m_configuration, GC_TYPE_CODE);
  wxString isAutoAnswer = node->GetAttribute(wxT("auto_answer"), wxT("no"));
  if(isAutoAnswer == wxT("yes"))
    group->AutoAnswer(true);
  int i = 1;
  wxString answer;
  wxString question;
  while (node->GetAttribute(wxString::Format(wxT("answer%i"),i),&answer))
  {
    if(node->GetAttribute(wxString::Format(wxT("question%i"),i),&question))
      group->SetAnswer(question,answer);
    else
      group->SetAnswer(wxString::Format(wxT("Question #%i"),i),answer);
    i++;
  }
  ParseCommonGroupCellAttrs(node, group.get());
  return group;
}


std::unique_ptr<Cell> MathParser::HandleNullPointer(Cell *cell)
{
  std::unique_ptr<Cell> retval(cell);
  if (!retval)
  {
    retval = make_unique<TextCell>(NULL, m_configuration, _("Bug: Missing contents"));
    retval->SetToolTip(_("The xml data from maxima or from the .wxmx file was missing data here.\n"
                         "If you find a way how to reproduce this problem please file a bug "
                         "report against wxMaxima."));
    retval->SetStyle(TS_ERROR);
  }
  return retval;
}

OwningCellPtr MathParser::ParseEditorTag(wxXmlNode *node)
{
  auto editor = MakeOwned<EditorCell>(nullptr, m_configuration);
  wxString type = node->GetAttribute(wxT("type"), wxT("input"));
  if (type == wxT("input"))
    editor->SetType(MC_TYPE_INPUT);
  else if (type == wxT("text"))
    editor->SetType(MC_TYPE_TEXT);
  else if (type == wxT("title"))
    editor->SetType(MC_TYPE_TITLE);
  else if (type == wxT("section"))
    editor->SetType(MC_TYPE_SECTION);
  else if (type == wxT("subsection"))
    editor->SetType(MC_TYPE_SUBSECTION);
  else if (type == wxT("subsubsection"))
    editor->SetType(MC_TYPE_SUBSUBSECTION);
  else if (type == wxT("heading5"))
    editor->SetType(MC_TYPE_HEADING5);
  else if (type == wxT("heading6"))
    editor->SetType(MC_TYPE_HEADING6);

  wxString text;
  wxXmlNode *line = node->GetChildren();
  while (line)
  {
    if (line->GetName() == wxT("line"))
    {
      if (!text.IsEmpty())
        text += wxT("\n");
      text += line->GetNodeContent();
    }
    line = line->GetNext();
  } // end while
  editor->SetValue(text);
  return editor;
}

OwningCellPtr MathParser::ParseFracTag(wxXmlNode *node)
{
  auto frac = MakeOwned<FracCell>(nullptr, m_configuration);
  frac->SetFracStyle(m_FracStyle);
  frac->SetHighlight(m_highlight);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  frac->SetNum(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  frac->SetDenom(HandleNullPointer(ParseTag(child, false)));
  
  if (node->GetAttribute(wxT("line")) == wxT("no"))
    frac->SetFracStyle(FracCell::FC_CHOOSE);
  if (node->GetAttribute(wxT("diffstyle")) == wxT("yes"))
    frac->SetFracStyle(FracCell::FC_DIFF);
  frac->SetType(m_ParserStyle);
  frac->SetStyle(TS_VARIABLE);
  frac->SetupBreakUps();
  ParseCommonAttrs(node, frac);
  return frac;
}

OwningCellPtr MathParser::ParseDiffTag(wxXmlNode *node)
{
  auto diff = MakeOwned<DiffCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  if (child)
  {
    int fc = m_FracStyle;
    m_FracStyle = FracCell::FC_DIFF;

    diff->SetDiff(HandleNullPointer(ParseTag(child, false)));
    m_FracStyle = fc;
    child = GetNextTag(child);

    diff->SetBase(HandleNullPointer(ParseTag(child, true)));
    diff->SetType(m_ParserStyle);
    diff->SetStyle(TS_VARIABLE);
  }
  ParseCommonAttrs(node, diff);
  return diff;
}

OwningCellPtr MathParser::ParseSupTag(wxXmlNode *node)
{
  auto expt = MakeOwned<ExptCell>(nullptr, m_configuration);
  if (node->GetAttributes())
    expt->IsMatrix(true);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  wxString baseString;
  {
    auto base = HandleNullPointer(ParseTag(child, false));
    baseString = base->ToString();
    expt->SetBase(std::move(base));
  }
  child = GetNextTag(child);
  wxString powerString;
  {
    auto power = HandleNullPointer(ParseTag(child, false));
    power->SetExponentFlag();
    expt->SetType(m_ParserStyle);
    expt->SetStyle(TS_VARIABLE);
    powerString = power->ToString();
    expt->SetPower(std::move(power));
  }
  ParseCommonAttrs(node, expt);
  if (node->GetAttribute(wxT("mat"), wxT("false")) == wxT("true"))
    expt->SetAltCopyText(baseString + wxT("^^") + powerString);

  return expt;
}

OwningCellPtr MathParser::ParseSubSupTag(wxXmlNode *node)
{
  auto subsup = MakeOwned<SubSupCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  subsup->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  wxString pos;
  if (child && !child->GetAttribute("pos", wxEmptyString).empty())
  {
    while(child)
    {
      pos = child->GetAttribute("pos", wxEmptyString);
      auto cell = HandleNullPointer(ParseTag(child, false));
      if (pos == "presub")
        subsup->SetPreSub(std::move(cell));
      else if (pos == "presup")
        subsup->SetPreSup(std::move(cell));
      else if (pos == "postsup")
        subsup->SetPostSup(std::move(cell));
      else if (pos == "postsub")
        subsup->SetPostSub(std::move(cell));
      child = SkipWhitespaceNode(child);
      child = GetNextTag(child);
    }
  }
  else
  {
    {
      auto index = HandleNullPointer(ParseTag(child, false));
      index->SetExponentFlag();
      subsup->SetIndex(std::move(index));
    }
    child = GetNextTag(child);
    {
      auto power = HandleNullPointer(ParseTag(child, false));
      power->SetExponentFlag();
      subsup->SetExponent(std::move(power));
    }
    subsup->SetType(m_ParserStyle);
    subsup->SetStyle(TS_VARIABLE);
    ParseCommonAttrs(node, subsup);
  }
  return subsup;
}

OwningCellPtr MathParser::ParseMmultiscriptsTag(wxXmlNode *node)
{
  auto subsup = MakeOwned<SubSupCell>(nullptr, m_configuration);
  bool pre = false;
  bool subscript = true;
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  subsup->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  while (child)
  {
    if (child->GetName() == "mprescripts")
    {
      pre = true;
      subscript = true;
      child = GetNextTag(child);
      continue;
    }
    
    if (child->GetName() != "none")
    {
      std::unique_ptr<Cell> cell(ParseTag(child,false));
      if (pre && subscript)
        subsup->SetPreSub(std::move(cell));
      if (pre && !subscript)
        subsup->SetPreSup(std::move(cell));
      if (!pre && subscript)
        subsup->SetPostSub(std::move(cell));
      if (!pre && !subscript)
        subsup->SetPostSup(std::move(cell));
    }
    subscript = !subscript;
    child = SkipWhitespaceNode(child);
    child = GetNextTag(child);
  }
  return subsup;
}

OwningCellPtr MathParser::ParseSubTag(wxXmlNode *node)
{
  auto sub = MakeOwned<SubCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  sub->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  {
    auto index = HandleNullPointer(ParseTag(child, false));
    index->SetExponentFlag();
    sub->SetIndex(std::move(index));
  }
  sub->SetType(m_ParserStyle);
  sub->SetStyle(TS_VARIABLE);
  ParseCommonAttrs(node, sub);
  return sub;
}

OwningCellPtr MathParser::ParseAtTag(wxXmlNode *node)
{
  auto at = MakeOwned<AtCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  at->SetBase(HandleNullPointer(ParseTag(child, false)));
  at->SetHighlight(m_highlight);
  child = GetNextTag(child);
  at->SetIndex(HandleNullPointer(ParseTag(child, false)));
  at->SetType(m_ParserStyle);
  at->SetStyle(TS_VARIABLE);
  ParseCommonAttrs(node, at);
  return at;
}

OwningCellPtr MathParser::ParseFunTag(wxXmlNode *node)
{
  auto fun = MakeOwned<FunCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  fun->SetName(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  fun->SetType(m_ParserStyle);
  fun->SetStyle(TS_FUNCTION);
  fun->SetArg(HandleNullPointer(ParseTag(child, false)));
  ParseCommonAttrs(node, fun);
  if (fun && fun->ToString().Contains(")("))
    fun->SetToolTip(_("If this isn't a function returning a lambda() expression,"
                      " a multiplication sign (*) between closing and opening parenthesis is missing here."));
  return fun;
}

OwningCellPtr MathParser::ParseText(wxXmlNode *node, TextStyle style)
{
  wxString str;
  std::unique_ptr<TextCell> retval;
  if ((node != NULL) && ((str = node->GetContent()) != wxEmptyString))
  {
    str.Replace(wxT("-"), wxT("\u2212")); // unicode minus sign

    wxStringTokenizer lines(str, wxT('\n'));
    while (lines.HasMoreTokens())
    {
      auto cell = make_unique<TextCell>(NULL, m_configuration);
      switch(style)
      {
      case TS_ERROR:
        cell->SetType(MC_TYPE_ERROR);
        break;

      case TS_WARNING:
        cell->SetType(MC_TYPE_WARNING);
        break;

      case TS_LABEL:
        cell->SetType(MC_TYPE_LABEL);
        break;

      case TS_USERLABEL:
        cell->SetType(MC_TYPE_LABEL);
        break;

      default:
        cell->SetType(m_ParserStyle);
      }
      cell->SetStyle(style);
      
      cell->SetHighlight(m_highlight);
      cell->SetValue(lines.GetNextToken());
      if (!retval)
        retval = std::move(cell);
      else
      {
        cell->ForceBreakLine(true);
        retval->AppendCell(std::move(cell));
      };
    }
  }

  if (!retval)
    retval = make_unique<TextCell>(NULL, m_configuration);

  ParseCommonAttrs(node, retval.get());
  return retval.release();
}

template <typename T>
void MathParser::ParseCommonAttrs(wxXmlNode *node, const OwningPtr<T> &ptr)
{
  return ParseCommonAttrs(node, ptr.get());
}

void MathParser::ParseCommonAttrs(wxXmlNode *node, Cell *cell)
{
  if (!cell || !node)
    return;

  if(node->GetAttribute(wxT("breakline"), wxT("false")) == wxT("true"))
    cell->ForceBreakLine(true);

  wxString val;
  
  if(node->GetAttribute(wxT("tooltip"), &val))
    cell->SetToolTip(val);
  if(node->GetAttribute(wxT("altCopy"), &val))
    cell->SetAltCopyText(val);
}

void MathParser::ParseCommonGroupCellAttrs(wxXmlNode *node, const OwningGroupPtr &group)
{ ParseCommonGroupCellAttrs(node, group.get()); }

void MathParser::ParseCommonGroupCellAttrs(wxXmlNode *node, GroupCell *group)
{
  if (!group || !node)
    return;

  if (node->GetAttribute(wxT("hideToolTip")) == wxT("true"))
    group->SetSuppressTooltipMarker(true);
}

OwningCellPtr MathParser::ParseCharCode(wxXmlNode *node)
{
  auto cell = MakeOwned<TextCell>(nullptr, m_configuration);
  wxString str;
  if (node && !(str = node->GetContent()).empty())
  {
    long code;
    if (str.ToLong(&code))
      str = wxString::Format(wxT("%c"), code);
    cell->SetValue(str);
    cell->SetType(m_ParserStyle);
    cell->SetStyle(TS_DEFAULT);
    cell->SetHighlight(m_highlight);
  }
  ParseCommonAttrs(node, cell);
  return cell;
}

OwningCellPtr MathParser::ParseSqrtTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  auto cell = MakeOwned<SqrtCell>(nullptr, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  ParseCommonAttrs(node, cell);
  return cell;
}

OwningCellPtr MathParser::ParseAbsTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  auto cell = MakeOwned<AbsCell>(nullptr, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  ParseCommonAttrs(node, cell);
  return cell;
}

OwningCellPtr MathParser::ParseConjugateTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  auto cell = MakeOwned<ConjugateCell>(nullptr, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  ParseCommonAttrs(node, cell);
  return cell;
}

OwningCellPtr MathParser::ParseParenTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  auto cell = make_unique<ParenCell>(NULL, m_configuration);
  // No special Handling for NULL args here: They are completely legal in this case.
  cell->SetInner(std::unique_ptr<Cell>(ParseTag(child, true)), m_ParserStyle);
  cell->SetHighlight(m_highlight);
  cell->SetStyle(TS_VARIABLE);
  if (node->GetAttributes())
    cell->SetPrint(false);
  ParseCommonAttrs(node, cell.get());
  return cell.release();
}

OwningCellPtr MathParser::ParseLimitTag(wxXmlNode *node)
{
  auto limit = MakeOwned<LimitCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  limit->SetName(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  limit->SetUnder(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  limit->SetBase(HandleNullPointer(ParseTag(child, false)));
  limit->SetType(m_ParserStyle);
  limit->SetStyle(TS_VARIABLE);
  ParseCommonAttrs(node, limit);
  return limit;
}

OwningCellPtr MathParser::ParseSumTag(wxXmlNode *node)
{
  auto sum = MakeOwned<SumCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  wxString type = node->GetAttribute(wxT("type"), wxT("sum"));

  if (type == wxT("prod"))
    sum->SetSumStyle(SM_PROD);
  sum->SetHighlight(m_highlight);
  sum->SetUnder(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  if (type != wxT("lsum"))
    sum->SetOver(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  sum->SetBase(HandleNullPointer(ParseTag(child, false)));
  sum->SetType(m_ParserStyle);
  sum->SetStyle(TS_VARIABLE);
  ParseCommonAttrs(node, sum);
  return sum;
}

OwningCellPtr MathParser::ParseIntTag(wxXmlNode *node)
{
  auto in = MakeOwned<IntCell>(nullptr, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  in->SetHighlight(m_highlight);
  wxString definiteAtt = node->GetAttribute(wxT("def"), wxT("true"));
  if (definiteAtt != wxT("true"))
  {
    in->SetBase(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetVar(HandleNullPointer(ParseTag(child, true)));
    in->SetType(m_ParserStyle);
    in->SetStyle(TS_VARIABLE);
  }
  else
  {
    // A Definite integral
    in->SetIntStyle(IntCell::INT_DEF);
    in->SetUnder(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetOver(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetBase(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetVar(HandleNullPointer(ParseTag(child, true)));
    in->SetType(m_ParserStyle);
    in->SetStyle(TS_VARIABLE);
  }
  ParseCommonAttrs(node, in);
  return in;
}

OwningCellPtr MathParser::ParseTableTag(wxXmlNode *node)
{
  auto matrix = MakeOwned<MatrCell>(nullptr, m_configuration);
  matrix->SetHighlight(m_highlight);

  if (node->GetAttribute(wxT("special"), wxT("false")) == wxT("true"))
    matrix->SetSpecialFlag(true);
  if (node->GetAttribute(wxT("inference"), wxT("false")) == wxT("true"))
  {
    matrix->SetInferenceFlag(true);
    matrix->SetSpecialFlag(true);
  }
  if (node->GetAttribute(wxT("colnames"), wxT("false")) == wxT("true"))
    matrix->ColNames(true);
  if (node->GetAttribute(wxT("rownames"), wxT("false")) == wxT("true"))
    matrix->RowNames(true);
  if (node->GetAttribute(wxT("roundedParens"), wxT("false")) == wxT("true"))
    matrix->RoundedParens(true);

  wxXmlNode *rows = SkipWhitespaceNode(node->GetChildren());
  while (rows)
  {
    matrix->NewRow();
    wxXmlNode *cells = SkipWhitespaceNode(rows->GetChildren());
    while (cells)
    {
      matrix->NewColumn();
      matrix->AddNewCell(HandleNullPointer(ParseTag(cells, false)));
      cells = GetNextTag(cells);
    }
    rows = GetNextTag(rows);
  }
  matrix->SetType(m_ParserStyle);
  matrix->SetStyle(TS_VARIABLE);
  matrix->SetDimension();
  ParseCommonAttrs(node, matrix);
  return matrix;
}

OwningCellPtr MathParser::ParseTag(wxXmlNode *node, bool all)
{
  OwningCellPtr retval;
  OwningCellPtr ownedCell;
  Cell *cell = {};
  bool warning = all;
  wxString altCopy;

  node = SkipWhitespaceNode(node);

  while (node)
  {
    if (node->GetType() == wxXML_ELEMENT_NODE)
    {
      // Parse XML tags. The only other type of element we recognize are text
      // nodes.
      wxString tagName(node->GetName());

      OwningCellPtr tmp;

      auto function = m_innerTags[tagName];
      if (function)
          tmp = CALL_MEMBER_FN(*this, function, node);

      if  (!tmp && node->GetChildren())
        tmp = ParseTag(node->GetChildren());

      // Append the cell we found (tmp) to the list of cells we parsed so far (cell).
      if (!tmp)
      {
        ParseCommonAttrs(node, tmp);
        if (!cell)
        {
          cell = tmp.get();
          ownedCell = std::move(tmp);
        }
        else
          cell->AppendCell(std::move(tmp));
      }
    }
    else
    {
      // We didn't get a tag but got a text cell => Parse the text.
      if (!cell)
      {
        ownedCell = ParseText(node);
        cell = ownedCell.get();
      }
      else
        cell->AppendCell(ParseText(node));
    }

    if (cell)
    {
      // Append the new cell to the return value
      if (!retval)
      {
        wxASSERT(ownedCell);
        retval = std::move(ownedCell);
      }
      else
        cell = cell->m_next.get();
    }
    else if (warning && !all)
    {
      // Tell the user we ran into problems.
      wxString name;
      name.Trim(true);
      name.Trim(false);
      if (cell) name = cell->ToString();
      if (name.Length() != 0)
      {
        LoggingMessageBox(_("Parts of the document will not be loaded correctly:\nFound unknown XML Tag name " + name),
                     _("Warning"),
                     wxOK | wxICON_WARNING);
        warning = false;
      }
    }

    node = GetNextTag(node);
    
    if (!all)
      break;
  }

  return retval;
}

OwningCellPtr MathParser::ParseLine(wxString s, CellType style)
{
  m_ParserStyle = style;
  m_FracStyle = FracCell::FC_NORMAL;
  m_highlight = false;
  OwningCellPtr cell;

  int showLength;

  switch ((*m_configuration)->ShowLength())
  {
    case 0:
      showLength = 6000;
      break;
    case 1:
      showLength = 20000;
      break;
    case 2:
      showLength = 250000;
      break;
    case 3:
      showLength = 0;
      break;
  default:
      showLength = 50000;    
  }

  m_graphRegex.Replace(&s, wxT("\uFFFD"));

  if (((long) s.Length() < showLength) || (showLength == 0))
  {
    wxXmlDocument xml;
    wxStringInputStream xmlStream(s);
    xml.Load(xmlStream, wxT("UTF-8"), wxXMLDOC_KEEP_WHITESPACE_NODES);
    wxXmlNode *doc = xml.GetRoot();
    if (doc)
      cell = ParseTag(doc->GetChildren());
  }
  else
  {
    cell = MakeOwned<TextCell>(nullptr, m_configuration,
                               _("(Expression longer than allowed by the configuration setting)"),
                               TS_WARNING);
    cell->SetToolTip(_("The maximum size of the expressions wxMaxima is allowed to display "
                       "can be changed in the configuration dialogue."
                       ));
    cell->ForceBreakLine(true);
  }
  return cell;
}

wxRegEx MathParser::m_graphRegex(wxT("[[:cntrl:]]"));
MathParser::MathCellFunctionHash MathParser::m_innerTags;
MathParser::GroupCellFunctionHash MathParser::m_groupTags;

