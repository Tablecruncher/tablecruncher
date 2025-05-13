#ifndef _CSVPARSER_HH
#define _CSVPARSER_HH


#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <istream>
#include <cstdint>
#include <inttypes.h>

// for reading utf16
#include <locale>
#include <codecvt>


#include "helper.hh"
#include "globals.hh"
#include "csvdatastorage.hh"

#ifndef TCRUNCHER_PARSER_WITHOUT_UPDATE
#include "csvapplication.hh"
#include "csvwindow.hh"
#endif


#define CSVPARSER_CONST_ENCLOSED 1
#define CSVPARSER_CONST_NOT_ENCLOSED 0
#define CSVPARSER_LINES_TO_TEST_FOR_CSV_TYPE 5



/**
 * \brief The custom CSV parser class.
 * 
 * Implements a finite state machine to parse the CSV data provided by a `std::istream` object.
 * 
 */
class CsvParser {
	typedef struct {
		int octet;
		union {
			uint16_t codeUnit16;
			uint32_t codeUnit32;
		};
	} codeUnitReturn_t;
public:
	std::map<long,long> parseCsvStream( std::istream *input, CsvDataStorage &storage, CsvDefinition *definition, int maxLines=0, bool resizeRows=true );
private:
	int parseCsvState = CSVPARSER_CONST_NOT_ENCLOSED;
	std::string parseCsvRemaining = "";
	
	void parseCsvLine(std::vector<std::string>& vector, const std::string &line, CsvDefinition *definition);
	static std::istream& myGetline(std::istream& is, std::string& t);
	static std::istream& myGetlineEncodings(std::istream& is, std::string& t, CsvDefinition::Encodings enc);
	static codeUnitReturn_t getNextCodeUnit(std::streambuf *sb, int unitLength, bool bigEndian=true);
};



#endif


