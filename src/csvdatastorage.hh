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

#ifndef _CSVDATASTORAGE_HH
#define _CSVDATASTORAGE_HH

#include <FL/Fl.H> // called by sort() to update UI

#include <algorithm>
#include <vector>
#include <string>
#include <tuple>
#include <iostream> // needed for dump()
#include <iomanip>  // needed for dump()
#include <chrono>

#include "globals.hh"
#include "utf8-cpp-utils/utf8_cpp_utils.hh"


/**
	\brief Storage for the CSV data table, not including the header row.  (The Model)

	The data is stored within `tableData`, a vector of strings, each string representing a single CSV row. The order of this
	vector defines the order of the CSV rows. Each row is a `std::string`, the fields are separated by `TCRUNCHER_UTF_8_DELIMITER`.
	This delimiter is a invalid UTF-8 byte, so it will never occur within a UTF-8 encoded cell data.

	The header row is stored in `CsvTable`. That's not a great design decision and should be healed someday.

	(Initially the application just used std::vector< std::vector<std::string> >, but that is really expensive in terms of memory usage.
	This class should help in decoupling the underlying data storage from the operations of CsvTable.cpp.)

 */
class CsvDataStorage
{

public:
	CsvDataStorage();							   						// Standard constructor
	CsvDataStorage(table_index_t R, table_index_t C);					// Defined size constructor

	void resize(table_index_t R, table_index_t C = 0);				  	// resizes the storage to dimensions R,C
	void clear();													  	// clears the storage
	table_index_t rows();											  	// returns number of rows
	table_index_t columns();										  	// returns number of columns
	void sort(table_index_t column, bool ascending, int sortType);	  	// sorts the table according to the given options
	std::string get(table_index_t R, table_index_t C);				  	// gets the cell content at R,C
	std::string getRow(table_index_t R);							  	// returns the row string (including illegal UTF-8 glue character)
	bool set(std::string content, table_index_t R, table_index_t C);  	// sets the content of cell at R,C – true if succeeded
	std::vector<std::string> row(table_index_t R);					  	// returns a single row as a vector of strings with length `numColumns`
	std::vector<std::string> rawRow(table_index_t R);				  	// returns a single row as a vector of strings, length depends on content
	void push_back(std::string rowString);							  	// adds a row at end of the table
	void push_back(std::vector<std::string> row);					  	// adds a row at end of the table
	void push_front(std::string rowString);							  	// adds a row at the beginning of the table
	void push_front(std::vector<std::string> row);					  	// adds a row at beginning of the table
	void deleteRows(table_index_t rowFrom, table_index_t rowTo);		// delete rows
	void deleteColumns(table_index_t colFrom, table_index_t colTo);	  	// delete columns
	void insertRow(table_index_t R, table_index_t before = false);	  	// inserts a row after (or before) row R
	void insertColumn(table_index_t C, bool before = false);			// inserts a column after (or before) column C
	void moveColumns(table_index_t colFromStart, table_index_t colFromEnd, bool right); 		// move multiple columns to the right or left
	bool cellContainsLineBreak(table_index_t R, table_index_t C);	  	// returns true if content of that cell contains line breaks (\n)
	void dump(table_index_t numRows = 10, bool raw = false);		  	// DEBUG: dumps content of tableData; if `raw`: strings are displayed

private:
	std::vector<std::string> tableData; 								// holds the data
	table_index_t numColumns = 0;										// number of columns
	static const unsigned char TCRUNCHER_UTF_8_DELIMITER = 0xFA;		// this byte is used as a separator for fields within std::string (it's an invalid UTF-8 character)

	static std::vector<std::string> splitString(std::string str);								 // splits a string at the internal CSV delimiter
	static std::string mergeString(std::vector<std::string> row);								 // merges the vector to a string
	static std::pair<table_index_t, table_index_t> getColumnIndizes(std::string rowString, table_index_t column); // returns the positions of the surrounding bytes
	static std::string getColumn(std::string row, table_index_t column);						 // gets the content of `column` in string row
	std::string setColumn(std::string row, table_index_t column, std::string content);			 // sets the content of `column` in row and returns the new string
	static std::string emptyCellsString(table_index_t num);										 // returns strings of delimiters
};

#endif
