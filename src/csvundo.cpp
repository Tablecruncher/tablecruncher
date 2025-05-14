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


#include "csvundo.hh"
#include "globals.hh"

/*
	TODO
	* check memory handling

 */


int CsvUndo::uniqNumber = 1;

CsvUndo::CsvUndo() {
	uniqNumber++;
}
CsvUndo::~CsvUndo() {
	// CsvWindow::undo() copies CsvUndo::table to windows[]->table, so no delete(table) is needed here
	// 
}

void CsvUndo::createUndoStateTable(CsvTable &table, std::string descr) {
	size_t size;
	this->id = uniqNumber;
	this->type = TCRUNCHER_UNDO_TYPE_TABLE;
	this->undoStorage = table.getStorage();
	size = table.headerRow->size();
	this->headerRow.resize(size);
	for( size_t r = 0; r < size; ++r) {
		this->headerRow.at(r) = table.headerRow->at(r);
	}
	this->descr = descr;
	this->hasCustomHeaderRow = table.customHeaderRowShown();
	this->flags = table.flags;
	this->selection = { table.s_top, table.s_left, table.s_bottom, table.s_right };
	// printf("createUndoStateTable: %s (%p) ID:%d\n", descr.c_str(), (void *)this->table, uniqNumber);
}

void CsvUndo::createUndoStateCell(std::string cellContent, table_index_t R, table_index_t C, bool hasCustomHeaderRow, std::string descr) {
	this->id = uniqNumber;
	this->type = TCRUNCHER_UNDO_TYPE_CELL;
	this->prevCellContent = cellContent;
	this->R = R;
	this->C = C;
	this->descr = descr;
	this->hasCustomHeaderRow = hasCustomHeaderRow;
}


CsvDataStorage &CsvUndo::getUndoStorage() {
	return undoStorage;
}

std::set<table_index_t, std::greater<table_index_t> > &CsvUndo::getFlags() {
	return flags;
}


std::vector<std::string> &CsvUndo::getHeaderRow() {
	return headerRow;
}



bool CsvUndo::getSwitchHeaderRow() {
	return hasCustomHeaderRow;
}


std::string CsvUndo::getDescr() {
	return descr;
}

int CsvUndo::getType() {
	return type;
}


std::tuple<table_index_t,table_index_t> CsvUndo::getCellPosition() {
	return std::make_tuple(R,C);
}

std::string CsvUndo::getCellContent() {
	return prevCellContent;
}

int CsvUndo::getId() {
	return id;
}


void CsvUndo::deleteTable() {
	// delete(table);
}

std::vector<table_index_t> CsvUndo::getSelection() {
	return selection;
}









