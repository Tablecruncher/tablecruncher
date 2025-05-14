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


#ifndef _CSVWINDOW_HH
#define _CSVWINDOW_HH


#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <deque>
#include <tuple>
#include <algorithm>
#include <unordered_map>		// for hash


#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_PNG_Image.H>

#include "json/json.hpp"

#include "helper.hh"
#include "csvwidgets.hh"
#include "globals.hh"
#include "csvundo.hh"
#include "csvgrid.hh"
#include "csvmenu.hh"
#include "csvtable.hh"
#include "csvapplication.hh"
#include "csvparser.hh"


namespace ui_icons {
	#ifndef _WIN64
	#pragma GCC diagnostic push
  	#pragma GCC diagnostic ignored "-Wunused-variable"
	#endif
	#include "icons/icon-add-col-left.xpm"
	#include "icons/icon-add-col-right.xpm"
	#include "icons/icon-add-row-above.xpm"
	#include "icons/icon-add-row-below.xpm"
	#include "icons/icon-del-col.xpm"
	#include "icons/icon-del-row.xpm"
	#include "icons/icon-info.xpm"
	#include "icons/icon-open.xpm"
	#include "icons/icon-save-as.xpm"
	#include "icons/icon-save.xpm"
	#include "icons/icon-separator.xpm"
	#ifndef _WIN64
	#pragma GCC diagnostic pop
	#endif
}


/**
 * \brief Represents a single CSV document window.
 * 
 * Contains pointers to the `grid` (class `CsvGrid` that handles the drawing of the table), the `table` (class `CsvTable`
 * that manages all the data) and to the `win` object (class `My_Fl_Double_Window` defined in `csvwidgets.hh` that shows
 * this window).
 * 
 * Further important objects referenced here are the `toolbar`, the `statusbar` and the `menu`.
 * 
 */
class CsvWindow {
public:
	My_Fl_Double_Window *win;										// FLTK window to show CSV table data
	CsvGrid *grid;													// The Fl_Table element
	CsvTable *table;												// CsvTable containing all the data
	Fl_Light_Button *showHeaderCheckbox;

	int windowMenuIndex = 0;										// Used to pass the selected Window menu item
	int createdWindowCount = 0;										// this window has been created as the createdWindowCount-th window in this session – used for sorting the Window menu

	CsvWindow();
	~CsvWindow();
	void create(int createdWindowCount = 0);
	void destroy();
	void setName(std::string name);
	std::string getName();
	bool getWindowSlotUsed();										// Is this window active (visible)?
	void setWindowSlotUsed(bool state);
	bool loadFile(std::string filename, bool askUser=false, bool reopen=false);
	void setUsed(bool used);										// has this been used since creation? Not to be confused with getWindowSlotUsed()
	bool isUsed();
	void setChanged(bool changed);
	bool isChanged();
	void setPath(std::string path);
	std::string getPath();
	void updateStatusbar(const std::string& msg);
	void showDefaultStatus();
	static void updateStatusbarCB(const char *msg, void *win);
	static void windowGetsClosedCB(Fl_Widget *widget, void *);
	void updateTable();												// updates the internals of CsvTable and the displayed grid
	void addUndoStateTable(std::string descr);
	void addUndoStateCell(std::string cellContent, int R, int C, std::string descr);
	void undo();
	bool hasUndoStates();
	void removeLastUndoState();
	void clearUndoList();
	void setUndoSaveState();
	void disableUndo();
	void enableUndo();
	bool isUndoDisabled();
	void showInfoWindow();
	void applyTheme();
	void storeWindowPreferences();
	void readWindowPreferences();
	void setTypeButton(CsvDefinition definition);
	std::string humanReadableSelection();
	void dumpSelection();
	CsvMenu *getWinMenuBar();
	

private:
	CsvMenu *winMenuBar;									// Holds the pointer to the menu bar
	My_Toolbar *toolbar;
	Fl_Box *statusbar;
	Fl_Button *typeButton;
	std::string name = "";									// name of this window: "Empty N" or filename of opened file
	std::string displayName = "";							// displayed name of this window: like `name`
	std::string path = "";									// Full path of the opened file
	int width;
	int height;
	bool used = false;										// Has this window been used (edited) since creation? Used to decide whether to open a new window
	bool changed = false;									// Has the content been changed – is a save necessary before close?
	bool changedWhileUndoDisabled = false;					// Has the content ever been changed? Used when Undo was disabled and has been enabled again
	std::string statusbarText;
	std::string typeButtonString;
	bool windowSlotUsed = false;							// Is this object used as a window shown on the screen? Has nothing to do with 'Has there been data entered?'
	std::deque< CsvUndo > undoList;
	int undoSaveState = -1;									// undoList.uniqNumber for which the last save command was issued
	bool undoDisabled = false;								// set true to disable undo (necessary for large files)

};




#endif




