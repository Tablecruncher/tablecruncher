/* 
 * SPDX-License-Identifier: GPL-3.0-or-later
 * 
 * Copyright (C) 2025 Stefan Fischerländer
 * 
 * This file is part of Tablecruncher.
 * 
 * Tablecruncher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * Tablecruncher is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Tablecruncher. If not, see <https://www.gnu.org/licenses/>.
 */


/************************************************************************************
*
*	Helper
*
************************************************************************************/


#include "helper.hh"


/**
	Calculates the memory usage of tableData
 */
size_t Helper::calculateMemUsage(size_t rows, size_t cols, size_t fileLength) {
	size_t cellLength = 0;
	if( rows || cols ) {
		cellLength = fileLength / (rows*cols);
	}
	return rows * 12  +  rows * cols * ( 26 + cellLength );
}



bool Helper::isUpdateAvailable(std::string myVersion, std::string serverVersion) {
	int major, minor, revision, build;
	sscanf(myVersion.c_str(), "%d.%d.%d.%d", &major, &minor, &revision, &build);
	std::tuple<int, int, int, int> myVersionTuple = { major,minor,revision,build };
	sscanf(serverVersion.c_str(), "%d.%d.%d.%d", &major, &minor, &revision, &build);
	std::tuple<int, int, int, int> serverVersionTuple = { major,minor,revision,build };
	return myVersionTuple < serverVersionTuple;
}


/**
 *	Creates generic column names like "A", "B", ..., "Z", "AA", "AB", ... , "ZZ", "COL", "COL"
 *	https://stackoverflow.com/questions/2294443/base-conversion-problem
 */
std::string Helper::createGenericColumnNames(int col) {
	int v = col + 1;
	int base = 26;
	std::string res = "";
	while( v != 0 ) {
		--v;
		res.insert(0, 1, (char)('A' + (v % base)));
		v /= base;
	}
	return res;
}


/**
	Returns index for excel style column name
	Return -1 if colName is not a valid uppercase excel style column
	Inverse function to createGenericColumnNames()
 */
int Helper::genericColumnNameToIndex(std::string colName) {
	int base = 26;
	int col = 0;
	int pos = 0;
	bool error = false;
	for( int c = colName.size() - 1; c >= 0 ; --c ) {
		if( colName[c] < 'A' || colName[c] > 'Z' ) {
			error = true;
			break;
		}
		unsigned char digit = ((unsigned char) colName[c]) - '@';
		col += std::pow(base, pos) * digit;
		++pos;
	}
	if( error ) {
		col = 0;		// causes to return -1
	}
	return col - 1;
}


/*
	create a grouped string from the number `num`
	s is managed by the calling code, we assume there are `max_len` bytes available
	123 	=> 123
	123		=> 321 => 
	1234 	=> 1,234
	12345 	=> 12,345
	123456 	=> 123,456
	1234567 => 1,234,567
*/
std::string Helper::groupedIntToString( int num, std::string sep ) {
	// Caution: Only works for pure 7bit-ASCII text!
	std::string ret = "";
	std::string num_sign = "";
	if( num < 0 ) {
		num_sign = "-";
		num = -num;
	}
	std::string num_str = std::to_string(num);
	std::reverse(num_str.begin(), num_str.end());
	for( size_t i = 0; i < num_str.length(); ++i ) {
		ret.insert(0, 1, num_str[i]);
		if( i % 3 == 2 && i > 0 && i < num_str.length() - 1 ) {
			ret.insert(0, sep);
		}
	}
	return num_sign + ret;
}



// fixes UTF8 inplace – replaces invalid octets with replace character
void Helper::fixUtf8(std::string& str) {
    std::string temp;
	try {
	    utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
	} catch( utf8::not_enough_room& e ) {
#ifdef DEBUG
std::cerr << "utf8::not_enough_room: " << str << std::endl;
#endif
	}
    str = temp;
}


// converts Latin-1 to UTF-8
std::string Helper::latin1toutf8(std::string text) {
	uint8_t ch;
	std::string out = "";
	
	for( char& c : text) {
		ch = (uint8_t) c;
		if(ch < 0x80) {
		    out.push_back(static_cast<char>(ch));
		} else {
			// N.B. Latin 1 has no printable characters within the range 0x80 to 0x9F
			// Latin 1
			if( ch > 0x9F) {
			    out.push_back( static_cast<char>(0xc0 | (ch & 0xc0) >> 6) );
				out.push_back( static_cast<char>(0x80 | (ch & 0x3f)) );
			}
		}
	}
	return out;
}


// converts Windows 1252 to UTF-8
std::string Helper::win1252toutf8(std::string text) {
	uint8_t ch;
	std::string out = "";
	
	for( char& c : text) {
		ch = (uint8_t) c;
		if(ch < 0x80) {
		    out.push_back(static_cast<char>(ch));
		} else {
			// Windows 1252
			if( ch >= 0x80 && ch <= 0x9F) {
				switch( ch ) {
					case 0x80:                        // €
					out += "\xE2\x82\xAC";
					break;
					case 0x82:                        // ‚
					out += "\xE2\x80\x9A";
					break;
					case 0x83:                        // ƒ
					out += "\xC6\x92";
					break;
					case 0x84:                        // „
					out += "\xE2\x80\x9E";
					break;
					case 0x85:                        // …
					out += "\xE2\x80\xA6";
					break;
					case 0x86:                        // †
					out += "\xE2\x80\xA0";
					break;
					case 0x87:                        // ‡
					out += "\xE2\x80\xA1";
					break;
					case 0x88:                        // ˆ
					out += "\xCB\x86";
					break;
					case 0x89:                        // ‰
					out += "\xE2\x80\xB0";
					break;
					case 0x8A:                        // Š
					out += "\xC5\xA0";
					break;
					case 0x8B:                        // ‹
					out += "\xE2\x80\xB9";
					break;
					case 0x8C:                        // Œ
					out += "\xC5\x92";
					break;
					case 0x8E:                        // Ž
					out += "\xC5\xBD";
					break;
					case 0x91:                        // ‘
					out += "\xE2\x80\x98";
					break;
					case 0x92:                        // ’
					out += "\xE2\x80\x99";
					break;
					case 0x93:                        // “
					out += "\xE2\x80\x9C";
					break;
					case 0x94:                        // ”
					out += "\xE2\x80\x9D";
					break;
					case 0x95:                        // •
					out += "\xE2\x80\xA2";
					break;
					case 0x96:                        // –
					out += "\xE2\x80\x93";
					break;
					case 0x97:                        // —
					out += "\xE2\x80\x94";
					break;
					case 0x98:                        // ˜
					out += "\xCB\x9C";
					break;
					case 0x99:                        // ™
					out += "\xE2\x84\xA2";
					break;
					case 0x9A:                        // š
					out += "\xC5\xA1";
					break;
					case 0x9B:                        // ›
					out += "\xE2\x80\xBA";
					break;
					case 0x9C:                        // œ
					out += "\xC5\x93";
					break;
					case 0x9E:                        // ž
					out += "\xC5\xBE";
					break;
					case 0x9F:                        // Ÿ
					out += "\xC5\xB8";
					break;					
				}
			} else {
			    out.push_back( static_cast<char>((0xc0 | (ch & 0xc0) >> 6)) );
				out.push_back( static_cast<char>(0x80 | (ch & 0x3f)) );
			}
		}
	}
	
	return out;
}



// converts UTF-8 to Latin-1
// https://stackoverflow.com/questions/23689733/convert-string-from-utf-8-to-iso-8859-1
// converts UTF8 to Latin1 or Win1252
// TODO Move UTF-8 decoding to its own method
std::string Helper::utf8tolatin1(std::string text, bool win1252) {
    std::string out;
	const char *in;
	
    if (text == "") {
        return out;
	}
	
	in = text.c_str();
	
    unsigned int codepoint = 0;
    while (*in != 0) {
        unsigned char ch = static_cast<unsigned char>(*in);
        if (ch <= 0x7f)
            codepoint = ch;
        else if (ch <= 0xbf)
            codepoint = (codepoint << 6) | (ch & 0x3f);
        else if (ch <= 0xdf)
            codepoint = ch & 0x1f;
        else if (ch <= 0xef)
            codepoint = ch & 0x0f;
        else
            codepoint = ch & 0x07;
        ++in;
        if( ((*in & 0xc0) != 0x80) && (codepoint <= 0x10ffff) ) {
            if( codepoint <= 127 ) {
                out.append(1, static_cast<char>(codepoint));
            } else {
				if( win1252 && unicode2win1252.count(codepoint) ) {
	                out.append(1, unicode2win1252[codepoint]);
				} else if( codepoint <= 255) {
	                out.append(1, static_cast<char>(codepoint));
				} else {
	                out.append(1, static_cast<char>(127));
				}
            }
        }
    }
    return out;
}



std::string Helper::utf8toutf16(std::string text, bool bigEndian) {
    std::string out;
    unsigned int codepoint = 0;
    const char *in;

    if (text == "") {
        return out;
	}
    in = text.c_str();
	
    while (*in != 0) {
        unsigned char ch = static_cast<unsigned char>(*in);
        // calculate codepoint
        if (ch <= 0x7f)
            codepoint = ch;
        else if (ch <= 0xbf)
            codepoint = (codepoint << 6) | (ch & 0x3f);
        else if (ch <= 0xdf)
            codepoint = ch & 0x1f;
        else if (ch <= 0xef)
            codepoint = ch & 0x0f;
        else
            codepoint = ch & 0x07;
        ++in;
        if( ((*in & 0xc0) != 0x80) && (codepoint <= 0x10ffff) ) {
			// codepoint is complete
			uint16_t high, low;
			std::tie(high, low) = encodeUtf16(codepoint);
			if( bigEndian ) {
				if( low ) {
					out += *((unsigned char *)(&high)+1);
					out += *((unsigned char *)(&high));
					out += *((unsigned char *)(&low)+1);
					out += *((unsigned char *)(&low));
				} else {
					out += *((unsigned char *)(&high)+1);
					out += *((unsigned char *)(&high));
				}
			} else {
				if( low ) {
					out += *((unsigned char *)(&high));
					out += *((unsigned char *)(&high)+1);
					out += *((unsigned char *)(&low));
					out += *((unsigned char *)(&low)+1);
				} else {
					out += *((unsigned char *)(&high));
					out += *((unsigned char *)(&high)+1);
				}
			}
        }
	}
	return out;
}


/*
 * Checks if string is a number - a little bit buggy ... TODO
 */
bool Helper::isNumber(const std::string& str) {
    bool isNum = true;
    size_t str_len = str.size();
    for(size_t i = 0; i < str_len; ++i ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if( !std::isdigit(c) && (c != '.' || c != ',' || c != '-') ) {
            isNum = false;
            break;
        }
    }
    return isNum;
}


/*
 * Checks if string is a float
 * decimal_point: defaults to '.'
 */
bool Helper::isFloat(const std::string& s, char decimal_point) {
	unsigned int count_decimal_points = 0;
    std::string::const_iterator it = s.begin();
	if( *it == '-' ) {
		++it;
	}
    while( it != s.end() && (std::isdigit(*it) || *it == decimal_point) ) {
		if( *it == decimal_point ) {
			++count_decimal_points;
		}
		++it;
	}
    return !s.empty() && it == s.end() && (count_decimal_points <= 1 );
}

/*
 * Checks if string is a number - a little bit buggy ... TODO
 */
bool Helper::isInteger(const std::string& s) {
	return std::all_of(s.begin(), s.end(), ::isdigit);
}


/*
 *	Parses the given string and returns an integer or a float.
 */
Helper::parseNumberStruct Helper::parseNumber(const std::string& s) {
	size_t myIndex;
	long long myInteger;
	long double myFloat;
	struct parseNumberStruct ret;
	
	try {										// convert to long long
		myInteger = std::stoll( s, &myIndex );
		if( myIndex < s.size() ) {
			throw myInteger;
		}
		ret.myType = parseNumberType::INT;
		ret.myInteger = myInteger;
	} catch(...) {
		try {									// convert to long double
			myFloat = std::stold( s, &myIndex );
			if( myIndex < s.size() ) {
				throw myFloat;
			}
			ret.myType = parseNumberType::FLOAT;
			ret.myFloat = myFloat;
		} catch(...) {							// use cell as string
			ret.myType = parseNumberType::NONE;
		}
	}
	return ret;
}



/*
 *	Returns the filename of a path: e.g. "file.csv" from "/a/b/file.csv"
 */
std::string Helper::getBasename(const std::string& path) {
	size_t found = path.find_last_of("/\\");
	return(path.substr(found+1));
}

/*
 *	Returns the directory part of a path: e.g. "/a/b/" from "/a/b/file.csv"
 */
std::string Helper::getDirectory(const std::string& path) {
	size_t found = path.find_last_of("/\\");
	return(path.substr(0, found));
}

/*
 *	Returns the <path without an extension,extension>:
 *		/a/b/file.csv		=> <"/a/b/file","csv">
 *		file.csv			=> <"file","csv">
 *		/a/b/file			=> <"/a/b/file","">
 *		/abc.xyz/file.csv	=> <"/abc.xyz/file","csv">
 *		/abc.xyz/file		=> <"/abc.xyz/file","">
 */
std::pair<std::string,std::string> Helper::getPathWithoutExtension(const std::string& path) {
	size_t lastSlash = path.find_last_of("/\\");
	size_t lastDot = path.find_last_of(".");
	if( lastDot == std::string::npos || lastDot < lastSlash) {
		return(std::make_pair(path,""));
	}
	std::string extension = path.substr(lastDot+1);
	std::string pathWithoutExtension = path.substr(0,lastDot);
	return(std::make_pair(pathWithoutExtension,extension));
}


std::string Helper::padInteger(int num, int length) {
	char buf[101];
	std::string formatString = "%0"+std::to_string(length)+"d";
	std::snprintf(buf,100,formatString.c_str(),num);
	std::string ret = buf;
	return(ret);
}

// https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
long Helper::getFileSize(std::string filename) {
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}


/*
 *	Returns true, if firstRow seems to be a header row
 *	@param		firstRow		The row to be checked.
 */
bool Helper::guessHasHeader(std::vector<std::string> firstRow) {
	int cols = firstRow.size();
	for( int c = 0; c < cols; ++c ) {
		// Number in first row => NO HEADER
		try {
			(void)std::stod(firstRow[c]);
			return false;
		} catch( const std::exception& ) {
			// intentionally left empty as we use lexical_cast() to check if its numeric
		}
		// E-Mail in first row => NO HEADER
		if( isEmailAddress(firstRow[c]) ) {
			return false;
		}
		// Something looks like a date string => NO HEADER
		if( isSomeDate(firstRow[c]) ) {
			return false;
		}
		// Seems to be a URL => NO HEADER
		if( firstRow[c].find("://") != std::string::npos ) {
			return false;
		}
		// Contains some other 'strange' characters => NO HEADER
		if( firstRow[c].find_first_of("{}*?!,.;\"'§$%&/\\=+") != std::string::npos ) {
			return false;
		}
		// Cell is empty string => NO HEADER
		if( firstRow[c] == "" ) {
			return false;
		}
	}
	return true;
}
bool Helper::isEmailAddress(const std::string& email) {
	const std::regex pattern ("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
	return std::regex_match(email, pattern);
}
bool Helper::isSomeDate(const std::string& dateString) {
	const std::regex pattern ("^\\d+[\\.|\\-|\\|]\\d+[\\.|\\-|\\|]\\d+");
   return std::regex_match(dateString, pattern);
}


/*
	https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string/236803#236803
 */
std::vector<std::string> Helper::splitString(std::string sep, std::string str, size_t maxSplits) {
	std::vector<std::string> tokens;
	std::size_t start = 0;
	std::size_t end = 0;
	std::size_t splitLen = sep.size();
	while( (end = str.find(sep, start)) != std::string::npos ) {
		tokens.push_back( str.substr(start, end - start) );
		start = end + splitLen;
		if( maxSplits > 0 && tokens.size() == maxSplits )
			break;
	}
	tokens.push_back(str.substr(start));
	return tokens;
}


/**
 *	Static helper method that dumps a vector of vector of string to cout.
 *	maxRows optional
 *	colWidth optional
 */
void Helper::dumpVecVec( std::vector<std::vector<std::string>> &vec, size_t maxRows, int colWidth) {
	for( size_t i = 0; i < vec.size(); ++i ) {
		for( size_t j = 0; j < vec.at(i).size(); ++j ) {
			printf( "%*s|", colWidth, vec.at(i).at(j).c_str() );
		}
		// std::cout << std::endl;
		printf("\n");
		if( i > maxRows )
			break;
	}
}




std::map<const unsigned int, const unsigned char> Helper::unicode2win1252 = {
	{0x20ac, 128},
	{0x201a, 130},
	{0x0192, 131},
	{0x201e, 132},
	{0x2026, 133},
	{0x2020, 134},
	{0x2021, 135},
	{0x02c6, 136},
	{0x2030, 137},
	{0x0160, 138},
	{0x2039, 139},
	{0x0152, 140},
	{0x017d, 142},
	{0x2018, 145},
	{0x2019, 146},
	{0x201c, 147},
	{0x201d, 148},
	{0x2022, 149},
	{0x2013, 150},
	{0x2014, 151},
	{0x02dc, 152},
	{0x2122, 153},
	{0x0161, 154},
	{0x203a, 155},
	{0x0153, 156},
	{0x017e, 158},
	{0x0178, 159}
};


//
//	Set path to resources folder in app bundle
//	Caution: PATH_MAX is defined in /usr/include/sys/syslimits.h
std::string Helper::getAppBundlePath() {
	#ifdef __APPLE__
	char path[PATH_MAX+1];
    #elif _WIN64
	char path[MAX_PATH+1];
    #endif
	std::string cpp_path = "";

	#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if( !CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX) ) {
		std::cout << "Error: CFURLGetFileSystemRepresentation" << std::endl;
	} else {
		cpp_path = path;
	}
	CFRelease(resourcesURL);
	#elif _WIN64
		char own_path[MAX_PATH+1];
		HMODULE hModule = GetModuleHandle(NULL);
		if (hModule != NULL) {
        	GetModuleFileName(hModule, own_path, MAX_PATH);
			cpp_path = own_path;
			const size_t last_slash_idx = cpp_path.find_last_of("\\/");
			if (std::string::npos != last_slash_idx) {
    			cpp_path.erase(last_slash_idx + 1);
			}
     	}
	#endif
	return cpp_path;
}

std::string Helper::getHomeDir() {
	const char *homeDir;
	std::string homeDirStr = "";
	#ifdef __APPLE__
	homeDir = getenv("HOME");
	if( !homeDir ) {
		struct passwd* pwd = getpwuid(getuid());
		if( pwd )
			homeDir = pwd->pw_dir;
	}
	#elif _WIN64
	homeDir = getenv("USERPROFILE");
	if( !homeDir ) {
		homeDir = "";
	}
	#elif __linux__
	homeDir = getenv("HOME");
	#endif
	if( homeDir != nullptr ) {
		homeDirStr.assign(homeDir);
	}
	return homeDirStr;
}

int64_t Helper::getPhysMemSize() {
	#ifdef __APPLE__
	int mib[2];
	int64_t physical_memory;
	size_t length;

	// Get the Physical memory size
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE;
	length = sizeof(int64_t);
	sysctl(mib, 2, &physical_memory, &length, NULL, 0);
	return physical_memory;
	#elif _WIN64
	return 8000000;			// TODO remove hard coded physical memory size on Windows
	#else
	return 8000000;			// TODO remove hard coded physical memory size on Linux
	#endif
}


// Pack bytes into a 16- or 32-byte variable
uint32_t Helper::pack32(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
	return (uint32_t) c0 | ((uint32_t) c1 << 8) | ((uint32_t) c2 << 16) | ((uint32_t) c3 << 24);
}
uint16_t Helper::pack16(uint8_t c0, uint8_t c1) {
	return (uint32_t) c0 | ((uint32_t) c1 << 8);
}


// Converts High- and Low-Surrogate to UTF-8 code units, stored in a std::string
std::string Helper::utf16_utf8(uint16_t high, uint16_t low) {
	if( high <= 0xD7FF || high >= 0xE000) {
		// just a BMP code point
		return encodeUtf8(high);
	}
	uint32_t cp = (high - 0xD800) * 0x400 + (low - 0xDC00) + 0x10000;  // TODO * 0x400 => >> 10
	return encodeUtf8(cp);
}



/*
 *	Returns a tuple (high,low) as UTF-16
 */
std::tuple<uint16_t, uint16_t> Helper::encodeUtf16(uint32_t cp) {
	if( cp <= 0xD7FF || (cp >= 0xE000 && cp <= 0xFFFF) ) {
		return std::make_tuple(cp,0);
	} else if( cp >= 0x10000 && cp <= 0x10FFFF ) {
		uint16_t high = ((cp - 0x10000) >> 10) + 0xD800;
		uint16_t low = ((cp - 0x10000) & 0x3FF) + 0xDC00;
		return std::make_tuple(high,low);
	} else {
		return std::make_tuple(cp,0);
	}
}



// Encode a code point as UTF-8
// https://stackoverflow.com/questions/23502153/utf-8-encoding-algorithm-vs-utf-16-algorithm
// printf("TEST: %s\n",Helper::encodeUtf8(65).c_str()); => A
// printf("TEST: %s\n",Helper::encodeUtf8(0xA2).c_str()); => ¢
// printf("TEST: %s\n",Helper::encodeUtf8(0x20AC).c_str()); => €
// printf("TEST: %s\n",Helper::encodeUtf8(0x1D11E).c_str()); => 𝄞
std::string Helper::encodeUtf8(uint32_t cp) {
	std::string ret = "";
	unsigned char chars[4];
	
	if( cp <= 0x7F ) {
		ret = (char) cp;
	} else if( cp > 0x10FFFF) {
		ret = "\xEF\xBF\xBD";  				// U+FFFD REPLACEMENT CHARACTER
	} else {
		int count;
		if (cp <= 0x7FF) {
			count = 1;
		} else if (cp <= 0xFFFF) {
			count = 2;
		} else {
			count = 3;
		}
		for (int i = 0; i < count; ++i) {
			chars[count-i] = 0x80 | (cp & 0x3F);
			cp >>= 6;
		}
		chars[0] = (0x1E << (6-count)) | (cp & (0x3F >> count));
		for(int i = 0; i <= count; ++i) {
			ret += chars[i];
		}
	}
	return ret;
}


unsigned long Helper::getTimestamp() {
	unsigned long myTime = (std::chrono::system_clock::now().time_since_epoch().count()) / 1000000;
	return myTime;
}



unsigned int Helper::getFltkFontCode(std::string fontname) {
	// 0=FL_HELVETICA, 4=FL_COURIER, 8=FL_TIMES, 13=FL_SCREEN
	unsigned int ret = 0;
	if(fontname == "FL_COURIER") {
		ret = 4;
	} else if(fontname == "FL_TIMES") {
		ret = 8;
	} else if(fontname == "FL_SCREEN") {
		ret = 13;
	}
	return ret;
}






