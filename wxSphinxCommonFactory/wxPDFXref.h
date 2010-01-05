/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxPdfXRef class. xref of pdf file
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
#include <wx/wfstream.h>

enum wxPdfErrorCodes
{
	ErrNone = 0,		// no error
	ErrOpenFile,		// couldn't open the PDF file
	ErrBadCatalog,		// couldn't read the page catalog
	ErrDamaged,			// PDF file was damaged and couldn't be repaired
	ErrEncrypted,		// file was encrypted and password was incorrect or not supplied
	ErrHighlightFile,	// nonexistent or invalid highlight file
	ErrBadPrinter,		// invalid printer
	ErrPrinting,		// error during printing
	ErrPermission,		// PDF file doesn't allow that operation
	ErrBadPageNum,		// invalid page number
	ErrFileIO			// file I/O error
};

/////////////////////////////////////////////////////////////////////////////
// wxPdfXRef
/////////////////////////////////////////////////////////////////////////////

class wxPdfXRef 
{
public:
	// Constructor, create an empty XRef, used for PDF writing
	wxPdfXRef(void);
	// Constructor.  Read xref table from stream.
	wxPdfXRef(wxFFileInputStream *pStream);
	// Destructor.
	virtual ~wxPdfXRef(void);

	// Is xref table valid?
	virtual bool IsOk() { return m_bIsOk; }

	// Get the error code (if isOk() returns false).
	wxInt32 GetErrorCode() { return m_nErrCode; }
	// Is the file encrypted?
	//bool IsEncrypted() { return m_bEncrypted; }
	// Direct access.
	wxInt32 GetSize() { return m_Entries.size(); }
	unsigned long GetOffset(int nIndex) { return m_Entries[nIndex]; }
	//PdfObject *GetTrailerDict() { return &m_TrailerDict; }

protected:
	wxFFileInputStream *m_pStream;		// input stream
	std::vector<unsigned long> m_Entries;	// xref entries
	bool m_bIsOk;							// true if xref table is valid
	wxInt32 m_nErrCode;					// error code (if <ok> is false)
	wxInt32 m_Offset;

protected:
	wxUint32 GetStartXref(void);
	bool ReadXRef(void);
	bool ReadXRefTable(wxUint32 nOff);
	bool ReadXRefStream(wxUint32 nOff);
	bool ConstructXRef(void);
	void AddOffset(long nOffset);
	long GetAsLong(unsigned char* pBuf, char nSize);
};
