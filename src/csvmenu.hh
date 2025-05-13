#ifndef _CSVMENU_HH
#define _CSVMENU_HH

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>


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