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
#include "wxPDFXref.h"
#include "wxPDFStream.h"
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>

/////////////////////////////////////////////////////////////////////////////
// wxPdfXRef
/////////////////////////////////////////////////////////////////////////////

wxPdfXRef::wxPdfXRef(void)
{
	m_bIsOk = true;
	m_nErrCode = ErrNone;
	//m_pStreamEnds = NULL;
	//m_nStreamEndsLen = 0;
//	m_pObjStr = NULL;
}

wxPdfXRef::wxPdfXRef(wxFFileInputStream *pStream)
{
	m_bIsOk = true;
	m_nErrCode = ErrNone;
	//m_pStreamEnds = NULL;
	//m_nStreamEndsLen = 0;
//	m_pObjStr = NULL;

	//m_bEncrypted = false;

	// read the trailer
	m_pStream = pStream;
	m_pStream->Reset();
	m_Offset = GetStartXref();

	// if there was a problem with the 'startxref' position, try to reconstruct the xref table
	if(m_Offset == 0)
	{
		if (!(m_bIsOk = ConstructXRef()))
		{
			m_nErrCode = ErrDamaged;
			return;
		}
	}	// read the xref table
	else
	{
		m_bIsOk = ReadXRef();

		// if there was a problem with the xref table, try to reconstruct it
		if (!m_bIsOk)
		{
			if (!(m_bIsOk = ConstructXRef()))
			{
				m_nErrCode = ErrDamaged;
				return;
			}
		}
	}
}

wxPdfXRef::~wxPdfXRef(void)
{
}

// Read the 'startxref' position.
wxUint32 wxPdfXRef::GetStartXref(void)
{
	char buf[1024];
	wxFileOffset nOff = m_pStream->SeekI(m_pStream->GetLength() - 1022);
	m_pStream->Read(buf, 1022);
	buf[1023] = '\0';
	wxString sData(buf, *wxConvCurrent, 1024);

	wxInt32 nPos = 0;
	if((nPos = sData.Find(wxT("startxref"))) == wxNOT_FOUND)
		return 0;
	nPos += 9;
	sData= sData.Right(sData.Len() - nPos);
	wxArrayString arr = wxStringTokenize(sData);
	if(arr.size() == 0)
		return 0;
	for(size_t i = 0; i < arr.size(); i++)
	{
		if(arr[i].IsNumber())
			return wxAtoi(arr[i]);
	}
	return 0;
}

// Attempt to construct an xref table for a damaged file.
bool wxPdfXRef::ConstructXRef(void)
{
	wxLogMessage(_("PDF file is damaged - attempting to reconstruct xref table..."));
	m_Entries.clear();
	m_pStream->SeekI(0);
	wxTextInputStream TextInputStream(*m_pStream);
	while(!m_pStream->Eof())
	{
		wxString sLineText = TextInputStream.ReadLine();
		if(sLineText.Find(wxT(" obj")) != wxNOT_FOUND)
		{
			off_t nOffset = m_pStream->TellI();
			nOffset -= sLineText.Len();
			AddOffset(nOffset);
		}
	}
	return m_Entries.size() > 0;
}

//// Read one xref table section.  Also reads the associated trailer
//// dictionary, and returns the prev pointer (if any).
bool wxPdfXRef::ReadXRef(void)
{
	//1.Check type of xref
	while(m_Offset != -1)
	{
		m_pStream->SeekI(m_Offset);
		char buff[10];
		m_pStream->Read(buff, 8);
		buff[9] = '\0';
		wxString sData(buff, *wxConvCurrent, 10);
		if(sData.Find(wxT("xref")) != wxNOT_FOUND)
		{
			if(!ReadXRefTable(m_Offset))
				return false;
		}
		else
		{
			if(!ReadXRefStream(m_Offset))
				return false;
		}
	}
	return true;
}

bool wxPdfXRef::ReadXRefTable(wxUint32 nOff)
{
	m_Offset = -1;
	m_pStream->SeekI(nOff);
	wxTextInputStream TextInputStream(*m_pStream, wxT("\n"));
	wxString sLineText = TextInputStream.ReadLine();
	if(sLineText != wxString(wxT("xref")))
		return false;
	sLineText = TextInputStream.ReadLine();
	while(1)
	{
		int nPos = sLineText.Find(wxT(" "));
		if(nPos == wxNOT_FOUND)
			return false;
		int nCount = wxAtoi(sLineText.Right(sLineText.Len() - nPos));
		for(size_t i = 0; i < nCount; i++)
		{
			sLineText = TextInputStream.ReadLine();
			wxArrayString arr = wxStringTokenize(sLineText, wxT(" "));
			if(arr.size() != 3)
				return false;
			if(arr[2].CmpNoCase(wxT("n")) == 0)
				AddOffset(wxAtol(arr[0]));
		}
		sLineText = TextInputStream.ReadLine();
		if(sLineText == wxString(wxT("trailer")))
			break;
	}
	//read XRefStm or Prev
	sLineText.Empty();
	while(sLineText.Find(wxT(">>")) == wxNOT_FOUND)
		sLineText += TextInputStream.ReadLine();

	long nXRefStm(-1), nPrev(-1);
	wxArrayString arr = wxStringTokenize(sLineText, wxT("/"));
	for(size_t i = 0; i < arr.size(); i++)
	{
		if(arr[i].Find(wxT("XRefStm")) != wxNOT_FOUND)
		{
			wxString sTemp = arr[i].Right(arr[i].Len() - 8);
			nXRefStm = wxAtol(sTemp);
			continue;
		}
		if(arr[i].Find(wxT("Prev")) != wxNOT_FOUND)
		{
			wxString sTemp = arr[i].Right(arr[i].Len() - 5);
			nPrev = wxAtol(sTemp);
			continue;
		}
	}
	if(nXRefStm != -1)
		if(!ReadXRefStream(nXRefStm))
			return false;
	m_Offset = nPrev;
	return true;
}

bool wxPdfXRef::ReadXRefStream(wxUint32 nOff)
{
	m_Offset = -1;
	m_pStream->SeekI(nOff);
	wxTextInputStream TextInputStream(*m_pStream, wxT("\n"));
	wxString sLineText = TextInputStream.ReadLine();
	if(sLineText.Find(wxT("<<")) == wxNOT_FOUND)
		sLineText = TextInputStream.ReadLine();
	while(sLineText.Find(wxT("stream")) == wxNOT_FOUND && sLineText.Find(wxT("endobj")) == wxNOT_FOUND)
		sLineText += TextInputStream.ReadLine();

	wxArrayString arr = wxStringTokenize(sLineText, wxT("/"));
	int nLength(-1);
	int nSize(-1);
	wxString sFilter;
	long nPrev(-1);
	int WArr[3] = {-1,-1,-1};
	int nPredictor(1), nColors(1), nBitsPerComponent(8), nColumns(1), nEarlyChange(-1);

	for(size_t i = 0; i < arr.size(); i++)
	{
		wxString sTemp = arr[i];
		sTemp.Replace(wxT("<<"), wxT(""));
		//sTemp.Replace(wxT("["), wxT(""));
		//sTemp.Replace(wxT("]"), wxT(""));
		int nPos = sTemp.Find(wxT(">>"));
		if(nPos != wxNOT_FOUND)
			sTemp = sTemp.Left(nPos);

		if(sTemp.Find(wxT("W")) != wxNOT_FOUND)
		{
			int nPos = sTemp.Find(wxT("]"));
			if(nPos != wxNOT_FOUND)
				sTemp = sTemp.Left(nPos);
			sTemp = sTemp.Remove(0, 2);
			wxArrayString swarr = wxStringTokenize(sTemp, wxT(" "));
			int nSize = swarr.size() > 3 ? 3 : swarr.size();
			for(size_t i = 0; i < nSize; i++)
				WArr[i] = wxAtoi(swarr[i]);
			continue;
		}
		if(sTemp.Find(wxT("Encrypt")) != wxNOT_FOUND)
		{
			//continue;
			wxLogError(_("Encrypt is unsupported"));
			return false;
		}
		if(sTemp.Find(wxT("Filter")) != wxNOT_FOUND)
		{
			sTemp = arr[i + 1];
			sTemp.Replace(wxT("<<"), wxT(""));
			sTemp.Replace(wxT("["), wxT(""));
			sTemp.Replace(wxT("]"), wxT(""));
			int nPos = sTemp.Find(wxT(">>"));
			if(nPos != wxNOT_FOUND)
				sTemp = sTemp.Left(nPos);

			sFilter = sTemp;
			sFilter.Trim(false);
			sFilter.Trim(true);
			continue;
		}
		if(sTemp.Find(wxT("Length")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 7);
			nLength = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("Size")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 5);
			nSize = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("Predictor")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 10);
			nPredictor = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("Colors")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 7);
			nColors = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("BitsPerComponent")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 17);
			nBitsPerComponent = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("Columns")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 8);
			nColumns = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("EarlyChange")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 12);
			nEarlyChange = wxAtoi(sTemp);
			continue;
		}
		if(sTemp.Find(wxT("Prev")) != wxNOT_FOUND)
		{
			sTemp = sTemp.Right(sTemp.Len() - 5);
			m_Offset = wxAtol(sTemp);
			continue;
		}
	}
	if(nLength == -1)
		return false;

	if(sFilter == wxString(wxT("RunLengthDecode")))
	{
		wxLogError(_("Unsupported filter [RunLengthDecode] in XRef of PDF file"));
	}
	else if(sFilter == wxString(wxT("FlateDecode")))
	{
		unsigned char* pBuff = new unsigned char[nLength];
		m_pStream->Read(pBuff, nLength);
		wxFlateStream stream(pBuff, nLength, nPredictor, nColumns, nColors, nBitsPerComponent);
		wxDELETEA(pBuff);
		if(!stream.IsOk())
		{
			wxLogError(_("Error in filter [FlateDecode] in XRef of PDF file"));
			return false;
		}
		int nSizeData = stream.GetDataLen();
		int nLineSize = WArr[0] + WArr[1] + WArr[2];
		int nCount = nSizeData / nLineSize;
		unsigned char* pBuf = (unsigned char*)stream.GetData();
		for(size_t i = 0; i < nCount; i++)
		{
			long val0 = GetAsLong(pBuf, WArr[0]);
			pBuf += WArr[0];
			long val1 = GetAsLong(pBuf, WArr[1]);
			pBuf += WArr[1];
			long val2 = GetAsLong(pBuf, WArr[2]);
			pBuf += WArr[2];
			if(val0 == 1)
			{
				AddOffset(val1);
			}
		}
	}
	else if(sFilter == wxString(wxT("LZWDecode")))
	{
		wxLogError(_("Unsupported filter [LZWDecode] in XRef of PDF file"));
	}
	else if(sFilter == wxString(wxT("ASCII85Decode")))
	{
		wxLogError(_("Unsupported filter [ASCII85Decode] in XRef of PDF file"));
	}
	else if(sFilter == wxString(wxT("ASCIIHexDecode")))
	{
		wxLogError(_("Unsupported filter [ASCIIHexDecode] in XRef of PDF file"));
	}
	else 
	{
		wxLogError(_("Unsupported filter [?] in XRef of PDF file"));
		//m_sStreamData = sFullData;
	}

	return true;
}

void wxPdfXRef::AddOffset(long nOffset)
{
	for(size_t i = 0; i < m_Entries.size(); i++)
		if(m_Entries[i] == (unsigned long)nOffset)
			return;
	m_Entries.push_back((unsigned long)nOffset);
}

long wxPdfXRef::GetAsLong(unsigned char* pBuf, char nSize)
{
	if(nSize == 0)
		return 1;
	int val = 0;
	for(size_t i = 0; i < nSize; i++)
		val = (val << 8) + (int)pBuf[i];
	return val;
}
