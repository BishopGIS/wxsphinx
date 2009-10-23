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
#include "wxSphinxUpdater.h"
#include "common.h"

wxSphinxUpdater::wxSphinxUpdater(bool bQuiet, wxXmlNode *pIndexPaths, wxSQLite3Database *pdb, wxSphinxFactories* pFactories)
{
	m_bQuiet = bQuiet;
	m_pIndexPaths = pIndexPaths;
	m_pDB = pdb;
	m_nOid = -1;
	m_pFactories = pFactories;
}

wxSphinxUpdater::~wxSphinxUpdater(void)
{
	for(size_t i = 0; i < m_IndexPaths.size(); i++)
		wxDELETE(m_IndexPaths[i]);
}

bool wxSphinxUpdater::Start(void)
{
	wxStopWatch sw;
	m_pFactories->PutCatalogRef(this);

	if(!m_pDB->TableExists(TABLE_NAME))
	{
		//1 create table
		m_pDB->ExecuteUpdate(wxString::Format(wxT("CREATE TABLE %s (oid INTEGER PRIMARY KEY ASC, path TEXT, type INTEGER, module TEXT, dt_changed INTEGER, dt_indexed INTEGER, deleted INTEGER, parent_id INTEGER, datatext TEXT)"), TABLE_NAME));
		//2 insert root
		m_pDB->ExecuteUpdate(wxString::Format(wxT("INSERT INTO %s (type) VALUES ('%u')"), TABLE_NAME, wxEnumSphinxRoot));
	}
	//get root id
	wxSQLite3ResultSet set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT oid FROM %s WHERE type = %u"), TABLE_NAME, wxEnumSphinxRoot));
	set.NextRow();
	m_nOid = set.GetInt(0, -1);
	if(m_nOid == -1)
		return false;
	//get root children 1 level
	set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT oid, path, dt_changed, dt_indexed, deleted FROM %s WHERE type = %u AND parent_id = %u"), TABLE_NAME, wxEnumSphinxContainer, m_nOid));
	while (set.NextRow())
	{
		int oid = set.GetInt(0, -1);
		wxString path = set.GetString(1);
		wxDateTime changed((time_t)set.GetInt(2));
		wxDateTime indexed((time_t)set.GetInt(3));
		bool deleted = set.GetBool(4);
		if(!deleted)
		{
			wxSphinxRootContainer *pwxRootContainer = new wxSphinxRootContainer(path, wxT("none"), static_cast<IwxSphinxCatalog*>(this), oid, changed, indexed, m_bQuiet);
			m_IndexPaths.push_back(pwxRootContainer);
		}
	}

	//create container object for each children in 1 level
	wxXmlNode* pChild = m_pIndexPaths->GetChildren();
	while(pChild)
	{
		wxString sPath = pChild->GetPropVal(wxT("path"), wxT(""));
		if(!sPath.IsEmpty())
		{
			bool bRecursive = pChild->GetPropVal(wxT("recursive"), wxT("t")) == wxT("t") ? true : false;
			bool bExist = false;
			for(size_t i = 0; i < m_IndexPaths.size(); i++)
			{
				if(m_IndexPaths[i]->GetPath() == sPath)
				{
					bExist = true;
					m_IndexPaths[i]->SetRecursive(bRecursive);
					m_IndexPaths[i]->SetState(wxEnumSphinxExistState);
				}
			}
			if(!bExist)
			{
				wxSphinxRootContainer *pwxRootContainer = new wxSphinxRootContainer(sPath, wxT("none"), static_cast<IwxSphinxCatalog*>(this));
				pwxRootContainer->SetState(wxEnumSphinxNewState);
				pwxRootContainer->SetRecursive(bRecursive);
				m_IndexPaths.push_back(pwxRootContainer);
			}
		}
		pChild = pChild->GetNext();
	}

	for(size_t i = 0; i < m_IndexPaths.size(); i++)
	{
		if(m_IndexPaths[i]->GetState() == wxEnumSphinxDeletedState)
			m_IndexPaths[i]->Delete();
		else
			m_IndexPaths[i]->DoWork();
	}
	wxLogMessage(_("The Start function took %ldms to execute"), sw.Time());
	return true;
}

void wxSphinxUpdater::List(bool bAll)
{
	if(m_pDB->TableExists(TABLE_NAME))
	{
		wxString query;
		if(bAll)
			query = wxString::Format(wxT("SELECT oid, path, module, dt_changed FROM %s"), TABLE_NAME);
		else
			query = wxString::Format(wxT("SELECT oid, path, module, dt_changed FROM %s WHERE type = %u"), TABLE_NAME, wxEnumSphinxFile);
		wxSQLite3ResultSet set = m_pDB->ExecuteQuery(query);
		int counter = 1;
		while (set.NextRow())
		{
		    long oid = set.GetInt(0);
			wxString path = set.GetString(1);
			wxString module = set.GetString(2);
			wxDateTime dtm((time_t)set.GetInt(3));
			wxString sdtm(wxT("<DIR>"));
			if(dtm.IsValid())
                sdtm = dtm.Format(_("%d-%m-%Y %H:%M:%S")).c_str();
			wxFprintf(stdout, wxT("%u(%u) %s\t%s\t%s\n"), counter, oid, module.c_str(), sdtm.c_str(), path.c_str());

			//wxLogMessage(wxT("%u %s %s"), counter, module.c_str(), path.c_str());
			counter++;
		}
	}
}


bool wxSphinxUpdater::GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray)
{
	for(size_t i = 0; i < m_pFactories->GetSize(); i++)
	{
		if(m_pFactories->IsEnabled(i))
			if(!m_pFactories->GetFactory(i)->GetChildren(pFileNames, pObjArray))
				return false;
	}
	return true;
}
