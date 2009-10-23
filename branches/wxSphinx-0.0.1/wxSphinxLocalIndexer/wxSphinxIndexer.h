/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxSphinxIndexer class.
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

class wxSphinxIndexer
{
public:
	wxSphinxIndexer(bool bQuiet, wxXmlNode *pConf, wxSQLite3Database *pdb, wxSphinxFactories* pFactories);
	~wxSphinxIndexer(void);
	virtual bool Start(long nMaxCount);
	virtual bool ShowDetails(long nIndex);
private:
	bool m_bQuiet;
	wxSQLite3Database *m_pDB;
	wxXmlNode *m_pConf;
    wxSphinxFactories* m_pFactories;
};
