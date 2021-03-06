// ThumbFishDocument.cpp : Implementation of the ThumbFishDocument class

#include "stdafx.h"
#include <propkey.h>
#include "ThumbFishDocument.h"

#define MAX_CACHE_RECORD_COUNT	4

// assuming a normal record would be a maximum of 10KB
#define MAX_CACHE_SIZE	(10 * 1024) * MAX_CACHE_RECORD_COUNT

// maximum size of mol supported is 100KB
#define MAX_MOL_SIZE	100 * 1024

HRESULT ThumbFishDocument::LoadFromStream(IStream* pStream, DWORD grfMode)
{
	STATSTG stat;
	TCHAR file[MAX_PATH];

	pantheios::log_NOTICE(_T("ThumbFishDocument::LoadFromStream> Called"));

	// get stream properties
	HRESULT hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
	if(hr == S_OK)
	{
		// initialize BUFFER
		m_Buffer.DataLength = stat.cbSize.LowPart;

		// pwcsName contains file name without path
		if(stat.pwcsName != NULL) _tcscpy_s(file, MAX_PATH, OLE2W(stat.pwcsName));

		pantheios::log_NOTICE(_T("ThumbFishDocument::LoadFromStream> Name="), file, 
		_T(", Size="), pantheios::integer(m_Buffer.DataLength));

		m_Buffer.DataFormat = CommonUtils::GetFormatFromFileName(file);

		pantheios::log_NOTICE(_T("ThumbFishDocument::LoadFromStream> Loading Stream..."));

		// load stream into BUFFER only when it contains data
		if((m_Buffer.DataLength > 0) && !LoadStream(pStream))
			pantheios::log_ERROR(_T("ThumbFishDocument::LoadFromStream> Could not load data."),
				file, _T(", DataLength"), pantheios::integer(m_Buffer.DataLength));
	}

	pantheios::log_NOTICE(_T("ThumbFishDocument::LoadFromStream> HRESULT="), pantheios::integer(hr));

	return S_OK;
}

void ThumbFishDocument::InitializeSearchContent()
{
	pantheios::log_NOTICE(_T("ThumbFishDocument::InitializeSearchContent> Called"));

	CString value;
	if(pExecuteFunc != NULL)
	{
		OPTIONS options;
		TCHAR** props = NULL;

		COMMANDPARAMS params(cmdGetProperties, &m_Buffer, (LPVOID)1);
		std::auto_ptr<OUTBUFFER> outBuffer(pExecuteFunc(&params, &options));
		props = (TCHAR**)outBuffer->pData;
		int propCount = (int)outBuffer->DataLength;
		
		bool m_propsGenerated = ((props != NULL) && (propCount > 0));

		if(m_propsGenerated)
		{
			pantheios::log_NOTICE(_T("ThumbFishDocument::InitializeSearchContent> Properties generated="), 
				pantheios::integer(propCount));

			// insert property values into list view control
			for(int i = 0; i < propCount; i++)
			{
				TCHAR* propName = props[2*i];
				TCHAR* propValue = props[2*i + 1];

				// "-" property names will not be included in search
				if(_tcsicmp(propName, _T("-")) != 0)
				{
					value.AppendFormat(_T("%ls_%ls;"), propName, propValue);
				}
			}

			delete[] props;
		}
		else
		{
			pantheios::log_WARNING(_T("ThumbFishDocument::InitializeSearchContent> Property generation failed."));
		}
	}
	else
	{
		pantheios::log_ERROR(_T("ThumbFishDocument::InitializeSearchContent> Property function is not available."));
	}

	SetSearchContent(value);
}

void ThumbFishDocument::SetSearchContent(CString& value)
{
	pantheios::log_NOTICE(_T("ThumbFishDocument::SetSearchContent> Called"));

	// Assigns search content to PKEY_Search_Contents key
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

void ThumbFishDocument::OnDrawThumbnail(HDC hDrawDC, LPRECT lprcBounds)
{
	pantheios::log_NOTICE(_T("ThumbFishDocument::OnDrawThumbnail> Called"));

	if(pExecuteFunc != NULL)
	{
		OPTIONS options;
		options.IsThumbnail = true;

		DRAWPARAMS drawParams(hDrawDC, *lprcBounds);
		COMMANDPARAMS params(cmdDraw, &m_Buffer, &drawParams);
		std::auto_ptr<OUTBUFFER> outBuffer(pExecuteFunc(&params, &options));
		if(!((BOOL)outBuffer->pData))
		{
			pantheios::log_NOTICE(_T("ThumbFishDocument::OnDrawThumbnail> Draw returned FALSE. Thumbnail NOT drawn."));
		}
	}
	else
	{
		pantheios::log_ERROR(_T("ThumbFishDocument::OnDrawThumbnail> Draw function not available."));
	}
}

/// <summary>
/// Loads the specified IStream into BUFFER object.
/// Loading data from IStream is required because the IStream does not provide full path to
/// the target file when IStream::Stat(...) is called.
/// </summary>
BOOL ThumbFishDocument::LoadStream(IStream* stream)
{
	pantheios::log_NOTICE(_T("ThumbFishDocument::LoadStream> Called"));

	char recordDelimiter[20];
	memset(recordDelimiter, 0, 20);
	
	m_Buffer.pData = NULL;

	if((m_Buffer.DataFormat == fmtSDFV2) || (m_Buffer.DataFormat == fmtSDFV3))
	{
		strcpy_s(recordDelimiter, 20, "$$$$");
	}
	else if((m_Buffer.DataFormat == fmtRDFV2) || (m_Buffer.DataFormat == fmtRDFV3))
	{
		strcpy_s(recordDelimiter, 20, "$RFMT");
	}
	else if(m_Buffer.DataFormat == fmtCML)
	{
		strcpy_s(recordDelimiter, 20, "</molecule>");
	}
	else if(m_Buffer.DataFormat == fmtSMILES)
	{
		strcpy_s(recordDelimiter, 20, "\r\n");
	}
	else
	{
		// for single molecule files such as MOL, read the whole data
		if(m_Buffer.DataLength < MAX_MOL_SIZE)
		{
			m_Buffer.pData = new char[m_Buffer.DataLength];

			if(FAILED(stream->Read(m_Buffer.pData, (ULONG)m_Buffer.DataLength, (ULONG*)&m_Buffer.DataLength))) return false;

			// set the exact connection table version (V2000, V3000) info for MOL and RXN files
			// the extension (mol, rxn) could not identify the exact CT version inside file
			if(m_Buffer.DataFormat == fmtMOLV2)
				m_Buffer.DataFormat = (CommonUtils::IdentifyCTVersion((PCHAR)m_Buffer.pData, m_Buffer.DataLength, 4) == 1) ? fmtMOLV2 : fmtMOLV3;
			else if(m_Buffer.DataFormat == fmtRXNV2)
				m_Buffer.DataFormat = (CommonUtils::IdentifyCTVersion((PCHAR)m_Buffer.pData, m_Buffer.DataLength, 1) == 1) ? fmtRXNV2 : fmtRXNV3;
		}
		else
		{
			pantheios::log_WARNING(_T("ThumbFishDocument::LoadStream> File too large and cannot be read. Size= "),
				pantheios::integer(m_Buffer.DataLength));

			m_Buffer.DataLength = -1;	// file too large
			return false;
		}

		return true;
	}

	/*	
		the following code runs for multimol files only and caches a number of records 
		because we cannot cache all the records.
	*/

	pantheios::log_NOTICE(_T("ThumbFishDocument::LoadStream> Reading multimol file..."));

	char readBuffer;
	ULONG readCount = 0;
	bool firstMolVersionChecked = false;
	char largeTempBuffer[MAX_CACHE_SIZE];
	size_t delimiterLen = strlen(recordDelimiter);
	int matchIndex = 0, totalReadBytes = 0, recordCount = 0, recordsReadBytes = 0;
	bool isSDForRDF = ((m_Buffer.DataFormat == fmtSDFV2) || (m_Buffer.DataFormat == fmtSDFV3) 
		|| (m_Buffer.DataFormat == fmtRDFV2) || (m_Buffer.DataFormat == fmtRDFV3));

	// read stream char by char, identify the record delimiters and cache 
	// MAX_CACHE_RECORDS number of records
	while(stream->Read(&readBuffer, 1, &readCount) == S_OK)
	{
		largeTempBuffer[totalReadBytes++] = readBuffer;
		if(totalReadBytes >= MAX_CACHE_SIZE) break;	// very big records, normally not possible but just in case

		// identify record delimiter in text
		if(readBuffer == recordDelimiter[matchIndex++])
		{
			if(matchIndex == delimiterLen) // whole delimiter string matches
			{
				if(!firstMolVersionChecked && isSDForRDF)
				{
					if(m_Buffer.DataFormat == fmtSDFV2)
					{
						m_Buffer.DataFormat = (CommonUtils::IdentifyCTVersion(largeTempBuffer, 
							(totalReadBytes - recordsReadBytes), 4) == 1) ? fmtSDFV2 : fmtSDFV3;
						firstMolVersionChecked = true;
					}
					else if((m_Buffer.DataFormat == fmtRDFV2) && (recordCount == 1))	// RDF has start tag unlike SDF ending tag $$$$
					{
						m_Buffer.DataFormat = (CommonUtils::IdentifyCTVersion(largeTempBuffer, 
							(totalReadBytes - recordsReadBytes), 4) == 1) ? fmtRDFV2 : fmtRDFV3;
						firstMolVersionChecked = true;
					}
				}

				recordsReadBytes = totalReadBytes;	// count of bytes upto which we got records
				recordCount++;	// RDF will have one less than MAX_CACHE_RECORD_COUNT coz it has starting tag
			}

			if(recordCount == MAX_CACHE_RECORD_COUNT) break;	// desired number of records read
		}
		else
		{
			matchIndex = 0;
		}
	}

	// a single SMILES string might not end with a \r\n so if there is data in 
	// buffer, we will consider it as a SMILES record
	if((m_Buffer.DataFormat == fmtSMILES) && (totalReadBytes > 0)) 
	{
		recordCount = 1;
		recordsReadBytes = (int)m_Buffer.DataLength;
	}

	if(recordCount > 0)
	{
		// approximate the total number of records
		m_Buffer.recordCount = ((int)m_Buffer.DataLength / (recordsReadBytes / recordCount));

		m_Buffer.pData = new char[recordsReadBytes];
		m_Buffer.DataLength = recordsReadBytes;
		memcpy(m_Buffer.pData, largeTempBuffer, recordsReadBytes);
		return TRUE;
	}
	else
	{
		m_Buffer.DataLength = 0;	// no records read
	}

	pantheios::log_NOTICE(_T("ThumbFishDocument::LoadStream> Records Read="), pantheios::integer(recordCount), 
		_T(", Bytes Read="), pantheios::integer(recordsReadBytes)/*,
		_T(", Approx Records="), pantheios::integer((int)m_Buffer.Extra)*/);

	return FALSE;
}

/// <summary>
/// This overload is needed to fix ATL's default monochrome bitmap
/// More Info: http://www.patthoyts.tk/blog/fixing-the-atl-ithumbnailprovider-implementation.html
/// </summary>
BOOL ThumbFishDocument::GetThumbnail(_In_ UINT cx, _Out_ HBITMAP* phbmp, _In_opt_ WTS_ALPHATYPE* /* pdwAlpha */)
{
	pantheios::log_NOTICE(_T("ThumbFishDocument::GetThumbnail> Called"));

    BOOL br = FALSE;
    HDC hdc = ::GetDC(NULL);
    HDC hDrawDC = CreateCompatibleDC(hdc);
    if (hDrawDC != NULL)
    {
        void *bits = 0;
        RECT rcBounds;
        SetRect(&rcBounds, 0, 0, cx, cx);

        BITMAPINFO bi = {0};
        bi.bmiHeader.biWidth = cx;
        bi.bmiHeader.biHeight = cx;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biSizeImage = 0;
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biClrUsed = 0;
        bi.bmiHeader.biClrImportant = 0;

        HBITMAP hBmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
        if (hBmp != NULL)
        {
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBmp);
            OnDrawThumbnail(hDrawDC, &rcBounds);
            SelectObject(hDrawDC, hOldBitmap);
            *phbmp = hBmp;
            br = TRUE;
        }
        DeleteDC(hDrawDC);
    }

    ReleaseDC(NULL, hdc);
    return br;
}

/// <summary>
/// Loads the specified file into BUFFER object by first creating an IStream on the 
/// file then reading data from it.
/// </summary>
HRESULT ThumbFishDocument::LoadFromFile(TCHAR* file)
{
	HRESULT retVal = S_FALSE;
	IStream* dataStream = NULL;

	// create a stream from file and load it in specified ThumbFishDocument object
	if(SUCCEEDED(SHCreateStreamOnFileEx(file, STGM_READ | STGM_SHARE_DENY_NONE, FILE_ATTRIBUTE_NORMAL, FALSE, NULL, &dataStream)))
		if(SUCCEEDED(LoadFromStream(dataStream, STGM_READ | STGM_SHARE_DENY_NONE)) && (m_Buffer.DataLength > 0))
			retVal = S_OK;
		else
			pantheios::log_ERROR(_T("CContextMenuHandler::LoadDocFromFile> LoadFromStream() method FAILED. File= "), file);
	else
		pantheios::log_ERROR(_T("CContextMenuHandler::LoadDocFromFile> SHCreateStreamOnFileEx() method FAILED. File= "), file);

	if(dataStream != NULL) dataStream->Release();
	return retVal;
}
