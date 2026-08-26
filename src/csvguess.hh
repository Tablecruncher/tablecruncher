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

#ifndef _CSVGUESS_HH
#define _CSVGUESS_HH

//
//	Working out a file's CSV dialect and encoding.
//
//	This lived in CsvApplication, but none of it touches FLTK — and getting it wrong is how a
//	file silently opens with the wrong number of columns, so it needs to be testable without
//	a UI.
//

#include <cstdint>
#include <istream>
#include <tuple>
#include <utility>
#include <vector>

#include "globals.hh"
#include "csvdatastorage.hh"
#include "utf8validate.hh"


namespace CsvGuess {

	// A candidate dialect together with the numbers it is ranked on, best first.
	struct Probe {
		CsvDefinition definition;
		int variance       = 0;		// rows shorter than the longest; 999 means "sort me last"
		int headerMismatch = 0;		// 1 when row 0 holds a different number of fields than the rest
		int orphanPercent  = 0;		// share of fields left holding an odd number of quote characters
		int score          = 0;		// number of columns, after the unusual-delimiter penalty
	};

	// (number of columns, number of rows shorter than the longest)
	std::pair<table_index_t, table_index_t> tableStatistics(CsvDataStorage& localStorage);

	// How many of the probed fields carry an ODD number of quote characters. A delimiter that
	// cuts through quoted content leaves orphaned quotes behind, which is the clearest signal
	// that a candidate is slicing up a field rather than separating fields.
	std::pair<long, long> quoteSanity(CsvDataStorage& localStorage, char quote);

	std::vector<Probe> makeProbes();
	// Measures `localStorage` — the result of parsing with this probe's dialect — and fills in
	// everything the ranking looks at.
	void scoreProbe(Probe& probe, CsvDataStorage& localStorage);
	std::pair<CsvDefinition, float> rankProbes(std::vector<Probe>& probes);

	// The dialect the data most likely uses, with a confidence in [0, 1].
	std::pair<CsvDefinition, float> definition(const char* data, uint64_t len);
	std::pair<CsvDefinition, float> definition(std::istream* input);

	// The encoding, and the length of the byte-order mark (0 when there is none).
	std::pair<CsvDefinition::Encodings, int> encoding(const char* data, uint64_t len,
	                                                  Utf8ValidationResult& validationOut);
}

#endif
