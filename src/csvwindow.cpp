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


#include "colorthemes.hh"
#include "csvapplication.hh"
#include "csvparser.hh"
#include "csvwindow.hh"
#include "helper.hh"

#include <fstream>
#include <json/json.hpp>
#include <map>
#include <sstream>
#include <tuple>


extern CsvApplication app;
extern CsvWindow windows[];
extern Fl_Preferences file_preferences;



/************************************************************************************
*
*	My_Fl_Double_Window: used for CSV windows
*
************************************************************************************/


My_Fl_Double_Window::My_Fl_Double_Window(int W,int H, const char* title) : Fl_Double_Window (W, H, title) {}
My_Fl_Double_Window::My_Fl_Double_Window(int X,int Y,int W,int H, const char* title) : Fl_Double_Window (X, Y, W, H, title) {}
My_Fl_Double_Window::~My_Fl_Double_Window() {}


/**
	Overrides Fl_Window::handle to handle some events by our methods. The original handle
	is also called.
	If the window gets the focus, the menubar is set.
	When a paste event is found, we call our own Paste() function.
	@param		event	The event that should be handled.
	@return				The return value from the original handle method.
*/
int My_Fl_Double_Window::handle(int event) {
	int windowIndex = app.getWindowByPointer(this);
	if( !windows[windowIndex].grid->areEventsAllowed() ) {
		return 1;
	}

	int key = Fl::event_key() & ~(FL_SHIFT+FL_COMMAND);

	switch( event ) {
		case FL_SHORTCUT:
			app.getWindowByPointer(this);
			if( Fl::event_command() && key == 'q' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				#ifdef __APPLE__
				// CMD q
				app.quitApplication(true);
				return(1);
				#endif
			} else
			#ifdef __APPLE__
			if( Fl::event_command() && key == 'a' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
			#else
			if( Fl::event_ctrl() && key == 'a' && !Fl::event_shift() && !Fl::event_alt() ) {
			#endif
				// CMD a
				windows[windowIndex].grid->selectAll();
			} else
			#ifdef __APPLE__
			if( Fl::event_command() && key == '+' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
			#else
			if( Fl::event_ctrl() && (key == '+' || key == '=') && !Fl::event_shift() && !Fl::event_alt() ) {
			#endif
				// CMD +
				windows[windowIndex].grid->biggerFont();
				return(1);
			} else
			#ifdef __APPLE__
			if( Fl::event_command() && key == '-' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
			#else
			if( Fl::event_ctrl() && key == '-' && !Fl::event_shift() && !Fl::event_alt() ) {
			#endif
				// CMD -
				windows[windowIndex].grid->smallerFont();
				return(1);
			} else
			#ifdef __APPLE__
			if( Fl::event_command() && key == '0' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt()) {
			#else
			if( Fl::event_ctrl() && (key == '0' || key == 0xE0) && !Fl::event_shift() && !Fl::event_alt()) {
			#endif
				// CMD 0
				windows[windowIndex].grid->defaultFont();
				return(1);
			}
			// if( Fl::event_command() && Fl::event_alt() && key == FL_Left && !Fl::event_shift() && !Fl::event_ctrl()) {
			// 	// CMD ALT LEFT
			// 	app.moveCols(false);
			// 	return(1);
			// }
			// if( Fl::event_command() && Fl::event_alt() && key == FL_Right && !Fl::event_shift() && !Fl::event_ctrl()) {
			// 	// CMD ALT RIGHT
			// 	app.moveCols(true);
			// 	return(1);
			// }
		break;
		case FL_FOCUS:
			app.setTopWindow( windowIndex );
			app.updateMenu( windowIndex );
		break;
		case FL_PASTE:
			// app.paste();		// needed??
		break;
	}
	return(Fl_Double_Window::handle(event));
}





/************************************************************************************
*
*	CsvWindow
*
************************************************************************************/



	

CsvWindow::CsvWindow() {
	// Set initial window size to 60%, or to 85% for smaller screens
	double factor = 0.6;
	if( Fl::w() < 1800 ) {
		factor = 0.7;
	}
	if( Fl::w() < 1500 ) {
		factor = 0.85;
	}
	width = std::ceil(Fl::w() * factor);
	height = std::ceil(Fl::h() * factor);
	#ifdef DEBUG
	printf("Screen width: %d\n", Fl::w());
	#endif


	typeButtonString = "UTF-8\nCOMMA";
	// macroButtonString = "Macro";
	path = "";
	name = "";
	// pasteMode = SFCSV_DEFAULT_PASTE_MODE;

}




CsvWindow::~CsvWindow() {
	destroy();
}


/**
 *	Creates the GUI for this window. Called after the constructor has been called.
 */
void CsvWindow::create(int createdWindowCount) {
	int gridHeight;
	int iconSize = TCRUNCHER_TOOLBAR_ICONSIZE;
	int iconPadding = TCRUNCHER_TOOLBAR_ICONPADDING;
	std::string name = "Empty "+std::to_string(createdWindowCount);
	this->createdWindowCount = createdWindowCount;
	std::string workDir = "";
	
	#ifdef DEBUG
	printf("CsvWindow::create\n"); fflush(stdout);
	#endif

	// set slot as used ...
	setWindowSlotUsed(true);
	// ... but set window as not changed
	changed = false;			// calling setChanged() without a window leads to a segfault
	changedWhileUndoDisabled = false;

	// Position window in the center. Addtional windows are positioned 40 pixels to the southeast
	int moveWinSouthEast = app.getCreatedWindowCount() > 4 ? 4 : app.getCreatedWindowCount();
	int posX = std::ceil((Fl::w() - width) / 2) + (moveWinSouthEast - 1)* 40;
	int posY = std::ceil((Fl::h() - height) / 2) + (moveWinSouthEast - 1) * 40;
	
	// Load images for toolbar
	static Fl_RGB_Image *addColBeforePng  = app.xpmResizer(ui_icons::icon_add_col_left, iconSize);
	static Fl_RGB_Image *addColPng        = app.xpmResizer(ui_icons::icon_add_col_right, iconSize);
	static Fl_RGB_Image *addRowAbovePng   = app.xpmResizer(ui_icons::icon_add_row_above, iconSize);
	static Fl_RGB_Image *addRowPng        = app.xpmResizer(ui_icons::icon_add_row_below, iconSize);
	static Fl_RGB_Image *delColPng        = app.xpmResizer(ui_icons::icon_del_col, iconSize);
	static Fl_RGB_Image *delRowPng        = app.xpmResizer(ui_icons::icon_del_row, iconSize);
	static Fl_RGB_Image *infoPng          = app.xpmResizer(ui_icons::icon_info, iconSize);
	static Fl_RGB_Image *openPng          = app.xpmResizer(ui_icons::icon_open, iconSize);
	static Fl_RGB_Image *saveAsPng        = app.xpmResizer(ui_icons::icon_save_as, iconSize);
	static Fl_RGB_Image *savePng          = app.xpmResizer(ui_icons::icon_save, iconSize);
	static Fl_RGB_Image *separatorPng     = app.xpmResizer(ui_icons::icon_separator, iconSize);
	
	// Create table
	table = new CsvTable(TCRUNCHER_STARTUP_ROWS,TCRUNCHER_STARTUP_COLS);

	// Create window, grid and toolbar
	win = new My_Fl_Double_Window(posX, posY, width, height, TCRUNCHER_APP_NAME);
	win->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	setName(name);
	
	win->begin();
	gridHeight = height - TCRUNCHER_TOPOFFSET - TCRUNCHER_ICON_BAR_HEIGHT - TCRUNCHER_STATUS_BAR_HEIGHT;
	grid = new CsvGrid(10, TCRUNCHER_TOPOFFSET + TCRUNCHER_ICON_BAR_HEIGHT, width-20, gridHeight);
	grid->begin();
	grid->setDataTable(table);
	grid->box(FL_NO_BOX);		// FL_NO_BOX FL_FLAT_BOX
	grid->color(ColorThemes::getColor(app.getTheme(), "table_bg"));
	grid->redraw();
	// Table rows
	grid->row_header(1);
	grid->row_header_width(80);
	grid->row_resize(1);
	grid->rows(TCRUNCHER_STARTUP_ROWS);
	// grid->row_height_all(TCRUNCHER_STARTUP_CELL_HEIGHT);
	// Table cols
	grid->col_header(1);
	grid->col_header_height(25);
	grid->col_resize(1);
	grid->cols(TCRUNCHER_STARTUP_COLS);
	grid->col_width_all(TCRUNCHER_STARTUP_CELL_WIDTH);
	grid->updateSelection(0,0,0,0);	// select top/left cell
	grid->end();
	win->end();

	win->begin();
	toolbar = new My_Toolbar(20, TCRUNCHER_TOPOFFSET, width, TCRUNCHER_ICON_BAR_HEIGHT);
	toolbar->AddButton("Open file", openPng, &CsvApplication::openFileCB, win, iconSize+iconPadding);
	toolbar->AddButton("Save file", savePng, &CsvApplication::saveFileCB, win, iconSize+iconPadding);
	toolbar->AddButton("Save file as ...", saveAsPng, &CsvApplication::saveFileAsCB, win, iconSize+iconPadding);
	toolbar->AddButton("", separatorPng, NULL, NULL, iconSize+iconPadding);
	toolbar->AddButton("Information", infoPng, &CsvApplication::showInfoWindowCB, grid, iconSize+iconPadding);
	toolbar->AddButton("", separatorPng, NULL, NULL, iconSize+iconPadding);
	toolbar->AddButton("Add Column Left", addColBeforePng, &CsvApplication::addColBeforeCB, win, iconSize+iconPadding);
	toolbar->AddButton("Add Column Right", addColPng, &CsvApplication::addColCB, win, iconSize+iconPadding);
	toolbar->AddButton("Add Row Above", addRowAbovePng, &CsvApplication::addRowAboveCB, win, iconSize+iconPadding);
	toolbar->AddButton("Add Row Below", addRowPng, &CsvApplication::addRowCB, win, iconSize+iconPadding);
	toolbar->AddButton("", separatorPng, NULL, NULL, iconSize+iconPadding);
	toolbar->AddButton("Delete Column(s)", delColPng, &CsvApplication::delColsCB, win, iconSize+iconPadding);
	toolbar->AddButton("Delete Row(s)", delRowPng, &CsvApplication::delRowsCB, win, iconSize+iconPadding);
	toolbar->AddButton("", separatorPng, NULL, NULL, iconSize+iconPadding);
	typeButton = toolbar->AddButton(typeButtonString.c_str(), NULL, &CsvApplication::setCsvPropertiesCB, grid);
	typeButton->labelsize(10);
	toolbar->AddButton("", separatorPng, NULL, NULL, iconSize+iconPadding);
	showHeaderCheckbox = toolbar->AddCheckButton("Header", &CsvApplication::switchHeaderRowCB, NULL, 65);
	//toolbar->box(FL_FLAT_BOX);
	toolbar->end();
	win->end();

	win->begin();
	statusbar = new Fl_Box(10, height-TCRUNCHER_STATUS_BAR_HEIGHT, width-20, TCRUNCHER_STATUS_BAR_HEIGHT);
	statusbar->box(FL_FLAT_BOX);
	statusbar->color(ColorThemes::getColor(app.getTheme(), "statusbar_bg"));
	statusbar->labelcolor(ColorThemes::getColor(app.getTheme(), "statusbar_text"));
	statusbar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	statusbar->redraw();
	win->end();

	#ifdef __APPLE__
	// On macOS, we're using a single menu bar object from CsvApplication!
	winMenuBar = app.getAppMenuBar();
	#else
	// On Windows or Linux: Create one menu per window
	win->begin();
	winMenuBar = new CsvMenu();
	winMenuBar->init();
	win->end();
	#endif
	
	win->callback( &windowGetsClosedCB );
	
	win->resizable(grid);
	win->show();
}



void CsvWindow::destroy() {
	delete(win);
	delete(table);
}


CsvMenu *CsvWindow::getWinMenuBar() {
	return winMenuBar;
}

void CsvWindow::setName(std::string name) {
	this->name = name;
	this->displayName = name;
	win->copy_label(this->displayName.c_str());
}

std::string CsvWindow::getName() {
	return name;
}


bool CsvWindow::getWindowSlotUsed() {
	return windowSlotUsed;
}

void CsvWindow::setWindowSlotUsed(bool state) {
	windowSlotUsed = state;
}



/**
 *	Loads file from 'filename'. Any checks if window objects is in use and so on has to be done before.
 *	Returns true, when the file could be loaded.
 */
bool CsvWindow::loadFile(std::string filename, bool askUser, bool reopen) {
	std::ifstream input;
	long fileLength;
	std::stringstream sstr;
	CsvParser *parser = new CsvParser();
	std::pair<CsvDefinition::Encodings, int> guessedEncoding;
	std::pair<CsvDefinition, float> guessedDefinition;
	CsvDefinition definition;
	std::map<long,long> histogram;

	if( app.isAlreadyOpened(filename) && !reopen ) {
		CsvApplication::myFlChoice("", "File is already open!", {"Okay"});
		return false;
	}
	
	input.open(filename);
	if( !input ) {
		CsvApplication::myFlChoice("", "Could not open file!", {"Okay"});
		return false;
	}
	
	// Length of file: needed for guessEncoding
	fileLength = Helper::getFileSize(filename);
	
	// guess properties
	guessedDefinition = app.guessDefinition(&input);
	definition = guessedDefinition.first;
	guessedEncoding = CsvApplication::guessEncoding(&input, fileLength);
	definition.encoding = guessedEncoding.first;
	definition.bomBytes = guessedEncoding.second;

	if( guessedEncoding.first == CsvDefinition::ENC_NONE || guessedDefinition.second < 0.6 || askUser ) {
		definition = CsvApplication::setTypeByUser(definition, &input, "Open");
		if( definition.cancelled ) {
			return false;
		}
	}
	
	// Switch off custom header row
	if( table->customHeaderRowShown() ) {
		showHeaderCheckbox->clear();
		table->switchHeader();
		updateTable();
	}
	
	// Start timing
	time_t startTime = std::time(0);
	
	// Tabelle leeren und geparste Daten laden
	table->clearTable();
	app.showImWorkingWindow("Opening file ...", true);
	histogram = parser->parseCsvStream(&input, table->getStorage(), &definition);
	app.hideImWorkingWindow();
	table->updateInternals();
	if( table->getNumberRows() == 0 || table->getNumberCols() == 0 ) {
		if( askUser ) {
			CsvApplication::myFlChoice("", "Could not open file with the choosen CSV definition!", {"Okay"});			
		} else {
			CsvApplication::myFlChoice("", "Could not open file with the guessed CSV definition. Please use 'File > Open with format ...' to choose an appropriate definition.", {"Okay"});
		}
		return false;
	}
	table->setDefinition(definition);


	// Grid-Größe anpassen
	grid->rows( table->getNumberRows() );
	grid->cols( table->getNumberCols() );
	
	int durationSecs = std::difftime(std::time(0), startTime);

	// Sets the filename as the window name
	setPath(filename);
	setName(Helper::getBasename(filename));
	setUsed(true);

	// Set statusbar information
	sstr.str("");
	sstr.clear();
	sstr << "File " << getName() << " opened in " << durationSecs << " seconds.";
	updateStatusbar(sstr.str());
	
	// update TypeButton
	setTypeButton(definition);
	
	// Clear undoList
	undoList = std::deque<CsvUndo>();
	undoSaveState = -1;
	
	// Guess Header
	if( table->getNumberRows() > 0 && Helper::guessHasHeader(table->row(0)) ) {
		showHeaderCheckbox->value( table->switchHeader() );
		updateTable();
	}
	
	// switch UNDO on
	enableUndo();

	// Tabelle darstellen
	grid->redraw();
	win->redraw();
	win->flush();
	Fl::check();
	delete(parser);
	
	//
	//	Show warning on large table
	//
	// float memPercentage = Helper::calculateMemUsage( table->getNumberRows(),  table->getNumberCols(), fileLength) / (float)app.getPhysMemSize();
	//if( memPercentage > 0.3 ) {
	//	// memory usage is greater than 1 GB: disable undo (if confirmed)
	//	int choice = CsvApplication::myFlChoice("Large File", "The table you're editing is rather large. It's recommended to disable Undo functionality.<br>&nbsp;<br>Should I disable Undo for you?", {"Yes", "No"}, 100, 240);
	//	if( choice == 0 ) {
	//		app.disableUndoCB(NULL, NULL, false);
	//	}
	//} else if( Helper::calculateMemUsage( table->getNumberRows(),  table->getNumberCols(), fileLength) > 4'000'000'000ull ) {
	//	// memory usage is greater than 4 GB: warning
	//	CsvApplication::myFlChoice("Large File", "The table you're editing is rather large. Some actions may take rather long.", {"OK"});
	//}
	// #ifdef DEBUG
	// printf( "Phys Mem Size: %.2f\n", (app.getPhysMemSize() / (1024*1024.0)) );
	// printf( "Used Mem: %.2f\n", Helper::calculateMemUsage( table->getNumberRows(),  table->getNumberCols(), fileLength) / (1024*1024.0) );
	// printf( "memPercentage: %.2f\n", memPercentage );
	// #endif
	
	
	if( histogram.size() != 1 ) {
		CsvApplication::myFlChoice("Warning", "Tablecruncher found "+std::to_string(histogram.size())+" different row lengths in your CSV file. Please check your data to be sure you used the correct definition.", {"OK"});
	}
	
	
	return true;
}


void CsvWindow::setUsed(bool used) {
	this->used = used;
}

bool CsvWindow::isUsed() {
	return this->used;
}


void CsvWindow::setChanged(bool changed) {
	std::string title = name;
	this->changed = changed;
	if( changed ) {
		title += " *";
		if( isUndoDisabled() ) {
			changedWhileUndoDisabled = true;
		}
	}
	win->copy_label(title.c_str());
	Fl::check();
}

bool CsvWindow::isChanged() {
	return this->changed;
}


void CsvWindow::setPath(std::string path) {
	this->path = path;
}

std::string CsvWindow::getPath() {
	return this->path;
}

void CsvWindow::dumpSelection() {
	int s_left, s_top, s_right, s_bottom;
	grid->get_selection(s_top, s_left, s_bottom, s_right);
	printf("%d,%d - %d,%d\n", s_top, s_left, s_bottom, s_right);
}



void CsvWindow::updateStatusbar(const std::string& msg) {
	statusbarText = msg;
	statusbar->copy_label(msg.c_str());
	statusbar->redraw();
	Fl::check();
}


void CsvWindow::showDefaultStatus() {
	DEBUG_PRINTF("#### CsvWindow::showDefaultStatus\n");
	std::stringstream sstr;
	std::string sep = "";				// Separator string between column name and row
	int s_left, s_top, s_right, s_bottom;
	if( table->customHeaderRowShown() ) {
		sep = ":";
	}
	grid->get_selection(s_top, s_left, s_bottom, s_right);
	if( s_top == s_bottom && s_left == s_right ) {
		sstr << "Selection: " << table->getHeaderCell(s_left, false) << sep << (s_top+1);
	} else {
		sstr << "Selection: " << table->getHeaderCell(s_left, false) << sep << (s_top+1) << " > " << table->getHeaderCell(s_right, false) << sep << (s_bottom+1);
	}
	updateStatusbar(sstr.str());
	DEBUG_PRINTF("<<<< CsvWindow::showDefaultStatus\n");
}


void CsvWindow::updateStatusbarCB(const char *msg, void *obj) {
	CsvWindow *oWin = (CsvWindow *)obj;
	oWin->updateStatusbar(msg);
	Fl::check();
}




void CsvWindow::showInfoWindow() {
	My_Fl_Small_Window *modal;
	Fl_Help_View *helpView;
	std::ostringstream htmlStream;
	CsvDefinition definition;
	
	definition = table->getDefinition();
	
	std::string separator = CsvDefinition::getDelimiterName(definition.delimiter);;
	std::string quote(1,definition.quote);
	std::string escape(1,definition.escape);
	std::string encoding = CsvDefinition::getEncodingName(definition.encoding);
	std::string filePath = getPath();
	if( filePath == "" ) {
		filePath = getName();
	}
	
	
	htmlStream <<
		"<html>" <<
			"<body bgcolor=\"" << TCRUNCHER_SMALL_WINDOW_BACKGROUND_HTMLCODE << "\">" <<
				"<h2>File</h2>"
					"<br>" <<
					filePath <<
				"<h2>Table Size</h2>"
					"<br>" <<
					"Rows: " <<
					table->getNumberRows() <<
					"<br>" <<
					"Columns: " <<
					table->getNumberCols() <<
					"<br>" <<
					"Flagged Rows: " <<
					table->countFlaggedRows() <<
				"<h2>CSV Format</h2>"
					"<br>" <<
					"Encoding: " <<
					encoding <<
					"<br>" <<
					"Separator: " <<
					separator <<
					"<br>" <<
					"Enclosed: " <<
					quote <<
			"</body>" <<
		"</html>";


	modal = new My_Fl_Small_Window(640,480);
	modal->set_modal();
	modal->color(TCRUNCHER_SMALL_WINDOW_BACKGROUND);
	modal->label("CSV Information");

	helpView = new Fl_Help_View(20,20, 600,440);
	helpView->textsize(12);
	helpView->textfont(FL_HELVETICA);
	helpView->box(FL_FLAT_BOX);
	helpView->value( htmlStream.str().c_str() );

	modal->show();
	while( modal->shown() ) {
		Fl::wait();
	}
	delete modal;
}


void CsvWindow::applyTheme() {
	if( getWindowSlotUsed() ) {
		typeButton->labelcolor(ColorThemes::getColor(app.getTheme(), "toolbar_text"));
		showHeaderCheckbox->color(ColorThemes::getColor(app.getTheme(), "toolbar_bg"));
		showHeaderCheckbox->labelcolor(ColorThemes::getColor(app.getTheme(), "toolbar_text"));
		showHeaderCheckbox->selection_color(ColorThemes::getColor(app.getTheme(), "header_button_on"));
		toolbar->color(ColorThemes::getColor(app.getTheme(), "toolbar_bg"));
		statusbar->color(ColorThemes::getColor(app.getTheme(), "statusbar_bg"));
		statusbar->labelcolor(ColorThemes::getColor(app.getTheme(), "statusbar_text"));
		grid->color(ColorThemes::getColor(app.getTheme(), "table_bg"));
		grid->vscrollbar()->color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
		grid->vscrollbar()->selection_color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		grid->vscrollbar()->labelcolor(ColorThemes::getColor(app.getTheme(), "statusbar_text"));
		grid->hscrollbar()->color(ColorThemes::getColor(app.getTheme(), "header_row_bg"));
		grid->hscrollbar()->selection_color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		grid->hscrollbar()->labelcolor(ColorThemes::getColor(app.getTheme(), "statusbar_text"));
		win->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		#ifndef __APPLE__
		winMenuBar->textcolor(ColorThemes::getColor(app.getTheme(), "toolbar_text"));
		winMenuBar->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		#else
		// winMenuBar->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		#endif
		Fl::redraw();
	}
}


/*
	Stores file preferences for this file in preferences file
	Creates a JSON 
 */
void CsvWindow::storeWindowPreferences() {
	nlohmann::json winJson;			// stores the preferences for this single window / file
	// get column widths
	std::vector<int> colWidths;
	for(table_index_t c = 0; c < table->getNumberCols(); ++c ) {
		colWidths.push_back( grid->col_width(c) );
	}
	// create JSON
	winJson["path"] = getPath();
	winJson["timestamp"] = std::to_string( Helper::getTimestamp() );
	winJson["header-set"] = table->customHeaderRowShown();
	winJson["col-widths"] = colWidths;
	// create hash for the file path
	std::size_t pathHash = std::hash<std::string>{}(winJson["path"]);
	std::string pathHashStr = std::to_string(pathHash);
	// store preferences
	file_preferences.set(pathHashStr.c_str(), winJson.dump().c_str());
}


/*
	Reads the file preferences for the opened file and set the window properties accordingly.
 */
void CsvWindow::readWindowPreferences() {
	nlohmann::json winJson;
	std::string pathHashStr = std::to_string( std::hash<std::string>{}(getPath()) );
	std::string filePref = app.getPreference(&file_preferences, pathHashStr, "");
	if( filePref != "" ) {
		try {
			winJson = nlohmann::json::parse( filePref );
			// set column widths
			int c = 0;
			for(const auto &w : winJson["col-widths"] ) {
				if( c < table->getNumberCols() ) {
					grid->col_width(c, w);
				}
				++c;
			}
			// show or hide headers if needed
			if( winJson["header-set"].get<bool>() != table->customHeaderRowShown() ) {
				app.switchHeaderRowCB(NULL,NULL);
			}
		} catch(const std::exception& e) {
			std::cerr << "Error reading file preferences." << std::endl;
			std::cerr << e.what() << std::endl;
		}
	}
}


void CsvWindow::setTypeButton(CsvDefinition definition) {
	std::string str = "NONE";
	str = std::string(CsvDefinition::getEncodingName(definition.encoding)) + "\n" + CsvDefinition::getDelimiterName(definition.delimiter);
	typeButton->copy_label(str.c_str());
}



std::string CsvWindow::humanReadableSelection() {
	long rows = table->getNumberRows();
	long cols = table->getNumberCols();
	std::vector<int> sel = grid->getSelection();
	std::string human = "";
	if(
		(sel[0] == sel[2] && sel[1] == sel[3]) ||
		(sel[0] == 0 && sel[1] == 0 && sel[2] == rows - 1 && sel[3] == cols - 1 )
	) {
		// Full Table
		human = "Full Table";
	} else if( sel[0] == 0 && sel[2] == rows - 1 ) {
		// Column(s)
		if( sel[1] == sel[3] ) {
			human = "Column '" + table->getHeaderCell(sel[1], false)+"'";
		} else {
			human = "Columns '" + table->getHeaderCell(sel[1], false) + "' > '" + table->getHeaderCell(sel[3], false) + "'";
		}
	} else {
		// Selection
		human = "Selection (" + table->getHeaderCell(sel[1], false) + ":" + std::to_string(sel[0]+1) + " > " + table->getHeaderCell(sel[3], false) + ":" + std::to_string(sel[2]+1) + ")";
	}
	return human;
}



void CsvWindow::windowGetsClosedCB(Fl_Widget *widget, void *) {
	int windowIndex = app.getWindowByPointer(widget);
	app.closeWindow(windowIndex);
}


/*
 *	Updates the internals of the underlying CsvTable and its display in CsvGrid
 */
void CsvWindow::updateTable() {
	table->updateInternals();
	grid->rows( table->getNumberRows() );
	grid->cols( table->getNumberCols() );
	grid->redraw();
}


/*
 *	Stores the complete table as Undo state
 */
void CsvWindow::addUndoStateTable(std::string descr) {
	if( undoDisabled )
		return;
	CsvUndo ustate;
	int row_top, row_bottom, col_top, col_bottom;
	grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	table->s_top = row_top;
	table->s_bottom = row_bottom;
	table->s_left = col_top;
	table->s_right = col_bottom;
	ustate.createUndoStateTable(*table, descr);
	undoList.push_back(ustate);
	app.setUndoMenuItem(true);
}

/*
 *	Stores just a single cell as an Undo state
 */
void CsvWindow::addUndoStateCell(std::string cellContent, int R, int C, std::string descr) {
	if( undoDisabled )
		return;
	CsvUndo ustate;
	ustate.createUndoStateCell(cellContent, R, C, table->customHeaderRowShown(), descr);
	undoList.push_back(ustate);
	app.setUndoMenuItem(true);
}



void CsvWindow::undo() {
	if( undoDisabled )
		return;
	bool undoSuccess = false;
	std::string undoDescr;
	size_t size;
	std::vector<int> selection;
	int undoType, R, C;
	if( !undoList.empty() ) {
		CsvUndo ustate = undoList.back();
		undoDescr = ustate.getDescr();
		undoType = ustate.getType();
		// int myUniqNumber = ustate.getId();
		switch( undoType ) {
			case TCRUNCHER_UNDO_TYPE_TABLE:
				table->setStorage( ustate.getUndoStorage() );
				delete(table->headerRow);						// delete recent headerRow
				size = ustate.getHeaderRow().size();
				table->headerRow = new std::vector<std::string>();
				table->headerRow->resize(size);
				for( size_t r = 0; r < size; ++r) {
					table->headerRow->at(r) = ustate.getHeaderRow().at(r);
				}
				table->flags = ustate.getFlags();
				selection = ustate.getSelection();
				if( selection.size() == 4 ) {
					grid->set_selection(selection[0],selection[1],selection[2],selection[3]);
				}
				table->updateInternals();
				grid->cols( table->getNumberCols() );
				grid->rows( table->getNumberRows() );
				grid->redraw();
				if( ustate.getSwitchHeaderRow() != table->customHeaderRowShown() ) {
					table->setCustomHeaderRowShown( ustate.getSwitchHeaderRow() );
					if( ustate.getSwitchHeaderRow() ) {
						showHeaderCheckbox->set();
					} else {
						showHeaderCheckbox->clear();
					}
				}
				showHeaderCheckbox->redraw();
				undoSuccess = true;
			break;
			case TCRUNCHER_UNDO_TYPE_CELL:
				if( ustate.getSwitchHeaderRow() != table->customHeaderRowShown() ) {
					app.switchHeaderRowCB(NULL, NULL);
					showHeaderCheckbox->redraw();
				}
				std::tie(R,C) = ustate.getCellPosition();
				table->setCell(ustate.getCellContent(), R, C);
				grid->redraw();
				undoSuccess = true;
			break;
			default:
			break;
		}
		undoList.pop_back();
		if( undoSuccess && !changedWhileUndoDisabled ) {
			bool setToChanged = true;							// set to false, if this UNDO resets to saved state
			if( undoSaveState == -1 && undoList.empty() ) {		// no UNDO state has been saved so far
				setToChanged = false;
			} else if( !undoList.empty() ) {
				ustate = undoList.back();
				if( undoSaveState == ustate.getId() ) {
					setToChanged = false;
				}
			}
			setChanged(setToChanged);
		}
	}
	if( undoList.empty() ) {
		app.setUndoMenuItem(false);
	}
	showDefaultStatus();
}

bool CsvWindow::hasUndoStates() {
	if( undoList.empty() ) {
		return false;
	} else {
		return true;
	}
}


/*
 *	Removes the top-most undo state without applying it – used for macros as we don't know what the macro is doing in advance
 */
void CsvWindow::removeLastUndoState() {
	if( hasUndoStates() ) {
		undoList.pop_back();
	}
}


/*
 *	Sets undoSaveState to the ID of the most recent undo state
 */
void CsvWindow::setUndoSaveState() {
	if( !undoList.empty() ) {
		CsvUndo ustate = undoList.back();
		undoSaveState = ustate.getId();
	}
}



void CsvWindow::disableUndo() {
	undoDisabled = true;
	clearUndoList();
}


void CsvWindow::enableUndo() {
	undoDisabled = false;
}


bool CsvWindow::isUndoDisabled() {
	return undoDisabled;
}



void CsvWindow::clearUndoList() {
	CsvUndo ustate;
	while( !undoList.empty() ) {
		ustate = undoList.back();
		// ustate.deleteTable();
		undoList.pop_back();
	}
}














