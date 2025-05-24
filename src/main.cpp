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


/** \file main.cpp
 * \brief The main application file.
 * 
 * We set the working directory, the theme and the font as stored in the preferences file or to their default values.
 * An optional onboarding is shown and we check for updates if it's allowed.
 * 
 * We finally open a new window and optionally load the files that were given on the command line, if any.
 * 
 */




#include "globals.hh"
#include "csvapplication.hh"
#include "csvwindow.hh"
#include "helper.hh"
#include "macro.hh"

#include <string>


#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>



/** Global function to access the application's preferences as stored in the preferences file. */
Fl_Preferences preferences(Fl_Preferences::USER, TCRUNCHER_PREF_FOLDER, TCRUNCHER_PREF_NAME);

/** Global function to access the application's macros as stored in the macros file. */
Fl_Preferences macros(Fl_Preferences::USER, TCRUNCHER_PREF_FOLDER, TCRUNCHER_MACRO_PREF_NAME);

/** Global function to access the application's preferences like column widths or header status. */
Fl_Preferences file_preferences(Fl_Preferences::USER, TCRUNCHER_PREF_FOLDER, TCRUNCHER_FILES_PREF_NAME);

/** The single instance of `CsvApplication` that handles all the application funcionality. */
CsvApplication app;

/** A list of all opened windows (class `CsvWindow`). */
CsvWindow windows[TCRUNCHER_MAX_WINDOWS];

/** The single macro instance. */
Macro macro;

#ifdef _WIN64
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
#else
int main(int argc, char** argv) {
#endif
	Fl_Window::default_xclass("tablecruncher");
	int runState;
	std::string homeDir;

	#ifdef __APPLE__
	fl_mac_set_about(&CsvApplication::aboutCB, NULL);
	#endif

	homeDir = Helper::getHomeDir();
	app.setWorkDir( app.getPreference(&preferences, TCRUNCHER_PREF_WORKDIR, homeDir) );
	app.setTheme( app.getPreference(&preferences, TCRUNCHER_PREF_THEME, "Bright") );
	app.setFont( app.getPreference(&preferences, TCRUNCHER_PREF_GRID_TEXT_FONT, TCRUNCHER_FALLBACK_FONT) );
	
	// Opens file dropped on the icon
	fl_open_callback(CsvApplication::droppedFileCB);

	#ifndef __APPLE__
	//Fl::scheme("gleam");
	#endif

	// Open an initial window
	app.createNewWindow();
	
	// Onboarding
	if( app.getPreference(&preferences, TCRUNCHER_PREF_SHOWED_ONBOARDING, "no") == "no" ) {
		if( app.showOnboardingProcess() ) {
			preferences.set(TCRUNCHER_PREF_SHOWED_ONBOARDING, "MY_VERSION");
		}
	}

	// Check Update
	if( app.getPreference(&preferences, TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED, "no") == "yes" ) {
		app.setUpdateCheck(true);
	} else {
		app.setUpdateCheck(false);
	}
	app.checkUpdate();

	#ifdef _WIN64
	// Set Theme needed on Windows
	app.setTheme(app.getTheme());
	#endif

	#ifndef _WIN64
	for( int i = 1; i < argc; ++i ) {
		// open all files passed on the command line
		CsvApplication::droppedFileCB(argv[i]);
	}
	#endif

	// run
	runState = Fl::run();

	return runState;
}




