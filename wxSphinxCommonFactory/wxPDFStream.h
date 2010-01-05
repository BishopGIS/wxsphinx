/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  Stream classes. for pdf parsing
 * Author:   Bishop (aka Barishnikov Dmitriy), polimax@mail.ru
 ******************************************************************************
*   Copyright (C) 2009 - 2010  Bishop
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
#include <wx/zipstrm.h>
#include <wx/buffer.h>
#include <zlib.h>

//////////////////////////////////////////////////////////
// wxFlateStream
//////////////////////////////////////////////////////////
class wxFlateStream
{
public:
	wxFlateStream(unsigned char* pBuff, wxUint32 nLen, int nPredictor = 1, int nColumns = 1, int nColors = 1, int nBits = 8);
	virtual ~wxFlateStream();
	virtual bool IsOk() { return m_bIsOk; }
	virtual void* GetData(void){return m_Buff.GetData();}
	virtual size_t GetDataLen(void){return m_Buff.GetDataLen();}
private:
	z_stream m_ZipStream;
	wxMemoryBuffer m_Buff;
	bool m_bIsOk;
};

//////////////////////////////////////////////////////////
// wxStreamPredictor
//////////////////////////////////////////////////////////

class wxStreamPredictor
{
public:
	wxStreamPredictor(wxMemoryBuffer Buff, int nPredictor = 1, int nColumns = 1, int nColors = 1, int nBits = 8);
	virtual ~wxStreamPredictor();
	virtual bool IsOk() { return m_bIsOk; }
	virtual wxMemoryBuffer GetMemmBuffer(void){return m_Buff;}
private:
	wxMemoryBuffer m_Buff;
	bool m_bIsOk;
};
//////////////////////////////////////////////////////////
// wxLZWStream
//////////////////////////////////////////////////////////
//
//class wxLZWStream
//{
//public:
//	wxLZWStream(unsigned char* pBuff, wxUint32 nLen, int nPredictor = 1, int nColumns = 1, int nColors = 1, int nBits = 8, int nEarly = 1);
//	virtual ~wxLZWStream();
//	virtual bool IsOk() { return m_bIsOk; }
////	virtual wxMemoryBuffer GetMemmBuffer(void){return m_Buff;}
//private:
////	wxMemoryBuffer m_Buff;
//	bool m_bIsOk;
//}

//////////////////////////////////////////////////////////
// wxCCITTFaxStream
//////////////////////////////////////////////////////////

//class wxCCITTFaxStream
//{
//public:
//	wxCCITTFaxStream(unsigned char* pBuff, wxUint32 nLen, int nEncoding = 0, bool bEndOfLine = false, bool bByteAlign = false, int nColumns = 1728, int nRows = 0, bool bEndOfBlock = true, bool bBlack = false);
//	virtual ~wxCCITTFaxStream();
//	virtual bool IsOk() { return m_bIsOk; }
////	virtual wxMemoryBuffer GetMemmBuffer(void){return m_Buff;}
//	virtual short GetTwoDimCode(unsigned char* pBuff, int nLen, bool bEndOfBlock, int nInputBits);
//	virtual short GetWhiteCode();
//	virtual short GetBlackCode();
//	virtual short LookBits(unsigned char* pBuff, int nLen, int n, int nInputBits);
//private:
////	wxMemoryBuffer m_Buff;
//	bool m_bIsOk;
//}