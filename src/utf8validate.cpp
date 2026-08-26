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

#include "utf8validate.hh"

#include <algorithm>
#include <vector>

#include "parallelutil.hh"
#include "utf8.h"


namespace {

// Below this there is nothing to parallelise; the serial answer is already instant.
const uint64_t PARALLEL_THRESHOLD = 1ull << 20;			// 1 MB
// Where the parallel region starts. The (tiny) head before it is validated once per start
// offset, which is why the three answers can share all the parallel work.
const uint64_t HEAD_END           = 64;


/**
 *	One range's contribution.
 *
 *	`s` is where this range's first code point actually begins: a range boundary can land in
 *	the middle of a multi-byte sequence, so the scan first skips forward over continuation
 *	bytes. `f` is where the scan stopped, which may overshoot `end` by up to three bytes
 *	because the last code point in the range is validated in full.
 */
struct RangeResult {
	uint64_t s     = 0;
	uint64_t f     = 0;
	bool     valid = true;
};


RangeResult validateRange(const char* data, uint64_t len, uint64_t start, uint64_t end) {
	RangeResult r;

	// resynchronise: skip the tail of whatever sequence straddles `start`
	uint64_t s = start;
	while( s < len && (((unsigned char) data[s]) & 0xC0) == 0x80 )
		++s;
	r.s = s;
	r.f = s;

	if( s >= end ) {
		// the whole range was continuation bytes; the neighbours' chain check covers it
		return r;
	}

	const char* p   = data + s;
	const char* eos = data + len;
	const char* lim = data + end;
	while( p < lim ) {
		// validate_next() rather than a hand-rolled DFA, so that overlongs, surrogates and
		// code points above U+10FFFF are accepted or rejected exactly as utf8::is_valid does
		if( utf8::internal::validate_next(p, eos) != utf8::internal::UTF8_OK ) {
			r.valid = false;
			r.f     = (uint64_t)(p - data);
			return r;
		}
	}
	r.f = (uint64_t)(p - data);
	return r;
}


/**
 *	Stitches a head range onto the shared parallel ranges.
 *
 *	The `f == next s` chain is what makes this EXACTLY equivalent to validating the whole
 *	region in one go. Without it, a run of orphan continuation bytes straddling a boundary
 *	would be skipped by the resynchronise step on one side and never reached on the other,
 *	and the buffer would be reported valid when it is not.
 */
bool chainValid(uint64_t startOffset, const RangeResult& head, const std::vector<RangeResult>& parts) {
	if( !head.valid || head.s != startOffset )
		return false;
	uint64_t expect = head.f;
	for( const RangeResult& r : parts ) {
		if( !r.valid || r.s != expect )
			return false;
		expect = r.f;
	}
	return true;
}

} // namespace


Utf8ValidationResult validateUtf8Parallel(const char* data, uint64_t len, int bomBytes, unsigned threads, bool forceSplit) {
	Utf8ValidationResult out;
	if( data == nullptr )
		return out;

	const uint64_t bom = std::min<uint64_t>( bomBytes < 0 ? 0 : (uint64_t) bomBytes, len );
	const uint64_t off4 = std::min<uint64_t>(4, len);

	// small buffer, or nothing worth splitting: one range each, no threads
	if( len <= HEAD_END || (len < PARALLEL_THRESHOLD && !forceSplit) ) {
		std::vector<RangeResult> none;
		out.validFrom0   = chainValid(0,    validateRange(data, len, 0,    len), none);
		out.validFrom4   = chainValid(off4, validateRange(data, len, off4, len), none);
		out.validFromBom = chainValid(bom,  validateRange(data, len, bom,  len), none);
		return out;
	}

	threads = Parallel::threadCount(threads);

	// carve [HEAD_END, len) into one range per thread
	const uint64_t body = len - HEAD_END;					// >= 1: len > HEAD_END here
	unsigned parts = (unsigned) std::min<uint64_t>(threads, body);

	std::vector<uint64_t> bounds(parts + 1);
	for( unsigned i = 0; i <= parts; ++i )
		bounds[i] = HEAD_END + (body * i) / parts;
	bounds[parts] = len;

	std::vector<RangeResult> results(parts);
	Parallel::parallelFor(parts, threads, nullptr, [&](size_t i) {
		results[i] = validateRange(data, len, bounds[i], bounds[i+1]);
	});

	// the three answers differ only in their head range – the parallel work is shared
	out.validFrom0   = chainValid(0,    validateRange(data, len, 0,    HEAD_END), results);
	out.validFrom4   = chainValid(off4, validateRange(data, len, off4, HEAD_END), results);
	out.validFromBom = chainValid(bom,  validateRange(data, len, bom,  HEAD_END), results);
	return out;
}
