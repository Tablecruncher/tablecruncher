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


#include "macro.hh"

extern Macro macro;

Macro::Macro() {
	context = duk_create_heap(NULL, NULL, NULL, NULL, fatalErrorHandler);
	// documented external functions
	duk_push_c_function(context, &Macro::apiSetCell, DUK_VARARGS);
	duk_put_global_string(context, "setCell");
	duk_push_c_function(context, &Macro::apiGetString, DUK_VARARGS);
	duk_put_global_string(context, "getString");
	duk_push_c_function(context, &Macro::apiGetInt, DUK_VARARGS);
	duk_put_global_string(context, "getInt");
	duk_push_c_function(context, &Macro::apiGetFloat, DUK_VARARGS);
	duk_put_global_string(context, "getFloat");
	duk_push_c_function(context, &Macro::apiFlagRow, DUK_VARARGS);
	duk_put_global_string(context, "flagRow");
	duk_push_c_function(context, &Macro::apiPrintln, DUK_VARARGS);
	duk_put_global_string(context, "println");
	// internal functions
	duk_push_c_function(context, &Macro::internalStoreInts, DUK_VARARGS);
	duk_put_global_string(context, "__storeInts");
	duk_push_c_function(context, &Macro::internalStoreStrings, DUK_VARARGS);
	duk_put_global_string(context, "__storeStrings");
	duk_push_c_function(context, &Macro::internalStoreStrings, DUK_VARARGS);
	duk_put_global_string(context, "__println");
}


Macro::~Macro() {
	duk_destroy_heap(context);
}

void *Macro::logDisplay = NULL;
bool Macro::tableModified = false;
std::vector<int> Macro::lastResultInts = {};
std::vector<std::string> Macro::lastResultStrings = {};

	
/*
 *	Execute macro code in `source`.
 *
 *	@return		<int retCode, string errMessage>
 *				retCode: 0 => no modification made, 1 => grid data modified, -1 => error happened 
 *
 *	TODO call duk_peval_string() in its own thread to avoid endless loops
 *	https://stackoverflow.com/a/56268886/2771733
 */
std::tuple<int, std::string> Macro::execute(CsvTable *table, std::tuple<int, int, int, int> selection, std::string source, void *logDisplay) {
	int retCode = 0;
	this->table = table;
	Macro::logDisplay = logDisplay;
	std::string errMessage = "";
	Macro::tableModified = false;
	source = "var ROWMIN = " + std::to_string(std::get<0>(selection)) + "; var COLMIN = " + std::to_string(std::get<1>(selection)) + "; var ROWMAX = " + std::to_string(std::get<2>(selection)) + "; var COLMAX = " + std::to_string(std::get<3>(selection)) + ";\n" + source;
	source = "var _ROWS = " + std::to_string(table->getNumberRows()) + "; var _COLS = " + std::to_string(table->getNumberCols()) + "; " + source;
	int error = duk_peval_string(context, source.c_str());
	if( error ) {
		errMessage = duk_safe_to_string(context, -1);
		retCode = -1;
	} else if( Macro::tableModified ) {
		retCode = 1;
	}
	return std::make_tuple(retCode,errMessage);
}


/*
 *
 */
duk_ret_t Macro::apiSetCell(duk_context *context) {
	std::string str;
	duk_idx_t numArgs = duk_get_top(context);
	if( numArgs == 3 ) {
		if( duk_get_type(context, 0) == DUK_TYPE_NUMBER && duk_get_type(context, 1) == DUK_TYPE_NUMBER) {
			duk_uint_t row = duk_to_uint(context, 0);
			duk_uint_t col = duk_to_uint(context, 1);
			if( duk_get_type(context, 2) == DUK_TYPE_STRING ) {
				str = duk_safe_to_string(context, 2);
			} else if( duk_get_type(context, 2) == DUK_TYPE_NUMBER ) {
				str = std::to_string(duk_to_number(context, 2));
				str.erase(str.find_last_not_of("0")+1);
				str.erase(str.find_last_not_of(".")+1);
			} else {
				str = "";
			}
			macro.table->setCell(str, row, col);
			Macro::tableModified = true;
		}
	}
	return 0;
}


duk_ret_t Macro::apiGetString(duk_context *context) {
	duk_idx_t numArgs = duk_get_top(context);
	if( numArgs == 2 ) {
		if( duk_get_type(context, 0) == DUK_TYPE_NUMBER && duk_get_type(context, 1) == DUK_TYPE_NUMBER ) {
			duk_uint_t row = duk_to_uint(context, 0);
			duk_uint_t col = duk_to_uint(context, 1);
			std::string cell_str = macro.table->getCell(row, col);
			duk_push_string(context, cell_str.c_str());
			return 1;
		}
	}
	return 0;
}
duk_ret_t Macro::apiGetInt(duk_context *context) {
	duk_idx_t numArgs = duk_get_top(context);
	if( numArgs == 2 ) {
		if( duk_get_type(context, 0) == DUK_TYPE_NUMBER && duk_get_type(context, 1) == DUK_TYPE_NUMBER ) {
			duk_uint_t row = duk_to_uint(context, 0);
			duk_uint_t col = duk_to_uint(context, 1);
			std::string cell_str = macro.table->getCell(row, col);
			try {
				long cell_int = std::stol(cell_str);
				duk_push_int(context, cell_int);
			} catch(...) {
				return 0;
			}
			return 1;
		}
	}
	return 0;
}
duk_ret_t Macro::apiGetFloat(duk_context *context) {
	duk_idx_t numArgs = duk_get_top(context);
	if( numArgs == 2 ) {
		if( duk_get_type(context, 0) == DUK_TYPE_NUMBER && duk_get_type(context, 1) == DUK_TYPE_NUMBER ) {
			duk_uint_t row = duk_to_uint(context, 0);
			duk_uint_t col = duk_to_uint(context, 1);
			std::string cell_str = macro.table->getCell(row, col);
			try {
				double cell_float = std::stod(cell_str);
				duk_push_number(context, cell_float);
			} catch(...) {
				return 0;
			}
			return 1;
		}
	}
	return 0;
}

/**
 *	Flags the given row
 */
duk_ret_t Macro::apiFlagRow(duk_context *context) {
	duk_idx_t numArgs = duk_get_top(context);
	if( numArgs == 1 ) {
		duk_uint_t row = duk_to_uint(context, 0);
		macro.table->flagRow(row, true);
		return 1;
	}
	return 0;
}


/**
 *	Prints the given arguments into the log area
 */
duk_ret_t Macro::apiPrintln(duk_context *context) {
	std::string str;
	duk_idx_t numArgs = duk_get_top(context);
	for( int i = 0; i < numArgs; ++i ) {
		if( duk_get_type(context, i) == DUK_TYPE_STRING ) {
			str = duk_safe_to_string(context, i);
		} else if( duk_get_type(context, i) == DUK_TYPE_NUMBER ) {
			str = std::to_string(duk_to_number(context, i));
			str.erase(str.find_last_not_of("0")+1);
			str.erase(str.find_last_not_of(".")+1);
		} else {
			str = "";
		}
		if( Macro::logDisplay ) {
			updateMacroLogBuffer(Macro::logDisplay, str);
		}
	}
	if( Macro::logDisplay ) {
		updateMacroLogBuffer(Macro::logDisplay, " \n");
	}
	return 0;
}





/**
 *	Stores the given arguments as Ints into lastResultInts.
 */
duk_ret_t Macro::internalStoreInts(duk_context *context) {
	// int dummyInt;
	duk_idx_t numArgs = duk_get_top(context);
	Macro::lastResultInts.clear();
	for( int i = 0; i < numArgs; ++i ) {
		if( duk_get_type(context, i) == DUK_TYPE_NUMBER ) {
			// dummyInt = duk_to_int(context, i);
			Macro::lastResultInts.push_back(duk_to_int(context, i));
		}
	}
	return 0;
}

/**
 *	Returns the last stored Int results
 */
std::vector<int> Macro::getLastResultInts() {
	return Macro::lastResultInts;
}


/**
 *	Stores the given arguments as Strings into lastResultStrings.
 */
duk_ret_t Macro::internalStoreStrings(duk_context *context) {
	duk_idx_t numArgs = duk_get_top(context);
	Macro::lastResultStrings.clear();
	for( int i = 0; i < numArgs; ++i ) {
		if( duk_get_type(context, i) == DUK_TYPE_STRING ) {
			// dummyInt = duk_to_int(context, i);
			Macro::lastResultStrings.push_back(duk_to_string(context, i));
		}
	}
	return 0;
}

std::vector<std::string> Macro::getLastResultStrings() {
	return Macro::lastResultStrings;
}


/**
 *	Prints the given arguments to stdout
 */
duk_ret_t Macro::internalPrintln(duk_context *context) {
	std::string str;
	duk_idx_t numArgs = duk_get_top(context);
	for( int i = 0; i < numArgs; ++i ) {
		if( duk_get_type(context, i) == DUK_TYPE_STRING ) {
			str = duk_safe_to_string(context, i);
		} else if( duk_get_type(context, i) == DUK_TYPE_NUMBER ) {
			str = std::to_string(duk_to_number(context, i));
			str.erase(str.find_last_not_of("0")+1);
			str.erase(str.find_last_not_of(".")+1);
		} else {
			str = "";
		}
		printf("%s\n", str.c_str()); fflush(stdout);
	}
	return 0;
}


void Macro::fatalErrorHandler(void *, const char *) {
	#ifdef DEBUG
	std::cerr << "Fatal JS Error!" << std::endl;
	#endif
}


