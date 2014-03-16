// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the INDIGOPROVIDER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// INDIGOPROVIDER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef INDIGOPROVIDER_EXPORTS
#define INDIGOPROVIDER_API extern "C" __declspec(dllexport)
#else
#define INDIGOPROVIDER_API extern "C" __declspec(dllimport)
#endif

//typedef std::basic_string<TCHAR> tstring;
//typedef struct
//{
//	void*			pData;
//	bool			isStream;
//	ULONG			DataLength;
//	TCHAR			FileName[MAX_PATH];
//} BUFFER, *LPBUFFER;
//
//typedef std::map<std::string, std::string> OPTIONS;
//typedef OPTIONS* LPOPTIONS;

//// This class is exported from the IndigoProvider.dll
//class INDIGOPROVIDER_API CIndigoProvider {
//public:
//	CIndigoProvider(void);
//	// TODO: add your methods here.
//};

INDIGOPROVIDER_API bool Draw(HDC hDC, RECT rect, LPBUFFER buffer, int* options);
INDIGOPROVIDER_API int GetProperties(LPBUFFER buffer, TCHAR*** properties, int* options);

void AddProperty(TCHAR*** properties, int startIndex, TCHAR* name, TCHAR* value);
