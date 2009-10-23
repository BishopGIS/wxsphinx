/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxMLFileFactory class. XML, SVG and so
 * Author:   Bishop (aka Barishnikov Dmitriy), polimax@mail.ru
 ******************************************************************************
*   Copyright (C) 2009  Bishop
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
 #pragma once

#include "common.h"
class wxMLFileFactory :
	public IwxSphinxObjectFactory,
	public wxObject
{
	DECLARE_DYNAMIC_CLASS(wxMLFileFactory)
public:
    wxMLFileFactory();
    virtual ~wxMLFileFactory();
	//IwxSphinxObjectFactory
	virtual bool GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray);
	virtual wxString GetCData(wxString sPath);
	virtual void SetConfig(wxXmlNode* pConfNode);
	virtual void GetNodeContent(wxXmlNode* pNode, wxString& sStr);
	wxString CleanOutput(wxString sOutString);
private:
    wxArrayString m_sExtArray;
};
