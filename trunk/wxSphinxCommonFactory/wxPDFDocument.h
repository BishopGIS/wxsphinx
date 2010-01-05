/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxPdfDocument class. for read purposes only
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

#include "wxpdfxref.h"

/////////////////////////////////////////////////////////////////////////////
// wxPdfDocument
/////////////////////////////////////////////////////////////////////////////

class wxPdfDocument :
	public wxObject
{
public:
	wxPdfDocument(void);
	wxPdfDocument(const wxString& filename, const wxString& ownerpassword = wxEmptyString, const wxString& userpassword = wxEmptyString);
	virtual ~wxPdfDocument(void);
	bool IsOk() const;
	bool Load(const wxString& filename, const wxString& ownerpassword, const wxString& userpassword);
	wxString GetText(void);
	wxString ParseText(wxString sText);
protected:
	bool m_bIsOk;
	wxFFileInputStream* m_pFFileInputStream;
	wxPdfXRef* m_pXRef;
	wxString m_sFileName;
};