/******************************************************************************
 * Project:  Sphinx Search local file system index utility
 * Purpose:  wxHTMLFileFactory class. html, htm
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
#include "wxHTMLFileFactory.h"
#include <wx/textfile.h>
#include "../wxSphinxCommon/wxContainer.h"
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/filesys.h>

IMPLEMENT_DYNAMIC_CLASS(wxHTMLFileFactory, wxObject)

wxHTMLFileFactory::wxHTMLFileFactory(void)
{
}

wxHTMLFileFactory::~wxHTMLFileFactory(void)
{
}

bool wxHTMLFileFactory::GetChildren(wxArrayString* pFileNames, wxObjectArray* pObjArray)
{
	for(size_t i = 0; i < pFileNames->GetCount(); i++)
	{
		wxString path = pFileNames->Item(i);
		wxFileName FileName(path);
		wxString ext = FileName.GetExt();
		ext.MakeLower();
		for(size_t j = 0; j < m_sExtArray.size(); j++)
		{
            if(ext == m_sExtArray[j])
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

wxString wxHTMLFileFactory::GetCData(wxString sPath)
{
    wxString out(wxT("empty"));
//	out.Alloc(8192);

    wxFileSystem FSys;
    wxFSFile* pFSFile = FSys.OpenFile(sPath);

	//wxFileInputStream txtfilestream(sPath);
	//if(!txtfilestream.IsOk())
	if(pFSFile == 0)
	{
	    wxLogError(_("Error open file '%s'"), sPath.c_str());
	    return out;
	}
	wxInputStream* pStream = pFSFile->GetStream();
    if(0 == pStream)
	{
	    wxLogError(_("Error open file '%s'"), sPath.c_str());
	    return out;
	}

    wxString sSearchStr;
    size_t streamSize = pStream->GetSize();
    char* src = new char[streamSize+1];
    pStream->Read(src, streamSize);
    src[streamSize] = 0;

    for(size_t i = 0; i < m_sEncodingsArray.size(); i++)
    {
        sSearchStr = wxString(src, wxCSConv(m_sEncodingsArray[i]));
        if(!sSearchStr.IsEmpty())
            break;
    }

    wxString sCharset;
    if(sSearchStr.IsEmpty())
    {
        sCharset = wxString(wxT("windows-1251"));
    }
    else
    {
        int beg = 0;
        if((beg = sSearchStr.Find(wxT("charset="))) == wxNOT_FOUND)
        {
            sCharset = wxString(wxT("windows-1251"));
        }
        else
        {
            sSearchStr = sSearchStr.Right(sSearchStr.Len() - beg - 8);
            sCharset = sSearchStr.Left(30);
        }
    }

    bool bCanEncode = false;
    for(size_t i = 0; i < m_sEncodingsArray.size(); i++)
    {
        if(sCharset.MakeLower().Find(m_sEncodingsArray[i].MakeLower()) != wxNOT_FOUND)
        {
            sCharset = m_sEncodingsArray[i];
            wxLogMessage(_("Charset set to %s"), sCharset.c_str());
            bCanEncode = true;
        }
    }
    if(!bCanEncode)
    {
        wxLogError(_("Wrong file '%s' encoding"), sPath.c_str());
        return out;
    }
    //wxCSConv Conv(sCharset);

    sSearchStr = wxString(src, wxCSConv(sCharset));
    delete[] src;
//    wxDELETE(pStream);
    wxDELETE(pFSFile);

    if(sSearchStr.IsEmpty())
	{
	    wxLogError(_("Error read full file '%s'"), sPath.c_str());
	    return out;
	}

	out = AnalyseHTML(sSearchStr);
/*	out = sSearchStr;

	//remove comments <!-- -->
	out = RemoveHTMLTags(out, wxT("<!-"), wxT("->"));
	//remove comments <head> </head>
	//out = RemoveHTMLTags(out, wxT("<head>"), wxT("</head>"));
	//remove comments <style> </style>
	out = RemoveHTMLTags(out, wxT("<sty"), wxT("</style>"));
	out = RemoveHTMLTags(out, wxT("<STY"), wxT("</STYLE>"));
	out = RemoveHTMLTags(out, wxT("<Sty"), wxT("</Style>"));
	//remove comments <script> </script>
	out = RemoveHTMLTags(out, wxT("<scr"), wxT("</script>"));
	out = RemoveHTMLTags(out, wxT("<SCR"), wxT("</SCRIPT>"));
	out = RemoveHTMLTags(out, wxT("<Scr"), wxT("</Script>"));
    //remove tags < >
	out = RemoveHTMLTags(out, wxT("<"), wxT(">"));
    //out = wxString(out, wxConvUTF8);

    out.Replace(wxT("]"), wxT(")"), true);
    out.Replace(wxT("["), wxT(")"), true);
//    out.Replace(wxT("&amp;"), wxT("&"), true);
//    out.Replace(wxT("&quot;"), wxT("\""), true);
//    out.Replace(wxT("&nbsp;"), wxT(" "), true);
//	out.Replace(wxT("&lt;"), wxT("("), true);
//	out.Replace(wxT("&gt;"), wxT(")"), true);
//	out.Replace(wxT("\t"), wxT(" "), true);
//	out.Replace(wxT("\n"), wxT(" "), true);
//	out.Replace(wxT("  "), wxT(" "), true);
*/
	return out;
}

void wxHTMLFileFactory::SetConfig(wxXmlNode* pConfNode)
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
    wxString sEncodings = pConfNode->GetPropVal(wxT("encodings"), wxT(""));
    wxStringTokenizer tkz_enc(sEncodings, wxString(wxT(",")), false);
    while( tkz_enc.HasMoreTokens() )
    {
        wxString token = tkz_enc.GetNextToken();
        token.Replace(wxT(","), wxT(""));
        token.MakeLower();
        token.Trim(true); token.Trim(false);
        m_sEncodingsArray.Add(token);
    }
}

wxString wxHTMLFileFactory::RemoveHTMLTags(wxString s, wxString sOpenTag, wxString sCloseTag)
{
	static bool inTag = false;
	bool done = false;
	while(!done)
	{
		//s.Replace(wxT("  "), wxT(" "));
		if(inTag)
		{
			// The previous line started an HTML tag
			// but didn't finish. Must search for '>'.
			size_t rightPos = s.Find(sCloseTag);
			if(rightPos != wxNOT_FOUND)
			{
				inTag = false;
				rightPos += sCloseTag.Len() - 1;
				s.erase(0, rightPos + 1);
			}
			else
			{
				done = true;
				s.erase();
			}
		}
		else
		{
			// Look for start of tag:
			size_t leftPos = s.Find(sOpenTag);
			if(leftPos != wxNOT_FOUND)
			{
				// See if tag close is in this line:
				size_t rightPos = s.Find(sCloseTag);
				if(rightPos == wxNOT_FOUND)
				{
					inTag = done = true;
					s.erase(leftPos);
				}
				else
				{
					rightPos += sCloseTag.Len() - 1;
					s.erase(leftPos, rightPos - leftPos + 1);
                    s.insert(leftPos, wxT(" "));
				}
			}
			else
				done = true;
		}
	}
	return s;
}

//void wxHTMLFileFactory::ReadString(wxString& str, wxInputStream* s, wxString sEnc)
//{
//    size_t streamSize = s->GetSize();
//
//    if (streamSize == ~(size_t)0)
//    {
//        const size_t bufSize = 4095;
//        char buffer[bufSize+1];
//        size_t lastRead;
//
//        do
//        {
//            s->Read(buffer, bufSize);
//            lastRead = s->LastRead();
//            buffer[lastRead] = 0;
//            //str.Append(wxString(buffer, conv));
//            str.Append(wxString(buffer, wxCSConv(sEnc)));
//        }
//        while (lastRead == bufSize);
//    }
//    else
//    {
//        char* src = new char[streamSize+1];
//        s->Read(src, streamSize);
//        src[streamSize] = 0;
//        //str = wxString(src, conv);
//        str = wxString(src, wxCSConv(sEnc));
//        delete[] src;
//    }
//}

wxString wxHTMLFileFactory::AnalyseHTML(wxString sInStr)
{
    wxString sOutstr;
    sOutstr.Alloc(sInStr.Len());
    int nOpenTagCount(0);
    bool bStyle(false), bScript(false), bW(false);

    wxString sTagName;

    for(size_t i = 0; i < sInStr.Len(); i++)
    {
        if(i > 2000000)
            break;
//        if(i % 100000 == 0)
//            wxLogMessage(wxT("analyse doc. len - %u; chars %u - %u%%"), sInStr.Len(), i, i * 100 / sInStr.Len() );
        wxChar sSymbol = sInStr[i];
        switch(sSymbol)
        {
            case '<':
                nOpenTagCount++;
                if(nOpenTagCount == 1)
                    sTagName.Clear();
                continue;
            case '>':
                nOpenTagCount--;
                sTagName.Trim(true); sTagName.Trim(false);
                sTagName.MakeLower();
                if(sTagName == wxString(wxT("style")))
                {
                    bStyle = true;
                    bScript = false;
                    bW = false;
                    sTagName.Clear();
                }
                else if(sTagName == wxString(wxT("script")))
                {
                    bStyle = false;
                    bScript = true;
                    bW = false;
                    sTagName.Clear();
                }
                else if(sTagName.Find(wxT("w:")) != wxNOT_FOUND)
                {
                    bStyle = false;
                    bScript = false;
                    bW = true;
                    sTagName.Clear();
                }
                else
                {
                    bStyle = false;
                    bScript = false;
                    bW = false;
                }
                //wxLogMessage(sTagName);
                continue;
        }

        if(nOpenTagCount > 0)
            sTagName += sSymbol;
        else
        {
            if(bStyle || bScript || bW)
                continue;
//            if(sTagName[0] == '/')
//                continue;
            if(sOutstr[sOutstr.Len() - 1] == ' ' && sSymbol == ' ')
                continue;
            if(sOutstr[sOutstr.Len() - 1] == '\n' && sSymbol == '\n')
                continue;
            if(sSymbol == '\t' || sSymbol == '\"' || sSymbol == '\'')
                sSymbol = ' ';
            if(sSymbol == '[')
                sSymbol = '(';
            if(sSymbol == ']')
                sSymbol = ')';
            if(sSymbol == '&' && sInStr[i + 1] == 'n' && sInStr[i + 2] == 'b' && sInStr[i + 3] == 's')
            {
                i += 5;
                continue;
            }
            sOutstr += sSymbol;
        }
    }
//    wxLogMessage(sOutstr);

//	out = RemoveHTMLTags(out, wxT("<!-"), wxT("->"));
//	//remove comments <head> </head>
//	//out = RemoveHTMLTags(out, wxT("<head>"), wxT("</head>"));
//	//remove comments <style> </style>
//	out = RemoveHTMLTags(out, wxT("<sty"), wxT("</style>"));
//	out = RemoveHTMLTags(out, wxT("<STY"), wxT("</STYLE>"));
//	out = RemoveHTMLTags(out, wxT("<Sty"), wxT("</Style>"));
//	//remove comments <script> </script>
//	out = RemoveHTMLTags(out, wxT("<scr"), wxT("</script>"));
//	out = RemoveHTMLTags(out, wxT("<SCR"), wxT("</SCRIPT>"));
//	out = RemoveHTMLTags(out, wxT("<Scr"), wxT("</Script>"));
//    //remove tags < >
//	out = RemoveHTMLTags(out, wxT("<"), wxT(">"));
//    //out = wxString(out, wxConvUTF8);
//
//    out.Replace(wxT("]"), wxT(")"), true);
//    out.Replace(wxT("["), wxT(")"), true);
    return sOutstr;
}
