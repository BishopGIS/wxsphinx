/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxSphinxFactories class. factory array
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

class wxSphinxFactories
{
public:
    wxSphinxFactories(wxXmlNode* pConf);
    virtual ~wxSphinxFactories(void);

	typedef struct _objfactory_desc
	{
        wxString sName;
		IwxSphinxObjectFactory* pFactory;
		bool bIsEnabled;
	} OBJFACTORYDESC;

	//
	void LoadObjectFactories(wxXmlNode* pNode);
	void EmptyObjectFactories(void);
    IwxSphinxObjectFactory* GetFactory(wxString sName);
    size_t GetSize(){return m_ObjectFactoriesArray.size();};
    bool IsEnabled(int nIndex){return m_ObjectFactoriesArray[nIndex].bIsEnabled;};
    IwxSphinxObjectFactory* GetFactory(int nIndex){return m_ObjectFactoriesArray[nIndex].pFactory;};
    void PutCatalogRef(IwxSphinxCatalog* pCatalog);
protected:
    std::vector<OBJFACTORYDESC> m_ObjectFactoriesArray;
    IwxSphinxCatalog* m_pCatalog;
};
