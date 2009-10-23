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
#include "wxSphinxFactories.h"

wxSphinxFactories::wxSphinxFactories(wxXmlNode* pConf)
{
    LoadObjectFactories(pConf);
}

wxSphinxFactories::~wxSphinxFactories(void)
{
    EmptyObjectFactories();
}


void wxSphinxFactories::LoadObjectFactories(wxXmlNode* pNode)
{
	if(pNode == NULL)
		return;

	wxXmlNode* pChildren = pNode->GetChildren();
	while(pChildren)
	{
		if(pChildren->GetName() == wxT("factory"))
		{
			wxString sName = pChildren->GetPropVal(wxT("factory_name"), wxT(""));
			if(!sName.IsEmpty())
			{
				wxObject *obj = wxCreateDynamicObject(sName);
				IwxSphinxObjectFactory *Factory = dynamic_cast<IwxSphinxObjectFactory*>(obj);
				if(Factory != NULL)
				{
					Factory->SetConfig(pChildren);
					OBJFACTORYDESC desc = {sName, Factory, bool(wxAtoi(pChildren->GetPropVal(wxT("is_enabled"), wxT("1"))))};
					m_ObjectFactoriesArray.push_back( desc );
					wxLogMessage(_("wxSphinxUpdater: ObjectFactory %s initialize"), sName.c_str());
					//plugin->Init(child);
				}
				else
					wxLogError(_("wxSphinxUpdater: Error initializing ObjectFactory %s"), sName.c_str());
			}
		}
		pChildren = pChildren->GetNext();
	}
}

void wxSphinxFactories::EmptyObjectFactories(void)
{
	for(size_t i = 0; i < m_ObjectFactoriesArray.size(); i++)
	{
		wxDELETE(m_ObjectFactoriesArray[i].pFactory);
	}
	m_ObjectFactoriesArray.empty();
}

IwxSphinxObjectFactory* wxSphinxFactories::GetFactory(wxString sName)
{
    for(size_t i = 0; i < m_ObjectFactoriesArray.size(); i++)
    {
        if(m_ObjectFactoriesArray[i].sName == sName && m_ObjectFactoriesArray[i].bIsEnabled == true)
            return m_ObjectFactoriesArray[i].pFactory;
    }
    return NULL;
}

void wxSphinxFactories::PutCatalogRef(IwxSphinxCatalog* pCatalog)
{
    m_pCatalog = pCatalog;
    for(size_t i = 0; i < m_ObjectFactoriesArray.size(); i++)
    {
        m_ObjectFactoriesArray[i].pFactory->PutCatalogRef(m_pCatalog);
    }
};
					
