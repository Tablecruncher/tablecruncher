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
*	CsvTable
*
************************************************************************************/


#include "csvtable.hh"
#include "macro.hh"


extern Macro macro;		// to use JS for searching


CsvTable::CsvTable() {
	CsvTable(0,0);
}
CsvTable::CsvTable(table_index_t start_rows, table_index_t start_cols) {
	storage.resize(start_rows, start_cols);
	headerRow = new std::vector<std::string>;
	headerRow->resize(start_cols);
	// Preset all cells
	for (table_index_t r = 0; r < storage.rows(); r++) {
		for (table_index_t c = 0; c < storage.columns(); c++) {
			setCell("", r, c);
		}
	}
}

CsvTable::CsvTable(const CsvTable &table) {
	size_t size;
	
	this->storage = table.storage;
	size = table.headerRow->size();
	this->headerRow = new std::vector<std::string>();
	this->headerRow->resize(size);
	for( size_t r = 0; r < size; ++r) {
		this->headerRow->at(r) = table.headerRow->at(r);
	}

	this->definition = table.definition;
	this->fileDefinition = table.fileDefinition;
	this->hasCustomHeaderRow = table.hasCustomHeaderRow;
	this->s_left = table.s_left;
	this->s_top = table.s_top;
	this->s_right = table.s_right;
	this->s_bottom = table.s_bottom;
}


CsvTable::~CsvTable() {
	delete(headerRow);
}



table_index_t CsvTable::getNumberCols() {
	return storage.columns();
}

table_index_t CsvTable::getNumberRows() {
	return storage.rows();
}

/*
 *	Returns the content of a cell of headerRow.
 *	If returnEmpty is false: Instead of an empty cell, the generic column name is returned (used for draw_cell and sort)
 */
std::string CsvTable::getHeaderCell(table_index_t c, bool returnEmpty) {
	if( c >= 0 && c < (table_index_t) headerRow->size() ) {
		if( hasCustomHeaderRow && (headerRow->at(c) != "" || returnEmpty) ) {
			return headerRow->at(c);
		} else {
			return Helper::createGenericColumnNames(c);
		}
	} else {
		return "COL";
	}
}


std::string CsvTable::getCell(table_index_t row, table_index_t col) {
	if( row >= 0 and row < storage.rows() and col >= 0 and col <= storage.columns() ) {
		return storage.get(row,col);
	} else {
		return "";
	}
}


void CsvTable::setCell(std::string content, table_index_t row, table_index_t col) {
	if( row >= 0 and row < getNumberRows() and col >= 0 and col <= getNumberCols() ) {
		storage.set(content, row, col);
	} else {
		// TODO Handle error
	}
}



/**
 *	Returns a vector of vector of string defined by the given 0-based coordinates.
 */
std::vector< std::vector<std::string> > CsvTable::copyBlock(table_index_t row_top, table_index_t col_top, table_index_t row_bot, table_index_t col_bot) {
	std::vector< std::vector<std::string> > block;
	std::vector<std::string> line;
	table_index_t rows, cols;
	
	for( rows = row_top; rows <= row_bot; ++rows ) {
		for( cols = col_top; cols <= col_bot; ++cols ) {
			line.push_back( storage.get(rows,cols) );
		}
		block.push_back( line );
		line.clear();
	}

	
	return block;
}


/**
	Return R-th row
 */
std::vector<std::string> CsvTable::row(table_index_t R) {
	return storage.row(R);
}


/**
 *	Adds an empty row after (or before) the given rowNr
 */
void CsvTable::addRow(table_index_t rowNr, bool before) {
	if( rowNr >= storage.rows() )
		rowNr = storage.rows() - 1;
	if( rowNr < 0 )
		rowNr = 0;

	storage.insertRow(rowNr, before);
	if( before ) {
		moveFlags(rowNr, 1);
	} else {
		moveFlags(rowNr+1, 1);
	}
	updateInternals();
}


/**
 *	Adds an empty column after (or before) the given colNr
 */
void CsvTable::addCol(table_index_t colNr, bool before) {
	if( colNr >= storage.columns() )
		colNr = storage.columns() - 1;
	if( colNr < 0 )
		colNr = 0;

	storage.insertColumn(colNr, before);
	
	// add column to headerRow
	if( hasCustomHeaderRow ) {
		if( before )
			headerRow->insert(headerRow->begin()+colNr, "");
		else
			headerRow->insert(headerRow->begin()+colNr+1, "");
	} else {
		headerRow->clear();
		headerRow->resize(storage.columns() + 1);
	}
	updateInternals();
}



void CsvTable::delRows(table_index_t rowFrom, table_index_t rowTo, bool clearFlags, bool doUpdateInternals ) {
	table_index_t numToDel = rowTo - rowFrom + 1;
	if( numToDel >= storage.rows() )				// don't delete, if all rows are to be deleted
		return;
	if( rowFrom < 0 || rowTo < 0 || rowFrom >= storage.rows() || rowTo >= storage.rows() )		// don't delete if row range is not valid
		return;

	storage.deleteRows(rowFrom, rowTo);

	// erase flags from rows that are deleted
	if( clearFlags ) {
		for( table_index_t r = rowFrom; r <= rowTo; ++r ) {
			if( flags.find(r) != flags.end() ) {
				flags.erase(r);
			}
		}
		moveFlags(rowFrom, -numToDel);
	}
	if( doUpdateInternals ) {
		updateInternals();
	}
}


void CsvTable::delCols(table_index_t colFrom, table_index_t colTo) {
	if( (colTo - colFrom + 1) >= storage.columns() )				// don't delete if all columns are to be deleted
		return;
	if( colFrom < 0 || colTo < 0 || colFrom >= storage.columns() || colTo >= storage.columns() )		// don't delete if column range is not valid
		return;
	
	storage.deleteColumns(colFrom, colTo);
	
	headerRow->erase( headerRow->begin() + colFrom, headerRow->begin() + colTo + 1 );
	updateInternals();
}


/*
 *	Move columns right or left
 */
void CsvTable::moveCols(table_index_t colFromStart, table_index_t colFromEnd, bool right) {
	std::string buffer;
	if( colFromStart > colFromEnd ) {
		return;
	}
	
	// Move columns
	storage.moveColumns(colFromStart, colFromEnd, right);
	
	// Move header row
	if( right ) {
		if( colFromEnd < storage.columns() - 1 && colFromStart >= 0 ) {
			buffer = headerRow->at(colFromEnd + 1);
			for( table_index_t c = colFromEnd; c >= colFromStart; --c ) {
				headerRow->at(c + 1) = headerRow->at(c);
			}
			headerRow->at(colFromStart) = buffer;
		}
	} else {
		if( colFromStart > 0 && colFromEnd < storage.columns() ) {
			buffer = headerRow->at(colFromStart - 1);
			for( table_index_t c = colFromStart; c <= colFromEnd; ++c ) {
				headerRow->at(c - 1) = headerRow->at(c);
			}
			headerRow->at(colFromEnd) = buffer;
		}
	}
	updateInternals();
}


/*
 *	Searches for substring starting at the next cell from (myRow, myCol).
 *	Returns (row,col) if found, (-1,-1) if not
 */
std::tuple<table_index_t, table_index_t> CsvTable::findSubstring(std::string search, table_index_t startRow, table_index_t startCol, std::vector<table_index_t> sel, bool caseSensitive, bool useRegex) {
	table_index_t r;
	table_index_t c;
	int cnt;						// `cnt` is used to avoid endless loops in do-while below
	table_index_t searchAreaSize;
	std::string lowerSearch;
	std::string status;
	searchArea = sel;
	searchAreaSize = (sel[2]-sel[0]+1) * (sel[3]-sel[1]+1);
	std::tuple<table_index_t, std::string> executeReturn;				// for JS search
	
	//
	// if a Regex search, use JS 
	//
	if( useRegex ) {
		std::string js = createJsFindSubstring(search, startRow, startCol, caseSensitive);
		executeReturn = macro.execute(this, {sel[0],sel[1],sel[2],sel[3]}, js);
		// if( std::get<0>(executeReturn) == -1 ) {
		// 	printf("ERROR: %s\n", std::get<1>(executeReturn).c_str());
		// }
		std::vector<int> result = Macro::getLastResultInts();
		if( result.size() == 2 ) {
			return std::make_tuple(result[0], result[1]);
		}
		return std::make_tuple(-1, -1);
	}

	if( !caseSensitive ) {
		lowerSearch = Utf8CppUtils::utf8::casefold(search);
	}
	
	r = startRow;
	c = startCol;
	// step over to first cell within `searchArea`
	cnt = 0;
	do {
		std::tie(r,c) = nextField(r,c);
		++cnt;
	} while( (r < searchArea[0] || r > searchArea[2] || c < searchArea[1] || c > searchArea[3]) && cnt <= searchAreaSize );
	startRow = r;
	startCol = c;
	// start searching ...
	cnt = 0;
	// TODO Avoid multiple searches in the same row (to improve performance)
	do {
		if( caseSensitive ) {
			if(
				storage.getRow(r).find(search) != std::string::npos &&		// first search in rows – only if matching, search in cell
				getCell(r,c).find(search) != std::string::npos
			) {
				return std::make_tuple(r, c);
			}
		} else {
			if(
				Utf8CppUtils::utf8::casefold(storage.getRow(r)).find(lowerSearch) != std::string::npos &&
				Utf8CppUtils::utf8::casefold(getCell(r,c)).find(lowerSearch) != std::string::npos
			) {
				return std::make_tuple(r, c);
			}
		}
		std::tie(r,c) = nextField(r,c);
		++cnt;
	} while( (r != startRow || c != startCol) && cnt <= searchAreaSize );


	return std::make_tuple(-1, -1);
}

std::string CsvTable::createJsFindSubstring(std::string search, table_index_t startRow, table_index_t startCol, bool caseSensitive) {
	std::string js = "";
	js += R"(
		function get_next_cell(r,c) {
			++c;
			if(c>COLMAX) {
				c=COLMIN; ++r;
			}
			if(r>ROWMAX) {
				r=ROWMIN;
			}
			println("get_next_cell:", r, c);
			return [r,c];
		}
	)";
	js += "var re = /" + search + "/";
	if( !caseSensitive ) {
		js += "i";
	}
	js += ";";
	js += "var start_row = " + std::to_string(startRow) + ";";
	js += "var start_col = " + std::to_string(startCol) + ";";
	js += R"(
		var found_row = -1, found_col = -1;
		pos = [start_row, start_col];
		println("Start:",pos[0],pos[1]);
		outer_loop:
		do {
			pos = get_next_cell(pos[0], pos[1]);
			var cell = getString(pos[0], pos[1]);
			if( m = cell.match(re) ) {
				found_row = pos[0];
				found_col = pos[1];
				break outer_loop;
			}
		} while(pos[0] != start_row || pos[1] != start_col);
		__storeInts(found_row, found_col);
	)";
	return js;
}


/*
 *	Find in cell r,c
 */
bool CsvTable::findInCell(std::string search, table_index_t r, table_index_t c, bool caseSensitive, bool useRegex) {
	std::string lowerSearch;
	std::tuple<int, std::string> executeReturn;				// for JS search
	
	if( useRegex ) {
		std::string js = "";
		js += "var R = " + std::to_string(r) + "; var C = " + std::to_string(c) + ";";
		js += "var pattern = /" + search + "/";
		if( !caseSensitive ) {
			js += "i";
		}
		js += ";";
		js += R"(
			var found = 0;
			var cell = getString(R,C);
			if( m = cell.match(pattern) ) {
				found = 1;
			}
			__storeInts(found);
		)";
		executeReturn = macro.execute(this, {-1,-1,-1,-1}, js);
		if( std::get<0>(executeReturn) == -1 ) {
			printf("ERROR: %s\n", std::get<1>(executeReturn).c_str());
		}
		std::vector<int> found = Macro::getLastResultInts();
		if( found.size() == 1 && found.at(0) == 1 ) {
			return true;
		}
		return false;
	}
	
	if( !caseSensitive ) {
		lowerSearch = Utf8CppUtils::utf8::casefold(search);
	}
	if( caseSensitive ) {
		if(
			storage.getRow(r).find(search) != std::string::npos &&
			getCell(r,c).find(search) != std::string::npos
		) {
			return true;
		}
	} else {
		if(
			Utf8CppUtils::utf8::casefold(storage.getRow(r)).find(lowerSearch) != std::string::npos &&
			Utf8CppUtils::utf8::casefold(getCell(r,c)).find(lowerSearch) != std::string::npos
		) {
			return true;
		}
	}
	return false;
}



/*
 *	Returns cell next to myRow, myCol.
 *	Jumps to next line if last cell in line. Jumps to first line if last cell in last line.
 */
std::tuple<table_index_t, table_index_t> CsvTable::nextField(table_index_t myRow, table_index_t myCol) {
	table_index_t newRow, newCol;
	newRow = myRow;
	newCol = myCol + 1;
	if( newCol > searchArea[3] ) {
		++newRow;
		newCol = searchArea[1];
	}
	if( newRow > searchArea[2] ) {
		newRow = searchArea[0];
	}
	return std::make_tuple(newRow, newCol);
}



int CsvTable::replaceInCurrentCell(table_index_t myRow, table_index_t myCol, std::string pattern, std::string replace, bool caseSensitive, bool useRegex) {
	std::string replaced;
	int count_replacements;
	std::tie(replaced,count_replacements) = replaceUtf8String( pattern, replace, getCell(myRow, myCol), caseSensitive, useRegex );
	setCell( replaced, myRow, myCol );
	return count_replacements;
}


/**
	Function that replaces all occurences of 'pattern' in 'str' by 'replace'.
	@param		pattern			The pattern to replace
	@pattern	replace			The replacement string
	@pattern	str				The string that should be changed
	@pattern	caseSensitive	If true, pattern is compared case sensitive
	@return 					The changed string and the number of replacements or -1 for JS replacements (can not count replacements)
 */
std::tuple<std::string, int> CsvTable::replaceUtf8String(std::string pattern, std::string replace, std::string str, bool caseSensitive, bool useRegex) {
	std::string lowerPattern;
	size_t pos, start_pos = 0;
	std::string returnString = str;
	int count_replacements = 0;
	std::tuple<int, std::string> executeReturn;				// for JS search
	
	if( useRegex ) {
		std::string js = "";
		std::stringstream jss;
		
		jss << R"(var str = )" << std::quoted(str, '\'', '\\') << R"(;)" << std::endl;
		jss << R"(var pattern = )" << std::quoted(pattern, '/', '\\') << R"(g)";
		if( !caseSensitive ) {
			jss << "i";
		}
		jss << R"(;)" << std::endl;
		// js += "var replace = ' " + replace + "';";
		jss << R"(var replace = )" << std::quoted(replace, '\'', '\\') << R"(;)" << std::endl;
		js = jss.str();
		
		js += R"(
			var changes = 0;
			var m;
			if( m = str.match(pattern) ) {
				changes = m.length;
			}
			var new_str = str.replace(pattern, replace);
			__storeStrings(new_str);
			__storeInts(changes);
		)";
		executeReturn = macro.execute(this, {-1,-1,-1,-1}, js);
		if( std::get<0>(executeReturn) == -1 ) {
			printf("ERROR: %s\n", std::get<1>(executeReturn).c_str());
		}
		std::vector<std::string> changedStr = Macro::getLastResultStrings();
		std::vector<int> changes = Macro::getLastResultInts();
		if( changedStr.size() == 1 ) {
			return std::make_tuple(changedStr[0], changes[0]);
		}
		return std::make_tuple(str, 0);
	}

	do {
		// TODO REGEX
		if( caseSensitive ) {
			pos = returnString.find(pattern, start_pos);
		} else {
			lowerPattern = Utf8CppUtils::utf8::casefold(pattern);
			pos = Utf8CppUtils::utf8::casefold(returnString).find(lowerPattern, start_pos);
		}
		if( pos != std::string::npos ) {
			// pattern did match at position 'pos'
			returnString = returnString.substr(0, pos) + replace + returnString.substr(pos + pattern.length(), std::string::npos);
			start_pos = pos + pattern.length() + 1;
			++count_replacements;
		}
	} while( pos != std::string::npos );
	return std::make_tuple(returnString,count_replacements);
}




void CsvTable::dumpStatus(std::string msg) {
	std::cerr << msg << ": table: " << getNumberRows() << "/" << getNumberCols() << " - storage: " << storage.rows() << "/" << storage.columns() << std::endl;
}



void CsvTable::clearTable() {
	storage.clear();
	flags.clear();
	updateInternals();
}



/**
 *	Resizes table so that it matches new_rows, new_cols.
 */
void CsvTable::resizeTable(table_index_t new_rows, table_index_t new_cols) {
	storage.resize(new_rows, new_cols);
	updateInternals();
}


/**
 	Updates internal values so that they are fitting to the data stored in storage
	Should get deprecated! Really?
 */
void CsvTable::updateInternals() {
	#ifdef DEBUG
	dumpStatus("updateInternals");
	#endif
	// Header anpassen
	if( (table_index_t) headerRow->size() != storage.columns() ) {
		#ifdef DEBUG
		std::cerr << "Adapting header row length!" << std::endl;
		#endif
		headerRow->resize(storage.columns());
	}
}



void CsvTable::setDefinition(CsvDefinition definition) {
	this->definition = definition;
}


CsvDefinition CsvTable::getDefinition() {
	return definition;
}


void CsvTable::setFileDefinition(CsvDefinition fileDefinition) {
	this->fileDefinition = fileDefinition;
}


/*
 *	eine Zeile als CSV-String setzen: nötig für Copy
 */
std::string CsvTable::encodeCsvLine(std::vector< std::string > *vec, CsvDefinition definition) {
	std::ostringstream line;

	for( std::vector<std::string>::size_type i = 0; i != vec->size(); ++i ) {
		if( toBeQuoted( vec->at(i), definition ) ) {
			quoteString(vec->at(i), line, definition);
		} else {
			line << vec->at(i);
		}
		if( i < vec->size()-1 ) {
			line << definition.delimiter;
		}
	}
	line << "\r\n";			// According to the CSV RFC 4180

	return line.str();
}



/*
 *	Switch header row: Default Headers (A,B,..) to Custom Headers (First Row in Data)
 *	Returns 1 if header is activated, 0 if not
 */
int CsvTable::switchHeader() {
	int ret;
	
	if( storage.rows() <= 1 && hasCustomHeaderRow == false) {
		return 0;
	}
	
	if( hasCustomHeaderRow ) {
		// Custom Headers => Default Headers
		std::vector< std::string > newRow(storage.columns());
		for( table_index_t c = 0; c < (table_index_t) newRow.size(); ++c ) {
			newRow.at(c) = getHeaderCell(c);
		}
		storage.push_front(newRow);
		for( table_index_t c = 0; c < (table_index_t) headerRow->size(); ++c ) {
			headerRow->at(c) = "";
		}
		hasCustomHeaderRow = false;
		moveFlags(0, 1);
		ret = 0;
	} else {
		// Default Headers => Custom Headers
		for( table_index_t c = 0; c < getNumberCols(); ++c ) {
			headerRow->at(c) = getCell(0,c);
		}
		storage.deleteRows(0,0);
		hasCustomHeaderRow = true;
		moveFlags(0, -1);
		ret = 1;
	}
	updateInternals();
	return ret;
}

bool CsvTable::customHeaderRowShown() {
	return hasCustomHeaderRow;
}


void CsvTable::setCustomHeaderRowShown(bool value) {
	hasCustomHeaderRow = value;
}



table_index_t CsvTable::findHeaderRow(std::string query, table_index_t startCol) {
	table_index_t matchCol = -1;
	std::string lowerQuery = Utf8CppUtils::utf8::casefold(query);
	for(table_index_t c = startCol; c < (table_index_t) headerRow->size(); ++c ) {
		if( Utf8CppUtils::utf8::casefold(headerRow->at(c)).find(lowerQuery) != std::string::npos ) {
			matchCol = c;
			break;
		}
	}
	return matchCol;
}



/**
 *
 *	@param	cb			Callback that updates statusbar
 *	@param	win			pointer to CsvWindow
 *	@param	fromRow		first row to save
 *	@param	toRow		last row to save, if lower than 0: save all the rest (needs fromRow to be set >= 0)
 */
int CsvTable::saveCsv(std::string path, void (*cb)(const char*, void *), void *win, bool flaggedOnly, table_index_t fromRow, table_index_t toRow) {
	std::stringstream sstr;
	table_index_t rowStart, rowEnd;
	const int MAX_MSG_LEN = 500;
	char msg[MAX_MSG_LEN + 1];
	std::string tempStr;
	int retCode = 0;
	std::ofstream output(path, std::ios::binary);
	
	// rather stupid TODO fix when `headerRow` gets fixed
	std::vector<std::string> headerRowCopy;
	headerRowCopy.resize( headerRow->size() );
	for( table_index_t i = 0; i < (table_index_t) headerRow->size(); ++i ) {
		headerRowCopy.at(i) = headerRow->at(i);
	}
	
	if( output ) {
		output.clear();
		if( definition.encoding == CsvDefinition::ENC_UTF16LE ) {
			output.put( static_cast<char>(static_cast<unsigned char>(0xFF)) );
			output.put( static_cast<char>(static_cast<unsigned char>(0xFE)) );
		} else if(definition.encoding == CsvDefinition::ENC_UTF16BE) {
			output.put( static_cast<char>(static_cast<unsigned char>(0xFE)) );
			output.put( static_cast<char>(static_cast<unsigned char>(0xFF)) );
		}
		if( hasCustomHeaderRow ) {
			output << encode( vec2string(headerRowCopy, definition), definition.encoding );
			output << encode( definition.linebreak, definition.encoding );
		}
		if( fromRow < 0 ) {
			rowStart = 0;
			rowEnd = storage.rows();
		} else {
			rowStart = fromRow;
			if( toRow >= 0 ) {
				rowEnd = toRow + 1;		// '+ 1' because of 'r < rowEnd' in for loop
			} else {
				rowEnd = storage.rows();
			}
		}
		for( table_index_t r = rowStart; r < rowEnd; ++r ) {
			if( !flaggedOnly || isFlagged(r) ) {
				output << encode( vec2string(storage.row(r), definition), definition.encoding );
				output << encode( definition.linebreak, definition.encoding );
				if( (r % 20000) == 0 ) {
					snprintf(msg, MAX_MSG_LEN, "Saved %d lines to file.", r);
					cb(msg, win);
				}
			}
		}
		output.close();
		retCode = saveReturnCode::SAVE_OKAY;
	} else {
		retCode = saveReturnCode::SAVE_ERROR;
	}
	
	return retCode;
}



/**
 *	@param	path			Export to path
 *	@param	cb				Callback that updates statusbar
 *	@param	win				pointer to CsvWindow
 *	@param	convertNumbers	Should every number-like value be converted to a number?
 */
int CsvTable::exportJSON(std::string path, void (*cb)(const char*, void *), void *win, bool convertNumbers) {
	table_index_t rowCount, colCount;
	int retCode = saveReturnCode::SAVE_OKAY;
	std::ofstream output(path, std::ios::binary);
	const int MAX_MSG_LEN = 500;
	char msg[MAX_MSG_LEN + 1];
	std::map<std::string, std::string> item;
	Helper::parseNumberStruct parsedNum;
	
	if( output ) {
		rowCount = storage.rows();
		colCount = storage.columns();
		output.clear();
		nlohmann::json json;
		output << "[";
		for( table_index_t i = 0; i < rowCount; i++) {
			json.clear();
			if( hasCustomHeaderRow ) {
				for (table_index_t c = 0; c < colCount; ++c) {
					if( convertNumbers ) {
						parsedNum = Helper::parseNumber( getCell(i,c) );
						if( parsedNum.myType == Helper::parseNumberType::INT ) {
							json[headerRow->at(c)] = parsedNum.myInteger;
						} else if( parsedNum.myType == Helper::parseNumberType::FLOAT ) {
							json[headerRow->at(c)] = parsedNum.myFloat;
						} else {
							json[headerRow->at(c)] = getCell(i,c);
						}
					} else {
						json[headerRow->at(c)] = getCell(i,c);
					}
				}
				output << json.dump();
			} else {
				for (table_index_t c = 0; c < colCount; ++c) {
					if( convertNumbers ) {
						parsedNum = Helper::parseNumber( getCell(i,c) );
						if( parsedNum.myType == Helper::parseNumberType::INT ) {
							json.push_back( parsedNum.myInteger );
						} else if( parsedNum.myType == Helper::parseNumberType::FLOAT ) {
							json.push_back( parsedNum.myFloat );
						} else {
							json.push_back(getCell(i,c));
						}
					} else {
						json.push_back(getCell(i,c));
					}
				}
				output << json.dump();
			}
			if( i == rowCount - 1 ) {			// letzte Zeile
				break;
			}
			output << ",";
			if( (i % 20000) == 0 ) {
				snprintf(msg, MAX_MSG_LEN, "Saved %d lines to file.", i);
				cb(msg, win);
			}
		}	// for
		output << "]" << std::endl;
		output.close();
	} else {
		retCode = saveReturnCode::SAVE_ERROR;
	}
	
	
	return retCode;
}


/*
 *	sortType 0:Numerical, 1:String, 2:String (ignore case) – default: 1
 */
void CsvTable::sortTable(table_index_t column, bool ascending, int sortType) {
	storage.sort(column, ascending, sortType);
}


/*
 *	splitColumn
 */
void CsvTable::splitColumn(table_index_t column, std::string splitStr) {
	if( column < 0 || column >= getNumberCols() ) {
		return;
	}
	addCol(column);		// new column to the right
	updateInternals();
	size_t R = getNumberRows();
	for( size_t row = 0; row < R; ++row ) {
		std::vector<std::string> tokens = Helper::splitString(splitStr, getCell(row,column), 1);
		setCell(tokens[0], row, column);
		if( tokens.size() > 1 ) {
			setCell(tokens[1], row, column+1);
		}
	}
}


/**
 *	mergeColumns
 *	Merges column `column` with `column+1`, using `mergeStr`.
 */
void CsvTable::mergeColumns(const table_index_t column, std::string mergeStr) {
	if( column < 0 || column >= getNumberCols() - 1 ) {
		return;
	}
	size_t R = getNumberRows();
	for( size_t row = 0; row < R; ++row ) {
		std::string newCell = getCell(row,column) + mergeStr + getCell(row,column+1);
		setCell(newCell, row, column);
	}
	delCols(column+1, column+1);		// delete old column to the right
	updateInternals();
}



/**
 *	Returns true, when the given column contains only numeric values.
 *	Tests up to maxNum cells or all, if maxNum == 0
 */
bool CsvTable::isNumericColumn(table_index_t col, table_index_t maxNum) {
	if( col < 0 || col >= getNumberCols() ) {
		return false;
	}
	if( maxNum == 0 || maxNum > getNumberRows() ) {
		maxNum = getNumberRows();
	}
	bool ret = true;
	for( table_index_t r = 0; r < maxNum; ++r ) {
		if( !Helper::isNumber( getCell(r,col) ) ) {
			ret = false;
			break;
		}
	}
	return ret;
}



/*
 *	Flags a row: special selection for further actions
 */
void CsvTable::flagRow(table_index_t rowNr, bool set) {
	if( set ) {
		flags.insert(rowNr);
	} else {
		flags.erase(rowNr);
	}
}

bool CsvTable::isFlagged(table_index_t rowNr) {
	if( flags.find(rowNr) != flags.end() ) {
		return true;
	}
	return false;
}


/*
 *	Returns the index of the next flagged row
 */
table_index_t CsvTable::getNextFlaggedRow(table_index_t rowNr) {
	std::set<table_index_t, std::greater<table_index_t> >::const_iterator iterReturn;
	if( flags.empty() ) {
		return -1;
	}
	auto it = flags.lower_bound(rowNr);
	auto lastIt = flags.end();
	--lastIt;						// lastIt now points to the last elemen (first flagged row)
	if( it == flags.end() ) {
		// 'it' above the first flagged row - starting at the last element (top-most in display)
		it = lastIt;
	} else {
		if( it == flags.begin() ) {
			it = lastIt;
		} else {
			--it;
		}
	}

	return *it;
}

/*
 *	Returns the index of the previous flagged row
 */
table_index_t CsvTable::getPrevFlaggedRow(table_index_t rowNr) {
	std::set<table_index_t, std::greater<table_index_t> >::const_iterator iterReturn;
	if( flags.empty() ) {
		return -1;
	}
	auto it = flags.upper_bound(rowNr);
	if( it == flags.end() ) {
		// it reached end - starting at the beginning
		it = flags.begin();
	}

	return *it;
}

/**
 *	Move all flagged rows
 */
void CsvTable::moveFlags(table_index_t rowNr, table_index_t moveRows) {
	std::set<table_index_t, std::greater<table_index_t> > newFlags;
	for (auto it = flags.begin(); it != flags.end(); ++it) {
		if( *it >= rowNr ) {
			// move flagged rows from rowNr and up (visually down)
			newFlags.insert(*it + moveRows);
		} else {
			// keep flagged rows that are lower than rowNr
			newFlags.insert(*it);
		}
	}
	flags = newFlags;
}


/**
 *	Delete all rows that have been flagged
 */
void CsvTable::deleteFlaggedRows(bool deleteFlagged) {
	std::set<table_index_t, std::greater<table_index_t> > newFlags = flags;
	if( deleteFlagged ) {
		// delete flagged rows
		for (auto it = newFlags.begin(); it != newFlags.end(); ++it) {
			delRows(*it,*it,false,false);		// TODO Call storage.deleteRows() instead?
		}
	// } else {
	// 	// TODO delete non-flagged rows
	// 	1;
	}
	updateInternals();
	flags.clear();
}


/**
 *	Inverse all flagged rows
 */
void CsvTable::invertFlags() {
	std::set<table_index_t, std::greater<table_index_t> > inverseFlags;
	for(table_index_t r = 0; r < getNumberRows(); ++r) {
		if( !isFlagged(r) ) {
			inverseFlags.insert(r);
		}
	}
	flags = inverseFlags;
	updateInternals();
}


/**
 *	Returns the number of flagged rows
 */
table_index_t CsvTable::countFlaggedRows() {
	return (table_index_t) flags.size();
}


/**
 *	Clears all flags
 */
void CsvTable::clearFlags() {
	flags.clear();
}


table_index_t CsvTable::flagInconsistentData(table_index_t column) {
	std::vector<enum CellContentType> types;
	std::map<enum CellContentType, table_index_t> type_distribution;
	table_index_t ROWS = getNumberRows();

	// Guess type of each cell
	for( table_index_t r = 0; r < ROWS; ++r ) {
		enum CellContentType my_type = guessContentType(getCell(r, column));
		types.push_back(my_type);
		auto it = type_distribution.find(my_type);
		if(  it != type_distribution.end() ) {
			++it->second;
		} else {
			type_distribution.emplace(my_type, 1);
		}
	}

	// Find most popular type
	enum CellContentType best_guess = CellContentType::CELL_ANY;
	table_index_t num_cells_with_best_guess = 0;
	for( auto it = type_distribution.begin(); it != type_distribution.end(); ++it ) {
		if( it->second > num_cells_with_best_guess ) {
			best_guess = it->first;
			num_cells_with_best_guess = it->second;
		}
	}

	// Flag rows that are different than the best guess
	table_index_t count_flagged_rows = 0;
	for( table_index_t r = 0; r < ROWS; ++r ) {
		if( types.at(r) != best_guess ) {
			flagRow(r, true);
			++count_flagged_rows;
		}
	}
	return count_flagged_rows;
}


enum CsvTable::CellContentType CsvTable::guessContentType(std::string content) {
	enum CellContentType type = CellContentType::CELL_ANY;
	if( Helper::isInteger(content) ) {
		type = CellContentType::CELL_INTEGER;
	} else if( Helper::isFloat(content, '.') ) {
		type = CellContentType::CELL_FLOAT_POINT;
	} else if( Helper::isFloat(content, ',') ) {
		type = CellContentType::CELL_FLOAT_COMMA;
	} else if( Helper::isSomeDate(content) ) {
		type = CellContentType::CELL_DATE;
	} else if( Helper::isEmailAddress(content) ) {
		type = CellContentType::CELL_EMAIL;
	} else if( Helper::isNumber(content) ) {
		type = CellContentType::CELL_NUMBER;
	}
	return type;
}



/**
	Returns reference to storage, used for Undo
 */
CsvDataStorage &CsvTable::getStorage() {
	return storage;
}


void CsvTable::setStorage(CsvDataStorage &storage) {
	this->storage = storage;
}


bool CsvTable::cellContainsLineBreak(table_index_t R, table_index_t C) {
	return storage.cellContainsLineBreak(R,C);
}


/*
    Returns maximum length and average length of the contents of a given column (in std::string bytes)
 */
std::pair<int, int> CsvTable::maximumContentLength(table_index_t col, table_index_t max_probe_rows) {
    int max_length = 0, average_length = 0;
    uint64_t sum_length = 0;
    table_index_t probe_rows = getNumberRows();
	if( max_probe_rows > 0 )
		probe_rows = std::min(probe_rows, max_probe_rows);
    if( probe_rows > 0 ) {
        for(table_index_t r = 0; r < probe_rows; ++r) {
            int cell_length = getCell(r, col).length();
            if( cell_length > max_length ) {
                max_length = cell_length;
            }
            sum_length += cell_length;
        }
        average_length = (int)(sum_length / probe_rows);
    }
    return std::make_pair(max_length, average_length);
}



/************************************************************************************
 *
 *	Private
 *
 ************************************************************************************/



std::string CsvTable::vec2string(const std::vector<std::string> &line, CsvDefinition definition) {
	std::stringstream ret;
	size_t cellLength;
	for( size_t i = 0; i != line.size(); ++i) {
		if( toBeQuoted(line.at(i), definition ) ) {
			cellLength = line.at(i).length();
			ret << definition.quote;
			for( size_t j = 0; j < cellLength; ++j ) {
				if( line.at(i).at(j) == definition.quote ) {
					ret << definition.quote;
				}
				ret << line.at(i).at(j);
			}
			ret << definition.quote;
		} else {
			ret << line.at(i);
		}
		if( i < line.size()-1 ) {
			ret << definition.delimiter;
		}
	}
	return ret.str();
}


// returns true if field has to be quoted
bool CsvTable::toBeQuoted(std::string field, CsvDefinition definition) {
	if( definition.quoteStyle == CsvDefinition::QUOTE_STYLE_ALL ) {
		return true;
	}
	if( definition.quoteStyle == CsvDefinition::QUOTE_STYLE_STRING && !Helper::isNumber(field) && field != "" ) {
		return true;
	}
	for( char& c : field ) {
		// Line breaks, Field enclosures (double quotes) and Field separators (comma, semicolon, tab, bar) lead to quotation
		if( c == '\n' || c == definition.quote || c == definition.delimiter ) {
			return true;
		}
	}
	return false;
}


//
//	writes string "data" with quotation to ofstream "output" – all in UTF-8
//
void CsvTable::quoteString(std::string &data, std::ostream &output, CsvDefinition definition) {
	int lineLen = data.size();
	output << definition.quote;
	for( int i = 0; i < lineLen; i++ ) {
		if( data[i] == definition.quote ) {
			output << definition.quote;
		}
		output << data[i];
	}
	output << definition.quote;
}


//
//	überprüft, ob ein <std::vector<std::string>> leer ist
//
bool CsvTable::isEmptyLineVector(std::vector<std::string> *line) {
	for( std::vector<std::string>::size_type i = 0; i < line->size(); ++i ) {
		if( line->at(i).size() ) {
			return false;
		}
	}
	return true;
}



//
//	übersetzt text von UTF-8 in das angegebene Encoding 
//
std::string CsvTable::encode(std::string text, CsvDefinition::Encodings encoding) {
	if( encoding == CsvDefinition::ENC_UTF8 ) {
		return text;
	}
	if( encoding == CsvDefinition::ENC_Latin1 ) {
		return Helper::utf8tolatin1(text);
	}
	if( encoding == CsvDefinition::ENC_Win1252 ) {
		return Helper::utf8tolatin1(text, true);
	}
	if( encoding == CsvDefinition::ENC_UTF16LE ) {
		return Helper::utf8toutf16(text, false);
	}
	if( encoding == CsvDefinition::ENC_UTF16BE ) {
		return Helper::utf8toutf16(text, true);
	}
	return text;
}
std::string CsvTable::encode(const char ch, CsvDefinition::Encodings encoding) {
	std::string text(1,ch);
	return encode(text, encoding);
}













