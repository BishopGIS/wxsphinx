/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxContainer class.
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

//-----------------------------------------------------------------------------
// wxSphinxContainer
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CMN wxSphinxContainer :
	public wxSphinxObject,
	public wxDirTraverser
{
public:
	wxSphinxContainer(wxString sPath, wxString sModule, IwxSphinxCatalog* pCatalog, long nOID = -1, wxDateTime dtChanged = wxDateTime::Now(), wxDateTime dtIndexed = wxDateTime::Now(), bool bQuiet = false);
	virtual ~wxSphinxContainer(void){};
	virtual void DoWork(void);
	virtual void Delete(void);
	virtual OBJECTTYPE GetType(void){return wxEnumSphinxContainer;};
	//wxDirTraverser
	virtual wxDirTraverseResult OnFile(const wxString& filename);
	virtual wxDirTraverseResult OnDir(const wxString& dirname);
protected:
	bool m_bQuiet;
	bool m_bRecursive;
	wxObjectArray m_Children;
	wxArrayString m_FileNames;
};

//-----------------------------------------------------------------------------
// wxSphinxRootContainer
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CMN wxSphinxRootContainer :
	public wxSphinxContainer
{
public:
	wxSphinxRootContainer(wxString sPath, wxString sModule, IwxSphinxCatalog* pCatalog, long nOID = -1, wxDateTime dtChanged = wxDateTime::Now(), wxDateTime dtIndexed = wxDateTime::Now(), bool bQuiet = false);
	virtual ~wxSphinxRootContainer(void){};
	virtual void SetRecursive(bool bRecursive){m_bRecursive = bRecursive;};
	virtual void DoWork(void);
	virtual void Delete(void);
protected:
	bool m_bRecursive;
};

// \/:*?<>|