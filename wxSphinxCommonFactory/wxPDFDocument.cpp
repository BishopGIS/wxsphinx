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
#include "wxPDFDocument.h"
#include "wxPDFStream.h"
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>

/////////////////////////////////////////////////////////////////////////////
// wxPdfDocument
/////////////////////////////////////////////////////////////////////////////

wxPdfDocument::wxPdfDocument(void) : m_bIsOk(false), m_pFFileInputStream(NULL), m_pXRef(NULL)
{
}

wxPdfDocument::wxPdfDocument(const wxString& filename, const wxString& ownerpassword, const wxString& userpassword) : m_bIsOk(false), m_pFFileInputStream(NULL), m_pXRef(NULL)
{
	m_bIsOk = Load(filename, ownerpassword, userpassword);
}

wxPdfDocument::~wxPdfDocument(void)
{
	wxDELETE(m_pXRef);
	wxDELETE(m_pFFileInputStream);
}

bool wxPdfDocument::IsOk() const
{
	return m_bIsOk;
}

bool wxPdfDocument::Load(const wxString& filename, const wxString& ownerpassword, const wxString& userpassword)
{
	m_sFileName = filename;
	m_pFFileInputStream = new wxFFileInputStream(filename);
	if(!m_pFFileInputStream && !m_pFFileInputStream->IsOk())
	{
		wxLogError(_("Couldn't open file '%s'"), filename.c_str());
		return false;
	}
	if(!m_pFFileInputStream->IsSeekable())
	{
		wxLogError(_("Document base stream is not seekable, file '%s'"), filename.c_str());
		return false;
	}
	m_pFFileInputStream->Reset();
	//check header
	
	char hdrBuf[128];

	m_pFFileInputStream->Read(hdrBuf, 126);
	hdrBuf[127] = '\0';
	wxString sHead(hdrBuf, *wxConvCurrent, 128);
	wxInt32 nPos = 0;
	if((nPos = sHead.Find(wxT("%PDF-"))) == wxNOT_FOUND)
	{
		wxLogError(_("Not a PDF file '%s'"), filename.c_str());
		return false;
	}

	// read xref table
	m_pXRef = new wxPdfXRef(m_pFFileInputStream);
	if(!m_pXRef->IsOk())
		return false;
	return true;
}

wxString wxPdfDocument::GetText(void)
{
	if(m_pXRef == NULL)
		return wxEmptyString;
	if(!m_pXRef->IsOk())
		return wxEmptyString;
	wxString sResulText;
	for(size_t i = 0; i < m_pXRef->GetSize(); i++)
	{
		unsigned long nOffset = m_pXRef->GetOffset(i);
		m_pFFileInputStream->SeekI(nOffset);

		wxTextInputStream TextInputStream(*m_pFFileInputStream, wxT("\n"));
		wxString sLineText = TextInputStream.ReadLine();
		if(sLineText.Find(wxT("<<")) == wxNOT_FOUND)
			sLineText = TextInputStream.ReadLine();
		while(sLineText.Find(wxT("stream")) == wxNOT_FOUND && sLineText.Find(wxT("endobj")) == wxNOT_FOUND)
			sLineText += TextInputStream.ReadLine();

		if(sLineText.Find(wxT("stream")) == wxNOT_FOUND)
		{
			continue;
		}

		wxArrayString arr = wxStringTokenize(sLineText, wxT("/"));
		int nLength(-1);
		int nSize(-1);
		wxString sFilter;
		int nPredictor(1), nColors(1), nBitsPerComponent(8), nColumns(1), nEarlyChange(1);
		int nK(0), nRows(0), nDamagedRowsBeforeError(0);
		bool bEndOfLine(false), bEncodedByteAlign(false), bEndOfBlock(true), bBlackIs1(false);

		for(size_t i = 0; i < arr.size(); i++)
		{
			wxString sTemp = arr[i];
			sTemp.Replace(wxT("<<"), wxT(""));
			sTemp.Replace(wxT("["), wxT(""));
			sTemp.Replace(wxT("]"), wxT(""));
			int nPos = sTemp.Find(wxT(">>"));
			if(nPos != wxNOT_FOUND)
				sTemp = sTemp.Left(nPos);

			if(sTemp.Find(wxT("Encrypt")) != wxNOT_FOUND)
			{
				continue;
			}
			if(sTemp.Find(wxT("BlackIs1")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 9);
				bBlackIs1 = sTemp.CmpNoCase(wxT("true")) == 0 ? true : false;
				continue;
			}
			if(sTemp.Find(wxT("EndOfBlock")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 11);
				bEndOfBlock = sTemp.CmpNoCase(wxT("true")) == 0 ? true : false;
				continue;
			}
			if(sTemp.Find(wxT("EncodedByteAlign")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 17);
				bEncodedByteAlign = sTemp.CmpNoCase(wxT("true")) == 0 ? true : false;
				continue;
			}
			if(sTemp.Find(wxT("EndOfLine")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 10);
				bEndOfLine = sTemp.CmpNoCase(wxT("true")) == 0 ? true : false;
				continue;
			}
			if(sTemp.Find(wxT("K")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 2);
				nK = wxAtoi(sTemp);
				continue;
			}
			if(sTemp.Find(wxT("Rows")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 5);
				nRows = wxAtoi(sTemp);
				continue;
			}
			if(sTemp.Find(wxT("DamagedRowsBeforeError")) != wxNOT_FOUND)
			{
				sTemp = sTemp.Right(sTemp.Len() - 23);
				nDamagedRowsBeforeError = wxAtoi(sTemp);
				continue;
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
				if(sTemp.Find(wxT(" R")) != wxNOT_FOUND)
					nLength = -2;
				else
				{
					sTemp = sTemp.Right(sTemp.Len() - 7);
					nLength = wxAtoi(sTemp);
				}
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
		}
		if(nLength == -1)
			continue;
		wxString sTempStr;
		if(sFilter == wxString(wxT("RunLengthDecode")))
		{
			wxLogError(_("Unsupported filter [RunLengthDecode] in PDF file '%s'"), m_sFileName.c_str());
			continue;
		}
		else if(sFilter == wxString(wxT("FlateDecode")))
		{
			wxMemoryBuffer MemBuf;
			if(nLength == -2)
			{
				unsigned char Buff[10000];
				bool bStop(false);
				while(!bStop)
				{
					m_pFFileInputStream->Read(Buff, 4096);
					for(size_t i = 0; i < m_pFFileInputStream->LastRead(); i++)
					{
						if(Buff[i] == 'e')
						{//endstream
							if(i < m_pFFileInputStream->LastRead() - 4)
							{
								wxString sStr((char*)(Buff + i), *wxConvCurrent, 4);
								if(sStr.CmpNoCase(wxT("ends")) == 0)
								{
									bStop = true;
									break;
								}
							}
						}
						MemBuf.AppendByte(Buff[i]);
					}
				}
				nLength = MemBuf.GetDataLen();
			}
			else
			{
				unsigned char* pBuff = new unsigned char[nLength];
				m_pFFileInputStream->Read(pBuff, nLength);
				MemBuf.AppendData(pBuff, nLength);
				wxDELETEA(pBuff);
			}
			wxFlateStream stream((unsigned char*)MemBuf.GetData(), nLength, nPredictor, nColumns, nColors, nBitsPerComponent);
			//
			if(!stream.IsOk())
			{
				wxLogError(_("Error in filter [FlateDecode] in PDF file '%s'"), m_sFileName.c_str());
				continue;
			}
			int nSizeData = stream.GetDataLen();
			unsigned char* pBuf = (unsigned char*)stream.GetData();
			sTempStr = wxString((char*)pBuf, *wxConvCurrent, nSizeData);
		}
		else if(sFilter == wxString(wxT("LZWDecode")))
		{
			wxLogError(_("Unsupported filter [LZWDecode] in PDF file '%s'"), m_sFileName.c_str());
			continue;
		}
		else if(sFilter == wxString(wxT("ASCII85Decode")))
		{
			wxLogError(_("Unsupported filter [ASCII85Decode] in PDF file '%s'"), m_sFileName.c_str());
			continue;
		}
		else if(sFilter == wxString(wxT("ASCIIHexDecode")))
		{
			wxLogError(_("Unsupported filter [ASCIIHexDecode] in PDF file '%s'"), m_sFileName.c_str());
			continue;
		}
		else if(sFilter == wxString(wxT("DCTDecode")))
		{
			continue;
		}
		else if(sFilter == wxString(wxT("Crypt")))
		{
			continue;
		}
		else if(sFilter == wxString(wxT("JPXDecode")))
		{
			continue;
		}
		else if(sFilter == wxString(wxT("JBIG2Decode")))
		{
			continue;
		}
		else if(sFilter == wxString(wxT("CCITTFaxDecode")))
		{
			continue;
		}
		else 
		{
			unsigned char* pBuff = new unsigned char[nLength];
			m_pFFileInputStream->Read(pBuff, nLength);
			sTempStr = wxString((char*)pBuff, *wxConvCurrent, nLength);
			wxDELETEA(pBuff);
		}
		//wxLogDebug(sTempStr);
		if(sTempStr.Find(wxT("BT")) == wxNOT_FOUND)
			continue;
		sResulText.Append(ParseText(sTempStr));
		sResulText += wxT(" ");
	}
	return sResulText;
}

wxString wxPdfDocument::ParseText(wxString sText)
{
	wxString sResulText;
	int nOpenCount(0);
	for(size_t i = 0; i < sText.Len(); i++)
	{
		if(sText[i] == '\\' && i + 1 < sText.Len())
		{
			bool bOp = false;
			if(sText[i + 1] == '(')
			{
				sResulText += sText[i + 1];
				bOp = true;
			}
			else if(sText[i + 1] == ')')
			{
				sResulText += sText[i + 1];
				bOp = true;
			}
			else if(sText[i + 1] == '\\')
			{
				sResulText += sText[i + 1];
				bOp = true;
			}
			else 
			{
				wxString sSubStr(&sText[i + 1], 3);
				if(sSubStr.IsNumber())
				{
					//sResulText += wxChar(wxAtoi(sSubStr));
					bOp = true;
					i += 2;
				}
			}
			if(bOp)
			{
				i++;
				continue;
			}
		}
		if(sText[i] == '(')
		{
			nOpenCount = 1;
			continue;
		}
		if(sText[i] == ')')
		{
			nOpenCount = 0;
			continue;
		}
		if(nOpenCount > 0)
			sResulText += sText[i];
	}
	return sResulText;
}
