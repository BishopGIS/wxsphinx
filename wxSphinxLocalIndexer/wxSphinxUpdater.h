/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxSphinxUpdater class.
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
#include "../wxSphinxCommon/wxContainer.h"
#include "wxSphinxFactories.h"

class wxSphinxUpdater :
	public IwxSphinxCatalog,
	public IwxSphinxObjectFactory
{
public:
	wxSphinxUpdater(bool bQuiet, wxXmlNode *pIndexPaths, wxSQLite3Database *pdb, wxSphinxFactories* pFactories);
	virtual ~wxSphinxUpdater(void);
	//
	virtual bool Start(void);
	virtual void List(bool bAll);
	virtual bool GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray);
    virtual void SetConfig(wxXmlNode* pConfNode){};
	virtual wxSQLite3Database* GetDB(void){return m_pDB;};
protected:
	bool m_bQuiet;
	wxXmlNode *m_pIndexPaths;
	wxSQLite3Database *m_pDB;
	std::vector<wxSphinxRootContainer*> m_IndexPaths;
    wxSphinxFactories* m_pFactories;
};
