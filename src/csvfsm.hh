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

#ifndef _CSVFSM_HH
#define _CSVFSM_HH

//
//	The CSV finite state machine, extracted so that every consumer runs the SAME
//	transitions: the serial parser, the parallel chunk prescan, and the parallel parse.
//	Writing a second state machine for the prescan would let the two drift apart, and a
//	drift here silently corrupts user data.
//
//	Deliberately depends on nothing: no FLTK, no iostream, no project headers. This file
//	is compiled into worker threads.
//

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>


/**
 *	The CSV dialect, reduced to what the state machine actually branches on.
 */
struct CsvDialect {
	char delimiter    = ',';
	char quote        = '"';
	char escape       = '"';
	bool escapeActive = false;			// == (quote != escape); when false the escape branch is dead

	// True for the (at most three) bytes the state machine has to look at individually.
	// Everything else is ordinary content and can be handled in runs – see csvScanLine().
	bool special[256] = { false };

	void buildSpecialTable() {
		for( int i = 0; i < 256; ++i ) special[i] = false;
		special[(unsigned char) delimiter] = true;
		special[(unsigned char) quote]     = true;
		if( escapeActive )
			special[(unsigned char) escape] = true;
	}
};


/*
 *	Sink contract – see csvScanLine() below.
 *
 *		void beginField();						a new field starts (the record's first, or one after a delimiter)
 *		void content(const char* p, size_t n);	append n raw bytes to the current field
 *		void endField();						the current field is complete
 *		void lineContinuation();				a physical line ended while still inside a quoted field
 *
 *	beginField() and endField() are always paired 1:1, so the number of either call is the
 *	record's field count.
 */


/**
 *	Discards everything. Used by the chunk prescan, which only cares about the `enclosed`
 *	bit at the chunk boundary – with this sink the whole content path is dead code and the
 *	scan collapses into a byte-set skip loop.
 */
struct StructuralSink {
	void beginField() {}
	void endField() {}
	void content(const char*, size_t) {}
	void lineContinuation() {}
};


/**
 *	Builds the glued row string that `CsvDataStorage` stores, straight from the input bytes:
 *	no transient vector, no per-field allocation, no separate merge pass. The byte sequence
 *	produced is identical to `CsvDataStorage::mergeString(fields)` – fields joined by exactly
 *	one glue byte, none leading or trailing.
 */
class MergedRowSink {
public:
	MergedRowSink(std::string* out, char glue) : out_(out), glue_(glue) {}

	// Begins a fresh record. `reserveHint` matters more than it looks: the finished string is
	// MOVED into storage, so the next record starts from a moved-from (small-buffer) string
	// and would otherwise re-cross the SSO boundary every row. The physical line length is an
	// exact upper bound – each delimiter becomes exactly one glue byte, and quoting only ever
	// removes bytes.
	void startRecord(size_t reserveHint = 0) {
		out_->clear();
		if( reserveHint )
			out_->reserve(reserveHint);
		fields_ = 0;
		beginField();
	}

	void beginField()                      { if( fields_++ > 0 ) out_->push_back(glue_); }
	void endField()                        {}
	void content(const char* p, size_t n)  { out_->append(p, n); }
	// LF is the right thing to do here even for a CRLF file: a CR would render as "^M"
	// in the multiline cell editor.
	void lineContinuation()                { out_->push_back('\n'); }

	int32_t fields() const { return fields_; }

private:
	std::string* out_;
	char         glue_;
	int32_t      fields_ = 0;
};


/**
 *	Scans ONE physical line [p, e) and drives `sink`.
 *
 *	`enclosed` is in/out and is the ONLY state that crosses a line boundary – at a physical
 *	line start the machine's structural state is exactly this one bit, which is what makes
 *	the parallel chunk prescan composable. On return, enclosed == false means the record is
 *	complete and the caller should emit endField(); otherwise the caller emits
 *	lineContinuation() and feeds the next physical line into the same sink.
 *
 *	PRECONDITION: [p, e) contains no NUL bytes. The serial reader strips them while
 *	splitting lines, and doing so BEFORE the scan is load-bearing: NUL removal shifts both
 *	the "is this the last character of the line" test and character adjacency, so `\` NUL `"`
 *	must behave exactly like `\"`. A caller working on raw bytes has to materialise a
 *	NUL-free line first.
 *
 *	The transitions below reproduce the historical parser exactly, quirks included:
 *	  - startField is unconditionally true at every physical line start, even mid-quote
 *	  - a doubled quote while NOT enclosed opens a quote and consumes only ONE character
 *	  - the escape lookahead is line-scoped: an escape char as the last byte of a line is
 *	    literal content, not an escape
 *	  - neither the escape branch nor the quote branch clears startField
 */
template <class Sink>
inline void csvScanLine(const char* p, const char* e, const CsvDialect& d, bool& enclosed, Sink& sink) {
	const size_t lineLen = (size_t)(e - p);
	bool startField = true;					// always true at a physical line start

	for( size_t i = 0; i < lineLen; ++i ) {
		const char c = p[i];

		if( !d.special[(unsigned char) c] ) {
			// Ordinary content. Take the whole run in one go: the per-byte path below would
			// otherwise call into the sink once per byte, which is the single hottest thing
			// in the parser. Semantics are unchanged – an ordinary byte only ever clears
			// startField and appends itself.
			size_t j = i + 1;
			while( j < lineLen && !d.special[(unsigned char) p[j]] )
				++j;
			startField = false;
			sink.content(p + i, j - i);
			i = j - 1;
			continue;
		}

		if( d.escapeActive && c == d.escape ) {
			if( i + 1 < lineLen ) {
				// not the last character: write the following one back verbatim
				sink.content(p + i + 1, 1);
				++i;
				continue;
			}
			// last character of the line: falls through and is treated as plain content
		}

		if( c == d.quote ) {
			if( i + 1 < lineLen && p[i+1] == d.quote ) {
				// doubled quote
				if( enclosed ) {
					sink.content(p + i, 1);
					++i;
					continue;
				} else {
					enclosed = true;		// consumes ONE character, not two
					continue;
				}
			} else {
				// single quote
				if( enclosed ) {
					enclosed = false;		// quoting ends
				} else if( startField ) {
					enclosed = true;		// not enclosed and at the start of a field
				} else {
					sink.content(p + i, 1);	// not enclosed, mid-field: literal content
				}
				continue;
			}
		}

		if( startField ) {
			startField = false;
		}

		if( c == d.delimiter && !enclosed ) {
			sink.endField();
			sink.beginField();
			startField = true;
			continue;
		}

		sink.content(p + i, 1);
	}
}



/**
 *	Walks the physical lines of a byte range exactly the way the historical stream reader
 *	(CsvParser::myGetline) does: LF, CRLF and a lone CR all terminate a line.
 *
 *	Line splitting is part of the observable semantics, not an implementation detail: NUL
 *	removal shifts the escape lookahead, and the chunk prescan's one-bit carry is only
 *	composable if the prescan splits lines identically to the parse. So there is exactly one
 *	implementation, shared by both.
 */
class CsvLineCursor {
public:
	CsvLineCursor(const char* data, uint64_t from, uint64_t to)
		: data_(data), end_(data + to), to_(to), pos_(from < to ? from : to) {}

	uint64_t pos() const { return pos_; }

	// Consumes one line. Returns false once the range is exhausted. `reachedEnd` reports that
	// the line ran into the end of the range with no terminator, which is what decides
	// whether the reader would have set eofbit.
	bool next(const char*& lineBegin, const char*& lineEnd, bool& reachedEnd) {
		const char* s = data_ + pos_;
		if( s >= end_ )
			return false;

		// next LF at or after s
		if( !lfDone_ && (lfPos_ == nullptr || lfPos_ < s) ) {
			lfPos_ = (const char*) memchr(s, '\n', (size_t)(end_ - s));
			if( lfPos_ == nullptr ) lfDone_ = true;
		}
		// Next CR at or after s – but never searched beyond the next LF, because a CR after it
		// can never win the min() below. Without that bound a ten-line dialect probe on an
		// LF-only file scans the entire buffer looking for a CR that isn't there, once per
		// candidate dialect.
		const char* crLimit = lfDone_ ? end_ : lfPos_;
		if( crPos_ == nullptr || crPos_ < s )
			crPos_ = ( crLimit > s ) ? (const char*) memchr(s, '\r', (size_t)(crLimit - s)) : nullptr;

		const char* term = end_;
		if( lfPos_ && lfPos_ < term ) term = lfPos_;
		if( crPos_ && crPos_ < term ) term = crPos_;

		lineBegin  = s;
		lineEnd    = term;
		reachedEnd = ( term >= end_ );
		if( reachedEnd ) {
			pos_ = to_;
		} else if( *term == '\n' ) {
			pos_ = (uint64_t)(term - data_) + 1;
		} else {
			pos_ = (uint64_t)(term - data_) + ((term + 1 < end_ && term[1] == '\n') ? 2 : 1);
		}
		return true;
	}

private:
	const char* data_;
	const char* end_;
	uint64_t    to_;
	uint64_t    pos_;
	const char* lfPos_  = nullptr;
	bool        lfDone_ = false;
	const char* crPos_  = nullptr;
};


// True if [from, to) holds a NUL anywhere. Checking once per range keeps the per-line check
// off the hot path for the overwhelmingly common NUL-free file.
inline bool csvRangeHasNul(const char* data, uint64_t from, uint64_t to) {
	return to > from && memchr(data + from, 0, (size_t)(to - from)) != nullptr;
}


// Materialises [lineBegin, lineEnd) into `scratch` without its NUL bytes and re-points
// [p, e) at it. Returns false – leaving p/e alone – when there is nothing to strip, which is
// what lets the state machine read straight out of the mapping.
inline bool csvStripNuls(const char* lineBegin, const char* lineEnd, std::string& scratch,
                         const char*& p, const char*& e) {
	if( lineEnd <= lineBegin || memchr(lineBegin, 0, (size_t)(lineEnd - lineBegin)) == nullptr )
		return false;
	scratch.clear();
	scratch.reserve((size_t)(lineEnd - lineBegin));
	for( const char* q = lineBegin; q < lineEnd; ++q ) {
		if( *q != '\0' ) scratch.push_back(*q);
	}
	p = scratch.data();
	e = p + scratch.size();
	return true;
}


#endif
