/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxMLFileFactory class. XML, HTML, SVG and so
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
#include "wxMLFileFactory.h"
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/tokenzr.h>

#include "../wxSphinxCommon/wxContainer.h"

IMPLEMENT_DYNAMIC_CLASS(wxMLFileFactory, wxObject)

wxMLFileFactory::wxMLFileFactory()
{
    //ctor
}

wxMLFileFactory::~wxMLFileFactory()
{
    //dtor
}

bool wxMLFileFactory::GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray)
{
	for(size_t i = 0; i < pFileNames->GetCount(); i++)
	{
		wxString path = pFileNames->Item(i);
		wxFileName FileName(path);
		wxString ext = FileName.GetExt();
		ext.MakeLower();
		for(size_t j = 0; j < m_sExtArray.size(); j++)
		{
            if(ext == m_sExtArray[j] )
            {
                wxSphinxObject* pObject = new wxSphinxObject(path, GetClassInfo()->GetClassName(), m_pCatalog);
                pObjArray->push_back(pObject);
                pFileNames->RemoveAt(i);
                i--;
                break;
            }
		}
	}
	return true;
}

wxString wxMLFileFactory::GetCData(wxString sPath)
{
	wxString out;
	wxXmlDocument doc;
	if (!doc.Load(sPath))
		return out;

	wxXmlNode *child = doc.GetRoot()->GetChildren();
	while (child)
	{
		GetNodeContent(child, out);
		child = child->GetNext();
	}
	return CleanOutput(out);
}

void wxMLFileFactory::SetConfig(wxXmlNode* pConfNode)
{
    wxString sExts = pConfNode->GetPropVal(wxT("exts"), wxT(""));
    wxStringTokenizer tkz(sExts, wxString(wxT(",")), false);
    while( tkz.HasMoreTokens() )
    {
        wxString token = tkz.GetNextToken();
        token.Replace(wxT(","), wxT(""));
        token.MakeLower();
        token.Trim(true); token.Trim(false);
        m_sExtArray.Add(token);
    }
}

void wxMLFileFactory::GetNodeContent(wxXmlNode* pNode, wxString& sStr)
{
	wxString sContent = pNode->GetNodeContent();
	if(sContent.Len() < 1000 || sContent.Find(wxT(" ")) != wxNOT_FOUND)
		sStr += wxT(" ") + sContent;
	wxXmlNode* pChildNode = pNode->GetChildren();
	while(pChildNode)
	{
		GetNodeContent(pChildNode, sStr);
		pChildNode = pChildNode->GetNext();
	}
}

wxString wxMLFileFactory::CleanOutput(wxString sOutString)
{
	wxString out = sOutString;
    out = out.Left(1990000);
    out.Replace(wxT("]"), wxT(")"), true);
    out.Replace(wxT("["), wxT(")"), true);
    out.Replace(wxT("\""), wxT(" "), true);
    out.Replace(wxT("'"), wxT(" "), true);
//    out.Replace(wxT("&amp;"), wxT("&"), true);
//    out.Replace(wxT("&quot;"), wxT("\""), true);
//    out.Replace(wxT("&nbsp;"), wxT(" "), true);
//	out.Replace(wxT("&lt;"), wxT("("), true);
//	out.Replace(wxT("&gt;"), wxT(")"), true);
//	out.Replace(wxT("\t"), wxT(" "), true);
//	out.Replace(wxT("\n"), wxT(" "), true);
//	out.Replace(wxT("  "), wxT(" "), true);

	return out;
}
