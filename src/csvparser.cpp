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
*	CsvParser
*
************************************************************************************/


#include "csvparser.hh"
#include "helper.hh"

#include <sstream>
// for reading utf16
#include <locale>
#include <codecvt>


#ifndef TCRUNCHER_PARSER_WITHOUT_UPDATE
// needed for updateStatusbar()
extern CsvApplication app;
extern CsvWindow windows[];
#endif


/**
 *	\brief Parses 'input' and stores the CSV data into the given `CsvDataStorage` object.

 *	'definition' tells what CSV dialect is used.
 *	The vec-of-vec gets enlarged while ensuring all vectors have the same length.

 *	IMPORTANT: Update any variables afterwards that store the dimension of the table.
 *	
 *	\param input The stream that should get parsed
 *	\param storage The `CsvDataStorage` object where the data gets stored using `push_back()`.
 *	\param definition Pointer to the definition of the CSV data in `input`.
 *	\param maxLines	maximum number of lines to return – used for probing
 *	\param resizeRows	True: resize rows, all rows have the same length, False: don't resize – used for probing.
 *				if !resize: CsvDataStorage::numColumns doesn't get updated!
 *
 *	@return		list indicating different row lengths

 */
std::map<long,long> CsvParser::parseCsvStream( std::istream *input, CsvDataStorage &storage, CsvDefinition *definition, int maxLines, bool resizeRows ) {
	std::string line;
	std::stringstream sstr;
	long act_rows = 0;
	long act_cols = 0;
	std::vector<std::string> vec;
	std::map<long,long> rowLengths;
	
	// skip bomBytes
	input->ignore(definition->bomBytes);

	// read lines from istream `input` into `line`
	while( myGetlineEncodings( *input, line, definition->encoding) ) {
		if( parseCsvState == CSVPARSER_CONST_NOT_ENCLOSED ) {
			// if this line has been fully parsed, clear previous line vector
			vec.clear();
		}

		// translate encodings to valid UTF-8
		switch( definition->encoding ) {
			case CsvDefinition::ENC_NONE:
			case CsvDefinition::ENC_UTF8:
				Helper::fixUtf8(line);				// replaces invalid chars with replacement char
			break;
			case CsvDefinition::ENC_Latin1:
				line = Helper::latin1toutf8(line);
			break;
			case CsvDefinition::ENC_Win1252:
				line = Helper::win1252toutf8(line);
			break;
			case CsvDefinition::ENC_UTF16LE:			// translation happens in myGetlineEncodings()
			case CsvDefinition::ENC_UTF16BE:			// translation happens in myGetlineEncodings()
			default:
			break;
		}
		// parse `line` and put cells into `vec`
		parseCsvLine(vec, line, definition);
		if( parseCsvState == CSVPARSER_CONST_NOT_ENCLOSED ) {
			// if this line has been fully parsed
			
			// calculate histogram data
			size_t vecSize = vec.size();
			if( input->rdstate() != std::ios_base::eofbit ) {
				// only record when we're not at the last line
				rowLengths[vecSize]++;
			}
			
			// resize rows
			if( resizeRows && vec.size() ) {
				if( act_cols < (long)vec.size() ) {
					// parsed line is longer than columns(): resize storage
					storage.resize(0, (long)vec.size());
				} else if( act_cols > (long)vec.size() ) {
					// die aktuelle Zeile ist kürzer als die bisherigen Zeilen: die aktuelle Zeile verlängern
					vec.resize( act_cols );
				}
				act_cols = std::max( act_cols, (long)vec.size() );
			}
			

// #ifdef DEBUG
// printf("--------------------------------------\n");
// printf("DEL: _%c_\n", definition->delimiter);
// printf("ENC: %s\n", CsvDefinition::getEncodingName(definition->encoding).c_str());
// printf("ESC: %c\n", definition->escape);
// printf("QUO: %c\n", definition->quote);
// printf("ROW: %s\n", line.c_str());
// printf("Columns: %ld\n", (long) vec.size());
// for( size_t i = 0; i < vec.size(); ++i) printf("%ld: |%s|\n", (long) i, vec[i].c_str());
// printf("**************************************\n");
// #endif

			// add parsed line back to table storage
			storage.push_back(vec);
			++act_rows;
			// show an update every 25,000 rows in the status bar – very expensive!
			if( act_rows % 25000 == 0 ) {
				sstr.clear();
				sstr.str("");
				sstr << "Parsed " << act_rows << " lines.";
				#ifndef TCRUNCHER_PARSER_WITHOUT_UPDATE
				windows[app.getTopWindow()].updateStatusbar(sstr.str());
				#endif
			}
			if( maxLines && act_rows >= maxLines) {
				break;
			}
		}
		// stop parsing if input has been fully consumed
		if( input->rdstate() == std::ios_base::eofbit )
			break;
	} // end of while( myGetlineEncodings() )

	// Delete last row if storage is not empty
	if( storage.rows() )
		storage.deleteRows( storage.rows() - 1, storage.rows() - 1 );
	
	// #ifdef DEBUG
	// printf("rowLengths: %zu\n", rowLengths.size());
	// for( auto const & kv : rowLengths) {
	// 	printf("Length %ld: %ld\n", kv.first, kv.second);
	// }
	// #endif
	
	return rowLengths;
}


/*
 	Expects 'line' to be in UTF8.

 */
void CsvParser::parseCsvLine(std::vector<std::string>& vector, const std::string &line, CsvDefinition *definition) {
	bool enclosed = false;				// true: Falls wir innerhalb von Quotes sind
	bool startField = true;				// true: immer zu Beginn eines Feldes

	long lineLen = line.size();
	std::string real_field = "";
	
	if( parseCsvState == CSVPARSER_CONST_ENCLOSED ) {
		enclosed = true;
		real_field = parseCsvRemaining + "\n";		// LF "\n" is the right thing to do, as CRLF "\r\n" would be shown as "^M" in CODE_INPUT_WIDGET TODO doesn't this change inlne "\r\n" to "\n"??
	} else {
		vector.clear();			
	}

	real_field.reserve(10000);			// reserve 10kb to increase parsing performance: boosts performance by factor of 4

	for( long i = 0; i < lineLen; i++ ) {

// #ifdef DEBUG
// printf("  Char: %c", line[i]);
// if( enclosed ) printf(" (enclosed)");
// printf("\n");
// #endif

		if( definition->quote != definition->escape && line[i] == definition->escape ) {
			if( i < lineLen - 1 ) {
				// nicht das letzte Zeichen: folgendes Zeichen zurückschreiben
				real_field.push_back(line[i+1]);
				++i;
				continue;
			}
		}

		if( line[i] == definition->quote ) {
			// QUOTE gefunden
			if( i < lineLen - 1 && line[i+1] == definition->quote ) {
				// Doppelquote: ""
				if( enclosed ) {
					real_field.push_back(line[i]);
					++i;
					continue;		
				} else {
					enclosed = true;
					continue;
				}
			} else {
				// Einfaches Quote: "
				if( enclosed ) {
					// Quotierung endet
					enclosed = false;
				} else if( startField ) {
					// Wir sind nicht enclosed, und am Beginn eines Feldes
					enclosed = true;
				} else {
					// Wir sind nicht enclosed, aber mitten im Feld: einfaches Quote zurückschreiben
					real_field.push_back(line[i]);
				}
				continue;
			}
		}

		if( startField ) {
			// wir sind am Beginn eines Feldes, aber wir haben kein Quotierungszeichen
			startField = false;
		}


		if( line[i] == definition->delimiter && !enclosed ) {
			// Zeichen ist ein Seperator und wir sind nicht quotiert: aktuelles Feld zurückschreiben
			vector.push_back( real_field );
			real_field = "";
			enclosed = false;
			startField = true;
			continue;
		}
		
		real_field.push_back(line[i]);

	}	// END for

	if( !enclosed ) {
		// Zeile abgearbeitet und wir sind nicht mehr quotiert: aktuelles Feld zurückschreiben
		vector.push_back( real_field );
		parseCsvState = CSVPARSER_CONST_NOT_ENCLOSED;
	} else {
		// wir sind am Zeilenende quotiert!? neue Zeile quotiert beginnen und Zeile noch nicht an storage anhängen
		parseCsvState = CSVPARSER_CONST_ENCLOSED;
		parseCsvRemaining = real_field;
	}

}




//
/*
	myGetline()

	Used by myGetlineEncodings()

	http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
	Just works for ASCII-based 8-bit encodings like ASCII, Latin1, Latin9, Win1252 and UTF8
 */
std::istream& CsvParser::myGetline(std::istream& is, std::string& t) {
	t.clear();

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case EOF:
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
		case '\0':
			// skip NULL bytes
			break;
        default:
            t += (char)c;
        }
    }
}


/*
 *	For UTF16 and UTF32 encodings with LF or CRLF newline – doesn't work with CR only newline
 *	https://stackoverflow.com/a/50714844/2771733
 *	TODO UTF-32LE and UTF-32BE
 */
std::istream& CsvParser::myGetlineEncodings(std::istream& is, std::string& t, CsvDefinition::Encodings enc) {
	if( enc == CsvDefinition::ENC_UTF8 ||
		enc == CsvDefinition::ENC_Latin1 ||
		enc == CsvDefinition::ENC_Latin9 ||
		enc == CsvDefinition::ENC_Win1252
	 ) {
		return myGetline(is, t);
	}
	codeUnitReturn_t nextCodeUnit;
	int unitLength = 1;
	bool bigEndian = true;
	
#ifdef DEBUG
	std::cerr << "myGetlineEncodings" << std::endl;
#endif
	
	// get length of used code unit
	if( enc == CsvDefinition::ENC_UTF16BE || enc == CsvDefinition::ENC_UTF16LE )
		unitLength = 2;
	if( enc == CsvDefinition::ENC_UTF32BE || enc == CsvDefinition::ENC_UTF32LE )
		unitLength = 4;
	// get endian order
	if( enc == CsvDefinition::ENC_UTF16LE || enc == CsvDefinition::ENC_UTF32LE )
		bigEndian = false;

	t.clear();
    std::istream::sentry se(is, true);
    std::streambuf *sb = is.rdbuf();

    for(;;) {
		nextCodeUnit = getNextCodeUnit(sb, unitLength, bigEndian);
		if( enc == CsvDefinition::ENC_UTF16BE || enc == CsvDefinition::ENC_UTF16LE ) {
			if( nextCodeUnit.codeUnit16 == 0x000A ) {
				break;
			}
		}
		if( nextCodeUnit.octet == EOF ) {
		    if(t.empty()) {
				is.setstate(std::ios::eofbit);
		    }
			break;
		}
		if( unitLength == 2 ) {
			if( nextCodeUnit.codeUnit16 >= 0xD800 && nextCodeUnit.codeUnit16 <= 0xDBFF ) {
				codeUnitReturn_t temp = getNextCodeUnit(sb, unitLength, bigEndian);
				t += Helper::utf16_utf8(nextCodeUnit.codeUnit16, temp.codeUnit16);
			} else {
				t += Helper::utf16_utf8(nextCodeUnit.codeUnit16,0);
			}
		}
		
    }
	
	if( t.length() && t.back() == '\r') {
		t.pop_back();
	}

	return is;
}




// returns a code unit (and the value of the last octet so that calling methods can see an EOF marker)
CsvParser::codeUnitReturn_t CsvParser::getNextCodeUnit(std::streambuf *sb, int unitLength, bool bigEndian) {
	uint8_t streamBytes[4];
	codeUnitReturn_t ret;
	int octet = 0;
	// write unitLength bytes into codeUnit32 in given order (BE or LE)
	for(int i=0; i<unitLength; ++i) {
		octet = sb->sbumpc();
		if( octet == EOF )
			break;
		streamBytes[i] = (uint8_t) octet;
	}
	ret.octet = octet;
	if( unitLength == 4 ) {
		if( bigEndian )
			ret.codeUnit32 = Helper::pack32(streamBytes[3],streamBytes[2],streamBytes[1],streamBytes[0]);	// Big Endian
		else
			ret.codeUnit32 = Helper::pack32(streamBytes[0],streamBytes[1],streamBytes[2],streamBytes[3]);	// Little Endian
	} else if( unitLength == 2 ) {
		if( bigEndian )
			ret.codeUnit16 = Helper::pack16(streamBytes[1],streamBytes[0]);						// Big Endian
		else
			ret.codeUnit16 = Helper::pack16(streamBytes[0],streamBytes[1]);						// Little Endian
	}
	return ret;
}








