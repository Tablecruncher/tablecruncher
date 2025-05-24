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


#ifndef _GLOBALS_HH
#define _GLOBALS_HH


typedef int table_index_t;


#define TCRUNCHER_APP_NAME "Tablecruncher"
#define TCRUNCHER_CSVGRID_INPUT_WIDGET Fl_Multiline_Input



#define TCRUNCHER_PREF_FOLDER "com.tablecruncher"
#define TCRUNCHER_PREF_NAME "tablecruncher"				// Default Tablecruncher preferences like theme and working directory
#define TCRUNCHER_MACRO_PREF_NAME "macros"				// All defined macros
#define TCRUNCHER_FILES_PREF_NAME "files"				// Details about recently stored files: column width, header settings, etc.
#define TCRUNCHER_PREF_WORKDIR "myWorkDir"
#define TCRUNCHER_PREF_THEME "myTheme"
#define TCRUNCHER_PREF_LAST_UPDATE_CHECK "lastUpdateCheck"
const int TCRUNCHER_PREF_UPDATE_CHECK_DAYS = 2;				// check two days
#define TCRUNCHER_PREF_UPDATE_CHECK_ALLOWED "updateCheckAllowed"
#define TCRUNCHER_PREF_SHOWED_ONBOARDING "showedOnboarding"
#define TCRUNCHER_PREF_GRID_TEXT_FONT "gridTextFont"
#define TCRUNCHER_PREF_RECENT_FILES_STUB "recent_file_"
#define TCRUNCHER_PREF_RECENT_FILES_NUM 9

#define TCRUNCHER_ICON_DIR "icons"

#define TCRUNCHER_VERSION_CHECK_HOST "version.tablecruncher.com"
#define TCRUNCHER_VERSION_CHECK_PATH "/version.txt"
#define TCRUNCHER_VERSION_CHECK_TIMEOUT 3000				// milliseconds


#define TCRUNCHER_MENU_BAR_DISABLE_UNDO_STRING "&Disable Undo ..."
#define TCRUNCHER_MENU_BAR_ENABLE_UNDO_STRING "&Enable Undo"



#ifdef __APPLE__
	const int TCRUNCHER_TOPOFFSET = 0;						// offset for table widget
	const int TCRUNCHER_ICON_BAR_HEIGHT = 60;
#else
	const int TCRUNCHER_TOPOFFSET = 25;
	const int TCRUNCHER_ICON_BAR_HEIGHT = 40;
#endif
	

const int TCRUNCHER_MAX_WINDOWS = 10;
const int TCRUNCHER_NUM_UTF8_TEST_BYTES = 200 * 1024 * 1024;		// how many bytes are used to test for a valid UTF8 encoding
const int TCRUNCHER_MAX_PREVIEW_ROWS = 20;							// how many rows should be shown in preview while opening
const int TCRUNCHER_MAX_PROBE_ROWS_ARRANGE_COLS = 10000;			// maximum number of rows to probe for automatic column arrangement

const int TCRUNCHER_PATH_MAX_LENGTH = 2000;
const int TCRUNCHER_PREF_VALUE_MAX_LENGTH = 5000;

const int TCRUNCHER_STATUS_BAR_HEIGHT = 32;

const int TCRUNCHER_TOOLBAR_ICONSIZE = 24;
const int TCRUNCHER_TOOLBAR_ICONPADDING = 4;

const int TCRUNCHER_UNDO_TYPE_TABLE = 1;
const int TCRUNCHER_UNDO_TYPE_CELL = 2;
const int TCRUNCHER_UNDO_TYPE_HEADERROW = 3;

const int TCRUNCHER_SMALL_WINDOW_BACKGROUND = 0xF0F0F000;
#define TCRUNCHER_SMALL_WINDOW_BACKGROUND_HTMLCODE "#F0F0F0"

const int TCRUNCHER_MYFLCHOICE_MAGICAL = 42;		// just has to be larger than 2


/*
 *	Grid sizes
 */
const int TCRUNCHER_STARTUP_ROWS = 5;
const int TCRUNCHER_STARTUP_COLS = 4;
const int TCRUNCHER_STARTUP_CELL_WIDTH = 150;
// const int TCRUNCHER_STARTUP_CELL_HEIGHT = 30;
const int TCRUNCHER_MAX_CELL_LENGTH = 50000;
const int TCRUNCHER_ROW_HEIGHT_ADD = 10;


/*
 *	Grid visual appearance
 */
const int TCRUNCHER_INITIAL_FONT_SIZE = 14;
const int TCRUNCHER_MAX_FONT_SIZE = 30;
const int TCRUNCHER_MIN_FONT_SIZE = 8;
const int TCRUNCHER_FONT_NUMBER = 9900;
#define TCRUNCHER_FALLBACK_FONT "Menlo"


/************************************************************************************
*
*	Class and Struct definitions
*
************************************************************************************/


/**
 *	Data structure that stores all the informations used to recreate a CSV file from a vec-of-vec table.
 */ 
class CsvDefinition {
public:
	enum Encodings {
		ENC_NONE,										// no known encoding was detected
		ENC_UTF8,
		ENC_UTF16LE,
		ENC_UTF16BE,
		ENC_UTF32LE,
		ENC_UTF32BE,
		ENC_Latin1,
		ENC_Latin9,
		ENC_Win1252
	};
	enum BOMs {
		BOM_NONE,										// no BOM has been detected
		BOM_UTF8,
		BOM_UTF16LE,
		BOM_UTF16BE,
		BOM_UTF32LE,
		BOM_UTF32BE
  	};
	enum QuoteStyles {
		QUOTE_STYLE_RFC,								// Just quote when necessary as defined by RFC 4180
		QUOTE_STYLE_ALL,								// Quote all cells
		QUOTE_STYLE_STRING								// Quote all cells that are non-numeric
	};
	Encodings encoding = ENC_UTF8;					// encoding of the connected file
	char delimiter = ',';							// Delimiter
	char quote = '"';								// Quote character
	char escape = '"';								// Escape character
	int bomBytes = 0;								// length of a ByteOrderMarker (zero if no BOM present)
	const char *linebreak = "\n";					// linebreak
	bool cancelled = false;							// Used by setTypeByUser() to signal a user abortion
	QuoteStyles quoteStyle = QUOTE_STYLE_RFC;		// the quote style used for exports
	
	static const char *getEncodingName(Encodings encoding) {
		const char *str = "";
		switch( encoding ) {
			case ENC_UTF8:
				str = "UTF-8";
			break;
			case ENC_Latin1:
				str = "Latin-1";
			break;
			case ENC_Win1252:
				str = "Win1252";
			break;
			case ENC_UTF16LE:
				str = "UTF-16LE";
			break;
			case ENC_UTF16BE:
				str = "UTF-16BE";
			break;
			case ENC_UTF32LE:
			case ENC_UTF32BE:
			case ENC_Latin9:
			case ENC_NONE:
				str = "NONE";
			break;
		}
		return str;
	}
	static const char *getDelimiterName(char delimiter) {
		const char *str = "undef";
		switch(delimiter) {
			case ',':
				str = "COMMA";
			break;
			case ';':
				str = "SEMI";
			break;
			case '\t':
				str = "TAB";
			break;
			case ':':
				str = "COLON";
			break;
			case '|':
				str = "PIPE";
			break;
			case '*':
				str = "ASTER";
			break;
			default:
			break;
		}
		return str;
	}
};





#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: " __VA_ARGS__)
#else
    #define DEBUG_PRINTF(...) do {} while (0)
#endif




#endif




