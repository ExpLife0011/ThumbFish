#pragma once
#include "stdafx.h"

const TCHAR c_OnlineLink[] = _T("http://abhimanyusirohi.github.io/ThumbFish/");
const COMDLG_FILTERSPEC c_SaveTypes[] =
{
    {L"Portable Network Graphics (*.png)",	L"*.png"},
	{L"Portable Document Format (*.pdf)",	L"*.pdf"},
    {L"Scalable Vector Graphics (*.svg)", L"*.svg"},
    {L"Enhanced Metafile (*.emf)", L"*.emf"},
    {L"ChemDraw XML Format (*.cdxml)", L"*.cdxml"}
};

class Utils
{
public:
	Utils(void);
	~Utils(void);

	static HRESULT DoFileSaveDialog(HWND owner, PWSTR filePath);
	static HRESULT GetSystemFolder(const KNOWNFOLDERID folderID, TCHAR* outPath);
	static BOOL ShellExecuteLink(const TCHAR* link);
};

