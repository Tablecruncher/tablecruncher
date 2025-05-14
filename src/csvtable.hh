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


#ifndef _CSVTABLE_HH
#define _CSVTABLE_HH


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <set>


// #include <FL/Fl.H>
// #include <FL/fl_ask.H>

#include "json/json.hpp"

#include "helper.hh"
#include "globals.hh"
#include "csvdatastorage.hh"

// Used for stringstream and std::quoted in replaceUtf8String
#include <iomanip>
#include <sstream>



/**
* \brief Manages the data in the `CsvGrid` of a `CsvWindow`.  (The Controller)
* 
* The controller for the CSV data, connecting the `CsvGrid` view with the `CsvDataStorage` (in property `storage`).
*
*/
class CsvTable {

/************************************************************************************
*	Public
************************************************************************************/

public:
	std::vector<std::string> *headerRow;							// 1-dim array containing the row header names when custom header names are switched on / TODO: vec-of-vec for multiple header rows
	std::set<table_index_t, std::greater<table_index_t> > flags;	// stores flagged rows, greater values are accessed first – sorted order is used for more effective deletion
	table_index_t s_left, s_top, s_right, s_bottom;					// current selection – only set by undo handling

	enum saveReturnCode {
		SAVE_OKAY,
		SAVE_ERROR
	};

	enum CellContentType {
		CELL_UNKNOWN,
		CELL_ANY,
		CELL_INTEGER,
		CELL_NUMBER,
		CELL_FLOAT_POINT,
		CELL_FLOAT_COMMA,
		CELL_DATE,
		CELL_EMAIL,
		// CELL_ASCII_STRING,
		// CELL_DIGITS_AND_DASHES,				// 123-32455, 
		// CELL_DATE_YYYYMMDD,					// Date: 19990131
		// CELL_DATE_YYYY_MM_DD_DASH,			// Date: 1999-01-31
		// CELL_DATE_YYYY_MM_DD_SLASH,			// Date: 1999/01/31
		// CELL_DATE_MM_DD_YYYY_SLASH,			// Date: 01/31/1999
		// CELL_DATE_DD_MM_YYYY_DOT,			// Date: 31.01.1999
	};

	CsvTable();
	CsvTable(table_index_t start_rows, table_index_t start_cols);
	CsvTable(const CsvTable &table);
	~CsvTable();
	table_index_t getNumberCols();
	table_index_t getNumberRows();
	std::string getHeaderCell(table_index_t c, bool returnEmpty=true);
	std::string getCell(table_index_t row, table_index_t col);
	void setCell(std::string content, table_index_t row, table_index_t col);
	std::vector< std::vector<std::string> > copyBlock(table_index_t row_top, table_index_t col_top, table_index_t row_bot, table_index_t col_bot);
	std::vector<std::string> row(table_index_t R);
	void addCol(table_index_t colNr, bool before=false);
	void addRow(table_index_t rowNr, bool before=false);
	void delRows(table_index_t rowFrom, table_index_t rowTo, bool clearFlags=true, bool doUpdateInternals=true);
	void delCols(table_index_t colFrom, table_index_t colTo);
	void moveCols(table_index_t colFromStart, table_index_t colFromEnd, bool right);
	std::tuple<table_index_t, table_index_t> findSubstring(std::string search, table_index_t startRow, table_index_t startCol, std::vector<table_index_t> sel, bool caseSensitive = false, bool useRegex = false);
	bool findInCell(std::string search, table_index_t r, table_index_t c, bool caseSensitive = false, bool useRegex = false);
	std::tuple<table_index_t, table_index_t> nextField(table_index_t myRow, table_index_t myCol);
	int replaceInCurrentCell(table_index_t myRow, table_index_t myCol, std::string pattern, std::string replace, bool caseSensitive, bool useRegex=false);
	void appendLine(std::vector<std::string> line);
	void dumpStatus(std::string msg);
	void clearTable();
	void resizeTable(table_index_t rows, table_index_t cols);
	void updateInternals();
	void setDefinition(CsvDefinition definition);
	void setFileDefinition(CsvDefinition fileDefinition);
	CsvDefinition getDefinition();
	std::string encodeCsvLine(std::vector< std::string > *vec, CsvDefinition definition);
	int switchHeader();
	bool customHeaderRowShown();
	void setCustomHeaderRowShown(bool value);
	table_index_t findHeaderRow(std::string query, table_index_t startCol = 0);
	int saveCsv(std::string path, void (*cb)(const char*, void *), void *win, bool flaggedOnly = false, table_index_t fromRow=-1, table_index_t toRow=-1);
	int exportJSON(std::string path, void (*cb)(const char*, void *), void *win, bool convertNumbers = true);
	void sortTable(table_index_t column, bool ascending, int sortType=1);
	void splitColumn(table_index_t column, std::string splitStr);
	void mergeColumns(const table_index_t column, std::string mergeStr);
	bool isNumericColumn(table_index_t col, table_index_t maxNum=0);
	void flagRow(table_index_t rowNr, bool set);
	bool isFlagged(table_index_t rowNr);
	table_index_t getNextFlaggedRow(table_index_t rowNr);
	table_index_t getPrevFlaggedRow(table_index_t rowNr);
	void moveFlags(table_index_t rowNr, table_index_t moveRows);
	void deleteFlaggedRows(bool deleteFlagged = true);
	void invertFlags();
	table_index_t countFlaggedRows();
	void clearFlags();
	table_index_t flagInconsistentData(table_index_t column);
	CsvDataStorage &getStorage();
	void setStorage(CsvDataStorage &storage);
	bool cellContainsLineBreak(table_index_t R, table_index_t C);										// returns true if content of that cell contains line breaks (\n)
    std::pair<int, int> maximumContentLength(table_index_t col, table_index_t max_probe_rows = 0 );     // returns the maximum and average length of the content of the given column (in characters)
	

/************************************************************************************
*	Private
************************************************************************************/

private:
	CsvDataStorage storage;							// holds all the table data, but not the header row
	CsvDefinition definition;						// the definition of the data in this table – may be change by users
	CsvDefinition fileDefinition;					// the definition of the file as it has been opened
	bool hasCustomHeaderRow = false;				// first row of CSV is considered a header row?
	std::vector<table_index_t> searchArea;			// where findSubstring should search resp. where nextFields() iterates

	std::string vec2string(const std::vector<std::string> &line, CsvDefinition definition);
	bool toBeQuoted(std::string field, CsvDefinition definition);
	void quoteString(std::string &data, std::ostream &output, CsvDefinition definition);
	bool isEmptyLineVector(std::vector<std::string> *line);
	std::string encode(std::string text, CsvDefinition::Encodings encoding);
	std::string encode(const char ch, CsvDefinition::Encodings encoding);
	std::string createJsFindSubstring(std::string search, table_index_t startRow, table_index_t startCol, bool caseSensitive);
	std::tuple<std::string, int> replaceUtf8String(std::string pattern, std::string replace, std::string str, bool caseSensitive, bool useRegex=false);
	enum CellContentType guessContentType(std::string content);

};






#endif




