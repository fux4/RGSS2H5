#pragma once
#include <Windows.h>
#include <fstream>

class Tool {

private:
	static int GetPNGWidthHeight(const char* path, unsigned int* punWidth, unsigned int* punHeight);
	static int GetJPEGWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight);

public:
	static wchar_t* char2wchar(char*);
	static void convertString(char* ,char*);
	static void GetPicWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight);
	static void GetFileBaseName(char**, char*);
	static void GetFileExtendName(char**, char*);
	static bool CopyEntireFile(char* src, char* dst);
	static char *ReadEntireFile(const char*);
	static char *ReadEntireUTF8File(char*);
	static char *GetJsonBegin(char*);
	static char *GetDirByFullName(char*);
	static char *string2UTF8(char*);
	static char *UTF82string(char*);
};