/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxFolderFactory class.
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
#include "wxFolderFactory.h"
#include <wx/filename.h>
#include <wx/dir.h>

#include "../wxSphinxCommon/wxContainer.h"

IMPLEMENT_DYNAMIC_CLASS(wxFolderFactory, wxObject)

wxFolderFactory::wxFolderFactory(void)
{
}

wxFolderFactory::~wxFolderFactory(void)
{
}

bool wxFolderFactory::GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray)
{
	for(size_t i = 0; i < pFileNames->GetCount(); i++)
	{
		wxString path = pFileNames->Item(i);
		if(wxFileName::DirExists(path))
		{
			//wxString name;
			//wxFileName::SplitPath(path, NULL, NULL, &name, NULL);
			wxSphinxContainer* pFolder = new wxSphinxContainer(path, GetClassInfo()->GetClassName(), m_pCatalog);
			pObjArray->push_back(static_cast<wxSphinxObject*>(pFolder));
			pFileNames->RemoveAt(i);
			i--;
		}
	}
	return true;
}
