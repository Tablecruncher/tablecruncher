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


#ifndef _CSVMENU_HH
#define _CSVMENU_HH

#include <string>
#include <vector>

#include <FL/Fl_Preferences.H>




#ifdef __APPLE__
	#include <FL/Fl_Sys_Menu_Bar.H>
#else
	#include <FL/Fl_Menu_Bar.H>
#endif


#define TCRUNCHER_MENUTEXT_OPEN 						"&Open ..."
#define TCRUNCHER_MENUTEXT_OPEN_WITH_FORMAT				"&Open with format ..."
#define TCRUNCHER_MENUTEXT_OPEN_RECENT					"&Open Recent"
#define TCRUNCHER_MENUTEXT_UNDO							"&Undo"
// #define TCRUNCHER_MENUTEXT_UNDO_NONE					"&No Undo Available"
#define TCRUNCHER_MENU_NUM_INTEGERS						20

#include "globals.hh"


/**
 * \brief Creates the application menu and provides a callback handler in `MyMenuCallback`.
 * 
 * 
 */
#ifdef __APPLE__
class CsvMenu : public Fl_Sys_Menu_Bar {
#else
class CsvMenu : public Fl_Menu_Bar {
#endif
public:
	CsvMenu();
	~CsvMenu();
	void init();
	// const void activateUndoMenuItem( bool activate = true );
	void updateOpenRecentMenu(std::vector<std::string> files);

private:
	static void MyMenuCallback(Fl_Widget *w, void *data);
	const int INTEGERS[TCRUNCHER_MENU_NUM_INTEGERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
	
	// std::string undoLabelText;
};



#endif