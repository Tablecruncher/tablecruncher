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


#ifndef _CSVPARSER_HH
#define _CSVPARSER_HH


#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <istream>
#include <cstdint>
#include <inttypes.h>
#include <atomic>
#include <functional>

// for reading utf16
#include <locale>
#include <codecvt>


#include "helper.hh"
#include "globals.hh"
#include "csvdatastorage.hh"
#include "csvfsm.hh"




/**
 * \brief The custom CSV parser class.
 * 
 * Implements a finite state machine to parse the CSV data provided by a `std::istream` object.
 * 
 */
class CsvParser {
	typedef struct {
		int octet;
		union {
			uint16_t codeUnit16;
			uint32_t codeUnit32;
		};
	} codeUnitReturn_t;
public:
	std::map<long,long> parseCsvStream( std::istream *input, CsvDataStorage &storage, CsvDefinition *definition, int maxLines=0, bool resizeRows=true );

	//
	//	Same parse, but over a flat buffer (a memory mapping) instead of an istream. Produces
	//	byte-identical output for every encoding bufferPathSupports() accepts, while avoiding
	//	the byte-at-a-time streambuf reads and – for the common all-valid-UTF-8, no-NUL case –
	//	copying each line at all: the state machine reads straight out of the mapping.
	//
	// `assumeValidUtf8`: the caller has already proven [bomBytes, len) is valid UTF-8, so
	// Helper::fixUtf8() is provably the identity and the per-line validity check – a second
	// full pass over the file – can be skipped.
	std::map<long,long> parseCsvBuffer( const char* data, uint64_t len, CsvDataStorage &storage, CsvDefinition *definition, int maxLines=0, bool resizeRows=true, bool assumeValidUtf8=false );
	static bool bufferPathSupports(CsvDefinition::Encodings enc);		// UTF-16/32 need the code-unit reader, so they stay on the stream

	/**
	 *	One self-contained slice of a buffer.
	 *
	 *	A slice always starts on a record boundary. Non-final slices stop cleanly at `to`,
	 *	which must be a physical line start; only the final slice applies end-of-input
	 *	semantics (the trailing phantom record, and leaving the last record out of the
	 *	histogram). No slice ever deletes the trailing row – the caller does that once, after
	 *	all slices have been concatenated.
	 */
	struct RangeOptions {
		uint64_t from       = 0;
		uint64_t to         = 0;			// exclusive
		bool     finalRange = true;			// `to` is the end of the input
		int      maxLines   = 0;
		bool     resizeRows = true;
		bool     assumeValidUtf8 = false;		// see parseCsvBuffer()
	};
	// `endedEnclosed`, when given, reports whether the slice stopped inside a quoted field.
	std::map<long,long> parseCsvRange( const char* data, const RangeOptions& opt, CsvDataStorage &storage,
	                                   CsvDefinition *definition, bool* endedEnclosed = nullptr );

	//
	//	Progress / cancellation hooks.
	//
	//	CsvParser must stay free of FLTK so that it can also run on a worker thread (see
	//	docs/dev/parallel-loading-plan.md). The owning thread injects whatever it needs here.
	//
	std::function<void(long)> onProgress;					// called every 25,000 rows – owning thread ONLY (serial path)
	std::atomic<long>* sharedRowCounter = nullptr;			// incremented in batches of 4096 – safe to read from another thread
	std::atomic<bool>* cancelRequested  = nullptr;			// polled every 4096 rows; parsing stops cooperatively when set

	bool rowLimitExceeded = false;							// set when parsing stopped because the table hit INT_MAX rows

	static CsvDialect makeDialect(const CsvDefinition& definition);		// CsvDefinition -> what the FSM branches on
	static std::string transcodeToUtf8(CsvDefinition::Encodings enc, const std::string& line);	// Latin-1 / Win1252 -> UTF-8; everything else passes through
	static size_t estimateRowCount(std::istream& input, int64_t fileLength);	// samples the first 64 KB; for reserveRows()
	static size_t estimateRowCount(const char* data, uint64_t len);				// ditto, over a mapped buffer
	static size_t clampRowEstimate(double estimate, uint64_t bytes);			// >= 1 row, and never more than the memory budget allows

private:
	// Everything a parse loop accumulates while walking records, plus the two settings that
	// are fixed for the whole parse. Shared so the stream path and the buffer path cannot
	// drift apart in their bookkeeping.
	struct CommitState {
		long act_rows   = 0;
		long act_cols   = 0;
		int  maxLines   = 0;
		bool resizeRows = true;
		std::vector<std::pair<long,long>> flatHisto;	// row-length histogram; 1-3 entries typically
	};
	// Appends one finished record. Returns false when parsing must stop.
	bool commitRecord(CsvDataStorage &storage, std::string &rowStr, long fieldCount,
	                  bool isLastLine, CommitState &st);

	static std::istream& myGetline(std::istream& is, std::string& t);
	static std::istream& myGetlineEncodings(std::istream& is, std::string& t, CsvDefinition::Encodings enc);
	static codeUnitReturn_t getNextCodeUnit(std::streambuf *sb, int unitLength, bool bigEndian=true);
};



#endif


