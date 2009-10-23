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
#include "wxContainer.h"

//-----------------------------------------------------------------------------
// wxContainer
//-----------------------------------------------------------------------------

wxSphinxContainer::wxSphinxContainer(wxString sPath, wxString sModule, IwxSphinxCatalog* pCatalog, long nOID, wxDateTime dtChanged, wxDateTime dtIndexed, bool bQuiet) : wxSphinxObject(sPath, sModule, pCatalog, nOID, dtChanged, dtIndexed)
{
	m_bQuiet = bQuiet;
	m_state = wxEnumSphinxDeletedState;
}

void wxSphinxContainer::DoWork(void)
{
	m_FileNames.clear();
	//1 if wxSphinxNewState create row
	if(m_state == wxEnumSphinxNewState)
	{
		int nParentOID = m_pParent->GetOID();
		//terminating
		wxString sInsertPath = m_sPath;
		sInsertPath.Replace(wxT("'"), wxT("''"));
		m_pDB->ExecuteUpdate(wxString::Format(wxT("INSERT INTO %s (path, type, module, parent_id) VALUES ('%s', '%u', '%s', '%u')"), TABLE_NAME, sInsertPath.c_str(), wxEnumSphinxContainer, m_sIndexModule.c_str(), nParentOID));
		m_nOid = m_pDB->GetLastRowId().ToLong();
	}
	//2 if wxSphinxExistState get children from DB
	if(m_state == wxEnumSphinxExistState)
	{
		wxSQLite3ResultSet set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT * FROM %s WHERE parent_id = %u"), TABLE_NAME, m_nOid));
		while (set.NextRow())
		{
			//0		1		2		3		4		5		6		7
			//oid	path	type	module	changed	indexed	deleted	parent_id
			int oid = set.GetInt(0, -1);
			wxString path = set.GetString(1);
			OBJECTTYPE type = (OBJECTTYPE)set.GetInt(2, wxEnumSphinxFile);
			wxString module = set.GetString(3);
			wxDateTime changed = set.GetDateTime(4);
			wxDateTime indexed = set.GetDateTime(5);
			switch(type)
			{
			case wxEnumSphinxContainer:
				{
					wxSphinxContainer *pwxContainer = new wxSphinxContainer(path, module, m_pCatalog, oid, changed, indexed, m_bQuiet); 
					if(pwxContainer->Attach(this))
						m_Children.push_back(pwxContainer);
					else
						wxDELETE(pwxContainer);
				}
				break;
			case wxEnumSphinxFile:
				{
					wxSphinxObject *pwxObject = new wxSphinxObject(path, module, m_pCatalog, oid, changed, indexed); 
					if(pwxObject->Attach(this))
						m_Children.push_back(pwxObject);
					else
						wxDELETE(pwxObject);
				}
				break;
			default:
				break;
			}
		}
	}
	//3 get chilren
	wxObjectArray TempObjectArray;
	IwxSphinxObjectFactory* pFactory = dynamic_cast<IwxSphinxObjectFactory*>(m_pCatalog);
	if(pFactory)
	{
		wxDir dir(m_sPath);
		if(dir.IsOpened())
			dir.Traverse(*this, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN );
		pFactory->GetChildren(&m_FileNames, &TempObjectArray);
	}

	//4 check each chilren if new, deleted or exist
	for(size_t i = 0; i < TempObjectArray.size(); i++)
	{
		bool bExist = false;
		for(size_t j = 0; j < m_Children.size(); j++)
		{
			if(m_Children[j]->GetPath() == TempObjectArray[i]->GetPath())
			{
				bExist = true;
				m_Children[j]->SetState(wxEnumSphinxExistState);
			}
		}
		if(!bExist)
		{
			TempObjectArray[i]->SetState(wxEnumSphinxNewState);
			if(TempObjectArray[i]->Attach(this))
			{
				//wxLogMessage(wxString::Format(_("Add new path: %s"), TempObjectArray[i]->GetPath().wc_str()));
				m_Children.push_back(TempObjectArray[i]);
				TempObjectArray[i] = NULL;
			}
		}
	}
	for(size_t i = 0; i < TempObjectArray.size(); i++)
		wxDELETE(TempObjectArray[i]);

	for(size_t i = 0; i < m_Children.size(); i++)
	{
		if(m_Children[i]->GetState() == wxEnumSphinxDeletedState)
			m_Children[i]->Delete();
		else
		{
	//4.3 if container dowork
			m_Children[i]->DoWork();
	//5 detach
			m_Children[i]->Detach();
		}
		wxDELETE(m_Children[i]);
	}
}

void wxSphinxContainer::Delete(void)
{
	for(size_t i = 0; i < m_Children.size(); i++)
		m_Children[i]->Delete();
//	m_pDB->ExecuteUpdate(wxString::Format(wxT("DELETE FROM %s WHERE oid = %u"), TABLE_NAME, m_nOid));
	m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET deleted = 1 WHERE oid = %u"), TABLE_NAME, m_nOid));
}

wxDirTraverseResult wxSphinxContainer::OnFile(const wxString& filename)
{
	m_FileNames.Add(filename);
	return wxDIR_CONTINUE;
}

wxDirTraverseResult wxSphinxContainer::OnDir(const wxString& dirname)
{
	m_FileNames.Add(dirname);
	return wxDIR_IGNORE;
}

//-----------------------------------------------------------------------------
// wxRootContainer
//-----------------------------------------------------------------------------

wxSphinxRootContainer::wxSphinxRootContainer(wxString sPath, wxString sModule, IwxSphinxCatalog* pCatalog, long nOID, wxDateTime dtChanged, wxDateTime dtIndexed, bool bQuiet) : wxSphinxContainer(sPath, sModule, pCatalog, nOID, dtChanged, dtIndexed, bQuiet)
{
	m_state = wxEnumSphinxDeletedState;
	m_bRecursive = true;
}

void wxSphinxRootContainer::DoWork(void)
{
	m_FileNames.clear();
	//1 if wxSphinxNewState create row
	if(m_state == wxEnumSphinxNewState)
	{
		int nRootOID = m_pCatalog->GetOID();
		m_pDB->ExecuteUpdate(wxString::Format(wxT("INSERT INTO %s (path, type, module, parent_id) VALUES ('%s', '%u', '%s', '%u')"), TABLE_NAME, m_sPath.c_str(), wxEnumSphinxContainer, m_sIndexModule.c_str(), nRootOID));
		m_nOid = m_pDB->GetLastRowId().ToLong();
	}
	//2 if wxSphinxExistState get children from DB
	if(m_state == wxEnumSphinxExistState)
	{
		wxSQLite3ResultSet set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT * FROM %s WHERE parent_id = %u"), TABLE_NAME, m_nOid));
		while (set.NextRow())
		{
			//0		1		2		3		4		5		6		7
			//oid	path	type	module	changed	indexed	deleted	parent_id
			int oid = set.GetInt(0, -1);
			wxString path = set.GetString(1);
			OBJECTTYPE type = (OBJECTTYPE)set.GetInt(2, wxEnumSphinxFile);
			wxString module = set.GetString(3);
			wxDateTime changed = set.GetDateTime(4);
			wxDateTime indexed = set.GetDateTime(5);
			switch(type)
			{
			case wxEnumSphinxContainer:
				{
					wxSphinxContainer *pwxContainer = new wxSphinxContainer(path, module, m_pCatalog, oid, changed, indexed, m_bQuiet); 
					if(pwxContainer->Attach(this))
						m_Children.push_back(pwxContainer);
					else
						wxDELETE(pwxContainer);
				}
				break;
			case wxEnumSphinxFile:
				{
					wxSphinxObject *pwxObject = new wxSphinxObject(path, module, m_pCatalog, oid, changed, indexed); 
					if(pwxObject->Attach(this))
						m_Children.push_back(pwxObject);
					else
						wxDELETE(pwxObject);
				}
				break;
			default:
				break;
			}
		}
	}
	//3 get chilren
	wxObjectArray TempObjectArray;
	IwxSphinxObjectFactory* pFactory = dynamic_cast<IwxSphinxObjectFactory*>(m_pCatalog);
	if(pFactory)
	{
		wxDir dir(m_sPath);
		if(dir.IsOpened())
			dir.Traverse(*this, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN );
		pFactory->GetChildren(&m_FileNames, &TempObjectArray);
	}

	//4 check each chilren if new, deleted or exist
	for(size_t i = 0; i < TempObjectArray.size(); i++)
	{
		bool bExist = false;
		for(size_t j = 0; j < m_Children.size(); j++)
		{
			if(m_Children[j]->GetPath() == TempObjectArray[i]->GetPath())
			{
				if(m_bRecursive || m_Children[j]->GetType() == wxEnumSphinxFile)
				{
					bExist = true;
					m_Children[j]->SetState(wxEnumSphinxExistState);
				}
			}
		}
		if(!bExist)
		{
			TempObjectArray[i]->SetState(wxEnumSphinxNewState);
			if(TempObjectArray[i]->Attach(this))
			{
				if(m_bRecursive || TempObjectArray[i]->GetType() == wxEnumSphinxFile)
				{
					//wxLogMessage(wxString::Format(_("Add new path: %s"), TempObjectArray[i]->GetPath().wc_str()));
					m_Children.push_back(TempObjectArray[i]);
					TempObjectArray[i] = NULL;
				}
			}
		}
	}
	for(size_t i = 0; i < TempObjectArray.size(); i++)
		wxDELETE(TempObjectArray[i]);

	for(size_t i = 0; i < m_Children.size(); i++)
	{
		if(m_Children[i]->GetState() == wxEnumSphinxDeletedState)
			m_Children[i]->Delete();
		else
		{
			if(m_bRecursive || m_Children[i]->GetType() == wxEnumSphinxFile)
	//4.3 if container dowork
				m_Children[i]->DoWork();
	//5 detach
			m_Children[i]->Detach();
		}
		wxDELETE(m_Children[i]);
	}
}

	//4.1 if exist get chilren time and compare changed time from DB. if changed time from DB < real changed time UPDATE DB
	//4.2 if new, create row

void wxSphinxRootContainer::Delete(void)
{
	for(size_t i = 0; i < m_Children.size(); i++)
		m_Children[i]->Delete();
	m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET deleted = 1 WHERE oid = %u"), TABLE_NAME, m_nOid));
}
