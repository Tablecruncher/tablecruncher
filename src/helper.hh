#ifndef _HELPER_HH
#define _HELPER_HH


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <regex>
#include <sys/types.h>
#include <chrono>
#include <sys/stat.h>
#include <codecvt>
#include <cmath>

#include "utf8.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <pwd.h>
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifdef _WIN64
#include <windows.h>
#endif



class Helper {
public:
	enum parseNumberType {NONE, INT, FLOAT};
	struct parseNumberStruct {
		parseNumberType myType = NONE;
		long long myInteger = 0;
		long double myFloat = 0;
	};
	static size_t calculateMemUsage(size_t rows, size_t cols, size_t cellLength);
	static bool isUpdateAvailable(std::string myVersion, std::string serverVersion);
	static std::string createGenericColumnNames(int C);
	static int genericColumnNameToIndex(std::string colName);
	static std::string groupedIntToString(int num, std::string sep=",");
	static void fixUtf8(std::string& str);
	static std::string latin1toutf8(std::string text);
	static std::string win1252toutf8(std::string text);
	static std::string utf8tolatin1(std::string text, bool win1252=false);
	static std::string utf8toutf16(std::string text, bool bigEndian=true);
	static bool isNumber(const std::string& s);
	static bool isFloat(const std::string& s, char decimal_point = '.');
	static bool isInteger(const std::string& s);
	static bool isEmailAddress(const std::string& email);
	static bool isSomeDate(const std::string& dateString);
	static struct parseNumberStruct parseNumber(const std::string& s);
	static std::string getBasename(const std::string& path);
	static std::string getDirectory(const std::string& path);
	static std::pair<std::string,std::string> getPathWithoutExtension(const std::string& path);
	static std::string padInteger(int num, int length);
	static long getFileSize(std::string filename);
	static bool guessHasHeader(std::vector<std::string> firstRow);
	static std::vector<std::string> splitString(std::string sep, std::string str, size_t maxSplits=0);
	static void dumpVecVec( std::vector<std::vector<std::string>> &vec, size_t maxRows = 10, int colWidth = 10 );
	static std::string getAppBundlePath();
	static std::string getHomeDir();
	static int64_t getPhysMemSize();
	static uint32_t pack32(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3);
	static uint16_t pack16(uint8_t c0, uint8_t c1);
	static std::string utf16_utf8(uint16_t high, uint16_t low);
	static std::tuple<uint16_t, uint16_t> encodeUtf16(uint32_t cp);
	static std::string encodeUtf8(uint32_t cp);
	static unsigned long getTimestamp();
	static unsigned int getFltkFontCode(std::string fontname);
	static std::string ws_to_utf8(std::wstring const& s);
	static std::wstring utf8_to_ws(std::string const& utf8);
	static void log(std::string msg);
private:
	static std::map<const unsigned int, const unsigned char> unicode2win1252;
};



#endif
