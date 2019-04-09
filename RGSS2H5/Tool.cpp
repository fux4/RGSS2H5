#include "stdafx.h"
#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>

#define MAKEUS(a, b)    ((unsigned short) ( ((unsigned short)(a))<<8 | ((unsigned short)(b)) ))
#define MAKEUI(a,b,c,d) ((unsigned int) ( ((unsigned int)(a)) << 24 | ((unsigned int)(b)) << 16 | ((unsigned int)(c)) << 8 | ((unsigned int)(d)) ))

#define M_DATA  0x00
#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_SOI   0xd8
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DNL   0xdc
#define M_DRI   0xdd
#define M_APP0  0xe0
#define M_APPF  0xef
#define M_COM   0xfe

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Tool.h"
using namespace std;

wchar_t* Tool::char2wchar(char *str)
{
	wchar_t *m_wchar;
	int len = MultiByteToWideChar(CP_ACP, 0, str, strlen(str), NULL, 0);
	if (len == 0) return NULL;
	m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), m_wchar, len);
	m_wchar[len] = 0;
	return m_wchar;
}

void Tool::convertString(char *retString,char *MultiByteStr) {
	static wchar_t WideCharStr[1024];
	MultiByteToWideChar(CP_UTF8, 0, MultiByteStr, -1, WideCharStr, 1024);
	WideCharToMultiByte(CP_ACP, 0, WideCharStr, -1, retString, 1024, 0, 0);
}

int Tool::GetPNGWidthHeight(const char* path, unsigned int* punWidth, unsigned int* punHeight)
{
	int Finished = 0;
	unsigned char uc[4];
	FILE *pfRead;

	*punWidth = 0;
	*punHeight = 0;

	if (fopen_s(&pfRead, path, "rb") != 0)
	{
		printf("[GetPNGWidthHeight]:can't open file:%s\n", path);
		return -1;
	}

	for (int i = 0; i < 4; i++)
		fread(&uc[i], sizeof(unsigned char), 1, pfRead);
	if (MAKEUI(uc[0], uc[1], uc[2], uc[3]) != 0x89504e47)
		printf("[GetPNGWidthHeight]:png format error\n");
	for (int i = 0; i < 4; i++)
		fread(&uc[i], sizeof(unsigned char), 1, pfRead);
	if (MAKEUI(uc[0], uc[1], uc[2], uc[3]) != 0x0d0a1a0a)
		printf("[GetPNGWidthHeight]:png format error\n");

	fseek(pfRead, 16, SEEK_SET);
	for (int i = 0; i < 4; i++)
		fread(&uc[i], sizeof(unsigned char), 1, pfRead);
	*punWidth = MAKEUI(uc[0], uc[1], uc[2], uc[3]);
	for (int i = 0; i < 4; i++)
		fread(&uc[i], sizeof(unsigned char), 1, pfRead);
	*punHeight = MAKEUI(uc[0], uc[1], uc[2], uc[3]);
	return 0;
}

int Tool::GetJPEGWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight)
{
	int Finished = 0;
	unsigned char id, ucHigh, ucLow;
	FILE *pfRead;

	*punWidth = 0;
	*punHeight = 0;

	if (fopen_s(&pfRead, path, "rb") != 0)
	{
		printf("[GetJPEGWidthHeight]:can't open file:%s\n", path);
		return -1;
	}

	while (!Finished)
	{
		if (!fread(&id, sizeof(char), 1, pfRead) || id != 0xff || !fread(&id, sizeof(char), 1, pfRead))
		{
			Finished = -2;
			break;
		}

		if (id >= M_APP0 && id <= M_APPF)
		{
			fread(&ucHigh, sizeof(char), 1, pfRead);
			fread(&ucLow, sizeof(char), 1, pfRead);
			fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR);
			continue;
		}

		switch (id)
		{
		case M_SOI:
			break;

		case M_COM:
		case M_DQT:
		case M_DHT:
		case M_DNL:
		case M_DRI:
			fread(&ucHigh, sizeof(char), 1, pfRead);
			fread(&ucLow, sizeof(char), 1, pfRead);
			fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR);
			break;

		case M_SOF0:
			fseek(pfRead, 3L, SEEK_CUR);
			fread(&ucHigh, sizeof(char), 1, pfRead);
			fread(&ucLow, sizeof(char), 1, pfRead);
			*punHeight = (unsigned int)MAKEUS(ucHigh, ucLow);
			fread(&ucHigh, sizeof(char), 1, pfRead);
			fread(&ucLow, sizeof(char), 1, pfRead);
			*punWidth = (unsigned int)MAKEUS(ucHigh, ucLow);
			return 0;

		case M_SOS:
		case M_EOI:
		case M_DATA:
			Finished = -1;
			break;

		default:
			fread(&ucHigh, sizeof(char), 1, pfRead);
			fread(&ucLow, sizeof(char), 1, pfRead);
			printf("[GetJPEGWidthHeight]:unknown id: 0x%x ;  length=%hd\n", id, MAKEUS(ucHigh, ucLow));
			if (fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR) != 0)
				Finished = -2;
			break;
		}
	}

	if (Finished == -1)
		printf("[GetJPEGWidthHeight]:can't find SOF0!\n");
	else if (Finished == -2)
		printf("[GetJPEGWidthHeight]:jpeg format error!\n");
	return -1;
}

void Tool::GetPicWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight)
{
	int len = strlen(path);
	if (len <= 4)
		printf("[GetPicWidthHeight]:picture name is too short\n");
	if (!strncmp(path + len - 3, "jpg", 3))
		GetJPEGWidthHeight(path, punWidth, punHeight);
	else if (!strncmp(path + len - 3, "png", 3))
		GetPNGWidthHeight(path, punWidth, punHeight);
	else
		printf("[GetPicWidthHeight]:only support jpg and png\n");
}


bool Tool::CopyEntireFile(char* src, char* dst)
{
	ifstream in(src, ios::binary);
	ofstream out(dst, ios::binary);
	if (!in.is_open()) return false;
	if (!out.is_open()) return false;
	if (src == dst) return false;
	char buf[2048];
	long long totalBytes = 0;
	while (in)
	{
		in.read(buf, 2048);
		out.write(buf, in.gcount());
		totalBytes += in.gcount();
	}
	in.close();
	out.close();
	return true;
}

void Tool::GetFileBaseName(char **baseName, char *fullName) {
	int len = strlen(fullName);
	char *ptr = fullName;
	ptr += len;
	while (--ptr != fullName) {
		if (ptr[0] == '\\' || ptr[0] == '/') {
			*baseName = ptr + 1;
			return;
		}
	}
	*baseName = fullName;
}

void Tool::GetFileExtendName(char **baseName, char *fullName) {
	int len = strlen(fullName);
	char *ptr = fullName;
	ptr += len;
	while (--ptr != fullName) {
		if (ptr[0] == '.') {
			*baseName = ptr + 1;
			return;
		}
	}
	*baseName = fullName;
}

char *Tool::ReadEntireFile(const char *filename) {
	FILE *hFile;
	fopen_s(&hFile,filename, "rb+");
	if (hFile) {
		fseek(hFile, 0, SEEK_END);
		long size = ftell(hFile);
		fseek(hFile, 0, SEEK_SET);
		char *buffer = new char[size + 1];
		fread(buffer, size, 1, hFile);
		fclose(hFile);
		return buffer;
	}
	else {
		return nullptr;
	}
}

char *Tool::ReadEntireUTF8File(char *filename) {
	char *tmpBuffer;
	char *ret;
	tmpBuffer = ReadEntireFile(filename);
	if (tmpBuffer) {
		ret = UTF82string(tmpBuffer);
		delete tmpBuffer;
		return ret;
	}
	else {
		return nullptr;
	}
}

char *Tool::GetJsonBegin(char *fullstring) {
	char *jsonBegin = fullstring;
	while (jsonBegin++ && jsonBegin - fullstring < 100) {
		if (jsonBegin[0] == '{') {
			return jsonBegin;
		}
	}
	return nullptr;
}

char *Tool::GetDirByFullName(char *fullstring) {
	int len = strlen(fullstring);
	char *jsonBegin = fullstring + len;
	while (--jsonBegin > fullstring) {
		if (jsonBegin[0] == '\\') {
			jsonBegin[0] = 0;
			return fullstring;
		}
	}
	return fullstring;
}

char *Tool::string2UTF8(char *str) {
	int len = strlen(str);
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wchar_t * pwBuf = new wchar_t[nwLen + 1];
	ZeroMemory(pwBuf, nwLen * 2 + 2);
	::MultiByteToWideChar(CP_ACP, 0, str, len, pwBuf, nwLen);
	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char * pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);
	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	string retStr(pBuf);
	delete[]pwBuf;
	pwBuf = NULL;
	return pBuf;
}

char *Tool::UTF82string(char *str){
	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	wchar_t * pwBuf = new wchar_t[nwLen + 1];
	memset(pwBuf, 0, nwLen * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, str, strlen(str), pwBuf, nwLen);
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char * pBuf = new char[nLen + 1];
	memset(pBuf, 0, nLen + 1);
	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	delete[]pwBuf;
	pwBuf = NULL;
	return pBuf;
}