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

#include "csvloader.hh"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "csvfsm.hh"
#include "csvparser.hh"
#include "parallelutil.hh"


namespace {

const uint64_t NO_RECORD_START = UINT64_MAX;
const size_t   MAX_CHUNKS      = 256;
const uint64_t TARGET_CHUNK    = 8ull << 20;
// Chunks per thread. Efficiency cores are ~2.5x slower, so one chunk per thread leaves the
// performance cores idle through the whole tail; oversubscribing lets work stealing even it
// out. Extra chunks are nearly free — prescan cost is per byte, not per chunk.
const size_t   CHUNKS_PER_THREAD = 4;


/*
 *	First physical line start strictly after `from`.
 *
 *	Unambiguous at byte level, which is what makes stateless splitting possible: a '\n' inside
 *	a quoted field is still a '\n' byte, and a nominal offset landing on the '\n' of a CRLF
 *	yields the same start the real reader would use.
 */
uint64_t nextLineStart(const char* data, uint64_t len, uint64_t from) {
	uint64_t i = from;
	while( i < len && data[i] != '\n' && data[i] != '\r' )
		++i;
	if( i >= len )
		return len;
	if( data[i] == '\r' && i + 1 < len && data[i+1] == '\n' )
		return i + 2;
	return i + 1;
}


struct ChunkPrescan {
	uint64_t begin = 0;
	uint64_t end   = 0;
	bool     endEnclosed[2]      = { false, false };			// indexed by the carry-in hypothesis
	uint64_t firstRecordStart[2] = { NO_RECORD_START, NO_RECORD_START };
};


/*
 *	Runs the structural state machine over one chunk under BOTH carry-in hypotheses at once.
 *
 *	The two runs converge almost immediately – a single quote character resynchronises them –
 *	so they are executed lock-step and the second is dropped the moment the two `enclosed`
 *	bits agree at a line boundary. Real cost is around 1.05x a single scan, fully parallel.
 */
void prescanChunk(const char* data, const CsvDialect& dialect, ChunkPrescan& out) {
	StructuralSink sink;
	std::string    scratch;

	// NUL removal shifts both character adjacency and the "last character of the line" test,
	// so the prescan has to strip them exactly like the parse does. Checking once per chunk
	// keeps the common (NUL-free) case free.
	const bool chunkHasNul = csvRangeHasNul(data, out.begin, out.end);

	bool encA = false;					// hypothesis: carry-in NOT enclosed
	bool encB = true;					// hypothesis: carry-in enclosed
	bool converged = false;
	uint64_t rsA = NO_RECORD_START;
	uint64_t rsB = NO_RECORD_START;

	CsvLineCursor cursor(data, out.begin, out.end);
	const char* lineBegin = nullptr;
	const char* lineEnd   = nullptr;
	bool        reachedEnd = false;

	for(;;) {
		const uint64_t lineStart = cursor.pos();
		// a record starts at any line start where the machine is not inside a quoted field
		if( !encA && rsA == NO_RECORD_START ) rsA = lineStart;
		if( !encB && rsB == NO_RECORD_START ) rsB = lineStart;

		if( !cursor.next(lineBegin, lineEnd, reachedEnd) )
			break;

		const char* p = lineBegin;
		const char* e = lineEnd;
		if( chunkHasNul )
			csvStripNuls(lineBegin, lineEnd, scratch, p, e);

		csvScanLine(p, e, dialect, encA, sink);
		if( !converged ) {
			csvScanLine(p, e, dialect, encB, sink);
			if( encA == encB )
				converged = true;
		} else {
			encB = encA;
		}
	}

	out.endEnclosed[0] = encA;
	out.endEnclosed[1] = encB;
	out.firstRecordStart[0] = rsA;
	out.firstRecordStart[1] = rsB;
}


} // namespace


/*
 *	One function, one place, one decision.
 */
LoadPlan CsvLoader::planLoad(const CsvDefinition& def, uint64_t fileLength,
                             const Utf8ValidationResult& validation,
                             bool haveBuffer, int maxLines, bool resizeRows, bool forceParallel) {
	LoadPlan plan;

	//
	//	Stream vs buffer first. Deriving the parallel gate from bufferPathSupports() rather
	//	than repeating an encoding list keeps "parallel is a subset of buffer" structural.
	//
	if( !haveBuffer ) {
		plan.reason = "no mapped buffer";
		return plan;
	}
	if( !CsvParser::bufferPathSupports(def.encoding) ) {
		plan.reason = "encoding needs the code-unit reader";
		return plan;
	}
	plan.path = LoadPath::Buffer;

	// the escape hatch you tell a bug reporter to flip
	if( std::getenv("TCRUNCHER_DISABLE_PARALLEL") ) {
		plan.reason = "disabled by TCRUNCHER_DISABLE_PARALLEL";
		return plan;
	}
	// probing, preview and paste always stay serial – loadParallel() cannot honour maxLines
	if( maxLines != 0 || !resizeRows ) {
		plan.reason = "probing / preview path";
		return plan;
	}
	// Latin-1/Latin-9/Win1252 are buffer-capable but transcode by DROPPING bytes 0x80-0x9F,
	// which changes line length and therefore the escape lookahead – a raw-byte prescan is
	// not faithful for them
	if( def.encoding != CsvDefinition::ENC_UTF8 && def.encoding != CsvDefinition::ENC_NONE ) {
		plan.reason = "encoding transcodes by dropping bytes";
		return plan;
	}
	// The gate. Helper::fixUtf8() is provably the identity only when the parsed region is
	// wholly valid UTF-8; otherwise utf8::replace_invalid can DISCARD up to three trailing
	// bytes of a line, which can swallow a closing quote and flip the enclosed state.
	if( !validation.validFromBom ) {
		plan.reason = "parsed region is not valid UTF-8";
		return plan;
	}
	// structural characters must be plain ASCII and must not collide with a line terminator
	const char specials[3] = { def.delimiter, def.quote, def.escape };
	for( char c : specials ) {
		if( ((unsigned char) c) >= 0x80 || c == '\n' || c == '\r' || c == '\0' ) {
			plan.reason = "delimiter/quote/escape is not a safe ASCII byte";
			return plan;
		}
	}
	if( fileLength < MIN_PARALLEL_BYTES && !forceParallel ) {
		plan.reason = "file too small to be worth splitting";
		return plan;
	}

	const unsigned n = Parallel::threadCount();
	if( n < 2 && !forceParallel ) {
		plan.reason = "not enough cores";
		return plan;
	}

	plan.path       = LoadPath::Parallel;
	plan.threads    = n;
	plan.chunkCount = std::min<size_t>(MAX_CHUNKS,
	                      std::max<size_t>(n * CHUNKS_PER_THREAD, (size_t)(fileLength / TARGET_CHUNK)));
	return plan;
}


LoadResult CsvLoader::loadParallel(const char* data, uint64_t len, const CsvDefinition& definition,
                                   const LoadPlan& plan, CsvDataStorage& storage, LoadProgress& progress) {
	LoadResult result;

	// Whoever is pumping the UI is waiting on this, so it has to be set on EVERY exit path –
	// including the cancellation returns below.
	struct FinishGuard {
		LoadProgress& p;
		~FinishGuard() { p.finished.store(true, std::memory_order_release); }
	} finishGuard{progress};

	// Set TCRUNCHER_LOADER_TIMING=1 to get a per-phase breakdown on stderr. Cheap enough to
	// leave in: one getenv and a few clock reads per load.
	const bool timing = ( std::getenv("TCRUNCHER_LOADER_TIMING") != nullptr );
	auto now  = []() { return std::chrono::steady_clock::now(); };
	auto t0   = now();
	auto mark = [&](const char* what) {
		if( !timing ) return;
		auto t = now();
		fprintf(stderr, "  [loader] %-14s %6.1f ms\n", what,
		        std::chrono::duration<double, std::milli>(t - t0).count());
		t0 = t;
	};

	CsvDefinition def = definition;
	const CsvDialect dialect = CsvParser::makeDialect(def);
	const uint64_t   start   = std::min<uint64_t>((uint64_t) def.bomBytes, len);

	//
	//	1. Split at line starts, statelessly.
	//
	std::vector<uint64_t> bounds;
	bounds.push_back(start);
	if( !plan.forcedNominalOffsets.empty() ) {
		for( uint64_t nominal : plan.forcedNominalOffsets ) {
			if( nominal <= start || nominal >= len ) continue;
			uint64_t b = nextLineStart(data, len, nominal);
			if( b > bounds.back() && b < len )
				bounds.push_back(b);
		}
	} else {
		const size_t k = std::max<size_t>(1, plan.chunkCount);
		for( size_t i = 1; i < k; ++i ) {
			uint64_t nominal = start + ((len - start) * i) / k;
			uint64_t b = nextLineStart(data, len, nominal);
			if( b > bounds.back() && b < len )
				bounds.push_back(b);
		}
	}
	bounds.push_back(len);

	std::vector<ChunkPrescan> chunks(bounds.size() - 1);
	for( size_t i = 0; i + 1 < bounds.size(); ++i ) {
		chunks[i].begin = bounds[i];
		chunks[i].end   = bounds[i+1];
	}
	mark("split");

	//
	//	2. Prescan every chunk in parallel, under both carry-in hypotheses.
	//
	if( !Parallel::parallelFor(chunks.size(), plan.threads, &progress.cancelRequested,
	                           [&](size_t i) { prescanChunk(data, dialect, chunks[i]); }) ) {
		result.cancelled = true;
		return result;
	}
	mark("prescan");

	//
	//	3. Compose the carry chain serially. O(k), k <= 256.
	//
	std::vector<uint64_t> recordStarts;
	bool carry = false;
	for( size_t i = 0; i < chunks.size(); ++i ) {
		const int h = carry ? 1 : 0;
		const uint64_t rs = chunks[i].firstRecordStart[h];
		if( rs != NO_RECORD_START )
			recordStarts.push_back(rs);				// a chunk wholly inside one quoted field
														// yields none and folds into its neighbour
		carry = chunks[i].endEnclosed[h];
	}
	if( recordStarts.empty() )
		recordStarts.push_back(start);

	//
	//	4. Parse the ranges in parallel. Each starts at a record boundary with an empty state,
	//	   so there is nothing to stitch afterwards.
	//
	const size_t rangeCount = recordStarts.size();
	std::vector<CsvDataStorage> pieces(rangeCount);
	std::vector<char> pieceRowLimitHit(rangeCount, 0);
	std::vector<std::map<long,long>> pieceHisto(rangeCount);

	if( !Parallel::parallelFor(rangeCount, plan.threads, &progress.cancelRequested, [&](size_t j) {
		CsvParser::RangeOptions opt;
		opt.from       = recordStarts[j];
		opt.to         = (j + 1 < rangeCount) ? recordStarts[j+1] : len;
		opt.finalRange = ( j + 1 == rangeCount );
		opt.maxLines   = 0;
		opt.resizeRows = true;
		// planLoad() has already required validation.validFromBom, so every line in this
		// range is valid UTF-8 and re-checking it per line would be a wasted second pass
		opt.assumeValidUtf8 = true;

		CsvParser parser;
		parser.sharedRowCounter = &progress.rowsDone;
		parser.cancelRequested  = &progress.cancelRequested;

		// the piece's row vector would otherwise double its way up from nothing, on every
		// worker at once
		pieces[j].reserveRows( CsvParser::estimateRowCount(data + opt.from, opt.to - opt.from) );

		CsvDefinition localDef = def;			// each worker owns its copy
		pieceHisto[j] = parser.parseCsvRange(data, opt, pieces[j], &localDef);
		pieceRowLimitHit[j] = parser.rowLimitExceeded ? 1 : 0;
	}) ) {
		result.cancelled = true;
		return result;
	}
	mark("parse");

	//
	//	5. Barrier: the global column count. Provably equal to the serial parser's final
	//	   act_cols, since every committed record contributes its field count to some piece.
	//
	table_index_t globalColumns = 0;
	uint64_t      totalRows     = 0;
	for( size_t j = 0; j < rangeCount; ++j ) {
		globalColumns = std::max(globalColumns, pieces[j].columns());
		totalRows    += (uint64_t) pieces[j].rows();
		if( pieceRowLimitHit[j] )
			result.rowLimitExceeded = true;
	}
	if( totalRows > (uint64_t) INT_MAX ) {
		// table_index_t is int: assembling these would make rows() wrap negative, and the
		// table would be unusable. Refuse rather than hand back something that misreports its
		// own size.
		result.rowLimitExceeded = true;
		return result;
	}

	bool anyNeedsPadding = false;
	for( size_t j = 0; j < rangeCount; ++j ) {
		if( pieces[j].columns() < globalColumns ) { anyNeedsPadding = true; break; }
	}

	//
	//	6. Phase B: bring every piece up to the global width. This is what replaces the serial
	//	   parser's quadratic re-padding of every earlier row. Usually there is nothing to do,
	//	   and fanning out for nothing costs more than the work.
	//
	if( anyNeedsPadding ) {
		if( !Parallel::parallelFor(rangeCount, plan.threads, &progress.cancelRequested, [&](size_t j) {
			if( pieces[j].columns() < globalColumns )
				pieces[j].resize(0, globalColumns);
		}) ) {
			result.cancelled = true;
			return result;
		}
	}
	mark("pad");

	//
	//	7. Phase C: concatenate in file order. Pointer copying only – roughly 24 bytes a row.
	//
	storage.assembleFrom(pieces, globalColumns);

	// merge the per-range histograms
	for( size_t j = 0; j < rangeCount; ++j ) {
		for( auto const& kv : pieceHisto[j] )
			result.histogram[kv.first] += kv.second;
	}

	// the serial parser drops the trailing row once, after everything has been appended
	if( storage.rows() )
		storage.deleteRows( storage.rows() - 1, storage.rows() - 1 );

	mark("assemble");

	progress.rowsDone.store((long) storage.rows(), std::memory_order_relaxed);
	return result;
}
