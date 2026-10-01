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
*	CsvGrid
*
************************************************************************************/

#include "colorthemes.hh"
#include "csvapplication.hh"
#include "csvgrid.hh"
#include "csvtable.hh"
#include "csvwindow.hh"
#include "helper.hh"

#include <algorithm>


extern CsvWindow windows[];
extern CsvApplication app;


CsvGrid::CsvGrid(int X,int Y,int W,int H,const char* L) : Fl_Table(X,Y,W,H,L) {
	DEBUG_PRINTF("#### CsvGrid::CsvGrid: %d, %d, %d, %d\n", X, Y, W, H);
	text_font_size = TCRUNCHER_INITIAL_FONT_SIZE;
	callback(&event_callback, (void*)this);
	when(FL_WHEN_RELEASE);
	row_height_all(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
	// Create input widget that we'll use whenever user clicks on a cell
	input = new TCRUNCHER_CSVGRID_INPUT_WIDGET(W/2,H/2,0,0);
	input->hide();
	input->callback(input_cb, (void*)this);
	input->when(FL_WHEN_ENTER_KEY_ALWAYS);			// callback triggered when user hits Enter
	input->maximum_size(TCRUNCHER_MAX_CELL_LENGTH);
	input->textfont(app.getCustomFont(CsvApplication::FontUsage::TEXT));
	row_edit = col_edit = 0;
	end();
	col_header_color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
	row_header_color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
	redraw();
}

Fl_Scrollbar *CsvGrid::vscrollbar() {
	return Fl_Table::vscrollbar;
}

Fl_Scrollbar *CsvGrid::hscrollbar() {
	return Fl_Table::hscrollbar;
}

CsvGrid::~CsvGrid() {
	DEBUG_PRINTF("#### CsvGrid::~CsvGrid\n");
	delete(input);
}


int CsvGrid::handle(int event) {
	DEBUG_PRINTF("#### CsvGrid::handle event=%d\n", event);
	if( !areEventsAllowed() )
		return(Fl_Table::handle(event));
	int row_top, col_left, row_bot, col_right;
	int R, C;
	get_selection(row_top, col_left, row_bot, col_right);
	R = row_top;
	C = col_left;

	switch (event) {
		case FL_KEYBOARD:							// key press in table?
			if( Fl::event_key() == FL_Tab && !Fl::event_command() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// TAB
				doneEditing();
				if( Fl::event_shift() ) {
					if( C == 0 && C == col_edit && R > 0 ) {
						// SHIFT-TAB pressed while in the left-most column
						R--;
						C = dataTable->getNumberCols() - 1;
					} else if (C > 0 ){
						C--;
					}
				} else {
					if( C == dataTable->getNumberCols()-1 && C == col_edit && R < dataTable->getNumberRows()-1 ) {
						// TAB pressed while in the right-most column
						R++;
						C = 0;
					} else if (C < dataTable->getNumberCols()-1 ){
						C++;
					}
				}
				updateSelection(R,C,R,C);				// select the current cell
				setVisibleArea( R, C );
				if(	dataTable->cellContainsLineBreak(R,C) ) {
					app.editSingleCell();
				} else {
					startEditing(R,C);					// start new edit
				}
				return(1);
			} else if( Fl::event_key() == FL_Enter && !Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// ENTER
				doneEditing();						// finish any previous editing
				updateSelection(R, C, R, C);		// select the current cell
				if(	dataTable->cellContainsLineBreak(R,C) ) {
					app.editSingleCell();
				} else {
					startEditing(R,C);					// start new edit
					if (Fl::event() == FL_KEYBOARD && Fl::e_text[0] != '\r') {
						input->handle(Fl::event());		// pass keypress to input widget
					}
				}
				return(1);
			} else if( Fl::event_key() >= 32 && Fl::event_key() <= 126 && !Fl::event_command() && !Fl::event_alt() && !Fl::event_ctrl() ) {
				// [druckbares Zeichen]
				updateSelection(R, C, R, C);			// select the current cell
				startEditing(R,C);						// start new edit
				if (Fl::event() == FL_KEYBOARD && Fl::e_text[0] != '\r') {
					input->handle(Fl::event());			// pass keypress to input widget
				}
				return(1);
			} else if( Fl::event_key() == FL_Escape && !Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// ESC
				doneEditing(false);				// abort editing
				return(1);
			} else if( Fl::event_key() == FL_BackSpace && !Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// BACKSPACE
				deleteSelection();
				return(1);
			}			
		break;
		case FL_PUSH:
			// if( Fl::event_button() == FL_LEFT_MOUSE  && !Fl::event_shift() ) {
			// 	// Single Click
			// 	updateSelection(R,C,R,C);
			// } else if( Fl::event_button() == FL_LEFT_MOUSE && Fl::event_shift() ) {
			// 	// Single Shift-Click
			// 	updateSelection(row_top,col_left,R,C);
			// }
			if( Fl::event_button() == FL_LEFT_MOUSE && Fl::event_shift() && !Fl::event_command() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// Single Shift-Click
				//   Let's store the top-left of the current selection
				mouse_push_top = row_top;
				mouse_push_left = col_left;
			}
			break;
		case FL_RELEASE:
			if( Fl::event_button() == FL_LEFT_MOUSE ) {
				if( Fl::event_shift() && !Fl::event_command() && !Fl::event_ctrl() && !Fl::event_alt() ) {
					updateSelection(mouse_push_top, mouse_push_left, R, C);
				}
				windows[app.getTopWindow()].showDefaultStatus();
			}
			if( Fl::event_clicks() && Fl::event_button() == FL_LEFT_MOUSE ) {
				// Double Left Click
				doneEditing();						// finish any previous editing
				updateSelection(R,C,R,C);
				startEditing(R,C);					// start new edit
			}
			break;
	}
	int ret_val = Fl_Table::handle(event);
	return ret_val;
}



void CsvGrid::setDataTable(CsvTable *dataTable) {
	DEBUG_PRINTF("#### CsvGrid::setDataTable\n");
	this->dataTable = dataTable;
}


void CsvGrid::allowEvents(bool allow) {
	this->eventsAllowed = allow;
}


bool CsvGrid::areEventsAllowed() {
	return eventsAllowed;
}


// Handle drawing all cells in table
void CsvGrid::draw_cell(TableContext context, int R,int C, int X,int Y,int W,int H) {
	DEBUG_PRINTF("#### CsvGrid::draw_cell: %d, %d\n", R, H);
	int cell_orientation = FL_ALIGN_LEFT;
	int row_top=-1, col_left=-1, row_bot=-1, col_right=-1;						// the previous selection
	static char s[TCRUNCHER_MAX_CELL_LENGTH + 1];
	std::tuple<int, int> lastFound = app.lastFoundPosition();

	get_selection(row_top, col_left, row_bot, col_right);
	switch( context ) {
		case CONTEXT_COL_HEADER:								// table wants us to draw a column heading (C is column)
			fl_font(app.getCustomFont(CsvApplication::FontUsage::COL_HEADER), text_font_size);		// set font for heading to bold
			fl_push_clip(X,Y,W,H);								// clip region for text
			{
				if( deletionHighlight && isToBeDeleted(R,C) && !delHighlightType) {
					fl_color(ColorThemes::getColor(app.getTheme(), "deletion_bg"));
				} else {
					fl_color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
				}
				fl_rectf(X,Y,W,H);
				fl_color(ColorThemes::getColor(app.getTheme(), "header_row_text"));
				if( deletionHighlight && isToBeDeleted(R,C) && !delHighlightType ) {
					fl_color(ColorThemes::getColor(app.getTheme(), "deletion_text"));
				}
				snprintf(s, TCRUNCHER_MAX_CELL_LENGTH, "%s", dataTable->getHeaderCell(C,false).c_str());
				fl_draw(s, X,Y,W,H, FL_ALIGN_CENTER);
				fl_color(ColorThemes::getColor(app.getTheme(), "grid_border"));
				fl_rect(X,Y,W,H);				
			}
			fl_pop_clip();
			fl_font(app.getCustomFont(CsvApplication::FontUsage::TEXT), text_font_size);		// set font for heading to normal
			return;

		case CONTEXT_ROW_HEADER:								// table wants us to draw a row heading (R is row)
			fl_font(app.getCustomFont(CsvApplication::FontUsage::ROW_HEADER), text_font_size);	// set font for row heading to bold
			fl_push_clip(X,Y,W,H);
			{
				if( deletionHighlight && isToBeDeleted(R,C) && delHighlightType ) {
					fl_color(ColorThemes::getColor(app.getTheme(), "deletion_bg"));
				} else {
					fl_color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
				}
				fl_rectf(X,Y,W,H);
				// fl_draw_box(FL_BORDER_BOX , X,Y,W,H, row_header_color());
				fl_color(ColorThemes::getColor(app.getTheme(), "header_row_text"));
				if( deletionHighlight && isToBeDeleted(R,C) && delHighlightType ) {
					fl_color(ColorThemes::getColor(app.getTheme(), "deletion_text"));
				}
				// snprintf(s, TCRUNCHER_MAX_CELL_LENGTH, "%d", R+1);
				snprintf(s, TCRUNCHER_MAX_CELL_LENGTH, "%s", Helper::groupedIntToString(R+1, "\u200A").c_str());
				fl_draw(s, X,Y,W,H, FL_ALIGN_CENTER);
				fl_color(ColorThemes::getColor(app.getTheme(), "grid_border"));
				fl_rect(X,Y,W,H);				
			}
			fl_pop_clip();
			fl_font(app.getCustomFont(CsvApplication::FontUsage::TEXT), text_font_size);		// set font for heading to normal
			return;

		case CONTEXT_CELL: {								// table wants us to draw a cell
			// don't draw for cell with input widget over it
			if( R == row_edit && C == col_edit && input->visible() ) {
				return;
			}
			fl_font(app.getCustomFont(CsvApplication::FontUsage::TEXT), text_font_size);

			// BACKGROUND
			// Keyboard nav and mouse selection highlighting
			if( R >= row_top && R <= row_bot && C >= col_left && C <= col_right ) {
				// selected
				if( dataTable->isFlagged(R) ) {
					// flagged row
					fl_color(ColorThemes::getColor(app.getTheme(), "selection_flagged_bg"));
				} else {
					fl_color(ColorThemes::getColor(app.getTheme(), "selection_bg"));
				}
			} else {
				// not selected
				if( dataTable->isFlagged(R) ) {
				// flagged row
					fl_color(ColorThemes::getColor(app.getTheme(), "cell_flagged_bg"));
				} else {
					fl_color(ColorThemes::getColor(app.getTheme(), "cell_bg"));
				}
			}
			// highlight cells to be deleted
			if( deletionHighlight && isToBeDeleted(R,C)) {
				fl_color(ColorThemes::getColor(app.getTheme(), "deletion_bg"));
			}
			// found cell
			if( R == std::get<0>(lastFound) && C == std::get<1>(lastFound) ) {
				fl_color(ColorThemes::getColor(app.getTheme(), "cell_found_bg"));
			}
			fl_rectf(X,Y,W,H);

			// draw a visual hint when cell contains multiline content
			int textWidth = 0, textHeight = 0;
			fl_font(app.getCustomFont(CsvApplication::FontUsage::TEXT), text_font_size);
			fl_measure( dataTable->getCell(R,C).c_str(), textWidth, textHeight );
			if( textWidth > W || textHeight > H ) {
				fl_color(ColorThemes::getColor(app.getTheme(), "cell_ellipsis"));
				fl_rectf(X+W-5,Y+H-5,3,3);
				fl_rectf(X+W-10,Y+H-5,3,3);
			}
			

			// TEXT
			fl_push_clip(X+3, Y+3, W-6, H-6);
			{
				fl_color(ColorThemes::getColor(app.getTheme(), "cell_text"));
				if( deletionHighlight && isToBeDeleted(R,C)) {
					fl_color(ColorThemes::getColor(app.getTheme(), "deletion_text"));
				}
				if( R >= row_top && R <= row_bot && C >= col_left && C <= col_right ) {
					// selected
					if( dataTable->isFlagged(R) ) {
						fl_color(ColorThemes::getColor(app.getTheme(), "selection_flagged_text"));
					} else {
						fl_color(ColorThemes::getColor(app.getTheme(), "selection_text"));
					}
				} else {
					// not selected
					if( dataTable->isFlagged(R) ) {
						fl_color(ColorThemes::getColor(app.getTheme(), "cell_flagged_text"));
					} else {
						fl_color(ColorThemes::getColor(app.getTheme(), "cell_text"));
					}
				}
				if( R < dataTable->getNumberRows() && C < dataTable->getNumberCols() ) {
					if( Helper::isNumber(dataTable->getCell(R,C).c_str()) )
						cell_orientation = FL_ALIGN_RIGHT;
					else
						cell_orientation = FL_ALIGN_LEFT;
					fl_draw(dataTable->getCell(R,C).c_str(), X+3, Y+3, W-6, H-6, cell_orientation, 0, 0);
				} else {
					fl_draw("", X+3,Y+3,W-6,H-6, cell_orientation);
				}
			}
			fl_pop_clip();
			
			// BORDER
			fl_color(ColorThemes::getColor(app.getTheme(), "grid_border"));
			fl_rect(X,Y,W,H);
			return;
		}

		case CONTEXT_RC_RESIZE: {			// table resizing rows or columns
			if (!input->visible())
				return;
			find_cell(CONTEXT_TABLE, row_edit, col_edit, X, Y, W, H);
			if (X==input->x() && Y==input->y() && W==input->w() && H==input->h()) {
				return;					// no change? ignore
			}
			input->resize(X,Y,W,H);
			return;
		}

		default:
			return;
	}
}


void CsvGrid::event_callback2() {
	DEBUG_PRINTF("#### CsvGrid::event_callback2\n");
	if( !areEventsAllowed() )
		return;
	int R = callback_row();											// returns the row the event occured on
	int C = callback_col();
	TableContext context = callback_context();
	int event = Fl::event();
	int row_top, col_left, row_bot, col_right;						// the previous selection

	DEBUG_PRINTF(".... 20\n");
	get_selection(row_top, col_left, row_bot, col_right);
	DEBUG_PRINTF(".... 30\n");
	int topwin = app.getTopWindow();
	DEBUG_PRINTF(".... topwin=%d\n", topwin);
	windows[topwin].showDefaultStatus();
	DEBUG_PRINTF(".... 40\n");

	// Debug only!
	// #include <FL/names.h>
	// printf(">>> grid > event_callback2: event = %s (%d) | context = %d\n", fl_eventnames[event], event, context);
	// printf("... selection = %d,%d %d,%d\n", row_top, col_left, row_bot, col_right);
	// End debug
	
	switch ( context ) {
		case CONTEXT_COL_HEADER:
			switch(event) {
				case FL_RELEASE:
					DEBUG_PRINTF(".... CONTEXT_COL_HEADER > FL_RELEASE\n");
					if( Fl::event_button() == FL_RIGHT_MOUSE && !Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
						// Col Header Right_Click
						app.sort(C);
					}
					if( Fl::event_button() == FL_LEFT_MOUSE && Fl::event_shift() && !Fl::event_command() && !Fl::event_ctrl() && !Fl::event_alt() ) {
						// Col Header Shift-Left_Click
						updateSelection(0, std::min(mouse_push_left,C), rows()-1, std::max(mouse_push_left,C));
					}
				break;
			}
			break; // case CONTEXT_COL_HEADER
		case CONTEXT_ROW_HEADER:
			switch(event) {
				case FL_RELEASE:
					DEBUG_PRINTF(".... CONTEXT_ROW_HEADER > FL_RELEASE\n");
					if( Fl::event_button() == FL_LEFT_MOUSE && Fl::event_alt() && !Fl::event_command() && !Fl::event_ctrl() && !Fl::event_shift() ) {
						// Row Header Alt-Left_Click
						if( windows[app.getTopWindow()].table->isFlagged(R) ) {
							windows[app.getTopWindow()].table->flagRow(R, false);
						} else {
							windows[app.getTopWindow()].table->flagRow(R, true);
						}
						Fl::redraw();
					}
				break;
			}
			break; // case CONTEXT_ROW_HEADER
		default:
			break;
	}
	DEBUG_PRINTF(".... showDefaultStatus\n");
	windows[app.getTopWindow()].showDefaultStatus();
	DEBUG_PRINTF("<<<< CsvGrid::event_callback2\n");
}


void CsvGrid::setValueHide(bool save) {
	DEBUG_PRINTF("#### CsvGrid::setValueHide\n");
	if( save ) {
		windows[app.getTopWindow()].addUndoStateCell(dataTable->getCell(row_edit,col_edit), row_edit, col_edit, "Edit cell");
		dataTable->setCell(input->value(), row_edit, col_edit);
		windows[app.getTopWindow()].setUsed(true);
		windows[app.getTopWindow()].setChanged(true);
	}
	input->hide();
	window()->cursor(FL_CURSOR_DEFAULT);			// XXX: if we don't do this, cursor can disappear!
}


void CsvGrid::startEditing(int R, int C) {
	DEBUG_PRINTF("#### CsvGrid::startEditing\n");
	row_edit = R;									// Now editing this row/col
	col_edit = C;
	int X,Y,W,H;
	find_cell(CONTEXT_CELL, R,C, X,Y,W,H);			// Find X/Y/W/H of cell
	input->resize(X,Y,W,H);							// Move CODE_INPUT_WIDGET widget there
	#ifdef DEBUG
	std::cerr << "startEditing/getCell(" << R << "," << C << ")" << std::endl;
	#endif
	strncpy(input_buffer, dataTable->getCell(R,C).c_str(), TCRUNCHER_MAX_CELL_LENGTH );
	input->value(input_buffer);
	input->textsize(text_font_size);
	input->insert_position(0,strlen(input_buffer));		// select entire input field
	input->show();									// Show the input widget, now that we've positioned it
	input->take_focus();
}


void CsvGrid::doneEditing(bool save) {
	DEBUG_PRINTF("#### CsvGrid::doneEditing\n");
	if (input->visible()) {							// input widget visible, ie. edit in progress?
		setValueHide( save );						// Transfer its current contents to cell and hide
	}
}



// Sets the visible area of the table in a way that the given cell is central
void CsvGrid::setVisibleArea(int R, int C) {
	DEBUG_PRINTF("#### CsvGrid::setVisibleArea: %d, %d\n", R, C);
	int r1, r2, c1, c2;
	visible_cells(r1, r2, c1, c2);
	if( C >= c2 ) {
		col_position( c1 + (C - c2 + 1) );
	} else if( C < c1) {
		col_position( C );
	}
	if( R >= r2 ) {
		row_position( r1 + (R - r2 + 1) );
	} else if( R < r1 ) {
		row_position( R );
	}
}



void CsvGrid::deleteSelection() {
	DEBUG_PRINTF("#### CsvGrid::deleteSelection\n");
	int myrow, mycol;
	int s_left, s_top, s_right, s_bottom;
	
	get_selection(s_top, s_left, s_bottom, s_right);

	if( s_top == s_bottom && s_left == s_right) {
		windows[app.getTopWindow()].addUndoStateCell(dataTable->getCell(s_top,s_left), s_top, s_left, "Delete Selection");
	} else {
		windows[app.getTopWindow()].addUndoStateTable("Delete Selection");
	}
	windows[app.getTopWindow()].setUsed(true);
	windows[app.getTopWindow()].setChanged(true);

	for(myrow = s_top; myrow <= s_bottom; myrow++) {
		for(mycol = s_left; mycol <= s_right; mycol++) {
			dataTable->setCell("", myrow, mycol);
		}
	}
	this->redraw();
}


void CsvGrid::selectAll() {
	DEBUG_PRINTF("#### CsvGrid::selectAll\n");
	updateSelection(0, 0, dataTable->getNumberRows()-1, dataTable->getNumberCols()-1);
}


std::vector<int> CsvGrid::getSelection() {
	DEBUG_PRINTF("#### CsvGrid::getSelection\n");
	int s_left, s_top, s_right, s_bottom;
	get_selection(s_top, s_left, s_bottom, s_right);
	return {s_top, s_left, s_bottom, s_right};
}



/**
 *	Move selection by the given rows and cols
 */
void CsvGrid::moveSelection(int rows, int cols) {
	DEBUG_PRINTF("#### CsvGrid::moveSelection: %d, %d\n", rows, cols);
	int row_top, col_left, row_bot, col_right;
	get_selection(row_top, col_left, row_bot, col_right);
	row_top += rows;
	row_bot += rows;
	col_left += cols;
	col_right += cols;
	if( row_top < 0 )
		row_top = 0;
	if( col_left < 0 )
		col_left = 0;
	set_selection(row_top, col_left, row_bot, col_right);
}




void CsvGrid::setDeletionHighlight(bool type, int start, int end) {
	DEBUG_PRINTF("#### CsvGrid::setDeletionHighlight\n");
	delHighlightType = type;
	delHighlightStart = start;
	delHighlightEnd = end;
	deletionHighlight = true;
	this->redraw();
}

void CsvGrid::removeDeletionHighlight() {
	DEBUG_PRINTF("#### CsvGrid::removeDeletionHighlight\n");
	deletionHighlight = false;
	this->redraw();
}

bool CsvGrid::isToBeDeleted(int R, int C) {
	// printf("isToBeDeleted: %d,%d\n", R, C);
	if( delHighlightType ) {
		// delete row(s)
		if( R >= delHighlightStart && R <= delHighlightEnd ) {
			return true;
		}
	} else {
		// delete col(s)
		if( C >= delHighlightStart && C <= delHighlightEnd ) {
			return true;
		}
	}
	return false;
}

void CsvGrid::biggerFont() {
	DEBUG_PRINTF("#### CsvGrid::biggerFont\n");
	if( text_font_size < TCRUNCHER_MAX_FONT_SIZE ) {
		text_font_size += 2;
		// TODO allow resizing heights for small tables
		// row_height_all(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
		// col_header_height(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
		redraw();
	}
}

void CsvGrid::smallerFont() {
	DEBUG_PRINTF("#### CsvGrid::smallerFont\n");
	if( text_font_size > TCRUNCHER_MIN_FONT_SIZE ) {
		text_font_size -= 2;
		// TODO allow resizing heights for small tables
		// row_height_all(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
		// col_header_height(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
		redraw();
	}
}

void CsvGrid::defaultFont() {
	DEBUG_PRINTF("#### CsvGrid::defaultFont\n");
	text_font_size = TCRUNCHER_INITIAL_FONT_SIZE;
	// TODO allow resizing heights for small tables
	// row_height_all(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
	// col_header_height(text_font_size + TCRUNCHER_ROW_HEIGHT_ADD);
	redraw();
}

void CsvGrid::updateSelection(int row_top, int col_left, int row_bottom, int col_right) {
	DEBUG_PRINTF("#### CsvGrid::updateSelection\n");
	set_selection(row_top, col_left, row_bottom, col_right);
}

int CsvGrid::innerWidth() {
	DEBUG_PRINTF("#### CsvGrid::defaultFont\n");
    return this->tiw;
}






