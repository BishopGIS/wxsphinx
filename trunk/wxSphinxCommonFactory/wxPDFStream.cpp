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
#include "wxPDFStream.h"

//------------------------------------------------------------------------
// wxFlateStream
//------------------------------------------------------------------------


wxFlateStream::wxFlateStream(unsigned char* pBuff, wxUint32 nLen, int nPredictor, int nColumns, int nColors, int nBits) : m_bIsOk(false)
{
	memset(&m_ZipStream, 0, sizeof(m_ZipStream));
	unsigned char out_buf[4096];
	wxMemoryBuffer Buff;
	if(Z_OK == inflateInit(&m_ZipStream))
	{
		m_ZipStream.next_in = pBuff;
		m_ZipStream.avail_in = nLen;
		while(1)
		{
			m_ZipStream.avail_out = sizeof(out_buf);
			m_ZipStream.next_out = out_buf;

			int status = inflate(&m_ZipStream, Z_SYNC_FLUSH);
			if(status == Z_OK || status == Z_STREAM_END)
			{
				Buff.AppendData(out_buf, sizeof(out_buf) - m_ZipStream.avail_out);
			}
			else
			{
				return;
			}
			if(status == Z_STREAM_END)
				break;
		}
	}
	//decode
	if(nPredictor != 1)
	{
		wxStreamPredictor predict(Buff, nPredictor, nColumns, nColors, nBits);
		if(predict.IsOk())
		{
			m_Buff = predict.GetMemmBuffer();
		}
		else return;
	}
	else
		m_Buff = Buff;
	m_bIsOk = true;
}

wxFlateStream::~wxFlateStream()
{
  inflateEnd(&m_ZipStream);
}

//------------------------------------------------------------------------
// wxStreamPredictor
//------------------------------------------------------------------------

wxStreamPredictor::wxStreamPredictor(wxMemoryBuffer Buff, int nPredictor, int nColumns, int nColors, int nBits) : m_bIsOk(false)
{
	int nVals = nColumns * nColors;
	int nPixBytes = (nColors * nBits + 7) >> 3;
	int nRowBytes = ((nVals * nBits + 7) >> 3) + nPixBytes;
	if (nColumns <= 0 || nColors <= 0 || nBits <= 0 || nColors > 32 || nBits > 16 || nColumns >= INT_MAX / nColors || nVals >= (INT_MAX - 7) / nBits)
	{ 
		return;
	}

	unsigned char* pBuff = (unsigned char*)Buff.GetData();

	unsigned char* pPredLine = new unsigned char[nRowBytes];
	memset(pPredLine, 0, nRowBytes);
	int nPredIdx = nRowBytes;
	// read the raw line, apply PNG (byte) predictor
	unsigned char upLeftBuf[65];//32 * 2 + 1

	int nCounter = 0;
	for(int m = 0; nCounter < Buff.GetDataLen(); m += (nRowBytes - nPixBytes))
	{
		int nCurPred;
		// get PNG optimum predictor number
		if (nPredictor >= 10)
		{
			nCurPred = pBuff[nCounter];
			nCounter++;
			nCurPred += 10;
		}
		else
		{
			nCurPred = nPredictor;
		}
		memset(upLeftBuf, 0, nPixBytes + 1);
		for(int i = nPixBytes; i < nRowBytes; ++i)
		{
			for (int j = nPixBytes; j > 0; --j)
			{
				upLeftBuf[j] = upLeftBuf[j-1];
			}
			upLeftBuf[0] = pPredLine[i];
			unsigned char c = pBuff[nCounter];
			nCounter++;
			if(nCounter >= Buff.GetDataLen())
			{
				//if (i > nPixBytes)
				//{
				//	// this ought to return false, but some (broken) PDF files
				//	// contain truncated image data, and Adobe apparently reads the
				//	// last partial line
				//	break;
				//}
				//return;
				break;
			}
			switch (nCurPred)
			{
			case 11:			// PNG sub
				pPredLine[i] = pPredLine[i - nPixBytes] + c;
			break;
			case 12:			// PNG up
				pPredLine[i] = pPredLine[i] + c;
			break;
			case 13:			// PNG average
				pPredLine[i] = ((pPredLine[i - nPixBytes] + pPredLine[i]) >> 1) + c;
			break;
			case 14:			// PNG Paeth
				{
					int left = pPredLine[i - nPixBytes];
					int up = pPredLine[i];
					int upLeft = upLeftBuf[nPixBytes];
					int p = left + up - upLeft;
					int pa, pb, pc;
					if ((pa = p - left) < 0) pa = -pa;
					if ((pb = p - up) < 0) pb = -pb;
					if ((pc = p - upLeft) < 0) pc = -pc;
					if (pa <= pb && pa <= pc) pPredLine[i] = left + c;
					else if (pb <= pc) pPredLine[i] = up + c;
					else pPredLine[i] = upLeft + c;
				}
			break;
		case 10:			// PNG none
		default:			// no predictor or TIFF predictor
			pPredLine[i] = c;
			break;
			}
		}

		// apply TIFF (component) predictor
		if (nPredictor == 2)
		{
			if (nBits == 1)
			{
				unsigned long nInBuf = pPredLine[nPixBytes - 1];
				for(int i = nPixBytes; i < nRowBytes; i += 8)
				{
					// 1-bit add is just xor
					nInBuf = (nInBuf << 8) | pPredLine[i];
					pPredLine[i] ^= nInBuf >> nColors;
				}
			}
			else if (nBits == 8)
			{
				for(int i = nPixBytes; i < nRowBytes; ++i)
				{
					pPredLine[i] += pPredLine[i - nColors];
				}
			}
			else
			{
				memset(upLeftBuf, 0, nColors + 1);
				unsigned long nBitMask = (1 << nBits) - 1;
				unsigned long nInBuf(0), nOutBuf(0);
				int inBits(0), outBits(0);
				int j(nPixBytes), k(nPixBytes);
				for(int i = 0; i < nColumns; ++i)
				{
					for(int kk = 0; kk < nColors; ++kk)
					{
						if(inBits < nBits)
						{
							nInBuf = (nInBuf << 8) | (pPredLine[j++] & 0xff);
							inBits += 8;
						}
						upLeftBuf[kk] = ((upLeftBuf[kk] + (nInBuf >> (inBits - nBits))) & nBitMask);
						inBits -= nBits;
						nOutBuf = (nOutBuf << nBits) | upLeftBuf[kk];
						outBits += nBits;
						if(outBits >= 8)
						{
							pPredLine[k++] = (nOutBuf >> (outBits - 8));
							outBits -= 8;
						}
					}
				}
				if(outBits > 0)
				{
					pPredLine[k++] = ((nOutBuf << (8 - outBits)) + (nInBuf & ((1 << (8 - outBits)) - 1)));
				}
			}
		}
		m_Buff.AppendData((void*)(pPredLine + nPixBytes), (size_t)(nRowBytes - nPixBytes));
	}

	//// reset to start of line
	//predIdx = pixBytes;
	m_bIsOk = true;
	//return gTrue;
}

wxStreamPredictor::~wxStreamPredictor()
{	
}



//------------------------------------------------------------------------
// wxLZWStream
//------------------------------------------------------------------------

//wxLZWStream::wxLZWStream(unsigned char* pBuff, wxUint32 nLen, int nPredictor, int nColumns, int nColors, int nBits, int nEarly) : m_bIsOk(false)
//{
//
//  if(nPredictor != 1)
//  {
//    pred = new StreamPredictor(this, predictor, columns, colors, bits);
//    if (!pred->isOk()) {
//      delete pred;
//      pred = NULL;
//    }
//  } else {
//    pred = NULL;
//  }
//  early = earlyA;
//  eof = gFalse;
//  inputBits = 0;
//  clearTable();
//}
//
//GBool LZWStream::processNextCode() {
//	int code;
//	int nextLength;
//	int i, j;
//
//	// check for EOF
//	if (eof) {
//		return gFalse;
//	}
//
//	// check for eod and clear-table codes
//start:
//	code = getCode();
//	if (code == EOF || code == 257) {
//		eof = gTrue;
//		return gFalse;
//	}
//	if (code == 256) {
//		clearTable();
//		goto start;
//	}
//	if (nextCode >= 4097) {
//		error(getPos(), "Bad LZW stream - expected clear-table code");
//		clearTable();
//	}
//
//	// process the next code
//	nextLength = seqLength + 1;
//	if (code < 256) {
//		seqBuf[0] = code;
//		seqLength = 1;
//	} else if (code < nextCode) {
//		seqLength = table[code].length;
//		for (i = seqLength - 1, j = code; i > 0; --i) {
//			seqBuf[i] = table[j].tail;
//			j = table[j].head;
//		}
//		seqBuf[0] = j;
//	} else if (code == nextCode) {
//		seqBuf[seqLength] = newChar;
//		++seqLength;
//	} else {
//		error(getPos(), "Bad LZW stream - unexpected code");
//		eof = gTrue;
//		return gFalse;
//	}
//	newChar = seqBuf[0];
//	if (first) {
//		first = gFalse;
//	} else {
//		table[nextCode].length = nextLength;
//		table[nextCode].head = prevCode;
//		table[nextCode].tail = newChar;
//		++nextCode;
//		if (nextCode + early == 512)
//			nextBits = 10;
//		else if (nextCode + early == 1024)
//			nextBits = 11;
//		else if (nextCode + early == 2048)
//			nextBits = 12;
//	}
//	prevCode = code;
//
//	// reset buffer
//	seqIndex = 0;
//
//	return gTrue;
//}
//
//void LZWStream::clearTable() {
//  nextCode = 258;
//  nextBits = 9;
//  seqIndex = seqLength = 0;
//  first = gTrue;
//}
//
//int LZWStream::getCode() {
//  int c;
//  int code;
//
//  while (inputBits < nextBits) {
//    if ((c = str->getChar()) == EOF)
//      return EOF;
//    inputBuf = (inputBuf << 8) | (c & 0xff);
//    inputBits += 8;
//  }
//  code = (inputBuf >> (inputBits - nextBits)) & ((1 << nextBits) - 1);
//  inputBits -= nextBits;
//  return code;
//}

//------------------------------------------------------------------------
// wxCCITTFaxStream
//------------------------------------------------------------------------

//wxCCITTFaxStream::wxCCITTFaxStream(unsigned char* pBuff, wxUint32 nLen, int nEncoding, bool bEndOfLine, bool bByteAlign, int nColumns, int nRows, bool bEndOfBlock, bool bBlack) :  m_bIsOk(false)
//{
//	if (nColumns < 1)
//	{
//		nColumns = 1;
//	}
//	else if (nColumns > INT_MAX - 2)
//	{
//		nColumns = INT_MAX - 2;
//	}
//	// 0 <= codingLine[0] < codingLine[1] < ... < codingLine[n] = columns
//	// ---> max codingLine size = columns + 1
//	// refLine has one extra guard entry at the end
//	// ---> max refLine size = columns + 2
//	codingLine = (int *)gmallocn_checkoverflow(columns + 1, sizeof(int));
//	refLine = (int *)gmallocn_checkoverflow(columns + 2, sizeof(int));
//
//	int* pCodingLine = new int[nColumns + 1];
//	int* pRefLine = new int[nColumns + 2];
//
//	pCodingLine[0] = nColumns;
//
//	inputBits = 0;
//	a0i = 0;
//	outputBits = 0;
//	short code1, code2, code3;
//	int b1i, blackPixels, i, bits;
//	GBool gotEOL;
//
//
//
//	// 2-D encoding
//	if (nEncoding < 0)
//	{
//		for (int i = 0; pCodingLine[i] < nColumns; ++i)
//		{
//			pRefLine[i] = pCodingLine[i];
//		}
//		pRefLine[i++] = nColumns;
//		pRefLine[i] = nColumns;
//		nCodingLine[0] = 0;
//		a0i = 0;
//		b1i = 0;
//		blackPixels = 0;
//		while (pCodingLine[a0i] < nColumns)
//		{
//			code1 = getTwoDimCode();
//				switch (code1) {
//	case twoDimPass:
//		addPixels(refLine[b1i + 1], blackPixels);
//		if (refLine[b1i + 1] < columns) {
//			b1i += 2;
//		}
//		break;
//	case twoDimHoriz:
//		code1 = code2 = 0;
//		if (blackPixels) {
//			do {
//				code1 += code3 = getBlackCode();
//			} while (code3 >= 64);
//			do {
//				code2 += code3 = getWhiteCode();
//			} while (code3 >= 64);
//		} else {
//			do {
//				code1 += code3 = getWhiteCode();
//			} while (code3 >= 64);
//			do {
//				code2 += code3 = getBlackCode();
//			} while (code3 >= 64);
//		}
//		addPixels(codingLine[a0i] + code1, blackPixels);
//		if (codingLine[a0i] < columns) {
//			addPixels(codingLine[a0i] + code2, blackPixels ^ 1);
//		}
//		while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//			b1i += 2;
//		}
//		break;
//	case twoDimVertR3:
//		addPixels(refLine[b1i] + 3, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			++b1i;
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVertR2:
//		addPixels(refLine[b1i] + 2, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			++b1i;
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVertR1:
//		addPixels(refLine[b1i] + 1, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			++b1i;
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVert0:
//		addPixels(refLine[b1i], blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			++b1i;
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVertL3:
//		addPixelsNeg(refLine[b1i] - 3, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			if (b1i > 0) {
//				--b1i;
//			} else {
//				++b1i;
//			}
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVertL2:
//		addPixelsNeg(refLine[b1i] - 2, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			if (b1i > 0) {
//				--b1i;
//			} else {
//				++b1i;
//			}
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case twoDimVertL1:
//		addPixelsNeg(refLine[b1i] - 1, blackPixels);
//		blackPixels ^= 1;
//		if (codingLine[a0i] < columns) {
//			if (b1i > 0) {
//				--b1i;
//			} else {
//				++b1i;
//			}
//			while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
//				b1i += 2;
//			}
//		}
//		break;
//	case EOF:
//		addPixels(columns, 0);
//		eof = gTrue;
//		break;
//	default:
//		error(getPos(), "Bad 2D code %04x in CCITTFax stream", code1);
//		addPixels(columns, 0);
//		err = gTrue;
//		break;
//				}
//			}
//
//			// 1-D encoding
//		} else {
//			codingLine[0] = 0;
//			a0i = 0;
//			blackPixels = 0;
//			while (codingLine[a0i] < columns) {
//				code1 = 0;
//				if (blackPixels) {
//					do {
//						code1 += code3 = getBlackCode();
//					} while (code3 >= 64);
//				} else {
//					do {
//						code1 += code3 = getWhiteCode();
//					} while (code3 >= 64);
//				}
//				addPixels(codingLine[a0i] + code1, blackPixels);
//				blackPixels ^= 1;
//			}
//		}
//
//		// byte-align the row
//		if (byteAlign) {
//			inputBits &= ~7;
//		}
//
//		// check for end-of-line marker, skipping over any extra zero bits
//		gotEOL = gFalse;
//		if (!endOfBlock && row == rows - 1) {
//			eof = gTrue;
//		} else {
//			code1 = lookBits(12);
//			while (code1 == 0) {
//				eatBits(1);
//				code1 = lookBits(12);
//			}
//			if (code1 == 0x001) {
//				eatBits(12);
//				gotEOL = gTrue;
//			} else if (code1 == EOF) {
//				eof = gTrue;
//			}
//		}
//
//		// get 2D encoding tag
//		if (!eof && encoding > 0) {
//			nextLine2D = !lookBits(1);
//			eatBits(1);
//		}
//
//		// check for end-of-block marker
//		if (endOfBlock && gotEOL) {
//			code1 = lookBits(12);
//			if (code1 == 0x001) {
//				eatBits(12);
//				if (encoding > 0) {
//					lookBits(1);
//					eatBits(1);
//				}
//				if (encoding >= 0) {
//					for (i = 0; i < 4; ++i) {
//						code1 = lookBits(12);
//						if (code1 != 0x001) {
//							error(getPos(), "Bad RTC code in CCITTFax stream");
//						}
//						eatBits(12);
//						if (encoding > 0) {
//							lookBits(1);
//							eatBits(1);
//						}
//					}
//				}
//				eof = gTrue;
//			}
//
//			// look for an end-of-line marker after an error -- we only do
//			// this if we know the stream contains end-of-line markers because
//			// the "just plow on" technique tends to work better otherwise
//		} else if (err && endOfLine) {
//			while (1) {
//				code1 = lookBits(13);
//				if (code1 == EOF) {
//					eof = gTrue;
//					return EOF;
//				}
//				if ((code1 >> 1) == 0x001) {
//					break;
//				}
//				eatBits(1);
//			}
//			eatBits(12); 
//			if (encoding > 0) {
//				eatBits(1);
//				nextLine2D = !(code1 & 1);
//			}
//		}
//
//		// set up for output
//		if (codingLine[0] > 0) {
//			outputBits = codingLine[a0i = 0];
//		} else {
//			outputBits = codingLine[a0i = 1];
//		}
//
//		++row;
//	}
//
//	// get a byte
//	if (outputBits >= 8) {
//		buf = (a0i & 1) ? 0x00 : 0xff;
//		outputBits -= 8;
//		if (outputBits == 0 && codingLine[a0i] < columns) {
//			++a0i;
//			outputBits = codingLine[a0i] - codingLine[a0i - 1];
//		}
//	} else {
//		bits = 8;
//		buf = 0;
//		do {
//			if (outputBits > bits) {
//				buf <<= bits;
//				if (!(a0i & 1)) {
//					buf |= 0xff >> (8 - bits);
//				}
//				outputBits -= bits;
//				bits = 0;
//			} else {
//				buf <<= outputBits;
//				if (!(a0i & 1)) {
//					buf |= 0xff >> (8 - outputBits);
//				}
//				bits -= outputBits;
//				outputBits = 0;
//				if (codingLine[a0i] < columns) {
//					++a0i;
//					outputBits = codingLine[a0i] - codingLine[a0i - 1];
//				} else if (bits > 0) {
//					buf <<= bits;
//					bits = 0;
//				}
//			}
//		} while (bits);
//	}
//	if (black) {
//		buf ^= 0xff;
//	}
//	return buf;
//}
//
//wxCCITTFaxStream::~wxCCITTFaxStream() 
//{
//}
//
//short wxCCITTFaxStream::GetTwoDimCode(unsigned char* pBuff, int nLen, bool bEndOfBlock, int nInputBits)
//{
//	short code(0);
//	const CCITTCode *p;
//	int n;
//
//	if (bEndOfBlock)
//	{
//		code = lookBits(pBuff, nLen, 7, nInputBits);
//		p = &twoDimTab1[code];
//		if (p->bits > 0)
//		{
//			eatBits(p->bits);
//			return p->n;
//		}
//	} 
//	else
//	{
//		for (n = 1; n <= 7; ++n)
//		{
//			code = lookBits(n);
//			if (n < 7) {
//				code <<= 7 - n;
//			}
//			p = &twoDimTab1[code];
//			if (p->bits == n) {
//				eatBits(n);
//				return p->n;
//			}
//		}
//	}
//	return EOF;
//}
//
//short wxCCITTFaxStream::LookBits(unsigned char* pBuff, int nLen, int n, int nInputBits)
//{
//	int nInputBuf = 0;
//	int nCounter = 0;
//	while (nInputBits < n)
//	{
//		if(nCounter == nLen)
//		{
//			if (nInputBits == 0)
//			{
//				return EOF;
//			}
//			return (nInputBuf << (n - nInputBits)) & (0xffff >> (16 - n));
//		}
//		int c = pBuff[nCounter];
//		nCounter++;
//		nInputBuf = (nInputBuf << 8) + c;
//		nInputBits += 8;
//	}
//	return (nInputBuf >> (nInputBits - n)) & (0xffff >> (16 - n));
//}
//
//short wxCCITTFaxStream::GetWhiteCode()
//{
//}
//
//short wxCCITTFaxStream::GetBlackCode()
//{
//}
//
