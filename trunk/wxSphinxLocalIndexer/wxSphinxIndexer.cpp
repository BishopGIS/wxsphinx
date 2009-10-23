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
#include "wxSphinxIndexer.h"
#include <map>
#include "wx/strconv.h"

wxSphinxIndexer::wxSphinxIndexer(bool bQuiet, wxXmlNode *pConf, wxSQLite3Database *pdb, wxSphinxFactories* pFactories)
{
	m_bQuiet = bQuiet;
	m_pDB = pdb;
	m_pConf = pConf;
    m_pFactories = pFactories;
}

wxSphinxIndexer::~wxSphinxIndexer(void)
{
}

bool wxSphinxIndexer::Start(long nMaxCount)
{
	std::vector<long> delete_arr;

	//1. create header
	wxFprintf(stdout, wxT("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"));
	wxFprintf(stdout, wxT("<sphinx:docset>\n\n"));
	wxFprintf(stdout, wxT("<sphinx:schema>\n"));
	wxFprintf(stdout, wxT("<sphinx:field name=\"content\"/>\n"));
	wxFprintf(stdout, wxT("<sphinx:attr name=\"accessed\" type=\"timestamp\"/>\n"));
	wxFprintf(stdout, wxT("<sphinx:attr name=\"last_updated\" type=\"timestamp\"/>\n"));
	wxFprintf(stdout, wxT("<sphinx:attr name=\"created\" type=\"timestamp\"/>\n"));
	wxFprintf(stdout, wxT("<sphinx:attr name=\"is_deleted\" type=\"bool\"/>\n"));
	wxFprintf(stdout, wxT("<sphinx:attr name=\"size\" type=\"int\"/>\n"));
	wxFprintf(stdout, wxT("</sphinx:schema>\n"));
	//2. connect DB
	wxSQLite3ResultSet set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT oid, path, dt_changed, dt_indexed, deleted, module FROM %s WHERE type = %u"), TABLE_NAME, wxEnumSphinxFile));
	//3. run throw DB recordset
	long nCounter = 0;
    while (set.NextRow())
	{
		long oid = set.GetInt(0, -1);
		wxString path = set.GetString(1);
		wxDateTime changed((time_t)set.GetInt(2));
		wxDateTime indexed((time_t)set.GetInt(3));
		bool deleted = set.GetBool(4);
		wxString module = set.GetString(5);

		IwxSphinxObjectFactory *Factory = m_pFactories->GetFactory(module);
		if(Factory == NULL)
			continue;

		wxFileName name(path);
		if(name.IsDir())
            continue;

        deleted = !wxFileName::FileExists( path );


		wxDateTime dtAccess, dtMod(wxDateTime::Now()), dtCreate;
		if(!deleted )
            name.GetTimes(&dtAccess, &dtMod, &dtCreate);
		if(!indexed.IsValid() || indexed < dtMod)
		{
		    if(nCounter == nMaxCount)
                break;
            //check long
            wxDateTime start = wxDateTime::Now();
            wxString out = Factory->GetCData(path);
            wxTimeSpan span = wxDateTime::Now() - start;

			if(out.IsEmpty() && out == wxString(wxT("empty")))
            {
                wxLogError(_("Error parse file (%u) '%s'"), oid, path.c_str());
                continue;
            }

			wxFprintf(stdout, wxT("<sphinx:document id=\"%u\">\n"), oid);
			if(deleted)
			{
				wxFprintf(stdout, wxT("<is_deleted>true</is_deleted>\n"), oid);
				//mark to delete row from db
				delete_arr.push_back(oid);
			}
			else
			{
			    wxLogMessage(_("Proceeding... document %u - path '%s'"), nCounter, path.c_str());

				wxFprintf(stdout, wxT("<is_deleted>false</is_deleted>\n"), oid);
				//check path if archive search ":"
				wxFprintf(stdout, wxT("<accessed>%u</accessed>\n"), dtAccess.GetTicks());
				wxFprintf(stdout, wxT("<last_updated>%u</last_updated>\n"), dtMod.GetTicks());
				wxFprintf(stdout, wxT("<created>%u</created>\n"), dtCreate.GetTicks());
				wxFprintf(stdout, wxT("<size>%u</size>\n"), name.GetSize().ToULong());
				wxFprintf(stdout, wxT("<content><![CDATA[%s]]></content>\n"), out.c_str());
				int nMaxWaitSec = wxAtoi(m_pConf->GetPropVal(wxT("maxwaitsec"), wxT("60")));
				if(span.GetSeconds().ToLong() > nMaxWaitSec)
				{
                    wxLogMessage(_("Max wait exceed (%u sec.)"), span.GetSeconds().ToLong());
                    try
                    {
                        m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET dt_indexed = %u, datatext = '%s' WHERE oid = %u"), TABLE_NAME, wxDateTime::Now().GetTicks(), out.c_str(), oid));//datatext nodedata
                    }
                    catch( wxSQLite3Exception &rErr )
                    {
                        wxLogError(rErr.GetMessage());
                    }
				}
				else
				{
				    m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET dt_indexed = %u WHERE oid = %u"), TABLE_NAME, wxDateTime::Now().GetTicks(), oid));
				}
			}
		//4. print out data
			wxFprintf(stdout, wxT("</sphinx:document>\n\n"));
			nCounter++;
		}
	}

	wxFprintf(stdout, wxT("</sphinx:docset>\n"));

	//delete oid's
	for(size_t i = 0; i < delete_arr.size(); i++)
	{
		m_pDB->ExecuteUpdate(wxString::Format(wxT("DELETE FROM %s WHERE oid = %u"), TABLE_NAME, delete_arr[i]));
	}

	return true;
}

bool wxSphinxIndexer::ShowDetails(long nIndex)
{
	//1. connect DB
	wxSQLite3ResultSet set = m_pDB->ExecuteQuery(wxString::Format(wxT("SELECT path, module, datatext FROM %s WHERE oid = %u"), TABLE_NAME, nIndex));
	//2. run throw DB recordset
	if(set.NextRow())
	{
		wxString path = set.GetString(0);
		wxString module = set.GetString(1);
		wxString datatext = set.GetString(2);

		IwxSphinxObjectFactory *pFactory = m_pFactories->GetFactory(module);
		if(pFactory == NULL)
			return false;

		wxFileName name(path);
		wxString sTitle = name.GetName();
		wxDateTime dtAccess, dtMod, dtCreate;
		name.GetTimes(&dtAccess, &dtMod, &dtCreate);
		wxString times = wxString::Format(_("Access %s, Modify %s, Create %s"), dtAccess.Format(_("%d-%m-%Y %H:%M:%S")).c_str(), dtMod.Format(_("%d-%m-%Y %H:%M:%S")).c_str(), dtCreate.Format(_("%d-%m-%Y %H:%M:%S")).c_str());

		bool bDataTextIsEmpty = datatext.IsEmpty();
		wxDateTime start = wxDateTime::Now();
		if(bDataTextIsEmpty)
		{
            datatext = pFactory->GetCData(path);
		}

        wxTimeSpan span = wxDateTime::Now() - start;

        int nMaxWaitSec = wxAtoi(m_pConf->GetPropVal(wxT("maxwaitsec"), wxT("60")));
        if(bDataTextIsEmpty && span.GetSeconds().ToLong() > nMaxWaitSec)
        {
            wxLogMessage(_("Max wait exceed (%u sec.)"), span.GetSeconds().ToLong());
            try
            {
                m_pDB->ExecuteUpdate(wxString::Format(wxT("UPDATE %s SET datatext = '%s' WHERE oid = %u"), TABLE_NAME, datatext.c_str(), nIndex));
            }
            catch( wxSQLite3Exception &rErr )
            {
                wxLogError(rErr.GetMessage());
            }
        }

        int nMaxSize = wxAtoi(m_pConf->GetPropVal(wxT("maxtext"), wxT("2048")));
        if(datatext.Len() > nMaxSize)
            datatext = datatext.Left(nMaxSize);

		wxFprintf(stdout, wxT("%s\n%s\n%s\n%s\n"), path.c_str(), sTitle.c_str(), times.c_str(), datatext.c_str());
	}

    return true;
}

