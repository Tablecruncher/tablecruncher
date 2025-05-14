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


