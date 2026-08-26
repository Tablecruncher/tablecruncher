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

#ifndef _CSVLOADER_HH
#define _CSVLOADER_HH

//
//	Multi-threaded CSV load over a mapped buffer.
//
//	The whole design turns on one observation: at a PHYSICAL LINE START the state machine's
//	structural state is exactly one bit – `enclosed`. `startField` is unconditionally true
//	there, and nothing else survives a line boundary. A one-bit transfer function composes in
//	O(k), so each chunk can be prescanned independently under both hypotheses (carry-in false
//	and true) and the true carry chain resolved serially afterwards. No guessing, no verify
//	round, no probabilistic fallback.
//
//	No FLTK and no iostream anywhere below – this runs on worker threads.
//

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "globals.hh"
#include "csvdatastorage.hh"
#include "utf8validate.hh"


/**
 *	Which engine reads the file.
 *
 *	Ordered by capability: every Parallel-eligible file is also Buffer-eligible, and planLoad()
 *	derives it that way rather than keeping a second encoding whitelist in step with the first.
 */
enum class LoadPath {
	Stream,			// istream + the code-unit reader – UTF-16/32, or no mapping available
	Buffer,			// the mapped buffer, single threaded
	Parallel		// the mapped buffer, prescanned and parsed across all cores
};


/**
 *	The single place that decides which engine runs, and why.
 */
struct LoadPlan {
	LoadPath    path       = LoadPath::Stream;
	unsigned    threads    = 1;
	size_t      chunkCount = 1;
	const char* reason     = "";			// why not the faster path – shown in DEBUG builds

	// Test hook, not used by the application: force these nominal split points instead of an
	// even division. Lets the harness put a chunk boundary on every byte offset in turn.
	std::vector<uint64_t> forcedNominalOffsets;
};


/**
 *	Shared between the coordinator thread and the main thread. The main thread owns every
 *	widget; workers only ever touch these atomics.
 */
struct LoadProgress {
	std::atomic<long> rowsDone{0};
	std::atomic<bool> cancelRequested{false};
	std::atomic<bool> finished{false};		// set by loadParallel() on every exit path
};


struct LoadResult {
	std::map<long,long> histogram;
	bool cancelled        = false;
	bool rowLimitExceeded = false;		// more rows than table_index_t can address – nothing was loaded
	bool failed           = false;		// ran out of memory, or the parse threw
};


namespace CsvLoader {

	// Files below this stay serial: the serial fast path already finishes in well under
	// 60 ms and the fixed costs of fanning out would dominate.
	const uint64_t MIN_PARALLEL_BYTES = 8ull * 1024 * 1024;

	// `forceParallel` is a TEST HOOK: it waives the file-size and core-count thresholds. It
	// deliberately does NOT waive the correctness gates (encoding, UTF-8 validity, ASCII
	// structural characters) – those must refuse a file no matter who is asking.
	LoadPlan planLoad(const CsvDefinition& def, uint64_t fileLength, const Utf8ValidationResult& validation,
	                  bool haveBuffer, int maxLines, bool resizeRows, bool forceParallel = false);

	LoadResult loadParallel(const char* data, uint64_t len, const CsvDefinition& definition,
	                        const LoadPlan& plan, CsvDataStorage& storage, LoadProgress& progress);
}

#endif
