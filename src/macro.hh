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


#ifndef _MACRO_HH
#define _MACRO_HH

#include "csvtable.hh"

#include <string>
#include <vector>

#include <duktape/duktape.h>


extern void updateMacroLogBuffer(void *logBuffer, std::string str);


class Macro {
public:
	Macro();
	~Macro();
	std::tuple<int, std::string> execute(CsvTable *table, std::tuple<int, int, int, int> selection, std::string source, void *logDisplay = NULL);
	static duk_ret_t apiSetCell(duk_context *context);
	static duk_ret_t apiGetString(duk_context *context);
	static duk_ret_t apiGetInt(duk_context *context);
	static duk_ret_t apiGetFloat(duk_context *context);
	static duk_ret_t apiFlagRow(duk_context *context);
	static duk_ret_t apiPrintln(duk_context *context);
	static duk_ret_t internalStoreInts(duk_context *context);		// place to store results from the macro to be used by the application internally
	static duk_ret_t internalStoreStrings(duk_context *context);	// place to store results from the macro to be used by the application internally
	static duk_ret_t internalPrintln(duk_context *context);			// prints to stdout
	static void fatalErrorHandler(void *, const char *);
	static std::vector<int> getLastResultInts();
	static std::vector<std::string> getLastResultStrings();
private:
	duk_context *context;
	CsvTable *table;
	static bool tableModified;
	static void *logDisplay;
	static std::vector<int> lastResultInts;					
	static std::vector<std::string> lastResultStrings;
};




#endif

