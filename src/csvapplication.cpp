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


#include "csvapplication.hh"
#include "csvmenu.hh"

#include "icons/abouticon.xpm"



/************************************************************************************
*
*	CsvApplication
*
************************************************************************************/


// I think this global function is very bad style ... TODO XXX
void updateMacroLogBuffer(void *logVoid, std::string str) {
	Fl_Text_Display *logDisplay = ((Fl_Text_Display *) logVoid);
	if( logDisplay ) {
		logDisplay->buffer()->append(str.c_str());
	}
}


extern CsvWindow windows[];

extern CsvApplication app;

extern Fl_Preferences preferences;
extern Fl_Preferences macros;
extern Macro macro;


/**
 *	Constructor
 */
CsvApplication::CsvApplication() {
	std::string prefTheme = getPreference(&preferences, TCRUNCHER_PREF_THEME, "Bright");
	std::string prefFont = getPreference(&preferences, TCRUNCHER_PREF_GRID_TEXT_FONT, TCRUNCHER_FALLBACK_FONT);

	struct My_Fl_Button::buttonColorStruct colorsDefaultButton;
	colorsDefaultButton.background = ColorThemes::getColor(prefTheme, "button_bg");
	colorsDefaultButton.label = ColorThemes::getColor(prefTheme, "button_text");
	colorsDefaultButton.border = ColorThemes::getColor(prefTheme, "button_border");
	colorsDefaultButton.windowBg = ColorThemes::getColor(prefTheme, "win_bg");
	colorsDefaultButton.borderWidth = ColorThemes::getColor(prefTheme, "button_border_width");
	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(prefTheme, "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(prefTheme, "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(prefTheme, "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(prefTheme, "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(prefTheme, "highlight_button_border_width");
    
    // Improve tooltip layout
	Fl_Tooltip::textcolor( ColorThemes::getColor(prefTheme, "header_row_text") );
	Fl_Tooltip::color( ColorThemes::getColor(prefTheme, "win_bg") );
	Fl_Tooltip::delay( 0.2 );
	Fl_Tooltip::size( 12 );
	Fl_Tooltip::margin_width(6);		// default: 3
	Fl_Tooltip::margin_height(4);		// default: 3
    
    
	// Grid fonts
	#ifdef __APPLE__
	const char* TCRUNCHER_FONTS_ARR[] = {"Andale Mono", "Courier New", "Helvetica", "Menlo", "Monaco"};
	const int TCRUNCHER_FONTS_ARR_COUNT = 5;
	#else
	const char* TCRUNCHER_FONTS_ARR[] = {"Consolas", "Courier New", "Helvetica"};
	const int TCRUNCHER_FONTS_ARR_COUNT = 3;
	#endif
    
	// Fl::set_font(9901, "Monaco");
	for( int i = 0; i < TCRUNCHER_FONTS_ARR_COUNT; ++i ) {
		Fl::set_font(TCRUNCHER_FONT_NUMBER + i, TCRUNCHER_FONTS_ARR[i]);
		fontMapping[std::string(TCRUNCHER_FONTS_ARR[i])] = TCRUNCHER_FONT_NUMBER + i;
	}
    
    //
	// Create Search Window
	searchWin = new My_Fl_Search_Window(600,350);
	searchWin->label("Find and Replace");
	searchWin->color(ColorThemes::getColor(prefTheme, "win_bg"));
	searchInput = new Fl_Input(120,20,370,20, "Find:");
	searchInput->box(FL_BORDER_BOX);
	searchInput->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	replaceInput = new Fl_Input(120,70,370,20, "Replace:");
	replaceInput->box(FL_BORDER_BOX);
	replaceInput->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	findButton = new My_Fl_Button(120,210,100,24, "Find Next");
	findButton->colors = colorsHighlightButton;
	findButton->callback(find_substring_CB, TCRUNCHER_MYFLCHOICE_MAGICAL);
	findButton->shortcut(FL_Enter);
	replaceButton = new My_Fl_Button(120,250,100,24, "Replace");
	replaceButton->colors = colorsDefaultButton;
	replaceButton->callback(find_replaceFind_CB, NULL);
	replaceFindButton = new My_Fl_Button(240,250,120,24, "Replace+Find");
	replaceFindButton->colors = colorsDefaultButton;
	replaceFindButton->callback(find_replaceFind_CB, NULL);
	replaceAllButton = new My_Fl_Button(380,250,110,24, "Replace All");
	replaceAllButton->colors = colorsDefaultButton;
	replaceAllButton->callback(find_replaceAll_CB, CsvApplication::ReplaceAllType::REPLACE);
	flagMatchingButton = new My_Fl_Button(120,290,170,24, "Flag Matching Rows");
	flagMatchingButton->colors = colorsDefaultButton;
	flagMatchingButton->callback(find_replaceAll_CB, CsvApplication::ReplaceAllType::FLAG);
	unflagMatchingButton = new My_Fl_Button(310,290,180,24, "Unflag Matching Rows");
	unflagMatchingButton->colors = colorsDefaultButton;
	unflagMatchingButton->callback(find_replaceAll_CB, CsvApplication::ReplaceAllType::UNFLAG);
	ignoreCase = new Fl_Check_Button(120,150,100,24, "Ignore Case");
	ignoreCase->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	useRegex = new Fl_Check_Button(310,150,180,24, "Use regular expression");
	useRegex->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	searchWinLabel = new Fl_Box(120,320,300,20);
	searchWinLabel->box(FL_NO_BOX);
	searchWinLabel->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	searchWinLabel->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	searchWinLabel->redraw();
	searchWinScope = new Fl_Box(120,110,200,24, "Scope: Full Table");
	searchWinScope->box(FL_NO_BOX);
	searchWinScope->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	searchWinScope->labelcolor(ColorThemes::getColor(prefTheme, "win_text"));
	searchWinScope->redraw();
	// ... completed
	searchWin->end();

	// Create Sort Window
	sortWin = new My_Fl_Small_Window(640,180);
	sortWin->end();
	
	#ifdef __APPLE__
	// Set application window
	appMenuBar = new CsvMenu();
	appMenuBar->init();
	static Fl_Menu_Item appMenuItems[] = {
		{ "Preferences...", FL_COMMAND + ',', CsvApplication::AppMenuPreferencesCB, 0, 0, 0, 0, 0, 0 },
	 	{ "Check for Updates", 0, CsvApplication::checkUpdateCB, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 } 
	};
	Fl_Mac_App_Menu::custom_application_menu_items(appMenuItems);
	// mini window that stays open when the last data window is closed
	remainOpenWin = new My_Fl_Small_Window((Fl::w() - 300)/2, (Fl::h() -200)/2, 300, 200, TCRUNCHER_APP_NAME);
	remainOpenWin->color(ColorThemes::getColor(prefTheme, "win_bg"));
	remainQuitButton = new My_Fl_Button(75,88,150,24, "Quit Application");
	remainQuitButton->colors = colorsHighlightButton;
	remainQuitButton->clear_visible_focus();
	remainQuitButton->callback(closeRemainOpenWindow, NULL);
	remainOpenWin->end();
	remainOpenWin->hide();
	#endif
	
	// imWorkingWindow
	imWorkingWindow = new My_Fl_Small_Window(400,120,"Processing ...");
	imWorkingWindow->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	imWorkingButton = new Fl_Button(10,10,380,100,"Please stand by while I'm working ...");
	imWorkingButton->box(FL_NO_BOX);
	imWorkingButton->visible_focus(0);
	imWorkingWindow->end();
	imWorkingWindow->hide();
	
	
	try {
		physMemSize = Helper::getPhysMemSize();
	} catch(...) {}

	plusPng = xpmResizer(ui_icons::icon_plus, macroWinImgButtonSize);
	minusPng = xpmResizer(ui_icons::icon_minus, macroWinImgButtonSize);


	// Custom Fonts
	customFonts.gridText = Helper::getFltkFontCode(getPreference(&preferences, TCRUNCHER_PREF_GRID_TEXT_FONT, "FL_HELVETICA"));

	// fill Open Recent menu from preferences file
	for( int i = TCRUNCHER_PREF_RECENT_FILES_NUM - 1; i >= 0; --i ) {
		std::string tmp_key = TCRUNCHER_PREF_RECENT_FILES_STUB + std::to_string(i);
		std::string tmp_val = getPreference(&preferences, tmp_key, "");
		if( tmp_val != "" ) {
			std::filesystem::path recent_file(tmp_val);
			if( std::filesystem::exists(recent_file) ) {
				recentFiles.add(tmp_val);
			}
		}
	}
	#ifdef __APPLE__
	appMenuBar->updateOpenRecentMenu(recentFiles.getRecentFiles());
    #endif
	app.getPreference(&preferences, TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "no");
}


/**
 *	Destructor
 */

CsvApplication::~CsvApplication() {
	delete searchInput;
	delete replaceInput;
	delete findButton;
	delete replaceButton;
	delete replaceFindButton;
	delete replaceAllButton;
	delete flagMatchingButton;
	delete unflagMatchingButton;
	delete ignoreCase;
	delete useRegex;
	delete searchWinLabel;
	delete searchWinScope;
	delete searchWin;
	
	delete sortWin;
	
	#ifdef __APPLE__
	delete appMenuBar;
	delete remainQuitButton;
	delete remainOpenWin;
	#endif

	delete imWorkingButton;
	delete imWorkingWindow;
}



std::map<std::string,int> CsvApplication::getFontMapping() {
	return fontMapping;
}


void CsvApplication::changeFontSize(int changeMode) {
	switch(changeMode) {
		case 0:
			windows[getTopWindow()].grid->defaultFont();
		break;
		case 1:
			windows[getTopWindow()].grid->biggerFont();
		break;
		case -1:
			windows[getTopWindow()].grid->smallerFont();
		break;
	}
}


/**
	Callback for Preferences in Application Menu
 */
void CsvApplication::AppMenuPreferencesCB(Fl_Widget *, void *) {
	std::string prefCheckUpdates = app.getPreference(&preferences, TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "no");
	My_Fl_Small_Window win(500, 150);
	Fl_Check_Button updateCheckAllowed( 30, 30, 260, 25," Regularly check for updates");
	updateCheckAllowed.visible_focus(0);
	updateCheckAllowed.labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	updateCheckAllowed.callback(CsvApplication::updateCheckButtonCB, NULL);
	if( app.isUpdateCheckAllowed() ) {
		updateCheckAllowed.value(1);
	} else {
		updateCheckAllowed.value(0);
	}
	// Fl_Choice gridTextFontChoice( 100, 60, 260, 30, "Grid font");
	//     // gridTextFontChoice.callback(choice_cb, 0);
	//     gridTextFontChoice.add("Helvetica");
	//     gridTextFontChoice.add("Courier");
	//     gridTextFontChoice.add("Times");
	//     gridTextFontChoice.add("Monaco");
	win.set_modal();
	win.label("Tablecruncher Preferences");
	win.color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	win.show();
	while( win.shown() ) {
		Fl::wait();
	}
}
void CsvApplication::updateCheckButtonCB(Fl_Widget *w, void *) {
	Fl_Check_Button *but = (Fl_Check_Button*)w;
	if( but->value() ) {
		app.setUpdateCheck(true);
		preferences.set(TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "yes");
	} else {
		app.setUpdateCheck(false);
		preferences.set(TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "no");
	}
}



/**
	Returns true if onboarding finished
 */
bool CsvApplication::showOnboardingProcess() {
    #ifdef _WIN64
    return true;
    #endif
	int choice;
	std::string msg;
	// Step 1: Ask for update permission
	msg = "<b>Step 1/2</b><br>&nbsp;<br>Should Tablecruncher check automatically for updates once a week?<br>&nbsp;<br><font size='2'>You may change your selection later via <i>Tablecruncher &gt; Preferences...</i></font>";
	choice = myFlChoice("Welcome to Tablecruncher", msg, {"Yes", "No", "Cancel"}, 100, 200);
	if( choice == -1 || choice == 2) {
		return false;
	}
	if( choice == 0 ) {
		setUpdateCheck(true);
		preferences.set(TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "yes");
	} else if( choice == 1 ) {
		setUpdateCheck(false);
		preferences.set(TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "no");
	}
	// Step 2: Inform about GPL
	msg = "<b>Step 2/2</b><br>&nbsp;<br>Tablecruncher is licensed under GPL v3";
	choice = myFlChoice("Welcome to Tablecruncher", msg, {"OK", "Cancel"}, 100, 200);
	if( choice == -1 || choice == 1) {
		return false;
	}
	return true;
}



/**
 *	Creates a new window
 *	https://stackoverflow.com/questions/5887615/creating-an-array-of-object-pointers-c
 */
int CsvApplication::createNewWindow() {
	int newWindowIndex = -1;

	// Look for an unused window slot
	for(int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot) {
		if( !windows[slot].getWindowSlotUsed() ) {
			newWindowIndex = slot;
			break;
		}
	}
	if( newWindowIndex == -1 ) {
#ifdef DEBUG
		dumpWindows();
#endif
		CsvApplication::myFlChoice("", "Maximum number of windows already open.", {"Okay"});
		return -1;
	}
	windows[newWindowIndex].create(createdWindowCount++);
	updateMenu(newWindowIndex);
	#ifdef __APPLE__
	remainOpenWin->hide();
	#endif
	// setMenuBar(windows[newWindowIndex].win);
	windows[newWindowIndex].win->show();
	return newWindowIndex;
}


size_t CsvApplication::getCreatedWindowCount() {
	return createdWindowCount;
}




void CsvApplication::openFileCB(Fl_Widget *, void *) {
	app.openFile(false);
}
void CsvApplication::saveFileCB(Fl_Widget *, void *) {
	app.saveFile(false);
}
void CsvApplication::saveFileAsCB(Fl_Widget *, void *) {
	app.saveFile(true);
}
void CsvApplication::splitCsvCB(Fl_Widget *, void *) {
	app.splitCsvFiles();
}
void CsvApplication::exportJsonCB(Fl_Widget *, void *) {
	app.saveFile(true, SaveType::JSON);
}
void CsvApplication::exportFlaggedCB(Fl_Widget *, void *) {
	app.saveFile(true, SaveType::CSV, true);
}
void CsvApplication::checkDataConsistencyCB(Fl_Widget *, void *) {
	app.checkDataConsistency();
}
void CsvApplication::showInfoWindowCB(Fl_Widget *, void *) {
	windows[app.getTopWindow()].showInfoWindow();
}
void CsvApplication::addColBeforeCB(Fl_Widget *, void *) {
	app.addCol(true);
}
void CsvApplication::addColCB(Fl_Widget *, void *) {
	app.addCol(false);
}
void CsvApplication::addRowAboveCB(Fl_Widget *, void *) {
	app.addRow(true);
}
void CsvApplication::addRowCB(Fl_Widget *, void *) {
	app.addRow(false);
}
void CsvApplication::delColsCB(Fl_Widget *, void *) {
	app.delCols();
}
void CsvApplication::delRowsCB(Fl_Widget *, void *) {
	app.delRows();
}
void CsvApplication::quitApplicationCB(Fl_Widget *, void *) {
	app.quitApplication(true);
}
void CsvApplication::closeRemainOpenWindow(Fl_Widget *, void *) {
	if( app.nfcIsOpen )
		return;
	app.quitApplication(true);
}
void CsvApplication::undoCB(Fl_Widget *, void *) {
	int winIndex = app.getTopWindow();
	if( windows[winIndex].hasUndoStates() ) {
		app.showImWorkingWindow("Undoing last step ...");
		windows[winIndex].undo();
		app.hideImWorkingWindow();
	} else {
		// myFlChoice("Info", "No Undo state available.", {"Okay"});
	}
	
}

void CsvApplication::disableUndoCB(Fl_Widget *, void *, bool confirmDialog) {
	int winIndex = app.getTopWindow();
	int choice = 0;
	if( confirmDialog ) {
		choice = CsvApplication::myFlChoice("Confirmation", "Do you really want to disable Undo functionality?<br><small>Only recommended for really large tables.</small>", {"Yes", "No"});
	}
	if( choice == 0 ) {
		windows[winIndex].disableUndo();
		app.updateMenu(winIndex);
	}
}

void CsvApplication::enableUndoCB(Fl_Widget *, void *) {
	int winIndex = app.getTopWindow();
	windows[winIndex].enableUndo();
	app.updateMenu(winIndex);
}


void CsvApplication::setCsvPropertiesCB(Fl_Widget *, void *) {
	CsvDefinition definition;
	int winIndex = app.getTopWindow();
	definition = windows[winIndex].table->getDefinition();
	definition = setTypeByUser(definition, NULL, "Change Type");
	windows[winIndex].table->setDefinition(definition);
	windows[winIndex].setTypeButton(definition);
}



/*
 *	Asks user for a file and opens it.
 *	askUser				Should the user be asked about CSV format?
 *	reopen				Should the file be opened in the same window? Caution: Changes are overwritten.
 */
void CsvApplication::openFile(bool askUser, bool reopen) {
	int winIndex;
	std::string fn;
	Fl_Native_File_Chooser fnfc;

	fnfc.title("Pick a file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);	
	fnfc.filter("CSV Files\t*.{txt,csv,tsv}");
	fnfc.directory(workDir.c_str());
	
	if( reopen ) {
		winIndex = app.getTopWindow();
		if( winIndex >= 0 ) {
			fn = windows[winIndex].getPath();
			windows[winIndex].loadFile(fn, true, true);
		}
	} else {
		nfcIsOpen = true;
		switch ( fnfc.show() ) {
			case -1:
				#ifdef DEBUG
				printf("ERROR: %s\n", fnfc.errmsg());
				#endif
			break;  // ERROR
			case 1:
				#ifdef DEBUG
				printf("CANCEL\n");
				#endif
			break;  // CANCEL
			default:
				#ifdef DEBUG
				printf("PICKED: %s\n", fnfc.filename());
				#endif
				fn = fnfc.filename();
				openFile(fn, askUser);
		}
		nfcIsOpen = false;
	}
}


void CsvApplication::openFile(std::string path, bool askUser) {
	int winIndex = app.getTopWindow();
	bool fileLoaded;
	bool createdNewWindow = false;
	
	if( windows[winIndex].isUsed() ) {
		winIndex = app.createNewWindow();
		createdNewWindow = true;
	}
	if( winIndex >= 0 && winIndex <= TCRUNCHER_MAX_WINDOWS ) {
		windows[winIndex].win->show();
		fileLoaded = windows[winIndex].loadFile(path, askUser);
		if( fileLoaded ) {
			app.setWorkDir( Helper::getDirectory(path) );
			windows[winIndex].setUsed(true);
			windows[winIndex].setWindowSlotUsed(true);
			// load file preferences
			windows[winIndex].readWindowPreferences();
			windows[winIndex].grid->redraw();
			Fl::check();
            // automatically arrange column widths
            app.arrangeColumnsCB(NULL, NULL);
			// add file to recent files
			recentFiles.add(path);
			updateMenu(winIndex);
		} else {
			if( createdNewWindow )
				closeWindow(winIndex);
		}
	}
}


void CsvApplication::openRecentFile(size_t index) {
	std::string filepath = recentFiles.get(index);
	if( filepath != "" ) {
		app.openFile(filepath, false);
	}
}


bool CsvApplication::splitCsvFiles() {
	int winIndex = getTopWindow();
	bool retOkay, mySaveState = false;
	long numOfRowsPerFile = 0;
	long numOfRows = windows[winIndex].table->getNumberRows();
	long fromRow, toRow;
	std::ifstream fileExistsTest;
	std::string numOfRowsPerFileStr, filename, msg;
	std::string pathWithoutExtension,extension;
	// TODO Return if no file has been opened or saved so far
	// TODO Ask if numOfFiles gets to big
		
	std::tie(retOkay, numOfRowsPerFileStr) = myFlAskString("Number of lines per file", "Split");
	if( retOkay ) {
		try {
			if( numOfRowsPerFileStr.find_first_not_of("0123456789") == std::string::npos ) {
				numOfRowsPerFile = std::stoi(numOfRowsPerFileStr);
			} else {
				numOfRowsPerFile = 0;
			}
		} catch(...) {}				// intentional empty clause
		if( numOfRowsPerFile > 0 ) {
			int numOfFiles = (int) std::ceil((double)numOfRows / numOfRowsPerFile);
			int digitalExtensionLength = std::ceil(std::log10(numOfFiles));
			if( numOfFiles > 1 ) {
				std::tie(pathWithoutExtension,extension) = Helper::getPathWithoutExtension(windows[winIndex].getPath());
				// Test if one of the splitted files already exists
				bool splitFileExists = false;
				for(int i=0; i<numOfFiles; ++i) {
					filename = splittedFileName(pathWithoutExtension, extension, i, digitalExtensionLength);
					fileExistsTest.open(filename.c_str());
					if( fileExistsTest.good() ) {
						splitFileExists = true;
						msg = "File " + Helper::getBasename(filename) + " does already exist!";
						myFlChoice("Error", msg, {"Okay"});
						break;
					}
				}
				if( !splitFileExists ) {
					showImWorkingWindow("Splitting CSV file ...");
					for(int i=0; i<numOfFiles; ++i) {
						fromRow = i * numOfRowsPerFile;
						if( i < numOfFiles -1 ) {
							toRow = fromRow + numOfRowsPerFile - 1;
						} else {
							toRow = -1;
						}
						filename = splittedFileName(pathWithoutExtension, extension, i, digitalExtensionLength);
						mySaveState = windows[winIndex].table->saveCsv(filename, &CsvWindow::updateStatusbarCB, &windows[winIndex], false, fromRow, toRow);
					}
					hideImWorkingWindow();
					msg = "The splitted CSV files have been stored. (No of Files: "+std::to_string(numOfFiles)+")";
					myFlChoice("Files stored", msg, {"Okay"});
				}
			} else {
				myFlChoice("Error", "The number of rows per splitted file has to be lower than the number of rows in the original file.", {"Okay"});
			}

		} else {
			myFlChoice("Error", "Invalid number given!", {"Okay"});
		}
	}
	return mySaveState;
}


std::string CsvApplication::splittedFileName(std::string pathWithoutExtension, std::string extension, int num, int digitalExtensionLength) {
	return pathWithoutExtension + ".split-" + Helper::padInteger(num,digitalExtensionLength) + "." + extension;
}


/*
 	Saves top-most window
 	saveAs				User clicked "Save As", default: false
 	type				What format to save data to, default: CsvApplication::SaveType CSV
	flaggedOnly			Only export flagged rows
 */
bool CsvApplication::saveFile(bool saveAs, CsvApplication::SaveType type, bool flaggedOnly) {
	Fl_Native_File_Chooser fnfc;
	int winIndex = getTopWindow();
	std::string path;						// path as stored in CsvWindow
	std::string fn = "";					// path as chosen by user
	bool repeatLoop = false;
	std::ifstream fileExistsTest;
	int choice;
	std::string baseFilename, pathFilename;
	bool ret = false;
	std::string fileFilter = "CSV Files\t*.{txt,csv,tsv}";
	std::string defaultExtension = ".csv";
	
	if( type == CsvApplication::SaveType::JSON ) {
		fileFilter = "JSON Files\t*.{json}";
		defaultExtension = ".json";
	}
	
	path = windows[winIndex].getPath();
	
	// Check if there are flagged rows when exporting flagged rows
	if( flaggedOnly && windows[winIndex].table->countFlaggedRows() == 0 ) {
		CsvApplication::myFlChoice("", "There are no flagged rows!", {"Okay"});
		return false;
	}

	// Ask for path where file should be stored
	if( path == "" || saveAs ) {
		do {
			repeatLoop = false;
			fn = "";
			fnfc.title("Pick a file");
			fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
			#ifdef DEBUG
			std::cerr << "  fnfc.filter" << std::endl;
			#endif
			fnfc.filter(fileFilter.c_str());
			fnfc.directory(workDir.c_str());
	
			switch ( fnfc.show() ) {
				case -1:
					#ifdef DEBUG
					printf("ERROR: %s\n", fnfc.errmsg());
					#endif
					break;  // ERROR
				case  1:
					#ifdef DEBUG
					printf("CANCEL\n");
					#endif
					break;  // CANCEL
				default:
					#ifdef DEBUG
					printf("PICKED: %s\n", fnfc.filename());
					#endif
					fn = fnfc.filename();
					// calculate path and add extension if necessary
					pathFilename = fn.substr(0, fn.find_last_of("/\\")+1);
					baseFilename = fn.substr(fn.find_last_of("/\\") + 1);
					if( baseFilename.find(".") == std::string::npos ) {
						// no dot in filename – needs an extension
						baseFilename.append(defaultExtension);
					}
					fn = pathFilename + baseFilename;
			}
			
			// File exists?
			fileExistsTest.open(fn.c_str());
			if( fileExistsTest.good() ) {
				// File exists!
				choice = myFlChoice("Confirmation", "File already exists. Overwrite it?", {"Yes", "No"});
				if( choice == 1  )			// overwrite == no
					repeatLoop = true;		// ask for another file name
				if( choice == -1 ) {		// ESC => don't save and don't ask again
					fn = "";
					repeatLoop = false;
				}
				fileExistsTest.close();
			}
		} while(repeatLoop);
	} else {
		fn = path;
	}
	
	// a path has been chosen
	if( fn != "" ) {
		int mySaveState = CsvTable::saveReturnCode::SAVE_ERROR;
		windows[winIndex].updateStatusbar("Saving ...");
		if( type == CsvApplication::SaveType::CSV ) {
			showImWorkingWindow("Saving CSV file ...");
			mySaveState = windows[winIndex].table->saveCsv(fn, &CsvWindow::updateStatusbarCB, &windows[winIndex], flaggedOnly);
			windows[winIndex].storeWindowPreferences();
			hideImWorkingWindow();
		} else if( type == CsvApplication::SaveType::JSON ) {
			showImWorkingWindow("Exporting JSON ...");
			mySaveState = windows[winIndex].table->exportJSON(fn, &CsvWindow::updateStatusbarCB, &windows[winIndex]);
			hideImWorkingWindow();
		}
		if( mySaveState == CsvTable::saveReturnCode::SAVE_OKAY ) {
			if( type == CsvApplication::SaveType::CSV && !flaggedOnly ) {
				windows[winIndex].setName(Helper::getBasename(fn));
				windows[winIndex].setPath(fn);
				windows[winIndex].setChanged(false);
				windows[winIndex].setUndoSaveState();
				app.setWorkDir( Helper::getDirectory(fn) );
			}
			windows[winIndex].updateStatusbar("Finished saving.");
			ret = true;
		} else {
			CsvApplication::myFlChoice("", "Could not save file!", {"Okay"});
			windows[winIndex].updateStatusbar("Could not save file.");
		}
	}
	return ret;
}




/**
 *	Closes the window indicated by windowIndex; default is -1 and closes the top-most window
 */
void CsvApplication::closeWindow(int windowIndex, bool forceClose) {
	int choice;
	if( windowIndex == -1 ) {
		windowIndex = app.topWindow;
	}
	if( windows[windowIndex].isUsed() && windows[windowIndex].isChanged() ) {
		choice = myFlChoice("Confirmation", "File has been modified. Save?", {"Yes", "No", "Cancel"});
		switch( choice ) {
			case 0:					// yes: call saveFile()
				if( !saveFile() ) {
					return;
				}
			break;
			case 1:					// no: do nothing and continue to close window
			break;
			case -1:
			case 2:					// cancel
				return;
			break;
			default: 				// ESC: cancel
				return;
		}
	}
	if( nfcIsOpen )
		return;
	if( windows[windowIndex].getWindowSlotUsed() ) {
		// This window has been in use and is showing
		windows[windowIndex].storeWindowPreferences();
		My_Fl_Double_Window *nextWinPtr = (My_Fl_Double_Window *) Fl::next_window(windows[windowIndex].win);
		if( nextWinPtr == NULL && !forceClose ) {
			#ifdef __APPLE__
			remainOpenWin->show();
			#endif
		}
		windows[windowIndex].setPath("");
		windows[windowIndex].setWindowSlotUsed(false);
		windows[windowIndex].createdWindowCount = 0;
		windows[windowIndex].table->clearTable();
		if( nextWinPtr ) {
			nextWinPtr->take_focus();
		}
		windows[windowIndex].win->hide();
		// delete undoList
		windows[windowIndex].clearUndoList();
	}
	
}



void CsvApplication::quitApplication(bool forceClose) {
	searchWin->hide();
	if( forceClose ) {
		#ifdef __APPLE__
		remainOpenWin->hide();
		#endif
	}
	// Store Open Recent…
	for( size_t i = 0; i < TCRUNCHER_PREF_RECENT_FILES_NUM; ++i ) {
		std::string tmp_key = TCRUNCHER_PREF_RECENT_FILES_STUB + std::to_string(i);
		std::string tmp_val = "";
		tmp_val = recentFiles.get(i);
		preferences.set( tmp_key.c_str(), tmp_val.c_str() );
	}
	// Close windows
	for( int i = 0; i < TCRUNCHER_MAX_WINDOWS; ++i ) {
		if( windows[i].getWindowSlotUsed() ) {
			// std::cerr << "Closing window: " << windows[i].getPath() << std::endl;
			closeWindow(i, forceClose);
		}
	}
}


int CsvApplication::getWindowByPointer(Fl_Widget *widget) {
	for( int i = 0; i < TCRUNCHER_MAX_WINDOWS; ++i ) {
		if( windows[i].win == widget ) {
			return i;
		}
	}
	return -1;
}



/**
 *	Sets the new topWindow or the first slot it encounters.
 *	(Hint: topWindow can get -1 if all slots are not used – should not occur)
 */
void CsvApplication::setTopWindow(int topWindow) {
	if( topWindow == -1 ) {
		for( int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot ) {
			if( windows[slot].getWindowSlotUsed() ) {
				topWindow = slot;
				break;
			}
		}
	}
	if( topWindow >= 0) {
		this->topWindow = topWindow;
		setUndoMenuItem( windows[topWindow].hasUndoStates() );
	}
}


int CsvApplication::getTopWindow() {
	return topWindow;
}




/*
 *	Guesses the definition of the given stream, returning some confidence value with it
 */
std::pair<CsvDefinition, float> CsvApplication::guessDefinition(std::istream *input) {
	const int MAXLINES = 10;					// number of lines to read for every test
	float confidence;
	CsvParser *parser = new CsvParser();
	CsvDataStorage localStorage;
	
	// Define definitions for probing
	std::vector< std::tuple<CsvDefinition,int,int> > definitions;
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	definitions.push_back( std::tuple<CsvDefinition, int, int>(CsvDefinition(),0,0) );
	std::get<0>(definitions.at(0)).delimiter = ',';
	std::get<0>(definitions.at(1)).delimiter = ';';
	std::get<0>(definitions.at(2)).delimiter = '\t';
	std::get<0>(definitions.at(3)).delimiter = '|';
	std::get<0>(definitions.at(4)).delimiter = ':';
	std::get<0>(definitions.at(5)).delimiter = ',';
	std::get<0>(definitions.at(6)).delimiter = ';';
	std::get<0>(definitions.at(6)).delimiter = '*';
	std::get<0>(definitions.at(5)).escape = '\\';
	std::get<0>(definitions.at(6)).escape = '\\';
	
	// get statistics for all probing definitions
	for( size_t i = 0; i < definitions.size(); ++i ) {
		std::pair<int, int> statistics;
		// reset stream
		input->clear(); // WHY????
		input->seekg(0);
		// clear localTable
		localStorage.clear();
		parser->parseCsvStream( input, localStorage, &(std::get<0>(definitions.at(i))), MAXLINES, false );
		statistics = tableStatistics(localStorage);
		// not so commonly used seperators and escape characters: decrease statistics value
		if(
			std::get<0>(definitions.at(i)).delimiter == ':' ||
			std::get<0>(definitions.at(i)).delimiter == '|' ||
			std::get<0>(definitions.at(i)).escape == '\\' ||
				std::get<0>(definitions.at(i)).escape == '*'
		) {
			statistics.first = statistics.first * 70 / 100;
		}
		std::get<1>(definitions.at(i)) = statistics.first;
		if( statistics.first <= 1 && statistics.second == 0) {
			// if statistics is (1,0), sort it at the end
			std::get<2>(definitions.at(i)) = 999;
		} else {
			std::get<2>(definitions.at(i)) = statistics.second;
		}
		#ifdef DEBUG
		printf("CSV = '%c' => %d / %d\n", std::get<0>(definitions.at(i)).delimiter, statistics.first, statistics.second);
		#endif
	}
	
	// sort probes: 3rd element INC, 2nd element DESC (C++14)
	std::sort(begin(definitions), end(definitions), [](auto &t1, auto &t2) {
		if( std::get<2>(t1) == std::get<2>(t2) ) {
			return std::get<1>(t1) > std::get<1>(t2);
		}
		return std::get<2>(t1) < std::get<2>(t2);
	});
	
	confidence = 1.0;
	// if there's no definition with zero variance: reduce confidence
	if( std::get<2>(definitions[0]) > 0 ) {
		confidence /= 2;
	}
	// if there are at least two definitions with the same number of columns: reduce confidence
	if( std::get<1>(definitions[0]) == std::get<1>(definitions[1]) ) {
		confidence /= 2;
	}
	// improve confidence, if it's a typical CSV separator
	if( std::get<0>(definitions.at(0)).delimiter == ',' || std::get<0>(definitions.at(0)).delimiter == '\t' ) {
		confidence += (1.0 - confidence) * 0.5;
	}
	
	// clear and reset
	delete(parser);
	input->clear();
	input->seekg(0);
	return {std::get<0>(definitions.at(0)), confidence};
}



/*
 *	Returns {Number of Cols, Some kind of Variance} of the data in localStorage
 *	Variance: number of rows that are shorter than the longest row
 *
 *	@return		std::pair		(Number of Columns, Number of rows that are shorter than longest row)
 */
std::pair<table_index_t, table_index_t> CsvApplication::tableStatistics(CsvDataStorage localStorage) {
	table_index_t maxCols = 0;
	table_index_t shorterRows = 0;

	// table is empty
	if( localStorage.rows() == 0 ) {
		return {0, 0};
	}
	// table has just one row
	if( localStorage.rows() == 1) {
		return { static_cast<table_index_t>(localStorage.rawRow(0).size()), 0 };
	}
	// get maximum columns
	for( table_index_t r = 1; r < localStorage.rows(); ++r ) {
		if( (table_index_t) localStorage.rawRow(r).size() > maxCols ) {
			maxCols = localStorage.rawRow(r).size();
		}
	}
	// count number of shorter rows
	for( table_index_t r = 1; r < localStorage.rows(); ++r ) {
		if( (table_index_t) localStorage.rawRow(r).size() < maxCols ) {
			++shorterRows;
		}
	}
	return {maxCols, shorterRows};
}


/*
 *	Returns guessed encoding and the length of a BOM sequence – or zero if no BOM is present.
 *	TODO remove 'bom' as it's not needed
 */
std::pair<CsvDefinition::Encodings, int> CsvApplication::guessEncoding(std::istream *input, long streamLength) {
	#ifdef DEBUG
	CsvDefinition::BOMs bom = CsvDefinition::BOM_NONE;
	#endif
	CsvDefinition::Encodings enc = CsvDefinition::ENC_NONE;
	int bomBytes = 0;
	unsigned char octet[4];
	
	
	// reset stream
	input->clear();
	input->seekg(0);


	std::istreambuf_iterator<char> it(input->rdbuf());
	std::istreambuf_iterator<char> eos;
	
	
	// read first 4 bytes for BOM
	input->read((char *) &octet, 1);
	input->read((char *) &octet + 1, 1);
	input->read((char *) &octet + 2, 1);
	input->read((char *) &octet + 3, 1);
	
	if( octet[0] == 0xEF && octet[1] == 0xBB && octet[2] == 0xBF ) {
		// UTF8 mit BOM
		#ifdef DEBUG
		bom = CsvDefinition::BOM_UTF8;
		#endif
		bomBytes = 3;
		enc = CsvDefinition::ENC_UTF8;
	} else if( octet[0] == 0x00 && octet[1] == 0x00 && octet[2] == 0xFE && octet[3] == 0xFF ) {
		// UTF32BE
		#ifdef DEBUG
		bom = CsvDefinition::BOM_UTF32BE;
		#endif
		bomBytes = 4;
		enc = CsvDefinition::ENC_UTF32BE;
	} else if( octet[0] == 0xFF && octet[1] == 0xFE && octet[2] == 0x00 && octet[2] == 0x00 ) {
		// UTF32LE
		#ifdef DEBUG
		bom = CsvDefinition::BOM_UTF32LE;
		#endif
		bomBytes = 4;
		enc = CsvDefinition::ENC_UTF32LE;
	} else if( octet[0] == 0xFE && octet[1] == 0xFF ) {
		// UTF16BE
		#ifdef DEBUG
		bom = CsvDefinition::BOM_UTF16BE;
		#endif
		bomBytes = 2;
		enc = CsvDefinition::ENC_UTF16BE;
	} else if( octet[0] == 0xFF && octet[1] == 0xFE ) {
		// UTF16LE
		#ifdef DEBUG
		bom = CsvDefinition::BOM_UTF16LE;
		#endif
		bomBytes = 2;
		enc = CsvDefinition::ENC_UTF16LE;
	}
	
	// Falls kein BOM auf UTF-8 testen (falls Datei nicht zu groß ist)
	if( streamLength < TCRUNCHER_NUM_UTF8_TEST_BYTES && utf8::is_valid(it, eos) ) {
		enc = CsvDefinition::ENC_UTF8;
	}
	
	
	#ifdef DEBUG
	printf("BOM: %d\nENC: %d\n", bom, enc);
	#endif
	
	
	// reset stream
	input->clear();
	input->seekg(0);
	
	return std::make_pair(enc, bomBytes);
}



/*
 *	Asks the user about file specifics.
 *	guessedDefinition			the definition the file most likely is (as returned by guessDefinition())
 */
CsvDefinition CsvApplication::setTypeByUser(CsvDefinition guessedDefinition, std::istream *input, std::string buttonText) {
	Fl_Choice *delChoice;
	Fl_Choice *escChoice;
	Fl_Choice *quoteChoice = nullptr;
	
	My_Fl_Button *but;
	CsvTable *table = nullptr;
	CsvGrid *grid = nullptr;
	std::string output;
	int encDefault = 0;
	int delDefault = 0;
	int quoteDefault = 0;
	int escDefault = 0;
	CsvDefinition definition = guessedDefinition;
	struct previewTableStruct previewTable;
	app.setTypeByUserCancelled = false;
	
	// when no encoding could be guessed, show as UTF8 – otherwise `CsvParser::parseCsvStream` would return a (0,0) table for preview
	if( definition.encoding == CsvDefinition::ENC_NONE ) {
		definition.encoding = CsvDefinition::ENC_UTF8;
	}
	
	// set guessed encoding as start value
	switch( definition.encoding ) {
		case CsvDefinition::ENC_UTF8:
			encDefault = 1;
			break;
		case CsvDefinition::ENC_Latin1:
			encDefault = 2;
			break;
		case CsvDefinition::ENC_Win1252:
			encDefault = 3;
			break;
		case CsvDefinition::ENC_UTF16LE:
			encDefault = 4;
			break;
		case CsvDefinition::ENC_UTF16BE:
			encDefault = 5;
			break;
		default:
			encDefault = 0;
			break;
	}
	
	// set guessed delimiter as start value
	switch( definition.delimiter ) {
		case ',':
			delDefault = 0;
		break;
		case ';':
			delDefault = 1;
		break;
		case '\t':
			delDefault = 2;
		break;
		case '|':
			delDefault = 3;
		break;
		case ':':
			delDefault = 4;
		break;
		case '*':
			delDefault = 5;
		break;
	}
	
	// set guessed escape character as start value
	switch( definition.escape ) {
		case '"':
			escDefault = 0;
		break;
		case '\\':
			escDefault = 1;
		break;
	}

	// set guessed quoting style as start value
	switch( definition.quoteStyle ) {
		case CsvDefinition::QUOTE_STYLE_RFC:
			quoteDefault = 0;
		break;
		case CsvDefinition::QUOTE_STYLE_ALL:
			quoteDefault = 1;
		break;
		case CsvDefinition::QUOTE_STYLE_STRING:
			quoteDefault = 2;
		break;
	}
	
	if( input )
		app.setTypeByUserWin = new My_Fl_Small_Window(600,480);
	else
		app.setTypeByUserWin = new My_Fl_Small_Window(600,190);
	app.setTypeByUserWin->set_modal();
	app.setTypeByUserWin->label("Choose CSV Type");
	app.setTypeByUserWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	app.setTypeByUserWin->callback(setTypeByUser_Done_CB);
	
	app.setTypeByUserWin->begin();
	
	previewTable.input = NULL;
	previewTable.definition = &definition;

	if( input ) {
		table = new CsvTable(0,0);
		grid = new CsvGrid(10, 145, app.setTypeByUserWin->w()-20, app.setTypeByUserWin->h()-155, 0);
		previewTable.input = input;
		previewTable.table = table;
		previewTable.grid = grid;
		grid->allowEvents(false);
		grid->setDataTable(table);
		grid->rows(3);
		grid->cols(3);
		grid->row_header(1);
		grid->row_header_width(80);
		grid->row_resize(1);
		grid->row_height_all(TCRUNCHER_INITIAL_FONT_SIZE + TCRUNCHER_ROW_HEIGHT_ADD);
		grid->col_header(1);
		grid->col_header_height(TCRUNCHER_INITIAL_FONT_SIZE + TCRUNCHER_ROW_HEIGHT_ADD);
		grid->col_resize(1);
		grid->col_width_all(TCRUNCHER_STARTUP_CELL_WIDTH);
		// grid->box(FL_FLAT_BOX);
		app.setTypeByUserWin->resizable(grid);
	}
	app.setTypeByUserWin->end();
	
	app.setTypeByUserWin->begin();
	app.encChoice = new Fl_Choice(110, 30, 160, 25, "Encoding: ");
	app.encChoice->add("Choose ...");						// 0
	app.encChoice->add("UTF-8");							// 1
	app.encChoice->add("Latin 1");							// 2
	app.encChoice->add("Windows 1252");						// 3
	app.encChoice->add("UTF-16LE");							// 4
	app.encChoice->add("UTF-16BE");							// 5
	app.encChoice->value(encDefault);
	app.encChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	app.encChoice->callback(setTypeByUser_Enc_CB, &previewTable);
	
	delChoice = new Fl_Choice(110, 80, 160, 25, "Delimiter: ");
	#ifdef _WIN64
	delChoice->add("Comma ,");							// 0
	delChoice->add("Semicolon ;");						// 1
	delChoice->add("Tab");							    // 2
	delChoice->add("Pipe \\|");							// 3
	delChoice->add("Colon :");							// 4
	delChoice->add("Asterisk *");						// 5
	#else
	delChoice->add(", Comma");							// 0
	delChoice->add("; Semicolon");						// 1
	delChoice->add("⇥ Tab");							// 2
	delChoice->add("╎ Pipe");							// 3
	delChoice->add(": Colon");							// 4
	delChoice->add("* Asterisk");						// 5
	#endif
	delChoice->value(delDefault);
	delChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	delChoice->callback(setTypeByUser_Type_CB, &previewTable);

	if( input ) {
		// Show Escape character chooser only when reading CSV data
		escChoice = new Fl_Choice(350, 30, 160, 25, "Escape: ");
		escChoice->add("\" Quote");							// 0
		escChoice->add("\\\\ Backslash");					// 1
		escChoice->value(escDefault);
		escChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
		escChoice->callback(setTypeByUser_Esc_CB, &previewTable);
	}
	
	if( !input ) {
		// don't show quote style chooser on "open with format ..."
		quoteChoice = new Fl_Choice(350, 30, 160, 25, "Quotes: ");
		quoteChoice->add("Only when needed");					// QUOTE_STYLE_RFC
		quoteChoice->add("On all fields");						// QUOTE_STYLE_ALL
		quoteChoice->add("Around all strings");					// QUOTE_STYLE_STRING
		quoteChoice->value(quoteDefault);
		quoteChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
		quoteChoice->callback(setTypeByUser_Quote_CB, &previewTable);
	}
	
	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");
	but = new My_Fl_Button(350,74,110,30, buttonText.c_str());
	but->set_colors( colorsHighlightButton );
	but->callback(setTypeByUser_Done_CB,1);

	app.setTypeByUserWin->end();

	if( input ) {
		showPreview(previewTable);
	}

	app.setTypeByUserWin->show();
	while( app.setTypeByUserWin->shown() ) {
		Fl::wait();
	}
	app.setTypeByUserWin->hide();
	Fl::check();
	
	if( input ) {
		delete table;
		delete grid;
	} else {
		delete quoteChoice;
	}
	delete but;
	delete delChoice;
	delete app.encChoice;
	delete app.setTypeByUserWin;
	
	if( app.setTypeByUserCancelled ) {
		definition.cancelled = true;
	}
	
	// reset stream
	if( input ) {
		input->clear();
		input->seekg(0, std::ios::beg);
	}
	
	return definition;
}


/*
 *	User clicked "Open"
 */
void CsvApplication::setTypeByUser_Done_CB(Fl_Widget *, long data) {
	int encType = app.encChoice->value();
	
	if( encType == 0 ) {
		myFlChoice("", "You have to choose an encoding!", {"Okay"});
	} else if( data <= 0 ) {
		app.setTypeByUserCancelled = true;
		app.setTypeByUserWin->hide();
	} else {
		app.setTypeByUserCancelled = false;
		app.setTypeByUserWin->hide();
	}
}

void CsvApplication::setTypeByUser_Type_CB(Fl_Widget *widget, void *data) {
	struct previewTableStruct previewTable;
	int type;
	
	previewTable = *(struct previewTableStruct *) data;
	type = ((Fl_Choice *)widget)->value();
	
	switch( type ) {
		case 0:
			previewTable.definition->delimiter= ',';
		break;
		case 1:
			previewTable.definition->delimiter= ';';
		break;
		case 2:
			previewTable.definition->delimiter= '\t';
		break;
		case 3:
			previewTable.definition->delimiter= '|';
		break;
		case 4:
			previewTable.definition->delimiter= ':';
		break;
		case 5:
			previewTable.definition->delimiter= '*';
		break;
	}
	if( previewTable.input )
		showPreview(previewTable);
}


void CsvApplication::setTypeByUser_Esc_CB(Fl_Widget *widget, void *data) {
	struct previewTableStruct previewTable;
	int type;
	
	previewTable = *(struct previewTableStruct *) data;
	type = ((Fl_Choice *)widget)->value();
	
	switch( type ) {
		case 0:
			previewTable.definition->escape= '"';
		break;
		case 1:
			previewTable.definition->escape= '\\';
		break;
	}
	if( previewTable.input )
		showPreview(previewTable);
}

void CsvApplication::setTypeByUser_Enc_CB(Fl_Widget *widget, void *data) {
	struct previewTableStruct previewTable;
	int type;
	
	previewTable = *(struct previewTableStruct *) data;
	type = ((Fl_Choice *)widget)->value();
	
	switch( type ) {
		case 0:
		break;
		case 1:
			previewTable.definition->encoding = CsvDefinition::ENC_UTF8;
		break;
		case 2:
			previewTable.definition->encoding = CsvDefinition::ENC_Latin1;
		break;
		case 3:
			previewTable.definition->encoding = CsvDefinition::ENC_Win1252;
		break;
		case 4:
			previewTable.definition->encoding = CsvDefinition::ENC_UTF16LE;
		break;
		case 5:
			previewTable.definition->encoding = CsvDefinition::ENC_UTF16BE;
		break;
	}

	if( previewTable.input )
		showPreview(previewTable);
}

void CsvApplication::setTypeByUser_Quote_CB(Fl_Widget *widget, void *data) {
	struct previewTableStruct previewTable;
	int quoteStyle;
	
	previewTable = *(struct previewTableStruct *) data;
	quoteStyle = ((Fl_Choice *)widget)->value();
	
	switch( quoteStyle ) {
		case CsvDefinition::QUOTE_STYLE_RFC:
			previewTable.definition->quoteStyle= CsvDefinition::QUOTE_STYLE_RFC;
		break;
		case CsvDefinition::QUOTE_STYLE_ALL:
			previewTable.definition->quoteStyle= CsvDefinition::QUOTE_STYLE_ALL;
		break;
		case CsvDefinition::QUOTE_STYLE_STRING:
			previewTable.definition->quoteStyle= CsvDefinition::QUOTE_STYLE_STRING;
		break;
	}
}


void CsvApplication::showPreview(struct previewTableStruct previewTable) {
	CsvParser *parser = new CsvParser();
	
	// parse 'input' with chosen 'definition'
	if( previewTable.input ) {
		previewTable.input->clear();
		previewTable.input->seekg(0, std::ios::beg);
	}
	previewTable.table->clearTable();
	parser->parseCsvStream(previewTable.input, previewTable.table->getStorage(), previewTable.definition, TCRUNCHER_MAX_PREVIEW_ROWS);
	// previewTable.table->getStorage().dump(10,true);
	previewTable.table->updateInternals();
	previewTable.grid->rows( previewTable.table->getNumberRows() );
	previewTable.grid->cols( previewTable.table->getNumberCols() );
	
	delete parser;
}


void CsvApplication::setFocusByWinIndex(int winIndex) {
	Fl::check();
	for( int i = 0; i < TCRUNCHER_MAX_WINDOWS; ++i ) {
		if( windows[i].getWindowSlotUsed() && windows[i].windowMenuIndex == winIndex ) {
			windows[i].win->show();
			windows[i].win->take_focus();
			break;
		}
	}
}

void CsvApplication::copySelection() {
	table_index_t row_top, row_bottom, col_top, col_bottom;
	std::string copy = "";
	std::vector<std::string> vec;
	std::vector<std::vector<std::string>> block;

	windows[topWindow].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	if( row_top == row_bottom && col_top == col_bottom ) {
		// copy a single cell
		copy = windows[topWindow].table->getCell(row_top, col_top);
		Fl::copy( copy.c_str(), strlen(copy.c_str()), 1 );
	} else {
		// copy multiple cells
		block = windows[topWindow].table->copyBlock(row_top, col_top, row_bottom, col_bottom);
	
		for( table_index_t i = 0; i < (table_index_t) block.size(); ++i ) {
			vec = block.at(i);
			copy.append( windows[topWindow].table->encodeCsvLine( &vec, windows[topWindow].table->getDefinition() ) );
		}
		Fl::copy( copy.c_str(), strlen(copy.c_str()), 1 );
	}
}


void CsvApplication::paste(bool askUser, bool fillSelection) {
	std::vector<std::vector<std::string>> block;
	CsvDataStorage localStorage;
	int block_rows, block_cols;
	int row_top, row_bottom, col_top, col_bottom;
	
	windows[topWindow].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	CsvDefinition activeDefinition = windows[topWindow].table->getDefinition();			// the definition of the table in the active window

	std::pair<CsvDefinition, float> guessedDefinition;
	CsvDefinition definition;
	CsvParser parser;
	
	Fl::paste(*windows[topWindow].win, 1, Fl::clipboard_plain_text);
	std::string pasted(Fl::event_text(), Fl::event_length());
	
	//
	// "Paste into Selection": Pastes content into all selected cells and returns
	if( fillSelection ) {
		windows[topWindow].addUndoStateTable("Fill");
		showImWorkingWindow("Pasting data ...");
		for( int r = row_top; r <= row_bottom; ++r ) {
			for( int c = col_top; c <= col_bottom; ++c ) {
				windows[topWindow].table->setCell(pasted, r,c);
			}
		}
		hideImWorkingWindow();
		windows[topWindow].updateTable();
		windows[topWindow].setChanged(true);
		windows[topWindow].setUsed(true);
		return;
	}
	
	std::istringstream input(pasted);
	std::cout << std::endl;
	
	
	//
	// if pasted string doesn't contain active delimiter and 
	// askUser is not set
	if( !askUser && pasted.find(activeDefinition.delimiter) == std::string::npos ) {
		windows[topWindow].addUndoStateCell(windows[topWindow].table->getCell(row_top,col_top), row_top, col_top, "Paste cell");
		windows[topWindow].table->setCell(pasted,row_top,col_top);
		windows[topWindow].setChanged(true);
		windows[topWindow].setUsed(true);
		windows[topWindow].grid->redraw();
		Fl::check();
		return;
	}

	// guess properties
	guessedDefinition = app.guessDefinition(&input);
	definition = guessedDefinition.first;
	definition.encoding = CsvDefinition::ENC_UTF8;
	
	
	// ask user if confidence is lower than 1.0, if the user should be asked or if the guessed delimiter is none of the top 3 delimiters and not the one used in the underlying table
	#ifdef DEBUG
	printf("confidence: %.2f\n", guessedDefinition.second);
	#endif
	if(
		guessedDefinition.second < 1.0 ||
		askUser ||
		(
			definition.delimiter != ',' &&
			definition.delimiter != ';' &&
			definition.delimiter != '\t' &&
			definition.delimiter != (windows[topWindow].table->getDefinition()).delimiter
		)
	) {
		definition = CsvApplication::setTypeByUser(definition, &input, "Paste");
		if( definition.cancelled ) {
			return;
		}
	}
	
	showImWorkingWindow("Pasting data ...");
	windows[topWindow].addUndoStateTable("Paste");

	parser.parseCsvStream(&input, localStorage, &definition);

#ifdef DEBUG
printf("Pasting ...\n");
localStorage.dump();
#endif

	// TODO compare block with selection: warn if sizes do not match (only if selection is larger than 1x1)

	block_rows = localStorage.rows();
	if( block_rows ) {
		block_cols = localStorage.columns();
		if( block_cols ) {
			windows[topWindow].table->resizeTable(row_top + block_rows, col_top + block_cols);
			for( int i = 0; i < block_rows; ++i ) {
				for( int j = 0; j < block_cols; ++j ) {
					windows[topWindow].table->setCell(localStorage.get(i,j), row_top + i, col_top + j );
					// windows[topWindow].grid->redraw();
					// Fl::check();
				}
			}
			windows[topWindow].updateTable();
			windows[topWindow].setChanged(true);
			windows[topWindow].setUsed(true);
		}
	}
	
	hideImWorkingWindow();
}




bool CsvApplication::isAlreadyOpened(std::string path) {
	for(int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot) {
		if( windows[slot].getWindowSlotUsed() && windows[slot].getPath() == path )
			return true;
	}
	return false;
}


/*
 *	Show the Find & Replace Window
 */
void CsvApplication::find() {
	int winIndex = app.getTopWindow();
	Fl_Window *topWin = Fl::first_window();
	searchWin->callback(find_substring_CB);
	std::string scopeMsg = "Scope: " + windows[winIndex].humanReadableSelection();

	searchWinScope->copy_label(scopeMsg.c_str());
	searchWin->show();
	while( searchWin->shown() ) {
		Fl::wait();
	}
	searchWinScope->copy_label("");
	searchWinLabel->copy_label("");
	searchWin->hide();
	app.lastFound = {-1,-1};
	windows[winIndex].grid->redraw();
	
	Fl::check();
	// restore active window
	if( topWin ) {
		Fl::first_window(topWin);
		topWin->show();
		topWin->take_focus();
	}
}
void CsvApplication::updateSearchWindow() {
	int winIndex = app.getTopWindow();
	std::string scopeMsg = "Scope: " + windows[winIndex].humanReadableSelection();
	searchWinScope->copy_label(scopeMsg.c_str());
	searchWinScope->redraw();
	// Fl::check();			// sometimes causes Seg Fault
}

void CsvApplication::find_substring_CB(Fl_Widget *, long data) {
	int keyCode = (int) data;
	if( keyCode <= 0 ) {
		// ESC, Red Close Button == 0 / ENTER, Find Next Button == TCRUNCHER_MYFLCHOICE_MAGICAL
		app.searchWin->hide();
		return;
	}
	int row, col;
	int startFindRow, startFindCol;
	std::tuple<int, int> found;
	std::string query(app.searchInput->value());
	int winIndex = app.getTopWindow();
	bool caseSensitive = true;
	bool useRegex = false;
	std::vector<int> sel = windows[winIndex].grid->getSelection();
	if( app.ignoreCase->value() ) {
		caseSensitive = false;
	}
	if( app.useRegex->value() ) {
		useRegex = true;
	}
	app.searchWinLabel->copy_label("Start searching ...");
	app.searchWinLabel->redraw();
	Fl::check();
	if( std::get<0>(app.lastFound) >= 0 && std::get<1>(app.lastFound) >= 0 ) {
		startFindRow = std::get<0>(app.lastFound);
		startFindCol = std::get<1>(app.lastFound);
	} else {
		startFindRow = sel[0];
		startFindCol = sel[1];
	}
	if( sel[2] == sel[0] && sel[3] == sel[1] ) {
		sel = {0, 0, windows[winIndex].table->getNumberRows()-1, windows[winIndex].table->getNumberCols()-1};
	}
	app.showImWorkingWindow("Searching ...");
	found = windows[winIndex].table->findSubstring(query, startFindRow, startFindCol, sel, caseSensitive, useRegex);
	app.hideImWorkingWindow();
	row = std::get<0>(found);
	col = std::get<1>(found);
	if( row != -1 && col != -1 ) {
		app.lastFound = found;
		windows[winIndex].grid->setVisibleArea(row, col);
		windows[winIndex].grid->redraw();
		windows[winIndex].showDefaultStatus();
		app.searchWinLabel->copy_label("");
	} else {
		app.lastFound = {-1,-1};
		windows[winIndex].grid->redraw();
		app.searchWinLabel->copy_label("No match found.");
	}
	app.searchWin->redraw();
	Fl::check();
}

/*
 *	Does Replacement, is called by callback functions below
 *	Returns number of replacements
 */
int CsvApplication::find_replace(bool callFindNext) {
	int row_top, row_bottom, col_top, col_bottom;
	int startFindRow, startFindCol;
	int numReplaces;
	std::string query(app.searchInput->value());
	std::string replace(app.replaceInput->value());
	int winIndex = app.getTopWindow();
	bool caseSensitive = true;
	bool useRegex = false;
	if( app.ignoreCase->value() ) {
		caseSensitive = false;
	}
	if( app.useRegex->value() ) {
		useRegex = true;
	}
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	if( std::get<0>(app.lastFound) >= 0 && std::get<1>(app.lastFound) >= 0 ) {
		startFindRow = std::get<0>(app.lastFound);
		startFindCol = std::get<1>(app.lastFound);
	} else {
		startFindRow = row_top;
		startFindCol = col_top;
	}
	app.showImWorkingWindow("Replacing ...");
	windows[winIndex].addUndoStateTable("Replace");
	windows[winIndex].setChanged(true);
	numReplaces = windows[winIndex].table->replaceInCurrentCell(startFindRow, startFindCol, query, replace, caseSensitive, useRegex);
	app.hideImWorkingWindow();
	if( numReplaces ) {
		if( callFindNext ) {
			find_substring_CB(NULL, TCRUNCHER_MYFLCHOICE_MAGICAL);
		}
		app.searchWinLabel->copy_label("Replaced.");
		windows[winIndex].grid->redraw();
	} else {
		app.searchWinLabel->copy_label("No match found.");
	}
	app.searchWinLabel->redraw();
	Fl::check();
	return numReplaces;
}

void CsvApplication::find_replace_CB(Fl_Widget *, void *) {
	find_replace(false);
}

void CsvApplication::find_replaceFind_CB(Fl_Widget *, void *) {
	find_replace(true);
}

/*
 *	Replaces all occurences of "Find" with "Replace" if data == CsvApplication::ReplaceAllType::REPLACE
 *	Flags all rows where "Find" is found if data == CsvApplication::ReplaceAllType::FLAG
 */
void CsvApplication::find_replaceAll_CB(Fl_Widget *, long data) {
	int row_top, row_bottom, col_top, col_bottom;
	int numReplaces = 0;
	int allReplaces = 0;
	int allChangedCells = 0;
	int row, col;
	std::string msg;
	std::string query(app.searchInput->value());
	std::string replace(app.replaceInput->value());
	int winIndex = app.getTopWindow();
	long long allCellCounter = 0, allCells = 0;
	bool caseSensitive = true;
	bool useRegex = false;
	if( app.ignoreCase->value() ) {
		caseSensitive = false;
	}
	if( app.useRegex->value() ) {
		useRegex = true;
	}

	if( data == CsvApplication::ReplaceAllType::REPLACE ) {
		app.showImWorkingWindow("Replacing ...");
		app.searchWinLabel->copy_label("Start replacing ...");
		windows[winIndex].addUndoStateTable("Replace All");
		windows[winIndex].setChanged(true);
	} else if( data == CsvApplication::ReplaceAllType::FLAG ) {
		app.showImWorkingWindow("Flagging ...");
		app.searchWinLabel->copy_label("Start flagging ...");
	} else if( data == CsvApplication::ReplaceAllType::UNFLAG ) {
		app.showImWorkingWindow("Unflagging ...");
		app.searchWinLabel->copy_label("Start unflagging ...");
	}

	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	if( row_top == row_bottom && col_top == col_bottom ) {
		row_top = 0;
		col_top = 0;
		row_bottom = windows[winIndex].table->getNumberRows() - 1;
		col_bottom = windows[winIndex].table->getNumberCols() - 1;
	}

	allCellCounter = 0;
	allCells = (col_bottom - col_top + 1) * (row_bottom - row_top + 1);
	if( allCells > 0 ) {
		for( col = col_top; col <= col_bottom; ++col) {
			for( row = row_top; row <= row_bottom; ++row) {
				if( data == CsvApplication::ReplaceAllType::REPLACE ) {
					// REPLACE
					numReplaces = windows[winIndex].table->replaceInCurrentCell(row, col, query, replace, caseSensitive, useRegex);
					if( numReplaces > 0 ) {
						allReplaces += numReplaces;
						++allChangedCells;
					}
				} else if( data == CsvApplication::ReplaceAllType::FLAG ) {
					// FLAG ROW
					if( windows[winIndex].table->findInCell(query, row, col, caseSensitive, useRegex) ) {
						if( !windows[winIndex].table->isFlagged(row) ) {
							windows[winIndex].table->flagRow(row, true);
							++allChangedCells;
						}
					}
				} else if( data == CsvApplication::ReplaceAllType::UNFLAG ) {
					// UNFLAG ROW
					if( windows[winIndex].table->findInCell(query, row, col, caseSensitive, useRegex) ) {
						if( windows[winIndex].table->isFlagged(row) ) {
							windows[winIndex].table->flagRow(row, false);
							++allChangedCells;
						}
					}
				}
				++allCellCounter;
				if( allCellCounter % 5000 == 0 ) {
					int processedPercent = allCellCounter * 100 / allCells;
					std::string msg = "Processed "+std::to_string(processedPercent)+"%";
					app.searchWinLabel->copy_label(msg.c_str());
					Fl::check();
				}
			}
		}
	}
	app.hideImWorkingWindow();

	if( allChangedCells > 0 ) {
		if( data == CsvApplication::ReplaceAllType::REPLACE ) {
			msg = "Replaced " + std::to_string(allReplaces) + " occurrence(s) in " + std::to_string(allChangedCells) + " cell(s).";
		} else if( data == CsvApplication::ReplaceAllType::FLAG ) {
			msg = "Flagged " + std::to_string(allChangedCells) + " new row(s).";
		} else if( data == CsvApplication::ReplaceAllType::UNFLAG ) {
			msg = "Unflagged " + std::to_string(allChangedCells) + " row(s).";
		}
		app.searchWinLabel->copy_label(msg.c_str());
		windows[winIndex].grid->redraw();		
	} else {
		app.searchWinLabel->copy_label("No match found.");
	}
	app.searchWinLabel->redraw();
	Fl::check();
}


/*
 * Sort table by column
 */
void CsvApplication::sort(unsigned int column) {
	Fl_Choice *colChoice;
	Fl_Choice *orderChoice;
	Fl_Choice *typeChoice;
	My_Fl_Button *sortButton;
	int windowIndex = getTopWindow();
	std::tuple<Fl_Widget *, Fl_Widget *, Fl_Widget *> widgets;
	int searchType = 1;
	if( windows[windowIndex].table->isNumericColumn(column, 1000000) ) {
		searchType = 0;
	}
	
	Fl_Window *topWin = Fl::first_window();
	
	// sortWin = new My_Fl_Small_Window(640,180);
	// sortWin->set_modal();
	sortWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	sortWin->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	sortWin->label("Sort By");
	sortWin->callback(doSortWinCB);
	
	sortWin->begin();
	colChoice = new Fl_Choice(90, 30, 120, 25, "Column: ");
	if( column >= windows[windowIndex].table->headerRow->size() )
		column = 0;
	for( size_t c = 0; c < windows[windowIndex].table->headerRow->size(); ++c ) {
		colChoice->add(windows[windowIndex].table->getHeaderCell(c,false).c_str());
	}
	colChoice->value(column);
	colChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));

	orderChoice = new Fl_Choice(280, 30, 120, 25, "Order: ");
	orderChoice->add("Ascending");
	orderChoice->add("Descending");
	orderChoice->value(0);
	orderChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	
	typeChoice = new Fl_Choice(470, 30, 120, 25, "Type: ");
	typeChoice->add("Numeric");
	typeChoice->add("String");
	typeChoice->add("String (ignore case)");
	typeChoice->value(searchType);
	typeChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));

	std::get<0>(widgets) = colChoice;
	std::get<1>(widgets) = orderChoice;
	std::get<2>(widgets) = typeChoice;

	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");
	sortButton = new My_Fl_Button(90,100,100,24, "Sort");
	sortButton->colors = colorsHighlightButton;
	sortButton->callback(doSortCB, &widgets);

	sortWin->end();

	// store active window
	topWin = Fl::first_window();

	sortWin->show();
	while( sortWin->shown() ) {
		Fl::wait();
	}
	
	// do sort
	if( sortWin->dataExchange == 0 ) {
		windows[windowIndex].setChanged(true);
		windows[windowIndex].setUsed(true);
		showImWorkingWindow("Sorting ...");
		windows[windowIndex].addUndoStateTable("Sort Table");
		windows[windowIndex].table->sortTable(
			colChoice->value(),
			orderChoice->value() == 0 ? true : false,
			typeChoice->value()
		);
		hideImWorkingWindow();
		windows[windowIndex].grid->redraw();
		Fl::check();
	}
	sortWin->hide();
	
	delete colChoice;
	delete orderChoice;
	delete typeChoice;
	delete sortButton;
	// restore active window
	if( topWin ) {
		Fl::first_window(topWin);
		topWin->show();
		topWin->take_focus();
	}
}
void CsvApplication::doSortCB(Fl_Widget *, void *) {
	app.sortWin->dataExchange = 0;
	app.sortWin->hide();
}
void CsvApplication::doSortWinCB(Fl_Widget *, long data) {
	if( data == TCRUNCHER_MYFLCHOICE_MAGICAL) {
		// pressed ENTER
		app.sortWin->dataExchange = 0;
		app.sortWin->hide();
	}
	if( data <= 0 ) {
		// ESC or red close button
		app.sortWin->dataExchange = -1;
		app.sortWin->hide();
	}
}



/**
 *	Flag all selected rows
 */
void CsvApplication::flagSelectedRowsCB() {
	int topWinIndex = app.getTopWindow();
	std::vector<int> selection = windows[topWinIndex].grid->getSelection();
	showImWorkingWindow("Flagging rows ...");
	for( int r = selection[0]; r <= selection[2]; ++r ) {
		windows[topWinIndex].table->flagRow(r, true);
	}
	hideImWorkingWindow();
	windows[topWinIndex].grid->redraw();
	Fl::check();
}



/**
 *	Unflag all flagged rows
 */
void CsvApplication::unflagRowsCB() {
	std::string msg;
	int choice;
	int topWinIndex = app.getTopWindow();
	int countFlaggedRows = windows[topWinIndex].table->countFlaggedRows();
	if( countFlaggedRows == 0 ) {
		myFlChoice("Info", "There are no flagged rows.", {"Okay"});
		return;
	}
	msg = "Really unflag " + std::to_string(countFlaggedRows) + " row(s)?";
	choice = myFlChoice("Confirm", msg, {"Yes", "No"});
	if( choice == 1 || choice == -1 ) {
		return;
	}
	showImWorkingWindow("Unflagging rows ...");
	windows[topWinIndex].table->clearFlags();
	hideImWorkingWindow();
	windows[topWinIndex].grid->redraw();
	Fl::check();
}



/**
 	Inverts the current flagged rows

 */
void CsvApplication::invertFlaggedCB() {
	showImWorkingWindow("Inverting flagged rows ...");
	windows[app.getTopWindow()].table->invertFlags();
	hideImWorkingWindow();
	windows[app.getTopWindow()].grid->redraw();
	Fl::check();
}


/**
 	Deletes all rows that have beend flagged
	Flag 'deleteFlagged = false' doesn't work right now!!

 */
void CsvApplication::deleteFlaggedCB(bool deleteFlagged) {
	std::string msg;
	int choice;
	int countFlaggedRows = windows[app.getTopWindow()].table->countFlaggedRows();
	if( countFlaggedRows == 0 ) {
		myFlChoice("Info", "There are no flagged rows.", {"Okay"});
		return;
	}
	if( deleteFlagged ) {
		msg = "Really delete " + std::to_string(countFlaggedRows) + " flagged row(s)?";
	} else {
		msg = "Really delete " + std::to_string(windows[app.getTopWindow()].table->getNumberRows() - countFlaggedRows) + " unflagged row(s)?";
	}
	choice = myFlChoice("Confirm", msg, {"Yes", "No"});
	if( choice == 1 || choice == -1 ) {
		return;
	}
	windows[app.getTopWindow()].setChanged(true);
	windows[app.getTopWindow()].setUsed(true);
	showImWorkingWindow("Deleting flagged rows ...");
	if( deleteFlagged ) {
		windows[app.getTopWindow()].addUndoStateTable("Delete Flagged Rows");
	} else {
		windows[app.getTopWindow()].addUndoStateTable("Delete Unflagged Rows");
	}
	windows[app.getTopWindow()].table->deleteFlaggedRows(deleteFlagged);
	hideImWorkingWindow();
	windows[app.getTopWindow()].grid->rows( windows[app.getTopWindow()].table->getNumberRows() );
	windows[app.getTopWindow()].grid->redraw();
	Fl::check();
}




void CsvApplication::executeMacro() {
	if( macroWinOpened )			// only open one macro window
		return;
	macroWinOpened = true;
	std::tuple<Fl_Text_Buffer *, Fl_Text_Display *, Fl_Text_Display *> widgets;		// tuple of widgets that are passed to callbacks
	std::string logContent = "";
	const int winWidth = 700;														// height of macro window
	const int winHeight = 450;														// width of macro window
	const int listWidth = 150;														// width of macro list
	const int gridMargin = 12;														// size of margins
	const int logHeight = 120;														// height of the log window
	const int buttonHeight = 24;													// height of the Execute button
	const int editorHeight = winHeight-logHeight-4*gridMargin-buttonHeight;			// height of the source editor
	// const int imgButtonSize = 15;													// size of the image buttons to manage macro list
	const int imgButtonDistance = 8;												// horizontal distance of image buttons to macro list
	std::string workDir = "";
	int selectedItem = 0;

	My_Fl_Small_Window *macroWin = new My_Fl_Small_Window(winWidth,winHeight);
	Fl_Text_Buffer *sourceBuffer = new Fl_Text_Buffer();
	Fl_Text_Editor *sourceEditor = new Fl_Text_Editor(listWidth+2*gridMargin, gridMargin, winWidth-listWidth-3*gridMargin, editorHeight);
	My_Fl_Button *executeButton = new My_Fl_Button(winWidth-gridMargin-100, editorHeight+2*gridMargin, 100, buttonHeight, "Execute");
	My_Fl_Button *insertLoopButton = new My_Fl_Button(listWidth+2*gridMargin, editorHeight+gridMargin+imgButtonDistance, 80, buttonHeight-8, "Insert Loop");
	Fl_Hold_Browser *macroList = new Fl_Hold_Browser(gridMargin, gridMargin, listWidth, editorHeight);
	Fl_Text_Buffer *logBuffer = new Fl_Text_Buffer();
	Fl_Text_Display *logDisplay = new Fl_Text_Display(gridMargin, editorHeight+3*gridMargin+buttonHeight, winWidth-2*gridMargin, logHeight);
	
	
	// Toolbar: add, delete
	addDelParamType addDelParam;
	std::get<0>(addDelParam) = macroList;
	std::get<1>(addDelParam) = sourceEditor;
	macroWin->begin();
	My_Toolbar *macroTb = new My_Toolbar(gridMargin, editorHeight+gridMargin+imgButtonDistance, listWidth, macroWinImgButtonSize);
	// #ifdef __APPLE__
	macroTb->AddButton("Add Macro", plusPng, &addNewMacroCB, &addDelParam, macroWinImgButtonSize);
	macroTb->AddButton("Delete Macro", minusPng, &deleteMacroCB, &addDelParam, macroWinImgButtonSize);
	// #else
	// Workaround for Windows: doesn't show PNG icons
	// macroTb->AddButton("Add Macro", NULL, &addNewMacroCB, &addDelParam, macroWinImgButtonSize, "+", 20);
	// macroTb->AddButton("Delete Macro", NULL, &deleteMacroCB, &addDelParam, macroWinImgButtonSize, "-", 24);
	// #endif
	macroTb->end();
	macroWin->end();

	// struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	// colorsHighlightButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
	// colorsHighlightButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
	// colorsHighlightButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
	// colorsHighlightButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	// colorsHighlightButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");
	struct My_Fl_Button::buttonColorStruct colorsButton;
	colorsButton.background = ColorThemes::getColor(app.getTheme(), "button_bg");
	colorsButton.label = ColorThemes::getColor(app.getTheme(), "button_text");
	colorsButton.border = ColorThemes::getColor(app.getTheme(), "button_border");
	colorsButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsButton.borderWidth = ColorThemes::getColor(app.getTheme(), "button_border_width");

	// macroWin: modal window
	macroWin->set_modal();
	macroWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	macroWin->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	macroWin->label("Macro");
	if( lastMacroWinX >= 0 && lastMacroWinY >= 0 ) {
		macroWin->position(lastMacroWinX,lastMacroWinY);
	}
	
	// sourceEditor: Macro source editor
	sourceEditor->buffer(sourceBuffer);
	sourceEditor->box(FL_FLAT_BOX);
	sourceEditor->textsize(15);
	sourceEditor->textfont(FL_COURIER);
	macroWin->resizable(sourceEditor);
	if( macros.entries() == 0 ) {
		sourceEditor->hide();
	}
	
	// executeButton
	// executeButton->colors = colorsHighlightButton;
	executeButton->colors = colorsButton;
	executeButton->shortcut(FL_Enter);

	// insertLoopButton
	insertLoopButton->colors = colorsButton;
	insertLoopButton->labelsize(11);
	
	// macroList: list of stored macros
	std::tuple<Fl_Text_Buffer *, Fl_Text_Editor *> executeMacroListData;
	std::get<0>(executeMacroListData) = sourceBuffer;
	std::get<1>(executeMacroListData) = sourceEditor;
	macroList->box(FL_FLAT_BOX);
	macroList->callback(executeMacroListCB, &executeMacroListData);
	if( macros.entries() > 0 ) {
		for(int i=0; i < macros.entries(); ++i) {
			macroList->add(macros.entry(i));
		}
	}
	selectedItem = selectMacroBrowserEntry(macroList, lastChoosenMacroName);
	if( !selectedItem && macroList->size() ) {
		selectedItem = 1;
		macroList->select(selectedItem);
		lastChoosenMacroName = macroList->text(selectedItem);
	}
	sourceBuffer->text( getPreference(&macros, lastChoosenMacroName, "").c_str() );
	sourceBuffer->tab_distance(getEditorTabDistance());
	lastSelectedMacro = selectedItem;
	// sourceBuffer->text( getPreference(&macros, macroList->text(selectedItem), "").c_str() );
	
	
	// logDisplay: logs macro output
	logDisplay->buffer(logBuffer);
	logDisplay->box(FL_FLAT_BOX);
	
	// set callbacks
	std::get<0>(widgets) = sourceBuffer;
	std::get<1>(widgets) = logDisplay;
	std::get<2>(widgets) = sourceEditor;
	executeButton->callback(executeMacroCB, &widgets);
	insertLoopButton->callback(insertLoopButtonCB, &widgets);

	macroWin->show();
	while( macroWin->shown() ) {
		Fl::wait();
	}
	
	// TODO code is duplicated in executeMacroListCB()
	char *macroSource = sourceBuffer->text();
	if( macroSource ) {
		const char *macroName = macroList->text(app.lastSelectedMacro);
		if( macroName ) {
			macros.set( macroName, macroSource );
		}
		delete macroSource;		// Fl_Text_Buffer::text() returns a pointer that has to be freed by the application
	}
	
	
	lastMacroWinX = macroWin->x();
	lastMacroWinY = macroWin->y();
	macroWinOpened = false;
	
	delete macroTb;
	delete macroList;
	delete insertLoopButton;
	delete executeButton;
	delete sourceEditor;
	delete sourceBuffer;
	delete macroWin;
}


void CsvApplication::addNewMacroCB(Fl_Widget *, void *data) {
	addDelParamType *addDelParam = (addDelParamType *)data;
	std::string macroName;
	int retType;
	std::tie(retType, macroName) = myFlAskString("Enter Macro Name", "OK");
	Fl_Browser *macroList = std::get<0>(*addDelParam);
	Fl_Text_Editor *sourceEditor = std::get<1>(*addDelParam);
	std::string newSource = "// "+macroName+"\n";

	if( retType ) {
		if( macroName != "") {
			if( !macros.entryExists(macroName.c_str()) ) {
				app.lastChoosenMacroName = macroName;
				macros.set( macroName.c_str(), newSource.c_str() );
				updateMacroBrowserList(macroList);
				int insertedIndex = selectMacroBrowserEntry(macroList, macroName);
				if( insertedIndex > 0 && insertedIndex <= macroList->size() ) {
					app.lastSelectedMacro = insertedIndex;
					Fl_Text_Buffer *sourceBuffer = sourceEditor->buffer();
					if( sourceBuffer ) {
						sourceBuffer->text( app.getPreference(&macros, macroName, "").c_str() );
					}
				}
				sourceEditor->show();
			} else {
				myFlChoice("", "This macro name is already in use. Please provide a new macro name.", {"Okay"});
			}
		} else {
			myFlChoice("", "You have to enter a macro name.", {"Okay"});
		}
	}
}

void CsvApplication::deleteMacroCB(Fl_Widget *, void *data) {
	addDelParamType *addDelParam = (addDelParamType *)data;
	std::string macroToBeDeleted;
	Fl_Browser *macroList = std::get<0>(*addDelParam);
	Fl_Text_Editor *sourceEditor = std::get<1>(*addDelParam);
	std::string confirmationStr;
	int deletedListIndex = 0;
	for(int i=1; i <= macroList->size(); ++i) {
		if( macroList->selected(i) ) {
			macroToBeDeleted = macroList->text(i);
			deletedListIndex = i;
			break;
		}
	}
	confirmationStr = "Really delete macro '" + macroToBeDeleted + "'?";
	int choice = myFlChoice("Confirmation", confirmationStr, {"Yes", "No"});
	if( choice == 0 ) {
		if( macros.entryExists(macroToBeDeleted.c_str()) ) {
			macros.deleteEntry(macroToBeDeleted.c_str());
		}
		if( macroList->size() > 0 ) {
			updateMacroBrowserList(macroList, deletedListIndex);
			if( deletedListIndex < 1 || deletedListIndex > macroList->size() ) {
				deletedListIndex = macroList->size();
			}
			if( macroList->size() > 0 ) {
				std::string newSelectedMacroName = macroList->text(deletedListIndex);
				selectMacroBrowserEntry(macroList, newSelectedMacroName);
				app.lastSelectedMacro = deletedListIndex;
				app.lastChoosenMacroName = newSelectedMacroName;
				Fl_Text_Buffer *buffer = sourceEditor->buffer();
				if( buffer ) {
					std::string source = app.getPreference(&macros, newSelectedMacroName, "");
					buffer->text( source.c_str() );
				}
			} else {
				sourceEditor->hide();				
			}
		} else {
			sourceEditor->hide();
		}
	}
}


void CsvApplication::updateMacroBrowserList(Fl_Browser *macroList, int selected) {
	macroList->clear();
	for(int i=0; i < macros.entries(); ++i) {
		macroList->add(macros.entry(i));
	}
	if( selected > 0 ) {
		if( selected > macroList->size() ) {
			selected = macroList->size();
		}
		macroList->select(selected);
	}
}

// Returns Fl_Browser index of entryName or 0 – Fl_Browser is 1-based!
int CsvApplication::selectMacroBrowserEntry(Fl_Browser *macroList, std::string ) {
	int ret = 0;
	for( int t=1; t<=macroList->size(); t++ ) {
		if( app.lastChoosenMacroName == macroList->text(t) ) {
			macroList->select(t);
			ret = t;
		}
	}
	return ret;
}

// executeMacroListCB: click into the Macro list on the left
void CsvApplication::executeMacroListCB(Fl_Widget *widget, void *data) {
	Fl_Hold_Browser *macroList = (Fl_Hold_Browser *)widget;
	std::tuple<Fl_Text_Buffer *, Fl_Text_Editor *> *widgets = (std::tuple<Fl_Text_Buffer *, Fl_Text_Editor *> *)data;
	Fl_Text_Buffer *sourceBuffer = std::get<0>(*widgets);
	int clickedItem = macroList->value();
	if( clickedItem > 0 && clickedItem <= macroList->size() ) {
		// clicked onto an item
		char *macroSource = sourceBuffer->text();
		if( macroSource ) {
			const char *macroName = macroList->text(app.lastSelectedMacro);
			if( macroName ) {
				macros.set( macroName, macroSource );
			}
			delete macroSource;		// Fl_Text_Buffer::text() returns a pointer that has to be freed by the application
		}
		sourceBuffer->text( app.getPreference(&macros, macroList->text(clickedItem), "").c_str() );

		app.lastChoosenMacroName = macroList->text(clickedItem);
		app.lastSelectedMacro = clickedItem;
	} else {
		// clicked outside of macro items
		macroList->select(app.lastSelectedMacro);
	}
}


void CsvApplication::executeMacroCB(Fl_Widget *, void *data) {
	int row_top, row_bottom, col_top, col_bottom;
	int winIndex = app.getTopWindow();
	std::tuple<int, int, int, int> selection;
	std::tuple<int, std::string> executeReturn;
	std::tuple<Fl_Text_Buffer *, Fl_Text_Display *, Fl_Text_Display *> *widgets = (std::tuple<Fl_Text_Buffer *, Fl_Text_Display *, Fl_Text_Display *> *) data;
	std::string source = ((Fl_Text_Buffer *) std::get<0>(*widgets))->text();
	void *logVoid = ((void *) std::get<1>(*widgets));
	Fl_Text_Display *logDisplay = (Fl_Text_Display *) logVoid;

    #ifdef DEBUG
	printf("logDisplay: %p\n", (void *)logDisplay);
    #endif
	fflush(stdout);


	// TODO Check if macro source is shorter than TCRUNCHER_PREF_VALUE_MAX_LENGTH
	// TODO Save to `macros` before execution – seems already be done

	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	selection = std::make_tuple(row_top, col_top, row_bottom, col_bottom);
	
	app.showImWorkingWindow("Executing Macro ...");
	windows[winIndex].addUndoStateTable("Execute Macro");
	bool isChanged = windows[winIndex].isChanged();
	windows[winIndex].setChanged(true);
	windows[winIndex].setUsed(true);

	logDisplay->buffer()->text( "Executing macro ...\n" );
	Fl::check();
	executeReturn = macro.execute( windows[winIndex].table, selection, source, logVoid );
	app.hideImWorkingWindow();
	if( std::get<0>(executeReturn) == -1 ) {
		// error
		logDisplay->buffer()->text( std::get<1>(executeReturn).c_str() );
	} else if( std::get<0>(executeReturn) == 0 ) {
		// table data not modified: reset undo state
		windows[winIndex].removeLastUndoState();
		windows[winIndex].setChanged(isChanged);
	}
	logDisplay->buffer()->append( "Done.\n" );

	windows[winIndex].grid->redraw();
	Fl::check();
}
void CsvApplication::insertLoopButtonCB(Fl_Widget *, void *data) {
	std::tuple<Fl_Text_Buffer *, Fl_Text_Display *, Fl_Text_Editor *> *widgets = (std::tuple<Fl_Text_Buffer *, Fl_Text_Display *, Fl_Text_Editor *> *) data;
	int pos = (std::get<2>(*widgets))->insert_position();
	((Fl_Text_Buffer *) std::get<0>(*widgets))->insert(pos, "for( r = 0; r<_ROWS; ++r) {\n}");
}



std::string CsvApplication::getPreference(Fl_Preferences *pref, std::string key, std::string def) {
	char val[TCRUNCHER_PREF_VALUE_MAX_LENGTH+1];
	char *valPtr = val;
	std::string ret;
	
	pref->get(key.c_str(), valPtr, def.c_str(), TCRUNCHER_PREF_VALUE_MAX_LENGTH);
	ret.assign(valPtr);
	return ret;
}

void CsvApplication::setWorkDir(std::string workDir) {
	this->workDir = workDir;
	preferences.set(TCRUNCHER_PREF_WORKDIR, workDir.c_str());
}

std::string CsvApplication::getWorkDir() {
	return workDir;
}


void CsvApplication::moveCols(bool right) {
	int row_top, row_bottom, col_top, col_bottom;
	int winIndex = app.getTopWindow();
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	if( (right && col_bottom < windows[winIndex].table->getNumberCols() - 1 && col_top >= 0) ||
		(!right && col_bottom < windows[winIndex].table->getNumberCols() && col_top > 0 ) ) {
		showImWorkingWindow("Moving column(s) ...");
		windows[winIndex].addUndoStateTable("Move column(s)");
		windows[winIndex].setChanged(true);
		windows[winIndex].setUsed(true);
		windows[winIndex].table->moveCols(col_top, col_bottom, right);
		if( right )
			windows[winIndex].grid->moveSelection(0,1);
		else
			windows[winIndex].grid->moveSelection(0,-1);
		hideImWorkingWindow();
		windows[winIndex].grid->redraw();
		Fl::check();
	}
}


void CsvApplication::addCol(bool before) {
	int row_top, row_bottom, col_top, col_bottom;
	int winIndex = app.getTopWindow();
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	showImWorkingWindow("Adding column ...");
	windows[winIndex].addUndoStateTable("Add column");
	windows[winIndex].setChanged(true);
	windows[winIndex].setUsed(true);
	if( before ) {
		windows[winIndex].table->addCol(col_top, true);
		windows[winIndex].grid->moveSelection(0,1);
	} else {
		windows[winIndex].table->addCol(col_bottom, false);
	}
	hideImWorkingWindow();
	windows[winIndex].grid->rows( windows[winIndex].table->getNumberRows() );
	windows[winIndex].grid->cols( windows[winIndex].table->getNumberCols() );
	windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::addRow(bool before) {
	int row_top, row_bottom, col_top, col_bottom;
	int winIndex = app.getTopWindow();
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	showImWorkingWindow("Adding row ...");
	windows[winIndex].addUndoStateTable("Add row");
	windows[winIndex].setChanged(true);
	windows[winIndex].setUsed(true);
	if( before ) {
		windows[winIndex].table->addRow(row_top, true);
		windows[winIndex].grid->moveSelection(1,0);
	} else {
		windows[winIndex].table->addRow(row_bottom, false);		
	}
	hideImWorkingWindow();
	windows[winIndex].grid->rows( windows[winIndex].table->getNumberRows() );
	windows[winIndex].grid->cols( windows[winIndex].table->getNumberCols() );
	windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::delCols() {
	int row_top, row_bottom, col_top, col_bottom;
	int newCol;
	int winIndex = app.getTopWindow();
	int choice;
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	
	if( col_top < 0 || col_bottom < 0 || col_top >= windows[winIndex].table->getNumberCols() || col_bottom >= windows[winIndex].table->getNumberCols() ) {
		myFlChoice("Info", "No columns selected to delete!", {"OK"});
		return;
	}

	windows[winIndex].grid->setDeletionHighlight(false, col_top, col_bottom);
	choice = myFlChoice("Confirmation", "Really delete "+std::to_string(col_bottom-col_top+1)+" column(s)?", {"Yes", "No"});
	windows[winIndex].grid->removeDeletionHighlight();
	if( choice != 0) {
		return;
	}

	// Close Search Window
	lastFound = {-1,-1};
	searchWin->hide();
	
	newCol = col_top > 0 ? col_top - 1 : 0;
	showImWorkingWindow("Deleting column(s) ...");
	windows[winIndex].addUndoStateTable("Delete column(s)");
	windows[winIndex].setChanged(true);
	windows[winIndex].setUsed(true);
	windows[winIndex].table->delCols(col_top, col_bottom);
	hideImWorkingWindow();
	windows[winIndex].grid->updateSelection(row_top, newCol, row_bottom, newCol);
	windows[winIndex].grid->rows( windows[winIndex].table->getNumberRows() );
	windows[winIndex].grid->cols( windows[winIndex].table->getNumberCols() );
	windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::delRows() {
	int row_top, row_bottom, col_top, col_bottom;
	int newRow;
	int winIndex = app.getTopWindow();
	int choice;
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);

	if( row_top < 0 || row_bottom < 0 || row_top >= windows[winIndex].table->getNumberRows() || row_bottom >= windows[winIndex].table->getNumberRows() ) {
		myFlChoice("Info", "No rows selected to delete!", {"OK"});
		return;
	}

	windows[winIndex].grid->setDeletionHighlight(true, row_top, row_bottom);
	choice = myFlChoice("Confirmation", "Really delete "+std::to_string(row_bottom-row_top+1)+" row(s)?", {"Yes", "No"});
	windows[winIndex].grid->removeDeletionHighlight();
	if( choice != 0) {
		return;
	}
	
	// Close Search Window
	lastFound = {-1,-1};
	searchWin->hide();

	newRow = row_top > 0 ? row_top - 1 : 0;
	showImWorkingWindow("Deleting row(s) ...");
	windows[winIndex].addUndoStateTable("Delete row(s)");
	windows[winIndex].setChanged(true);
	windows[winIndex].setUsed(true);
	windows[winIndex].table->delRows(row_top, row_bottom);
	windows[winIndex].grid->updateSelection(newRow, col_top, newRow, col_bottom);
	hideImWorkingWindow();
	windows[winIndex].grid->rows( windows[winIndex].table->getNumberRows() );
	windows[winIndex].grid->cols( windows[winIndex].table->getNumberCols() );
	windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::splitCol() {
	Fl_Choice *colChoice;
	int windowIndex = getTopWindow();
	std::vector<table_index_t> sel = windows[windowIndex].grid->getSelection();
	table_index_t column = sel[1];
	int callbackDataExchange = 0;

	My_Fl_Small_Window * splitColWin = new My_Fl_Small_Window(600, 175);
	splitColWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	splitColWin->label("Split Column");

	colChoice = new Fl_Choice(100, 50, 120, 25, "Column: ");
	if( column < 0 || column >= (table_index_t) windows[windowIndex].table->headerRow->size() )
		column = 0;
	for( table_index_t c = 0; c < (table_index_t) windows[windowIndex].table->headerRow->size(); ++c ) {
		colChoice->add(windows[windowIndex].table->getHeaderCell(c,false).c_str());
	}
	colChoice->value(column);
	colChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));

	Fl_Input *splitStringBox = new Fl_Input(330,50,220,20, "Split at:");
	splitStringBox->box(FL_BORDER_BOX);
	splitStringBox->value(app.lastSplitString.c_str());
	splitStringBox->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	splitStringBox->take_focus();
	
	My_Fl_Button *splitColumnButton = new My_Fl_Button(450,125,100,24, "Split");
	splitColumnButton->colors = CsvApplication::createButtonColor(My_Fl_Button::HIGHLIGHT);
	splitColumnButton->callback(splitColumn_CB, &callbackDataExchange);

	My_Fl_Button *cancelButton = new My_Fl_Button(330,125,100,24, "Cancel");
	cancelButton->colors = CsvApplication::createButtonColor(My_Fl_Button::DEFAULT);
	cancelButton->callback(splitColumn_Cancel_CB, &callbackDataExchange);
	
	splitColWin->set_modal();
	splitColWin->show();
	while( splitColWin->shown() ) {
		Fl::wait();
	}
	if(
		callbackDataExchange == TCRUNCHER_MYFLCHOICE_MAGICAL ||
		(Fl::event() == FL_KEYDOWN && Fl::event_key() == FL_Enter)
	) {
		windows[windowIndex].setChanged(true);
		windows[windowIndex].setUsed(true);
		showImWorkingWindow("Splitting ...");
		windows[windowIndex].addUndoStateTable("Split Column");
		app.lastSplitString = splitStringBox->value();
		windows[windowIndex].table->splitColumn(colChoice->value(), app.lastSplitString);
		hideImWorkingWindow();
		windows[windowIndex].grid->cols( windows[windowIndex].table->getNumberCols() );
		windows[windowIndex].grid->redraw();
		Fl::check();
	}

	delete cancelButton;
	delete splitColumnButton;
	delete splitStringBox;
	delete splitColWin;
}
void CsvApplication::splitColumn_CB(Fl_Widget *widget, void *data) {
	*((int *)data) = TCRUNCHER_MYFLCHOICE_MAGICAL;
	((My_Fl_Button *)widget)->parent()->hide();
}
void CsvApplication::splitColumn_Cancel_CB(Fl_Widget *widget, void *data) {
	*((int *)data) = 0;
	((My_Fl_Button *)widget)->parent()->hide();
}


void CsvApplication::mergeCols() {
	Fl_Choice *colChoice;
	int windowIndex = getTopWindow();
	std::vector<int> sel = windows[windowIndex].grid->getSelection();
	int column = sel[1];
	int callbackDataExchange = 0;

	My_Fl_Small_Window * mergeColWin = new My_Fl_Small_Window(600, 175);
	mergeColWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	mergeColWin->label("Merge Columns");

	colChoice = new Fl_Choice(100, 50, 120, 25, "Columns: ");
	if( column == (table_index_t) windows[windowIndex].table->headerRow->size() - 1 && column > 0 ) {
		--column;
	}
	if( column < 0 || column >= (table_index_t) windows[windowIndex].table->headerRow->size() )
		column = 0;
	for( size_t c = 0; c < windows[windowIndex].table->headerRow->size() - 1; ++c ) {
		std::string colChoiceItem = windows[windowIndex].table->getHeaderCell(c,false) + " - " + windows[windowIndex].table->getHeaderCell(c+1,false);
		colChoice->add(colChoiceItem.c_str());
	}
	colChoice->value(column);
	colChoice->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));

	Fl_Input *mergeStringBox = new Fl_Input(330,50,220,20, "Glue string:");
	mergeStringBox->box(FL_BORDER_BOX);
	mergeStringBox->value(app.lastGlueString.c_str());
	mergeStringBox->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	mergeStringBox->take_focus();
	
	My_Fl_Button *mergeColumnButton = new My_Fl_Button(450,125,100,24, "Merge");
	mergeColumnButton->colors = CsvApplication::createButtonColor(My_Fl_Button::HIGHLIGHT);
	mergeColumnButton->callback(splitColumn_CB, &callbackDataExchange);

	My_Fl_Button *cancelButton = new My_Fl_Button(330,125,100,24, "Cancel");
	cancelButton->colors = CsvApplication::createButtonColor(My_Fl_Button::DEFAULT);
	cancelButton->callback(splitColumn_Cancel_CB, &callbackDataExchange);
	
	mergeColWin->set_modal();
	mergeColWin->show();
	while( mergeColWin->shown() ) {
		Fl::wait();
	}
	if(
		callbackDataExchange == TCRUNCHER_MYFLCHOICE_MAGICAL ||
		(Fl::event() == FL_KEYDOWN && Fl::event_key() == FL_Enter)
	) {
		windows[windowIndex].setChanged(true);
		windows[windowIndex].setUsed(true);
		showImWorkingWindow("Merging ...");
		windows[windowIndex].addUndoStateTable("Merge Column");
		app.lastGlueString = mergeStringBox->value();
		windows[windowIndex].table->mergeColumns(colChoice->value(), app.lastGlueString);
		hideImWorkingWindow();
		windows[windowIndex].grid->cols( windows[windowIndex].table->getNumberCols() );
		windows[windowIndex].grid->redraw();
		Fl::check();
	}

	delete cancelButton;
	delete mergeColumnButton;
	delete mergeStringBox;
	delete mergeColWin;
}
void CsvApplication::mergeColumn_CB(Fl_Widget *widget, void *data) {
	*((int *)data) = TCRUNCHER_MYFLCHOICE_MAGICAL;
	((My_Fl_Button *)widget)->parent()->hide();
}
void CsvApplication::mergeColumn_Cancel_CB(Fl_Widget *widget, void *data) {
	*((int *)data) = 0;
	((My_Fl_Button *)widget)->parent()->hide();
}

void CsvApplication::jumpToRow() {
	std::string jumpString, rowString = "", colString = "";
	size_t delimiter_pos;
	int retOkay;
	int row = -1, col;
	int winIndex = app.getTopWindow();
	std::vector<int> sel = windows[winIndex].grid->getSelection();
	col = sel[1];
	std::tie(retOkay, jumpString) = myFlAskString("Jump to Row", "Jump");
	if( retOkay ) {
		if( (delimiter_pos = jumpString.find(':')) != std::string::npos ) {
			rowString = jumpString.substr(0,delimiter_pos);
			if( delimiter_pos + 1 < jumpString.size() ) {
				colString = jumpString.substr(delimiter_pos+1);
			}
			if( colString.size() > 0 ) {
				// Look for colString in header row
				if( windows[winIndex].table->customHeaderRowShown() ) {
					// Custom Headers: use colString as pattern
					col = windows[winIndex].table->findHeaderRow(colString);
				} else {
					// No custom headers: interpret colString as column number
					try {
						col = std::stoi(colString);
						--col;
					} catch(...) {
						col = Helper::genericColumnNameToIndex(colString);
					}
				}
				if( col < 0 ) {
					col = sel[1];
				}
			}
		} else {
			rowString = jumpString;
		}
		try {
			row = std::stoi(rowString);
			--row;
		} catch(...) {
			row = -1;
		}
		if( row < 0 ) {
			row = sel[0];
		}
	}
	if( row >= 0 && row < windows[winIndex].table->getNumberRows() ) {
		windows[winIndex].grid->set_selection(row,col,row,col);
		windows[winIndex].grid->setVisibleArea(row, col);
	} else if( retOkay != 0 ) {
		myFlChoice("Warning", "Invalid row number!", {"Okay"});
	}
}


void CsvApplication::jumpToFlaggedRow(bool direction) {
	int newRow;
	int winIndex = app.getTopWindow();
	std::vector<int> sel = windows[winIndex].grid->getSelection();
	if( windows[winIndex].table->countFlaggedRows() == 0 ) {
		myFlChoice("Information", "No flagged rows found.", {"Okay"});
		return;
	}
	if( direction ) {
		newRow = windows[winIndex].table->getNextFlaggedRow(sel[0]);
	} else {
		newRow = windows[winIndex].table->getPrevFlaggedRow(sel[0]);
	}
	if( newRow >= 0 && newRow < windows[winIndex].table->getNumberRows() ) {
		windows[winIndex].grid->set_selection(newRow,sel[1],newRow,sel[1]);
		windows[winIndex].grid->setVisibleArea(newRow, sel[1]);
	}
}


void CsvApplication::switchHeaderRowCB(Fl_Widget *, void *) {
	int winIndex = app.getTopWindow();
	if( windows[winIndex].table->getNumberRows() <= 1 && !windows[winIndex].table->customHeaderRowShown() ) {
		windows[winIndex].showHeaderCheckbox->clear();
		return;
	}
	
	if( windows[winIndex].table->customHeaderRowShown() ) {
		windows[winIndex].grid->moveSelection(1,0);
		windows[winIndex].showHeaderCheckbox->clear();
	} else {
		windows[winIndex].grid->moveSelection(-1,0);
		windows[winIndex].showHeaderCheckbox->set();
	}
	windows[winIndex].table->switchHeader();
	windows[winIndex].grid->rows( windows[winIndex].table->getNumberRows() );
	windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::arrangeColumnsCB(Fl_Widget *, void *) {
   	int winIndex = app.getTopWindow();
    int all_columns_width = 0;                      	// Sum of the width of all columns (in pixels)
    int all_columns_length = 0;                  	// Sum of all maximum lengths of all columns (in characters)
    const int min_column_length = 5;            		// Every column has at least this width (in characters)
    const int max_column_length = 200;          		// "Shrink" every column whose maximum length is greater than this to this value
    std::vector<table_index_t> content_length;
    std::vector<float> content_length_relative;

    // Calculate width of all columns and find maximum width for each column
    for( table_index_t c = 0; c < windows[winIndex].table->getNumberCols(); ++c ) {
        all_columns_width += windows[winIndex].grid->col_width(c);
        std::pair<int, int> col_length_data = windows[winIndex].table->maximumContentLength(c, TCRUNCHER_MAX_PROBE_ROWS_ARRANGE_COLS);
        int col_length = std::min( col_length_data.second, max_column_length);
        col_length = std::max(col_length, min_column_length);
        content_length.push_back(col_length);
        all_columns_length += col_length;
        #ifdef DEBUG
        printf("Col length (%zu): %d\n", c, col_length);
        #endif
    }
    all_columns_length = std::max(all_columns_length, (int) 1);                      // just to prevent division by zero
    #ifdef DEBUG
    printf("grid width: %d\n", windows[winIndex].grid->innerWidth() );                  // inner width
    printf("all_columns_width: %d\n", all_columns_width);
    printf("all_columns_length: %zu\n", all_columns_length);
    #endif
    all_columns_width = std::max( all_columns_width, windows[winIndex].grid->innerWidth());
    for( table_index_t c = 0; c < windows[winIndex].table->getNumberCols(); ++c ) {
        float tmp = (float)content_length.at(c) / all_columns_length;
        content_length_relative.push_back( tmp );
        int new_col_width = tmp * all_columns_width;
        windows[winIndex].grid->col_width(c, new_col_width);
        #ifdef DEBUG
        printf("relative (%zu): %.4f\n", c, tmp);
        printf("new_width (%zu): %d\n", c, new_col_width);
        #endif
    }

    windows[winIndex].grid->redraw();
	Fl::check();
}


void CsvApplication::checkDataConsistency() {
	int row_top, row_bottom, col_top, col_bottom;
	int winIndex = app.getTopWindow();
	windows[winIndex].grid->get_selection(row_top, col_top, row_bottom, col_bottom);
	if( row_top < 0 || row_bottom < 0 || row_top >= windows[winIndex].table->getNumberRows() || row_bottom >= windows[winIndex].table->getNumberRows() ) {
		myFlChoice("Info", "No column selected!", {"OK"});
		return;
	}
	// Check data consistency of column in col_top
	showImWorkingWindow("Checking data consistency ...");
	size_t countInconsistentRows = windows[winIndex].table->flagInconsistentData(col_top);
	hideImWorkingWindow();
	windows[winIndex].grid->redraw();
	std::string msg;
	if( countInconsistentRows == 0 ) {
		msg = "No inconsistent rows were found.";
	} else {
		msg = std::to_string(countInconsistentRows) + " inconsistent row(s) were found and flagged.";
	}
	myFlChoice("Info", msg, {"OK"});
}




/**
 *	Deactivating the Undo menu item is not working reliably, so it's deactivated right now.
 */
void CsvApplication::setUndoMenuItem(bool ) {
	// std::cerr << "CsvApplication::setUndoMenuItem: " << active << std::endl;
	// appMenuBar->activateUndoMenuItem(active);
	// return;

	// CsvMenu *thisMenu = windows[getTopWindow()].getMenuPointer();
	// Fl_Menu_Item *item = (Fl_Menu_Item*)thisMenu->find_item("&Edit/&Undo");
	// if( item != NULL ) {
	// 	if( active ) {
	// 		item->activate();
	// 	} else {
	// 		item->deactivate();
	// 	}
	// }
	// thisMenu->hide();
	// Fl::redraw();
	// thisMenu->show();
	// Fl::redraw();
}


void CsvApplication::setTheme(std::string theme) {
	this->theme = theme;
	if( !ColorThemes::isTheme(this->theme) ) {
		this->theme = "Bright";
	}
	// Style all openend data windows
	for( int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot) {
		windows[slot].applyTheme();
	}
	// Style Search Window
	struct My_Fl_Button::buttonColorStruct colorsDefaultButton;
	colorsDefaultButton.background = ColorThemes::getColor(this->theme, "button_bg");
	colorsDefaultButton.label = ColorThemes::getColor(this->theme, "button_text");
	colorsDefaultButton.border = ColorThemes::getColor(this->theme, "button_border");
	colorsDefaultButton.windowBg = ColorThemes::getColor(this->theme, "win_bg");
	colorsDefaultButton.borderWidth = ColorThemes::getColor(this->theme, "button_border_width");
	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(this->theme, "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(this->theme, "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(this->theme, "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(this->theme, "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(this->theme, "highlight_button_border_width");
	searchWin->color(ColorThemes::getColor(this->theme, "win_bg"));
	searchInput->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	replaceInput->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	findButton->colors = colorsHighlightButton;
	replaceButton->colors = colorsDefaultButton;
	replaceFindButton->colors = colorsDefaultButton;
	replaceAllButton->colors = colorsDefaultButton;
	ignoreCase->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	useRegex->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	flagMatchingButton->colors = colorsDefaultButton;
	unflagMatchingButton->colors = colorsDefaultButton;
	searchWinLabel->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	searchWinScope->labelcolor(ColorThemes::getColor(this->theme, "win_text"));
	// Store theme settings
	preferences.set(TCRUNCHER_PREF_THEME, this->theme.c_str());
}

std::string CsvApplication::getTheme() {
	return theme;
}


void CsvApplication::setFont(std::string font) {
	this->gridFont = font;
	// Store theme settings
	preferences.set(TCRUNCHER_PREF_GRID_TEXT_FONT, this->gridFont.c_str());
	Fl::redraw();
	// Style all openend data windows
	// for( int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot) {
	// 	windows[slot].applyTheme();
	// }
}

std::string CsvApplication::getFont() {
	return gridFont;
}


void CsvApplication::droppedFileCB(const char *path) {
	std::string sPath;
	sPath = path;
	std::cerr << "droppedFileCB: " << sPath << std::endl;
	app.openFile(sPath, false);
}



void CsvApplication::aboutCB(Fl_Widget *, void *) {
	My_Fl_Small_Window *aboutWin;
	Fl_Help_View *helpView;
	Fl_Box *iconBox;
	Fl_Box *nameBox;

	std::string html;
	
	std::string VERSION_STR(MY_VERSION);
	std::string BUILD_YEAR_STRING("2024");
	#ifdef MY_YEAR
	BUILD_YEAR_STRING = "" MY_YEAR "";
	#endif
	std::string BUILD_TODAY_STRING("2025-01-01");
	#ifdef MY_TODAY
	BUILD_TODAY_STRING = "" MY_TODAY "";
	#endif
	std::string BUILD_NOW_STRING("2025-01-01 00:00");
	#ifdef MY_NOW
	BUILD_NOW_STRING = "" MY_NOW "";
	#endif

	aboutWin = new My_Fl_Small_Window(640, 510);
	aboutWin->color(TCRUNCHER_SMALL_WINDOW_BACKGROUND);

	Fl_Pixmap icon(abouticon);
	iconBox = new Fl_Box(320-64,20,128,128);
	iconBox->image(icon);
	
	nameBox = new Fl_Box(FL_FLAT_BOX, 10,170, 630,60, "Tablecruncher" );
	nameBox->color(0xF0F0F000);
	nameBox->labelsize(30);
	nameBox->labelfont(FL_HELVETICA);
	
	
#ifdef DEBUG
	std::string debugInfos = "";
	std::map< std::string, std::string > debug;
	float debugH, debugV;
	int screenCount = Fl::screen_count();
	int scaleSupport = Fl::screen_scaling_supported();
	debug["screenCount"] = std::to_string(screenCount);
	debug["scaleSupport"] = std::to_string(scaleSupport);
	debugInfos += "screen_count: " + debug["screenCount"] + " - ";
	debugInfos += "scale_support: " + debug["scaleSupport"] + "<br>\n";
	for( int i = 0; i < screenCount; ++i ) {
		Fl::screen_dpi(debugH, debugV, i);
		debugInfos += "DPI: " + std::to_string((int)debugH) + "," + std::to_string((int)debugV) + " ";
	}
#endif

	html =
		"<html>"
			"<body bgcolor=\"" TCRUNCHER_SMALL_WINDOW_BACKGROUND_HTMLCODE "\">"
				"<center>"
					"<p>Version: " +
						VERSION_STR +
					"</p>"
					"<p>c 2017-" + BUILD_YEAR_STRING + " by Stefan Fischerländer</p>"
					"<p><a href=\"https://tablecruncher.com\">https://tablecruncher.com</a></p>"
					"<p><font size=\"2\"><a href=\"https://tablecruncher.com/doc.txt\">Documentation</a> – <a href=\"https://tablecruncher.com/readme.txt\">Readme</a></font></p>"
					"<p><font size=\"2\">Solarized and Solarized Dark themes by <a href=\"https://ethanschoonover.com/solarized/\">Ethan Schoonover</a></p>"
					"<p><font size=\"2\">Build date: " + BUILD_NOW_STRING + "</p>"
					#ifdef DEBUG
					"<p><font size=\"1\">" +
						debugInfos +
					"</font></p>"
					#endif
				"</center>"
			"</body>"
		"</html>";
	helpView = new Fl_Help_View(20,260, 600,260);
	helpView->textsize(16);
	helpView->textfont(FL_HELVETICA);
	helpView->box(FL_FLAT_BOX);
	helpView->value( html.c_str() );
	
	aboutWin->set_modal();
	aboutWin->show();
	while( aboutWin->shown() ) {
		Fl::wait();
	}

	delete helpView;
	delete nameBox;
	delete iconBox;
	delete aboutWin;
}



/*
 *	Generic styled alert method, displays up to three answer buttons.
 *	Button 0 is the highlighted button. Gets returned when pressed Enter.
 *
 *	@return		int		-1: ESC pressed, 0,1,2: for the button that was pressed
 */
int CsvApplication::myFlChoice(std::string title, std::string message, std::vector<std::string> options, int buttonWidth, int windowHeight) {
	My_Fl_Small_Window *choiceWin;
	Fl_Help_View *messageLabel;
	std::vector<My_Fl_Button *> buttons;
	Fl_Box *iconBox;
	Fl_RGB_Image *symbol;
	int optionsSize = options.size();
	int ret = -1;

	struct My_Fl_Button::buttonColorStruct colorsDefaultButton;
	colorsDefaultButton.background = ColorThemes::getColor(app.getTheme(), "button_bg");
	colorsDefaultButton.label = ColorThemes::getColor(app.getTheme(), "button_text");
	colorsDefaultButton.border = ColorThemes::getColor(app.getTheme(), "button_border");
	colorsDefaultButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsDefaultButton.borderWidth = ColorThemes::getColor(app.getTheme(), "button_border_width");
	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");
	
    // just use the first three options
	if( optionsSize > 3 )
		optionsSize = 3;

	// buttonData stores which button has been called: initially it gets its own index; when the callback is called,
	// it's filled with TCRUNCHER_MYFLCHOICE_MAGICAL, which has to be larger than the largest possible index (2).
	int32_t buttonData[3];
	buttonData[0] = 0;
	buttonData[1] = 1;
	buttonData[2] = 2;
	
    // Load icon
	if( optionsSize <= 1 )
		symbol = xpmResizer(ui_icons::icon_alert, 48);
	else
		symbol = xpmResizer(ui_icons::icon_question, 48);

	choiceWin = new My_Fl_Small_Window(500,windowHeight);
	choiceWin->copy_label(title.c_str());
	choiceWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	choiceWin->callback(myFlChoiceWin_CB);
	choiceWin->dataExchange = -1;
	
	iconBox = new Fl_Box(10,20,48,48);
	iconBox->image(symbol);

    messageLabel = new Fl_Help_View(80,20,400,windowHeight-80);
	messageLabel->textsize(14);
	messageLabel->textfont(FL_HELVETICA);
	messageLabel->textcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	messageLabel->box(FL_FLAT_BOX);
	messageLabel->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	messageLabel->value( message.c_str() );
	
	for( int i = 0; i < optionsSize; ++i ) {
		int x = 380 - i * (buttonWidth+20);		// calculate x pixels of button
		if( i == 2 ) {
			x = 80;								// the third button is place left-aligned
		}
		buttons.push_back( new My_Fl_Button(x ,windowHeight-50, buttonWidth,24, options[i].c_str()) );
		if( i == 0 ) {
			buttons[i]->colors = colorsHighlightButton;
		} else {
			buttons[i]->colors = colorsDefaultButton;
		}
		buttons[i]->callback(myFlChoice_CB, &(buttonData[i]));
		buttons[i]->clear_visible_focus();
	}
	
    choiceWin->set_modal();
	choiceWin->show();
	while( choiceWin->shown() ) {
		Fl::wait();
	}
	choiceWin->hide();
	Fl::check();
	
    if( choiceWin->dataExchange == TCRUNCHER_MYFLCHOICE_MAGICAL ) {
		// Button clicked
		for( int i = 0; i < optionsSize; ++i ) {
			if( buttonData[i] == TCRUNCHER_MYFLCHOICE_MAGICAL) {
				ret = i;
			}
			delete buttons[i];
		}
	} else {
		// Key pressed
		ret = choiceWin->dataExchange;
	}

    delete symbol;
	delete iconBox;
	delete messageLabel;
	delete choiceWin;
	
	return ret;
}
void CsvApplication::myFlChoice_CB(Fl_Widget *widget, void *data) {
	Fl_Widget *parent;
	parent = widget->parent();
	int32_t *intPtr = (int32_t *)data;
	((My_Fl_Small_Window *) parent)->dataExchange = TCRUNCHER_MYFLCHOICE_MAGICAL;
	*intPtr = TCRUNCHER_MYFLCHOICE_MAGICAL;
	((Fl_Widget *) parent)->hide();
}
void CsvApplication::myFlChoiceWin_CB(Fl_Widget *widget, long data) {
	My_Fl_Small_Window *myWidget = (My_Fl_Small_Window *)widget;
	if( data == TCRUNCHER_MYFLCHOICE_MAGICAL) {
		// pressed ENTER
		myWidget->dataExchange = 0;
		myWidget->hide();
	}
	if( data == 0 ) {
		// ESC or red close button
		myWidget->dataExchange = -1;
		myWidget->hide();
	}
}


/**
 *	Returns pair (retType,string)
 *	retType: 0 if cancelled, 1 otherwise
 */
std::pair<int,std::string> CsvApplication::myFlAskString(std::string title, std::string buttonText, int buttonWidth) {
	My_Fl_Small_Window *askWin;
	Fl_Input *input;
	My_Fl_Button *okButton;
	std::string retStr = "";
	int retInt = 1;
	app.myFlAskStringOk = false;
	
	const int winWidth = 400;
	const int winHeight = 120;
	const int gridMargin = 20;

	struct My_Fl_Button::buttonColorStruct colorsHighlightButton;
	colorsHighlightButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
	colorsHighlightButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
	colorsHighlightButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
	colorsHighlightButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsHighlightButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");

	
	askWin = new My_Fl_Small_Window(winWidth,winHeight);
	askWin->copy_label(title.c_str());
	askWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	askWin->callback(myFlAskStringCB);
	
	input = new Fl_Input(gridMargin,gridMargin,winWidth-2*gridMargin,24);
	input->box(FL_FLAT_BOX);
	
	okButton = new My_Fl_Button(winWidth-gridMargin-buttonWidth,winHeight-gridMargin-24,buttonWidth,24,buttonText.c_str());
	okButton->colors = colorsHighlightButton;
	okButton->shortcut(FL_Enter);
	okButton->callback(myFlAskStringOkCB, NULL);
	
	askWin->set_modal();
	askWin->show();
	while( askWin->shown() ) {
		Fl::wait();
	}
	askWin->hide();
	Fl::check();
	
	if( app.myFlAskStringOk ) {
		retStr = input->value();
		retInt = 1;
	} else {
		retInt = 0;
	}
	
	delete okButton;
	delete input;
	delete askWin;
	
	return std::make_pair(retInt, retStr);
}
void CsvApplication::myFlAskStringCB(Fl_Widget *widget, long data) {
	if( data > 0 ) {
		app.myFlAskStringOk = true;
	}
	widget->hide();
}
void CsvApplication::myFlAskStringOkCB(Fl_Widget *widget, void *) {
	app.myFlAskStringOk = true;
	Fl_Widget *parent = ((Fl_Choice *)widget)->parent();
	((Fl_Widget *) parent)->hide();
}

std::tuple<int, int> CsvApplication::lastFoundPosition() {
	return lastFound;
}



void CsvApplication::checkUpdateCB(Fl_Widget *, void *) {
	app.checkUpdate(true);
}


/**
	checkUpdate(bool ignore = false)

	Check for updates. If 'ignore' is true, ignores if update checks are not allowed and
	also ignores 'daysSinceLastCheck'
*/
void CsvApplication::checkUpdate(bool ignore) {
	if( !ignore && !isUpdateCheckAllowed() ) {
		#ifdef DEBUG
		printf("No update checks allowed\n");
		#endif
		return;
	}
	std::string updateUrl("https://tablecruncher.com/?utm_source=appUpdateWindow&utm_medium=");
	updateUrl += std::string(MY_VERSION);
	bool newerVersion = false;
	
	std::string prefLastCheck = getPreference(&preferences, TCRUNCHER_PREF_LAST_UPDATE_CHECK, "0");
	long lastCheckTs = 0;
	try {
		lastCheckTs = std::stol(prefLastCheck);
	} catch(...) {};
	long nowTs = static_cast<long>(std::time(nullptr));
	std::string nowString = std::to_string(nowTs);
	float daysSinceLastCheck = (nowTs - lastCheckTs) / (3600*24);
	#ifdef DEBUG
	printf("Now: %ld, lastCheck: %ld, days: %.2f\n", nowTs, lastCheckTs, daysSinceLastCheck);
	ignore = true;
	#endif
	
	if( ignore || daysSinceLastCheck > TCRUNCHER_PREF_UPDATE_CHECK_DAYS ) {
		auto timeout = std::chrono::milliseconds(TCRUNCHER_VERSION_CHECK_TIMEOUT);
		httplib::Client cli(TCRUNCHER_VERSION_CHECK_HOST, 80);
		cli.set_connection_timeout(timeout);
		cli.set_read_timeout(timeout);
		cli.set_write_timeout(timeout);
		auto versionResponse = cli.Get(TCRUNCHER_VERSION_CHECK_PATH);
		if( versionResponse ) {
			#ifdef DEBUG
			std::cerr << "MY_VERSION: " << std::string(MY_VERSION) << std::endl;
			#endif
			if( versionResponse->status == 200 ) {
				#ifdef DEBUG
				std::cerr << "SERVER_VERSION: " << versionResponse->body << std::endl;
				#endif
				if( Helper::isUpdateAvailable(std::string(MY_VERSION), versionResponse->body) ) {
					newerVersion = true;
					CsvApplication::myFlChoice("Update Notice", "A newer version is available. Please visit <a href=\""+updateUrl+"\">tablecruncher.com</a> to download it.", {"Okay"});
					#ifdef DEBUG
					std::cerr << "A new version " + versionResponse->body + " is available." << std::endl;
					#endif
				}
				preferences.set(TCRUNCHER_PREF_LAST_UPDATE_CHECK, nowString.c_str());
			} else {
				#ifdef DEBUG
				std::cerr << "ERROR: " << versionResponse->status << std::endl;
				#endif
			}
		}
		if( !newerVersion && ignore ) {
			CsvApplication::myFlChoice("Update Notice", "No newer version available.", {"Okay"});
		}
		
	}
	
}

void CsvApplication::setUpdateCheck(bool allowed) {
	checkUpdateAllowed = allowed;
}
	
	
bool CsvApplication::isUpdateCheckAllowed() {
	return checkUpdateAllowed;
}


int64_t CsvApplication::getPhysMemSize() {
	return physMemSize;
}


/**
	Is called whenever a new window gets focus to adjust the menu to window
	specific settings
 */
void CsvApplication::updateMenu(int winIndex) {
	#ifndef __APPLE__
	// Windows/Linux: Set app menu bar to the current top windows menu bar
	appMenuBar = windows[winIndex].getWinMenuBar();
	#endif
	// find item containing "Disable Undo" or "Enable Undo"
	int disableUndoItem = appMenuBar->find_index("&Edit/" TCRUNCHER_MENU_BAR_DISABLE_UNDO_STRING);
	int undoItem  = appMenuBar->find_index("&Edit/&Undo");
	if( disableUndoItem == -1 ) {
		disableUndoItem = appMenuBar->find_index("&Edit/" TCRUNCHER_MENU_BAR_ENABLE_UNDO_STRING);
	}
	if( undoItem >= 0 ) {
		if( windows[winIndex].isUndoDisabled() ) {
			appMenuBar->replace(disableUndoItem, TCRUNCHER_MENU_BAR_ENABLE_UNDO_STRING);
			int flags = appMenuBar->mode(undoItem);
			appMenuBar->mode(undoItem, flags | FL_MENU_INACTIVE);
		} else {
			appMenuBar->replace(disableUndoItem, TCRUNCHER_MENU_BAR_DISABLE_UNDO_STRING);
			int flags = appMenuBar->mode(undoItem);
			appMenuBar->mode(undoItem, flags & ~FL_MENU_INACTIVE);
		}
	}
	// update recent files
	appMenuBar->updateOpenRecentMenu(recentFiles.getRecentFiles());
	// update the widget
	appMenuBar->hide();
	Fl::redraw();
	appMenuBar->show();
	Fl::redraw();
}



CsvMenu *CsvApplication::getAppMenuBar() {
	return appMenuBar;
}


void CsvApplication::showImWorkingWindow(std::string message, bool showAlways) {
	int winIndex = getTopWindow();
	if( windows[winIndex].table->getNumberRows() > 10000 || showAlways ) {
		windows[winIndex].grid->allowEvents(false);
		imWorkingWindow->copy_label("Processing");
		imWorkingWindow->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		imWorkingButton->copy_label(message.c_str());
		imWorkingButton->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
		imWorkingButton->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
		imWorkingWindow->callback(showImWorkingWindowCB,0);
		imWorkingWindow->set_modal();
		imWorkingWindow->show();
		Fl::check();
	}
}
void CsvApplication::showImWorkingWindowCB(Fl_Widget *, long ) {
	// intentionally empty: showImWorkingWindow must not be closed by user
}
void CsvApplication::hideImWorkingWindow() {
	int winIndex = getTopWindow();
	windows[winIndex].grid->allowEvents(true);
	imWorkingWindow->hide();
	Fl::check();
}



void CsvApplication::editSingleCell() {
	int winWidth = 600;
	int winHeight = 450;
	int buttonHeight = 24;
	int margin = 20;
	int winIndex = getTopWindow();
	std::vector<int> selected = windows[winIndex].grid->getSelection();
	if( selected[0] < 0 || selected[0] >= windows[winIndex].table->getNumberRows() || selected[1] < 0 || selected[1] >= windows[winIndex].table->getNumberCols() ) {
		return;
	}
	bool changeContent = false;
	
	if( lastSingleEditWinWidth > 0 && lastSingleEditWinHeight > 0 ) {
		winWidth = lastSingleEditWinWidth;
		winHeight = lastSingleEditWinHeight;
	}
	
	My_Fl_Small_Window *singleCellEditWin = new My_Fl_Small_Window(winWidth,winHeight);
	singleCellEditWin->set_modal();
	singleCellEditWin->color(ColorThemes::getColor(app.getTheme(), "win_bg"));
	singleCellEditWin->labelcolor(ColorThemes::getColor(app.getTheme(), "win_text"));
	singleCellEditWin->label("Edit cell");

	Fl_Text_Buffer *cellBuffer = new Fl_Text_Buffer();
	Fl_Text_Editor *cellEditor = new Fl_Text_Editor(margin, margin, winWidth - 2*margin, winHeight - 3 * margin - buttonHeight);

	cellBuffer->text( windows[winIndex].table->getCell(selected[0], selected[1]).c_str() );
	cellBuffer->tab_distance(getEditorTabDistance());
	cellEditor->buffer(cellBuffer);
	cellEditor->box(FL_FLAT_BOX);
	cellEditor->textsize(15);
	cellEditor->textfont(FL_COURIER);
	
	struct My_Fl_Button::buttonColorStruct colorsButton;
	colorsButton.background = ColorThemes::getColor(app.getTheme(), "button_bg");
	colorsButton.label = ColorThemes::getColor(app.getTheme(), "button_text");
	colorsButton.border = ColorThemes::getColor(app.getTheme(), "button_border");
	colorsButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
	colorsButton.borderWidth = ColorThemes::getColor(app.getTheme(), "button_border_width");
	
	singleCellEditWin->begin();
	My_Fl_Button *saveButton = new My_Fl_Button(winWidth - margin - 100, winHeight - margin - buttonHeight, 100, buttonHeight, "Save");
	saveButton->colors = colorsButton;
	saveButton->shortcut(FL_Enter);
	saveButton->callback(editSingleCellSaveCB, &changeContent);
	singleCellEditWin->end();
	
	singleCellEditWin->resizable(cellEditor);
	
	singleCellEditWin->show();
	while( singleCellEditWin->shown() ) {
		Fl::wait();
	}
	
	if( changeContent ) {
		windows[winIndex].addUndoStateCell(windows[winIndex].table->getCell(selected[0], selected[1]), selected[0], selected[1], "Cell edited");
		windows[winIndex].setChanged(true);
		windows[winIndex].setUsed(true);
		std::string changedText(cellBuffer->text());
		windows[winIndex].table->setCell(changedText, selected[0], selected[1]);
		windows[winIndex].grid->redraw();
		Fl::check();
	}
	
	
	delete saveButton;
	delete cellEditor;
	delete cellBuffer;
	delete singleCellEditWin;
}
void CsvApplication::editSingleCellCB(Fl_Widget *, void *) {
	app.editSingleCell();
}
void CsvApplication::editSingleCellSaveCB(Fl_Widget *widget, void *data) {
	// My_Fl_Small_Window *singleCellEditWin = ((My_Fl_Button *)widget)->parent();
	bool *changeContent = (bool *) data;
	*changeContent = true;
	app.lastSingleEditWinWidth = ((My_Fl_Button *)widget)->parent()->w();
	app.lastSingleEditWinHeight = ((My_Fl_Button *)widget)->parent()->h();
	((My_Fl_Button *)widget)->parent()->hide();
}


int CsvApplication::getEditorTabDistance() {
	// TODO Read from preferences
	return 4;
}


struct My_Fl_Button::buttonColorStruct CsvApplication::createButtonColor(My_Fl_Button::ButtonStyle style) {
	struct My_Fl_Button::buttonColorStruct colorsButton;
	switch(style) {
		case My_Fl_Button::HIGHLIGHT:
			colorsButton.background = ColorThemes::getColor(app.getTheme(), "hightlight_button_bg");
			colorsButton.label = ColorThemes::getColor(app.getTheme(), "hightlight_button_text");
			colorsButton.border = ColorThemes::getColor(app.getTheme(), "hightlight_button_border");
			colorsButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
			colorsButton.borderWidth = ColorThemes::getColor(app.getTheme(), "highlight_button_border_width");
			
		break;
		case My_Fl_Button::DEFAULT:
		default:
			colorsButton.background = ColorThemes::getColor(app.getTheme(), "button_bg");
			colorsButton.label = ColorThemes::getColor(app.getTheme(), "button_text");
			colorsButton.border = ColorThemes::getColor(app.getTheme(), "button_border");
			colorsButton.windowBg = ColorThemes::getColor(app.getTheme(), "win_bg");
			colorsButton.borderWidth = ColorThemes::getColor(app.getTheme(), "button_border_width");
	}
	return colorsButton;
}


Fl_RGB_Image *CsvApplication::xpmResizer(const char* const* xpm_data, int size) {
    Fl_Pixmap pixmap((char**)xpm_data);
	Fl_RGB_Image *rgb = (Fl_RGB_Image *) pixmap.copy(size, size);
    return rgb;
}


int CsvApplication::getCustomFont(CsvApplication::FontUsage usage) {
	int ret = customFonts.gridText;
	switch(usage) {
		case CsvApplication::FontUsage::TEXT:
			ret = customFonts.gridText;
			if( fontMapping.count(gridFont) ) {
				ret = fontMapping[gridFont];
			}
		break;
		case CsvApplication::FontUsage::COL_HEADER:
			ret = customFonts.gridColHeader;
			// if( fontMapping.count(gridFont) ) {
			// 	ret = fontMapping[gridFont];
			// }
		break;
		case CsvApplication::FontUsage::ROW_HEADER:
			ret = customFonts.gridRowHeader;
			// if( fontMapping.count(gridFont) ) {
			// 	ret = fontMapping[gridFont];
			// }
		break;
	}
	return ret;
}


void CsvApplication::dumpWindows() {
	for( int slot = 0; slot < TCRUNCHER_MAX_WINDOWS; ++slot) {
		printf("Window %02d:\n", slot);
		if( windows[slot].getWindowSlotUsed() ) {
			printf("win: %p\n", (void *)windows[slot].win);
		} else {
			printf("-- not used --\n");
		}
	}
}


































