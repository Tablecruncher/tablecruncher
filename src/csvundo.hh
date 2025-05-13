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


