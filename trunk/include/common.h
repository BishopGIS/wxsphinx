/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  common data.
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

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/dynload.h>
#include <wx/dynlib.h>
#include <wx/xml/xml.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <vector>
#include "wx/wxsqlite3.h"

#ifdef WXMAKINGDLL_CMN
#    define WXDLLIMPEXP_CMN WXEXPORT
#    define WXDLLIMPEXP_DATA_CMN(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_CMN WXIMPORT
#    define WXDLLIMPEXP_DATA_CMN(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_CMN
#    define WXDLLIMPEXP_DATA_CMN(type) type
#endif

#define TABLE_NAME wxT("lf_index")

typedef enum _objecttype
{
	wxEnumSphinxRoot = 0,
	wxEnumSphinxContainer = 1,
	wxEnumSphinxFile = 2
} OBJECTTYPE;

typedef enum _objectstate
{
	wxEnumSphinxNewState = 0,
	wxEnumSphinxExistState = 1,
	wxEnumSphinxDeletedState = 2
} OBJECTSTATE;

class IwxSphinxCatalog
{
public:
	virtual ~IwxSphinxCatalog(void){};
	virtual long GetOID(void){return m_nOid;};
	//virtual int SetOID(int nNewOID){m_nOID = nNewOID;};
	virtual wxSQLite3Database* GetDB(void) = 0;
protected:
	long m_nOid;
};

class wxSphinxObject
{
public:
	wxSphinxObject(wxString sPath, wxString sModule, IwxSphinxCatalog* pCatalog, long nOID = -1, wxDateTime dtChanged = wxDateTime::Now(), wxDateTime dtIndexed = wxDateTime::Now())
	{
		m_sPath = sPath;
		m_nOid = nOID;
		m_dtChanged = dtChanged;
		m_dtIndexed = dtIndexed;
		m_pCatalog = pCatalog;
		m_pDB = pCatalog->GetDB();
		m_state = wxEnumSphinxDeletedState;
		m_sIndexModule = sModule;
	}
	virtual ~wxSphinxObject(void){};
	virtual wxString GetPath(void){return m_sPath;};
	virtual bool Attach(wxSphinxObject* pParent)
	{
		m_pParent = pParent;
		return true;
	}
	virtual void Detach(void){};
	virtual void Delete(void)
	{
		m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET deleted = 1 WHERE oid = %u"), TABLE_NAME, m_nOid));
	}
	virtual OBJECTTYPE GetType(void){return wxEnumSphinxFile;};
	virtual OBJECTSTATE GetState(void){return m_state;};
	virtual int GetOID(void){return m_nOid;};
	virtual void SetState(OBJECTSTATE state){m_state = state;};
	virtual void SetModule(wxString sIndexModule){m_sIndexModule = sIndexModule;};
	virtual void DoWork(void)
	{
		//1 if wxSphinxNewState create row
		if(m_state == wxEnumSphinxNewState)
		{
			int nParentOID = m_pParent->GetOID();
			//terminating
			wxString sInsertPath = m_sPath;
			sInsertPath.Replace(wxT("'"), wxT("''"));
			m_pDB->ExecuteUpdate(wxString::Format(wxT("INSERT INTO %s (path, type, module, parent_id) VALUES ('%s', %u, '%s', %u)"), TABLE_NAME, sInsertPath.c_str(), wxEnumSphinxFile, m_sIndexModule.c_str(), nParentOID));
			m_nOid = m_pDB->GetLastRowId().ToLong();
		}
		//2 get changed time
		wxFileName name(m_sPath);
		wxDateTime dtAccess, dtMod, dtCreate;
		name.GetTimes(&dtAccess, &dtMod, &dtCreate);
		if(!m_dtChanged.IsValid() || dtMod > m_dtChanged)
		{
			m_dtChanged = dtMod;
			m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET dt_changed = %u WHERE oid = %u"), TABLE_NAME, m_dtChanged.GetTicks(), m_nOid));
		}
	}
protected:
	wxString m_sPath;
	long m_nOid;
	wxDateTime m_dtChanged, m_dtIndexed;
	wxSQLite3Database *m_pDB;
	wxSphinxObject* m_pParent;
	OBJECTSTATE m_state;
	wxString m_sIndexModule;
	IwxSphinxCatalog* m_pCatalog;
	//load store
};

typedef std::vector<wxSphinxObject*> wxObjectArray;

class IwxSphinxObjectFactory
{
public:
	IwxSphinxObjectFactory(void) : m_pCatalog(NULL){};
	virtual ~IwxSphinxObjectFactory(void){};
	virtual bool GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray) = 0;
	virtual void PutCatalogRef(IwxSphinxCatalog* pCatalog){m_pCatalog = pCatalog;};
	virtual wxString GetCData(wxString sPath){return wxString();};
	virtual void SetConfig(wxXmlNode* pConfNode) = 0;
protected:
	IwxSphinxCatalog* m_pCatalog;
};
