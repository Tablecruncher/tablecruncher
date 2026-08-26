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

#include "csvguess.hh"

#include <algorithm>
#include <cstdio>

#include "csvparser.hh"
#include "utf8.h"


namespace {
	const int MAXLINES = 10;			// lines read per candidate dialect

}


/*
 *	Returns {Number of Cols, Some kind of Variance} of the data in localStorage
 *	Variance: number of rows that are shorter than the longest row
 */
std::pair<table_index_t, table_index_t> CsvGuess::tableStatistics(CsvDataStorage& localStorage) {
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
 *	Counts fields holding an ODD number of quote characters.
 *
 *	This is what tells a separator apart from a character that merely occurs inside the data.
 *	Parsed with the right dialect, a quoted field has its quotes consumed and an unquoted one
 *	carries them in balanced pairs. Parsed with a delimiter that happens to appear INSIDE
 *	quoted content — ':' in a column of JSON, say — the parser slices through the quoting and
 *	leaves orphans behind in nearly every field.
 */
std::pair<long, long> CsvGuess::quoteSanity(CsvDataStorage& localStorage, char quote) {
	long totalFields  = 0;
	long orphanFields = 0;
	for( table_index_t r = 0; r < localStorage.rows(); ++r ) {
		for( const std::string& field : localStorage.rawRow(r) ) {
			long quotes = 0;
			for( char c : field ) {
				if( c == quote ) ++quotes;
			}
			++totalFields;
			if( quotes & 1 ) ++orphanFields;
		}
	}
	return {totalFields, orphanFields};
}


/*
 *	The eight dialects that get probed.
 *
 *	NOTE a long-standing quirk, preserved: entry 6's delimiter is assigned twice, so ';' with
 *	a backslash escape is never actually probed and entry 7 stays at the default (a duplicate
 *	of entry 0). Changing it would change which dialect wins for some files.
 */
std::vector<CsvGuess::Probe> CsvGuess::makeProbes() {
	std::vector<Probe> probes(8);
	probes.at(0).definition.delimiter = ',';
	probes.at(1).definition.delimiter = ';';
	probes.at(2).definition.delimiter = '\t';
	probes.at(3).definition.delimiter = '|';
	probes.at(4).definition.delimiter = ':';
	probes.at(5).definition.delimiter = ',';
	probes.at(6).definition.delimiter = ';';
	probes.at(6).definition.delimiter = '*';
	probes.at(5).definition.escape = '\\';
	probes.at(6).definition.escape = '\\';
	return probes;
}


/*
 *	Measures one probe's parse and fills in everything the ranking looks at.
 */
void CsvGuess::scoreProbe(Probe& probe, CsvDataStorage& localStorage) {
	std::pair<int,int>   statistics = tableStatistics(localStorage);
	std::pair<long,long> sanity     = quoteSanity(localStorage, probe.definition.quote);
	const int            maxCols    = statistics.first;		// before the penalty below rewrites it

	// not so commonly used seperators and escape characters: decrease statistics value
	if(
		probe.definition.delimiter == ':' ||
		probe.definition.delimiter == '|' ||
		probe.definition.escape == '\\' ||
			probe.definition.escape == '*'
	) {
		statistics.first = statistics.first * 70 / 100;
	}
	probe.score = statistics.first;

	if( statistics.first <= 1 && statistics.second == 0) {
		// if statistics is (1,0), sort it at the end
		probe.variance = 999;
	} else {
		probe.variance = statistics.second;
	}

	// Does the first row hold as many fields as the rest? tableStatistics() deliberately skips
	// row 0, so on its own it cannot see a delimiter that splits the header into one field and
	// every data row into eighty-seven. Well-formed CSV agrees across all rows, header or not.
	probe.headerMismatch = 0;
	if( localStorage.rows() > 1 && (int) localStorage.rawRow(0).size() != maxCols ) {
		probe.headerMismatch = 1;
	}

	probe.orphanPercent = ( sanity.first > 0 )
		? (int)( (sanity.second * 100) / sanity.first )
		: 0;

	#ifdef DEBUG
	printf("CSV = '%c' => cols %d / var %d / header %s / orphan quotes %d%%\n",
	       probe.definition.delimiter, probe.score, probe.variance,
	       probe.headerMismatch ? "MISMATCH" : "ok", probe.orphanPercent);
	#endif
}


/*
 *	Sorts the probes and derives a confidence value for the winner.
 */
std::pair<CsvDefinition, float> CsvGuess::rankProbes(std::vector<Probe>& probes) {
	//
	//	Least variance first, exactly as before. The two signals after it only ever break a
	//	TIE, so no candidate that uniquely explains the row lengths can be displaced by them:
	//	  - a delimiter that disagrees with the first row is separating something other than
	//	    fields (this is what a column of JSON does to ':')
	//	  - a delimiter that leaves orphaned quotes behind is slicing through quoted content
	//	Only then does the historical "more columns wins" tie-break apply.
	//
	std::sort(begin(probes), end(probes), [](const Probe& a, const Probe& b) {
		if( a.variance != b.variance )             return a.variance < b.variance;
		if( a.headerMismatch != b.headerMismatch ) return a.headerMismatch < b.headerMismatch;
		if( a.orphanPercent != b.orphanPercent )   return a.orphanPercent < b.orphanPercent;
		return a.score > b.score;
	});

	float confidence = 1.0;
	// if there's no definition with zero variance: reduce confidence
	if( probes[0].variance > 0 ) {
		confidence /= 2;
	}
	// if there are at least two definitions with the same number of columns: reduce confidence
	if( probes[0].score == probes[1].score ) {
		confidence /= 2;
	}
	// improve confidence, if it's a typical CSV separator
	if( probes.at(0).definition.delimiter == ',' || probes.at(0).definition.delimiter == '\t' ) {
		confidence += (1.0 - confidence) * 0.5;
	}
	return { probes.at(0).definition, confidence };
}


/*
 *	Guesses the dialect of an already-mapped buffer. Saves eight full re-reads of the file's
 *	head plus the seekg() churn, and is what the load path uses.
 */
std::pair<CsvDefinition, float> CsvGuess::definition(const char* data, uint64_t len) {
	CsvDataStorage localStorage;
	std::vector<Probe> probes = makeProbes();

	for( Probe& probe : probes ) {
		CsvParser parser;
		localStorage.clear();
		parser.parseCsvBuffer( data, len, localStorage, &probe.definition, MAXLINES, false );
		scoreProbe( probe, localStorage );
	}
	return rankProbes(probes);
}


/*
 *	Same, for a stream. Used when the file could not be mapped, and by the paste path.
 */
std::pair<CsvDefinition, float> CsvGuess::definition(std::istream* input) {
	CsvDataStorage localStorage;
	std::vector<Probe> probes = makeProbes();

	for( Probe& probe : probes ) {
		CsvParser parser;
		input->clear();
		input->seekg(0);
		localStorage.clear();
		parser.parseCsvStream( input, localStorage, &probe.definition, MAXLINES, false );
		scoreProbe( probe, localStorage );
	}

	input->clear();
	input->seekg(0);
	return rankProbes(probes);
}


/*
 *	Encoding, and the length of any byte-order mark.
 *
 *	Validation is multi-threaded here, so – unlike the istream overload – there is no
 *	TCRUNCHER_NUM_UTF8_TEST_BYTES size cap. A UTF-8 file above that cap used to be reported as
 *	ENC_NONE, which forced the "choose your format" modal on open; it now simply opens. The
 *	parsed bytes are unchanged either way, because ENC_NONE and ENC_UTF8 both route through
 *	Helper::fixUtf8().
 *
 *	The UTF-8 check deliberately starts at byte offset 4, reproducing the istream version,
 *	where four read() calls consume 4 bytes before utf8::is_valid() runs on the
 *	already-constructed iterator. That is why an ASCII UTF-16 file WITH a BOM is reported as
 *	UTF-8 – it then parses correctly only because the reader strips NUL bytes. Do not "fix"
 *	this; it changes how real files open.
 */
std::pair<CsvDefinition::Encodings, int> CsvGuess::encoding(const char* data, uint64_t len,
                                                            Utf8ValidationResult& validationOut) {
	CsvDefinition::Encodings enc = CsvDefinition::ENC_NONE;
	int bomBytes = 0;
	unsigned char octet[4] = {0, 0, 0, 0};

	for( uint64_t i = 0; i < 4 && i < len; ++i )
		octet[i] = (unsigned char) data[i];

	enc = CsvDefinition::fromBom(octet, bomBytes);

	validationOut = validateUtf8Parallel(data, len, bomBytes);
	if( validationOut.validFrom4 ) {
		enc = CsvDefinition::ENC_UTF8;
	}

	return std::make_pair(enc, bomBytes);
}
