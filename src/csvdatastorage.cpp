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


#include "csvdatastorage.hh"


/*

	Change tableData from vec-vec-string to vec-string
	* All lines marked with CHANGE have to be changed when switching to vec-string
	* Row strings don't need to be of full length. Size is determined by `numColumns`.
	* csvparser.cpp: Look for occurrences of my delimiter octet in input strings.


*/


/**
	CsvDataStorage()
 */
CsvDataStorage::CsvDataStorage() {
	resize(0,0);
}


/**
	CsvDataStorage(table_index_t R, table_index_t C)
 */
CsvDataStorage::CsvDataStorage(table_index_t R, table_index_t C) {
	resize(R,C);
}


/**
	resize(long R, long C = 0)

	Resize to R,C dimensions. Table can grow only, shrinking has to be done by
	deleteColumns() or deleteRows().
	If R == 0 or C == 0: this dimension isn't changed
 */
void CsvDataStorage::resize(table_index_t R, table_index_t C) {
	table_index_t old_rows = rows();
	table_index_t old_columns = columns();

	if( C > old_columns ) {
		// build the padding once – this used to allocate a fresh string for every row
		const std::string pad = emptyCellsString(C - old_columns);
		for( table_index_t i = 0; i < old_rows; ++i) {
			tableData.at(i) += pad;
		}
	}
	if( R > 0 && R > rows() ) {
		tableData.resize(R);
		for( table_index_t i = old_rows; i < R; ++i) {	// add empty vectors into the newly created rows
			tableData.at(i) = emptyCellsString(C - 1);
		}
	}
	if( C > old_columns ) {
		numColumns = C;
	}
}


/**
	reserveRows(size_t n)

	Reserve capacity for `n` rows. Purely an optimisation: without it a bulk load
	reallocates and moves millions of std::strings, which both costs time and spikes
	peak RSS by ~1.5x. Never shrinks and never changes rows() or columns().
 */
void CsvDataStorage::reserveRows(size_t n) {
	if( n > tableData.capacity() ) {
		tableData.reserve(n);
	}
}


/**
	clear()

	Clear the table
 */
void CsvDataStorage::clear() {
	tableData.clear();
	tableData.shrink_to_fit();
	numColumns = 0;
}


/**
	rows()

	Returns number of rows
 */
table_index_t CsvDataStorage::rows() const {
	return (table_index_t) tableData.size();
}


/**
	columns()

	Returns number of columns
 */
table_index_t CsvDataStorage::columns() const {
	return numColumns;
}



/**
	sort(int column, bool ascending, int sortType)
	sortType 0:Numerical, 1:String, 2:String (ignore case) – default: 1
 */
void CsvDataStorage::sort(table_index_t column, bool ascending, int sortType) {

	if( rows() <= 1 || column < 0 || column >= columns() ) {
		return;
	}

	long counter = 0;
	std::sort( tableData.begin(), tableData.end(), [this, &column, &ascending, &sortType, &counter](const auto& lhs, const auto& rhs) {
		std::string s1 = getColumn(lhs, column);
		std::string s2 = getColumn(rhs, column);
		double d1, d2;
		std::string lowerS1;
		std::string lowerS2;
		if( counter % 50000 == 0 && progressCallback ) {
			progressCallback((size_t) counter);
		}
		++counter;
		switch( sortType ) {
			case 0:		// NUMERIC
				// TODO make this optional
				// std::replace( s1.begin(), s1.end(), ',', '.');
				// std::replace( s2.begin(), s2.end(), ',', '.');
				try {
					d1 = std::stod(s1);
				} catch(const std::exception&) {
					d1 = 0;
				}
				try {
					d2 = std::stod(s2);
				} catch(const std::exception&) {
					d2 = 0;
				}
				if( ascending ) {
					return d1 < d2;
				} else {
					return d1 > d2;
				}
			break;
			case 1:		// STRING with case
				if( ascending )
					return s1 < s2;
				else
					return s1 > s2;
			break;
			case 2:		// STRING ignore case
				lowerS1 = Utf8CppUtils::utf8::casefold(s1);
				lowerS2 = Utf8CppUtils::utf8::casefold(s2);
				if( ascending )
					return lowerS1 < lowerS2;
				else
					return lowerS1 > lowerS2;
			break;
			default:
				return true;
		}
	});


}



/**
	setProgressCallback(std::function<void(size_t)> cb)

	Injected by the UI layer so that sort() can keep the event loop alive without
	CsvDataStorage having to know about FLTK. Pass an empty std::function to detach.
 */
void CsvDataStorage::setProgressCallback(std::function<void(size_t)> cb) {
	progressCallback = std::move(cb);
}



/**
	get(long R, long C)

	Get content of a single cell.
	You can't be sure that C is lower than columns() as CsvParser::parseCsvStream() may be
	called with resizeRows == false for probing
 */
std::string CsvDataStorage::get(table_index_t R, table_index_t C) {
	if( C < 0 || R < 0 || R >= rows() || C >= columns() )
		return "";
	return getColumn(tableData.at(R), C);
}



/**
	getRow(long R)

	Returns the row string (including illegal UTF-8 glue character)
 */
std::string CsvDataStorage::getRow(table_index_t R) {
	if( R < 0 || R >= rows() )
		return "";
	return tableData.at(R);
}



/**
	assembleFrom(std::vector<CsvDataStorage>& pieces, table_index_t columns)

	Concatenates the pieces of a parallel load into one table.

	Kept as a single operation because the intermediate states are not valid tables: appending
	rows does not update numColumns, and setting numColumns does not pad rows. Exposing those
	two halves separately would put a way to build an inconsistent CsvDataStorage — one whose
	numColumns disagrees with its row contents — on the public API.

	Every row in every piece must already hold exactly `columns` glue-separated fields.
 */
void CsvDataStorage::assembleFrom(std::vector<CsvDataStorage>& pieces, table_index_t columns) {
	size_t total = 0;
	for( const CsvDataStorage& piece : pieces )
		total += piece.tableData.size();

	tableData.clear();
	tableData.shrink_to_fit();
	tableData.reserve(total);

	for( CsvDataStorage& piece : pieces ) {
		tableData.insert( tableData.end(),
		                  std::make_move_iterator(piece.tableData.begin()),
		                  std::make_move_iterator(piece.tableData.end()) );
		// release each piece's header array as we go, so peak overhead is one piece
		piece.tableData.clear();
		piece.tableData.shrink_to_fit();
		piece.numColumns = 0;
	}

	numColumns = columns;
}


/**
	set(std::string content, long R, long C)

	Set content of a single cell
 */
bool CsvDataStorage::set(std::string content, table_index_t R, table_index_t C) {
	if( C < 0 || R < 0 || R >= rows() || C >= columns() )
		return false;
	tableData.at(R) = setColumn(tableData.at(R), C, content);
	return true;
}



/**
	row(long R)

	Return a single row as a vector-of-string, no matter how tableData is implemented.
	The returned vector always contains columns() items
	@return		vector
 */
std::vector<std::string> CsvDataStorage::row(table_index_t R) {
	std::vector<std::string> row;
	table_index_t C = columns();
	if( R < rows() ) {
		for( table_index_t c = 0; c < C; ++c) {
			row.push_back( get(R,c) );
		}
	} else {
		row.resize(C);
	}
	return row;
}




/**
	rawRow(long R)

	Return a single row as a vector-of-string, no matter how tableData is implemented.
	The size of the returned vector is determined by the number of actual columns.

	@return		vector
 */
std::vector<std::string> CsvDataStorage::rawRow(table_index_t R) {
	std::vector<std::string> row;
	if( R >= 0 && R < rows() ) {
		return splitString(tableData.at(R));
	} else {
		return row;
	}
}


/**
	push_back(std::vector<std::string> > row)

	Adds row to the end of the table data
 */
void CsvDataStorage::push_back(const std::string& rowString) {
	tableData.push_back(rowString);
}

void CsvDataStorage::push_back(std::string&& rowString) {
	tableData.push_back(std::move(rowString));
}

void CsvDataStorage::push_back(const std::vector<std::string>& row) {
	tableData.push_back(mergeString(row));
}



/**
	push_front(std::vector<std::string> > row)
	Adds row to the beginning of the table data
 */
void CsvDataStorage::push_front(const std::string& row) {
		// TODO edit length histogram!?
	tableData.insert(tableData.begin(), row);
}
void CsvDataStorage::push_front(const std::vector<std::string>& row) {
		// TODO edit length histogram!?
	tableData.insert(tableData.begin(), mergeString(row));
}

/**
	deleteRows(long rowFrom, long rowTo)

	Delete rows from rowFrom to rowTo inclusively.
 */
void CsvDataStorage::deleteRows(table_index_t rowFrom, table_index_t rowTo) {
	if( rowFrom >= 0 && rowFrom < rows() && rowTo >= rowFrom && rowTo < rows() ) {
		tableData.erase( tableData.begin() + rowFrom, tableData.begin() + rowTo + 1 );
	}
}



/**
	deleteColumns(long colFrom, long colTo)

	Deletes multiple columns.
 */
void CsvDataStorage::deleteColumns(table_index_t colFrom, table_index_t colTo) {
	table_index_t R = rows();
	if( colFrom >= 0 && colFrom < columns() && colTo >= colFrom && colTo < columns() ) {
		for( table_index_t r = 0; r < R; ++r ) {
			std::vector<std::string> row = splitString(tableData.at(r));
			row.erase( row.begin() + colFrom, row.begin() + colTo + 1);
			tableData.at(r) = mergeString(row);
		}
		numColumns -= colTo - colFrom + 1;
	}
}


/**
	insertRow(long R, bool before=false)

	Insert a row after row `R`, or before if `before` is true.
 */

void CsvDataStorage::insertRow(table_index_t R, table_index_t before) {
	std::vector<std::string>::iterator it;
	std::string newRow = emptyCellsString(columns() - 1);
	if( R >= 0 && R < rows() ) {
		// insert new row
		it = tableData.begin();
		if( before ) {
			tableData.insert(it + R, newRow);
		} else {
			tableData.insert(it + R + 1, newRow);
		}
	}
}



/**
	insertColumn(long C, bool before=false)

	Insert a column after column `C`, or before if `before` is true.
	// a simple solution based on tableData.at(i).insert() throws std::bad_alloc execptions,
	// if the container is big (e.g. 500000 lines) – not the case anymore??
 */
void CsvDataStorage::insertColumn(table_index_t C, bool before) {
	table_index_t R = rows();
	if( C >= 0 && C < columns() ) {
		for( table_index_t r = 0; r < R; ++r ) {
			std::vector<std::string> row = splitString(tableData.at(r));
			if( before )
				row.insert(row.begin() + C, "");
			else
				row.insert(row.begin() + C + 1, "");
			tableData.at(r) = mergeString(row);
		}
		++numColumns;
	}
}



/**
	moveColumns(long colFrom, long colTo, bool right)

	Move columns.

	  0   1   2   3   4
	| A | B | C | D | E |
 */
void CsvDataStorage::moveColumns(table_index_t colFrom, table_index_t colTo, bool right) {
	table_index_t R = rows();
	table_index_t C = columns();
	std::string buffer = "";
	
	if( right ) {
		if( colTo < C - 1 && colFrom >= 0 ) {
			for( table_index_t r = 0; r < R; ++r ) {
				std::vector<std::string> row = splitString(tableData.at(r));
				buffer = row.at(colTo + 1);
				for( int c = colTo; c >= colFrom; --c ) {
					row.at(c + 1) = row.at(c);
				}
				row.at(colFrom) = buffer;
				tableData.at(r) = mergeString(row);
			}
		}
	} else {
		if( colFrom > 0 && colTo < C ) {
			for( table_index_t r = 0; r < R; ++r ) {
				std::vector<std::string> row = splitString(tableData.at(r));
				buffer = row.at(colFrom - 1);
				for( int c = colFrom; c <= colTo; ++c ) {
					row.at(c - 1) = row.at(c);
				}
				row.at(colTo) = buffer;
				tableData.at(r) = mergeString(row);
			}
		}
	}
}


bool CsvDataStorage::cellContainsLineBreak(table_index_t R, table_index_t C) {
	return get(R,C).find("\n") != std::string::npos;
}


/**
	columnContentLengths(table_index_t maxProbeRows)

	Maximum and average content length, in bytes, for every column at once.

	The obvious formulation – ask get(R,C) for each cell – rescans each row string from byte
	zero for every column, so probing C columns of a row costs O(L*C/2). On a 1000-column,
	10 KB-per-row table that is tens of billions of byte operations. This walks each row
	exactly once instead.

	Parity with the per-cell version is deliberate: the average uses integer division by the
	number of probed rows, and a row holding fewer than columns() fields contributes length 0
	for the missing tail, matching get()'s out-of-range "".
 */
std::vector<std::pair<int,int>> CsvDataStorage::columnContentLengths(table_index_t maxProbeRows) {
	const table_index_t C = columns();
	std::vector<std::pair<int,int>> out;
	if( C <= 0 )
		return out;
	out.assign((size_t) C, {0,0});

	table_index_t probeRows = rows();
	if( maxProbeRows > 0 )
		probeRows = std::min(probeRows, maxProbeRows);
	if( probeRows <= 0 )
		return out;

	std::vector<int>      maxLen((size_t) C, 0);
	std::vector<uint64_t> sumLen((size_t) C, 0);

	for( table_index_t r = 0; r < probeRows; ++r ) {
		const std::string& row = tableData[(size_t) r];
		const size_t n = row.size();
		table_index_t c = 0;
		size_t fieldStart = 0;
		for( size_t i = 0; i <= n; ++i ) {
			if( i == n || static_cast<unsigned char>(row[i]) == TCRUNCHER_UTF_8_DELIMITER ) {
				if( c < C ) {
					const int len = (int)(i - fieldStart);
					if( len > maxLen[(size_t) c] ) maxLen[(size_t) c] = len;
					sumLen[(size_t) c] += (uint64_t) len;
				}
				++c;
				fieldStart = i + 1;
			}
		}
	}

	for( table_index_t c = 0; c < C; ++c )
		out[(size_t) c] = { maxLen[(size_t) c], (int)(sumLen[(size_t) c] / (uint64_t) probeRows) };
	return out;
}

/**
	splitString()

	Splits the given string at the defined internal CSV delimiter
 */
std::vector<std::string> CsvDataStorage::splitString(const std::string& str) {
	std::vector<std::string> splitted;
	std::string tempStr = "";
	size_t str_size = str.size();
	for( size_t i = 0; i < str_size; ++i ) {
		if( static_cast<unsigned char>(str[i]) != CsvDataStorage::TCRUNCHER_UTF_8_DELIMITER ) {
			tempStr.push_back(str[i]);
		} else {
			splitted.push_back(tempStr);
			tempStr = "";
		}
	}
	splitted.push_back(tempStr);
	return splitted;
}


/**
	mergeVector()

	Merges the given vector-of-strings with the defined internal CSV delimiter
*/
std::string CsvDataStorage::mergeString(const std::vector<std::string>& row) {
	std::string merged;
	size_t total = row.empty() ? 0 : row.size() - 1;			// one glue byte between every pair of fields
	for( auto const& field : row )
		total += field.size();
	merged.reserve(total);
	for(size_t i = 0; i < row.size(); ++i ) {
		if( i > 0 )
			merged.push_back( static_cast<char>(CsvDataStorage::TCRUNCHER_UTF_8_DELIMITER) );
		merged.append(row[i]);
	}
	return merged;
}



/**
	getColumn(std::string row, long column)

	a#bcd#ef#hij
    0123456789ab
 */
std::string CsvDataStorage::getColumn(const std::string& rowString, table_index_t column) {
	std::pair<table_index_t,table_index_t> fromTo = getColumnIndizes(rowString, column);
	if( fromTo.second - fromTo.first - 1 <= (table_index_t) rowString.size() && fromTo.second > fromTo.first ) {
		return rowString.substr(fromTo.first + 1, (fromTo.second - fromTo.first - 1));
	} else {
		return "";
	}
}

std::pair<table_index_t,table_index_t> CsvDataStorage::getColumnIndizes(const std::string& rowString, table_index_t column) {
	bool colFound = false;
	size_t str_size = rowString.size();
	table_index_t fromIdx = -1;
	table_index_t toIdx = -1;
	table_index_t colCounter = 0;
	for( size_t i = 0; i < str_size; ++i ) {
		if( static_cast<unsigned char>(rowString[i]) == CsvDataStorage::TCRUNCHER_UTF_8_DELIMITER ) {
			if( colCounter == column ) {
				toIdx = i;
				colFound = true;
				break;
			}
			++colCounter;
			fromIdx = i;
		}
	}
	if( !colFound ) {
		if( column == colCounter ) {
			toIdx = rowString.size();
		} else {
			toIdx = -1;
		}
	}
	return std::make_pair(fromIdx, toIdx);
}


std::string CsvDataStorage::setColumn(const std::string& rowString, table_index_t column, const std::string& content) {
	std::vector<std::string> row = splitString(rowString);
	if( (table_index_t) row.size() < numColumns ) {
		row.resize(numColumns, "");
	}
	if( (table_index_t) row.size() > column && column >= 0 ) {
		row[column] = content;
		return mergeString(row);
	} else {
		return rowString;
	}	
}


/**
	Returns a string that consists of `num` internal delimiter characters
 */
std::string CsvDataStorage::emptyCellsString(table_index_t num) {
	std::string ret = "";
	for( table_index_t c = 0; c < num; ++c) {
		ret.push_back( static_cast<char>(TCRUNCHER_UTF_8_DELIMITER));
	}
	return ret;
}



/**
	dump(long numRows = 10, bool raw)

	Dump content of tableData, showing at most `numRows`.
 */
void CsvDataStorage::dump(table_index_t numRows, bool raw) {
	table_index_t R = rows();
	table_index_t C = columns();
	std::cout << "================================================================================================" << std::endl;
	std::cout << "ROWS: " << R << std::endl;
	std::cout << "COLS: " << C << std::endl;
	std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
	if( R > 0 && (C > 0 || raw) ) {
		for( table_index_t r = 0; r < std::min(numRows, R); ++r ) {
			std::cout << std::setw(3) << r << ": ";
			if( raw ) {
				std::cout << "|" << tableData.at(r) << "|" << std::endl;
			} else {
				for( table_index_t c = 0; c < C; ++c ) {
					std::cout << "|" << std::setw(12) << get(r,c);
				}
				std::cout << "|" << std::endl;
			}
		}
		if( R > numRows ) {
			std::cout << "... " << (R - numRows) << " rows omitted ..." << std::endl;
		}
	}
	std::cout << "================================================================================================" << std::endl;
	std::cout << std::setw(0);
}











