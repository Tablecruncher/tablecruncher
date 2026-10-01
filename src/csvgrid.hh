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


#ifndef _CSVGRID_HH
#define _CSVGRID_HH

#include "globals.hh"
#include "csvtable.hh"

#include <FL/Fl_Scrollbar.H>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/names.h>



/**
 * \brief The FLTK grid (`Fl_Table`) widget that displays a single CSV document. (The View)
 * 
 * The CSV grid view. It handles the drawing and events of the table grid, the visual representation
 * of the CSV document. This class gets instantiated in CsvWindow::create() for every CSV window.
 * 
 * The `dataTable` (class `CsvTable`) property points to the model for this view.
 * 
 */
class CsvGrid : public Fl_Table {
public:	
	CsvGrid(int X,int Y,int W,int H,const char* L=0);
	~CsvGrid();
	void setDataTable(CsvTable *dataTable);				// points to the data table
	void allowEvents(bool allow);						// call with false to forbid reacting to events
	bool areEventsAllowed();							// returns true if events are allowed
	void setVisibleArea(int R, int C);					// Sets the visible area of the table in a way that the given cell is central
	void setDeletionHighlight(bool, int, int);			// set deletion highlights
	void removeDeletionHighlight();
	void biggerFont();
	void smallerFont();
	void defaultFont();
	void updateSelection(int row_top, int col_left, int row_bottom, int col_right);
	void deleteSelection();								// deletes the content of the selected cells
	std::vector<int> getSelection();					// returns selection as a vector
	void moveSelection(int rows, int cols);				// moves the selection (e.g. after inserting rows or cols)
	void selectAll();									// deletes the content of the selected cells
	int handle(int);
    int innerWidth();                                   // return the inner width of the table grid 
	Fl_Scrollbar *vscrollbar();
	Fl_Scrollbar *hscrollbar();
	

protected:
	void draw_cell(TableContext context, int R,int C, int X,int Y,int W,int H);
	void event_callback2();								// table's event callback (instance)
	static void event_callback(Fl_Widget*, void *v) {	// table's event callback (static)
		((CsvGrid*)v)->event_callback2();
	}
	static void input_cb(Fl_Widget*, void* v) {			// input widget's callback
		((CsvGrid*)v)->setValueHide();
	}

private:
	CsvTable *dataTable;								// Pointer to the CsvTable object where data for this grid is stored
	int text_font_size;									// Grid font size
	int row_edit, col_edit;								// row/col being modified
	int mouse_push_top, mouse_push_left;				// stores the top-left of the current selection (used on FL_RELEASE for shift-click selecting)
	bool delHighlightType;								// deletion highlight: true => row, false => column
	int delHighlightStart, delHighlightEnd;				// start and end row or column to get deleted
	TCRUNCHER_CSVGRID_INPUT_WIDGET *input;				// single instance of CODE_INPUT_WIDGET widget
	char input_buffer[TCRUNCHER_MAX_CELL_LENGTH+1];		// storage for the input of CODE_INPUT_WIDGET
	bool eventsAllowed = true;							// false: don't react to events
	bool deletionHighlight = false;						// show deletion highlight
	
	void setValueHide(bool save=true);					// writes the entered string back to the dataTable (if save is true) and hides the input
	void startEditing(int R, int C);					
	void doneEditing(bool save=true);
	bool isToBeDeleted(int R, int C);					// checks wether (R,C) is within deletionHighlight
	
	
};



#endif







