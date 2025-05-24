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


#include "csvmenu.hh"
#include "csvapplication.hh"

#include <FL/Enumerations.H>
#include <filesystem>

extern Fl_Preferences preferences;
extern CsvApplication app;


#ifdef __APPLE__
	#define TC_CSVMENU_MENU_BAR_CLASS Fl_Sys_Menu_Bar
#else
	#define TC_CSVMENU_MENU_BAR_CLASS Fl_Menu_Bar
#endif

CsvMenu::CsvMenu() : TC_CSVMENU_MENU_BAR_CLASS(0,0,600,30) {
}


CsvMenu::~CsvMenu() {}


void CsvMenu::init() {
	int flags = 0;
	std::string prefTheme = app.getPreference(&preferences, TCRUNCHER_PREF_THEME, "BRIGHT");
	std::string prefFont = app.getPreference(&preferences, TCRUNCHER_PREF_GRID_TEXT_FONT, TCRUNCHER_FALLBACK_FONT);
	Fl::menu_linespacing(12);
	// undoLabelText = TCRUNCHER_MENUTEXT_UNDO;
	box(FL_NO_BOX);
	down_box(FL_NO_BOX);
	add("&File/&New", FL_COMMAND + 'n', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&File/" TCRUNCHER_MENUTEXT_OPEN, FL_COMMAND + 'o', MyMenuCallback, 0);
	add("&File/&Open with format ...", FL_COMMAND + FL_SHIFT + 'o', MyMenuCallback, 0);
	add("&File/&Reopen ...", FL_COMMAND + FL_SHIFT + FL_CTRL + 'o', MyMenuCallback, 0);
	add("&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT, 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER);
	add("&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT "/&(empty)", 0, 0, 0, FL_MENU_INACTIVE);
	add("&File/&Close", FL_COMMAND + 'w', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&File/&Save", FL_COMMAND + 's', MyMenuCallback);
	add("&File/&Save As ...", FL_SHIFT + FL_COMMAND + 's', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&File/&Split CSV ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&File/&Export JSON ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&File/&Set CSV Properties ...", 0, MyMenuCallback, 0);
	add("&File/&Info", FL_COMMAND + 'i', MyMenuCallback, 0, FL_MENU_DIVIDER);
	#ifndef __APPLE__
	add("&File/&Quit", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	#endif

	add("&Edit/" TCRUNCHER_MENUTEXT_UNDO, FL_COMMAND + 'z', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Copy", FL_COMMAND + 'c', MyMenuCallback);
	add("&Edit/&Paste", FL_COMMAND + 'v', MyMenuCallback, 0);
	add("&Edit/&Paste with format ...", FL_COMMAND + FL_SHIFT + 'v', MyMenuCallback, 0);
	add("&Edit/&Paste into Selection", FL_COMMAND + FL_CTRL + 'v', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Edit cell ...", FL_COMMAND + FL_ENTER, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Move Column(s) Left", FL_COMMAND + FL_ALT + 'k', MyMenuCallback);
	add("&Edit/&Move Column(s) Right", FL_COMMAND + FL_ALT + 'l', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Insert Row Above", 0, MyMenuCallback);
	add("&Edit/&Insert Row Below", 0, MyMenuCallback);
	add("&Edit/&Insert Column Left", 0, MyMenuCallback);
	add("&Edit/&Insert Column Right", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Delete Row(s)", 0, MyMenuCallback);
	add("&Edit/&Delete Column(s)", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/&Split Column ...", 0, MyMenuCallback);
	add("&Edit/&Merge Columns ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Edit/" TCRUNCHER_MENU_BAR_DISABLE_UNDO_STRING, 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	#ifndef __APPLE__
	add("&Edit/&Preferences...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	#endif

	add("&Data/&Find ...", FL_COMMAND + 'f', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Data/&Sort ...", FL_COMMAND + FL_CTRL + 's', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Data/&Flag Selected Row(s) ...", 0, MyMenuCallback);
	add("&Data/&Unflag Row(s) ...", 0, MyMenuCallback);
	add("&Data/&Invert Flagged Row(s)", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Data/&Delete Flagged Row(s) ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Data/&Export Flagged Row(s) ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Data/&Check Data Consistency ...", FL_COMMAND + FL_CTRL + 'c', MyMenuCallback);

	add("&Macro/&Execute Macro ...", FL_COMMAND + 'e', MyMenuCallback, 0);

	add("&View/&Switch Header Row", FL_COMMAND + FL_SHIFT + 'h', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&View/&Arrange Columns", FL_COMMAND + FL_SHIFT + 'c', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&View/&Jump to Cell ...", FL_COMMAND + 'l', MyMenuCallback, 0);
	add("&View/&Jump to Previous Flagged Row ...", FL_COMMAND + FL_SHIFT + 'j', MyMenuCallback, 0);
	add("&View/&Jump to Next Flagged Row ...", FL_COMMAND + 'j', MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&View/&Font", 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER);
	// Fonts
	for( auto const& [fontName, fontNum] : app.getFontMapping() ) {
		flags = prefFont == fontName ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO;
		std::string item_str = "&View/&Font/" + fontName;
		add(item_str.c_str(), 0, MyMenuCallback, 0, flags);
	}
	add("&View/&Default Font Size", FL_COMMAND + '0', MyMenuCallback);
	add("&View/&Bigger Font", FL_COMMAND + '+', MyMenuCallback);
	add("&View/&Smaller Font", FL_COMMAND + '-', MyMenuCallback, 0, FL_MENU_DIVIDER);
	// Theme
	flags = prefTheme == "Bright" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO;
	add("&View/&Themes/&Bright", 0, MyMenuCallback, 0, flags);
	flags = prefTheme == "Dark" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO;
	add("&View/&Themes/&Dark", 0, MyMenuCallback, 0, flags);
	flags = prefTheme == "Solarized Bright" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO;
	add("&View/&Themes/&Solarized Bright", 0, MyMenuCallback, 0, flags);
	flags = prefTheme == "Solarized Dark" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO;
	add("&View/&Themes/&Solarized Dark", 0, MyMenuCallback, 0, flags);

	#ifndef __APPLE__
	add("&Help/&About ...", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	add("&Help/&Check for Updates", 0, MyMenuCallback, 0, FL_MENU_DIVIDER);
	#endif

	#ifndef __APPLE__
	down_box(FL_FLAT_BOX);
	#endif
}




/*
	Activate or deactivate the Undo menu item – doesn't work…
*/
// const void CsvMenu::activateUndoMenuItem( bool activate ) {
// 	Fl_Menu_Item *item;
// 	if( ( item = (Fl_Menu_Item *)find_item(("&Edit/" + undoLabelText).c_str()) ) != NULL ) {
// 		std::cerr << "CsvMenu::activateUndoMenuItem: activate = " << activate << std::endl;
// 		if( activate ) {
// 			std::cerr << "CsvMenu::activateUndoMenuItem: activating Undo item" << std::endl;
// 			undoLabelText = TCRUNCHER_MENUTEXT_UNDO;
// 			item->activate();
// 			item->label(undoLabelText.c_str());
// 			item->show();
// 		} else {
// 			std::cerr << "CsvMenu::activateUndoMenuItem: deactivating Undo item" << std::endl;
// 			undoLabelText = TCRUNCHER_MENUTEXT_UNDO_NONE;
// 			item->deactivate();
// 			item->label(undoLabelText.c_str());
// 			item->show();
// 		}
// 		Fl::redraw();
// 	}
// }


void CsvMenu::updateOpenRecentMenu(std::vector<std::string> files) {
    std::string menuItemStr = "&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT;
	int menuIndex = find_index("&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT);
	if( menuIndex != -1 ) {
		clear_submenu(menuIndex);
		if( files.size() > 0 ) {
			std::string item_tpl = "&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT "/&";
			int i = 0;
			for( auto filepath : files ) {
				std::string filename_only = std::filesystem::path(filepath).filename().u8string();
				std::string item = item_tpl + filename_only; 					// + " (" + std::to_string(i+1) + ")";
				add(item.c_str(), FL_COMMAND + ('1' + i), MyMenuCallback, (void *) &(INTEGERS[i]), 0);
				++i;
			}
		} else {
			// no entries available
			add("&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT "/&(empty)", 0, 0, 0, FL_MENU_INACTIVE);
		}
	}
}



void CsvMenu::MyMenuCallback(Fl_Widget *w, void *data) {
	char ipath[256];
		
	TC_CSVMENU_MENU_BAR_CLASS *bar = (TC_CSVMENU_MENU_BAR_CLASS*)w;			// Get the menubar widget
	const Fl_Menu_Item *item = bar->mvalue();								// Get the menu item that was picked
	bar->item_pathname(ipath, sizeof(ipath));								// Get full pathname of picked item

	if( strcmp(item->label(), "&New") == 0 ) {
		app.createNewWindow();
	} else if( strcmp(item->label(), TCRUNCHER_MENUTEXT_OPEN) == 0 ) {
		app.openFile(false);
	} else if( strcmp(item->label(), "&Open with format ...") == 0 ) {
		app.openFile(true);
	} else if( strcmp(item->label(), "&Reopen ...") == 0 ) {
		app.openFile(true, true);
	} else if( strcmp(item->label(), "&Save") == 0 ) {
		app.saveFileCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Save As ...") == 0 ) {
		app.saveFileAsCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Split CSV ...") == 0 ) {
		app.splitCsvCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Export JSON ...") == 0 ) {
		app.exportJsonCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Close") == 0 ) {
		app.closeWindow();
	} else if( strcmp(item->label(), "&Set CSV Properties ...") == 0 ) {
		app.setCsvPropertiesCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Info") == 0 ) {
		app.showInfoWindowCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Quit") == 0 ) {
		app.quitApplication(true);
	} else if( strcmp(item->label(), TCRUNCHER_MENUTEXT_UNDO) == 0 ) {
		app.undoCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Preferences...") == 0 ) {
		app.AppMenuPreferencesCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Copy") == 0 ) {
		app.copySelection();
	} else if( strcmp(item->label(), "&Paste") == 0 ) {
		app.paste();
	} else if( strcmp(item->label(), "&Paste with format ...") == 0 ) {
		app.paste(true, false);
	} else if( strcmp(item->label(), "&Paste into Selection") == 0 ) {
		app.paste(true, true);
	} else if( strcmp(item->label(), "&Edit cell ...") == 0 ) {
		app.editSingleCellCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Move Column(s) Left") == 0 ) {
		app.moveCols(false);
	} else if( strcmp(item->label(), "&Move Column(s) Right") == 0 ) {
		app.moveCols(true);
	} else if( strcmp(item->label(), "&Insert Row Below") == 0 ) {
		app.addRow(false);
	} else if( strcmp(item->label(), "&Insert Row Above") == 0 ) {
		app.addRow(true);
	} else if( strcmp(item->label(), "&Insert Column Right") == 0 ) {
		app.addCol(false);
	} else if( strcmp(item->label(), "&Insert Column Left") == 0 ) {
		app.addCol(true);
	} else if( strcmp(item->label(), "&Delete Row(s)") == 0 ) {
		app.delRows();
	} else if( strcmp(item->label(), "&Delete Column(s)") == 0 ) {
		app.delCols();
	} else if( strcmp(item->label(), "&Split Column ...") == 0 ) {
		app.splitCol();
	} else if( strcmp(item->label(), "&Merge Columns ...") == 0 ) {
		app.mergeCols();
	} else if( strcmp(item->label(), TCRUNCHER_MENU_BAR_DISABLE_UNDO_STRING) == 0 ) {
		app.disableUndoCB(NULL, NULL);
	} else if( strcmp(item->label(), TCRUNCHER_MENU_BAR_ENABLE_UNDO_STRING) == 0 ) {
		app.enableUndoCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Find ...") == 0 ) {
		app.find();
	} else if( strcmp(item->label(), "&Sort ...") == 0 ) {
		app.sort();
	} else if( strcmp(item->label(), "&Flag Selected Row(s) ...") == 0 ) {
		app.flagSelectedRowsCB();
	} else if( strcmp(item->label(), "&Unflag Row(s) ...") == 0 ) {
		app.unflagRowsCB();
	} else if( strcmp(item->label(), "&Invert Flagged Row(s)") == 0 ) {
		app.invertFlaggedCB();
	} else if( strcmp(item->label(), "&Delete Flagged Row(s) ...") == 0 ) {
		app.deleteFlaggedCB(true);
	} else if( strcmp(item->label(), "&Export Flagged Row(s) ...") == 0 ) {
		app.exportFlaggedCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Check Data Consistency ...") == 0 ) {
		app.checkDataConsistencyCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Execute Macro ...") == 0 ) {
		app.executeMacro();
	} else if( strcmp(item->label(), "&Switch Header Row") == 0 ) {
		app.switchHeaderRowCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Arrange Columns") == 0 ) {
		app.arrangeColumnsCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Jump to Cell ...") == 0 ) {
		app.jumpToRow();
	} else if( strcmp(item->label(), "&Jump to Previous Flagged Row ...") == 0 ) {
		app.jumpToFlaggedRow(false);
	} else if( strcmp(item->label(), "&Jump to Next Flagged Row ...") == 0 ) {
		app.jumpToFlaggedRow(true);
	} else if( strcmp(item->label(), "&Default Font Size") == 0 ) {
		app.changeFontSize(0);
	} else if( strcmp(item->label(), "&Bigger Font") == 0 ) {
		app.changeFontSize(1);
	} else if( strcmp(item->label(), "&Smaller Font") == 0 ) {
		app.changeFontSize(-1);
	} else if( strcmp(item->label(), "&Bright") == 0 ) {
		app.setTheme("Bright");
	} else if( strcmp(item->label(), "&Dark") == 0 ) {
		app.setTheme("Dark");
	} else if( strcmp(item->label(), "&Solarized Bright") == 0 ) {
		app.setTheme("Solarized Bright");
	} else if( strcmp(item->label(), "&Solarized Dark") == 0 ) {
		app.setTheme("Solarized Dark");
	} else if( strcmp(item->label(), "&About ...") == 0 ) {
		app.aboutCB(NULL, NULL);
	} else if( strcmp(item->label(), "&Check for Updates") == 0 ) {
		app.checkUpdateCB(NULL, NULL);
	} else {
		bool stop_prop = false;
		std::string ipathStr( ipath );
		if( ipathStr.rfind("&File/" TCRUNCHER_MENUTEXT_OPEN_RECENT "/&", 0) == 0 ) {
			// ipathStr starts with File/Open Recent/…
			if( data ) {
				int recentIndex = *(int *)data;
				app.openRecentFile(recentIndex);
			}
			stop_prop = true;
		}
		if( !stop_prop ) {
			// Check for font names
			for( auto const& [fontName, fontNum] : app.getFontMapping() ) {
				if( strcmp(item->label(), fontName.c_str()) == 0 ) {
					app.setFont(fontName);
					stop_prop = true;
				}
			}
		}
		if( !stop_prop ) {

		}
		// if( callbackCode >= 1 ) {
		// 	// 	setFocusByWinIndex(callbackCode - 1);	// TODO ???
		// }
	}
}

