#ifndef _MACRO_HH
#define _MACRO_HH

#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "duktape/duktape.h"

#include "csvtable.hh"


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

