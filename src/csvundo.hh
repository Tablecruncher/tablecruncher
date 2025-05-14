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


#ifndef _CSVUNDO_HH
#define _CSVUNDO_HH

#include "globals.hh"
#include "csvtable.hh"
#include "csvdatastorage.hh"

#include <tuple>
#include <vector>
#include <set>

/*
 *	A single Undo state.
 */
class CsvUndo {

public:
	/******************************************************
	 *	Methods
	 ******************************************************/
	CsvUndo();
	~CsvUndo();
	void createUndoStateTable(CsvTable &table, std::string descr);
	void createUndoStateCell(std::string cellContent, table_index_t R, table_index_t C, bool hasCustomHeaderRow, std::string descr);
	CsvDataStorage &getUndoStorage();
	std::set<table_index_t, std::greater<table_index_t> > &getFlags();
	std::vector<std::string> &getHeaderRow();
	bool getSwitchHeaderRow();
	std::string getDescr();
	int getType();
	std::tuple<table_index_t,table_index_t> getCellPosition();
	std::string getCellContent();
	int getId();
	void deleteTable();
	std::vector<table_index_t> getSelection();

private:
	/******************************************************
	 *	Properties
	 ******************************************************/
	static int uniqNumber;
	int id;
	int type = TCRUNCHER_UNDO_TYPE_TABLE;
	std::string descr;
	CsvDataStorage undoStorage;
	std::vector<std::string> headerRow;
	std::set<table_index_t, std::greater<table_index_t> > flags;
	bool hasCustomHeaderRow;
	std::vector<table_index_t> selection;
	// stores a single cell: when just a cell has been affected
	std::string prevCellContent;
	table_index_t R;
	table_index_t C;
};


#endif


