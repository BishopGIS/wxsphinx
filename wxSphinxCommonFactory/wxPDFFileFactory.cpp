/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxPdfFileFactory class. pdf
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
#include "wxPDFFileFactory.h"
#include "wxPDFDocument.h"

////////////////////////////////////////////////////////////////////////////////
// wxPDFFileFactory
////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(wxPDFFileFactory, wxObject)

wxPDFFileFactory::wxPDFFileFactory()
{
    //ctor
}

wxPDFFileFactory::~wxPDFFileFactory()
{
    //dtor
}

bool wxPDFFileFactory::GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray)
{
	for(size_t i = 0; i < pFileNames->GetCount(); i++)
	{
		wxString path = pFileNames->Item(i);
		wxFileName FileName(path);
		wxString ext = FileName.GetExt();
		ext.MakeLower();
        if(ext == wxString(wxT("pdf")))
        {
            wxSphinxObject* pObject = new wxSphinxObject(path, GetClassInfo()->GetClassName(), m_pCatalog);
            pObjArray->push_back(pObject);
            pFileNames->RemoveAt(i);
            i--;
            break;
		}
	}
	return true;
}

wxString wxPDFFileFactory::GetCData(wxString sPath)
{
	wxPdfDocument doc(sPath, wxEmptyString, wxEmptyString);
	//TerraSAR-X_FAQ_Version_4.pdf Отчет 3 этапа_Геленджик Краснодарское ЛПУМГ.pdf PDF32000_2008.pdf
	wxString out(wxT("empty"));
	if(doc.IsOk())
		out = doc.GetText();
	else
		wxLogError(_("Error parse PDF file: %s"), sPath.c_str());
	return out;
}

void wxPDFFileFactory::SetConfig(wxXmlNode* pConfNode)
{
}

