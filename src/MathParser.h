// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2004-2015 Gunter Königsmann <wxMaxima@physikbuch.de>
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

/*! \file

The header file for the xml cell parser
 */

#ifndef MATHPARSER_H
#define MATHPARSER_H

#include <wx/xml/xml.h>

#include <wx/filesys.h>
#include <wx/fs_arc.h>

#include "MathCell.h"
#include "TextCell.h"

/*! This class handles parsing the xml representation of a cell tree.

The xml representation of a cell tree can be found in the file contents.xml 
inside a wxmx file
 */
class MathParser
{
public:
  MathParser(wxString zipfile = wxEmptyString);
  ~MathParser();
  MathCell* ParseLine(wxString s, int style = MC_TYPE_DEFAULT);
  MathCell* ParseTag(wxXmlNode* node, bool all = true);
private:
  /*! Convert XML to a group tree

    This function is responsible for creating
    a tree of groupcells when loading XML document.
    \attention Any changes in GroupCell structure or methods
    has to be reflected here in order to ensure proper
    loading of WXMX files.
  */
  MathCell* ParseCellTag(wxXmlNode* node);
  MathCell* ParseEditorTag(wxXmlNode* node);
  MathCell* ParseFracTag(wxXmlNode* node);
  MathCell* ParseText(wxXmlNode* node, int style = TS_DEFAULT);
  MathCell* ParseCharCode(wxXmlNode* node, int style = TS_DEFAULT);
  MathCell* ParseSupTag(wxXmlNode* node);
  MathCell* ParseSubTag(wxXmlNode* node);
  MathCell* ParseAbsTag(wxXmlNode* node);
  MathCell* ParseConjugateTag(wxXmlNode* node);
  MathCell* ParseUnderTag(wxXmlNode* node);
  MathCell* ParseTableTag(wxXmlNode* node);
  MathCell* ParseAtTag(wxXmlNode* node);
  MathCell* ParseDiffTag(wxXmlNode* node);
  MathCell* ParseSumTag(wxXmlNode* node);
  MathCell* ParseIntTag(wxXmlNode* node);
  MathCell* ParseFunTag(wxXmlNode* node);
  MathCell* ParseSqrtTag(wxXmlNode* node);
  MathCell* ParseLimitTag(wxXmlNode* node);
  MathCell* ParseParenTag(wxXmlNode* node);
  MathCell* ParseSubSupTag(wxXmlNode* node);
  int m_ParserStyle;
  int m_FracStyle;
  //! The maximum number of digits of a number that is to be displayed
  int m_displayedDigits;
  bool m_highlight;
  wxFileSystem *m_fileSystem; // used for loading pictures in <img> and <slide>
};

#endif // MATHPARSER_H
