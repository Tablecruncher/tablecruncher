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


/************************************************************************************
*
*	CsvParser
*
************************************************************************************/


#include "csvparser.hh"
#include "csvfsm.hh"

#include <climits>
#include <cstring>


/**
 *	\brief Parses 'input' and stores the CSV data into the given `CsvDataStorage` object.

 *	'definition' tells what CSV dialect is used.
 *	The vec-of-vec gets enlarged while ensuring all vectors have the same length.

 *	IMPORTANT: Update any variables afterwards that store the dimension of the table.
 *	
 *	\param input The stream that should get parsed
 *	\param storage The `CsvDataStorage` object where the data gets stored using `push_back()`.
 *	\param definition Pointer to the definition of the CSV data in `input`.
 *	\param maxLines	maximum number of lines to return – used for probing
 *	\param resizeRows	True: resize rows, all rows have the same length, False: don't resize – used for probing.
 *				if !resize: CsvDataStorage::numColumns doesn't get updated!
 *
 *	@return		list indicating different row lengths

 */
std::map<long,long> CsvParser::parseCsvStream( std::istream *input, CsvDataStorage &storage, CsvDefinition *definition, int maxLines, bool resizeRows ) {
	std::string line;
	CommitState st;
	st.maxLines   = maxLines;
	st.resizeRows = resizeRows;

	const CsvDialect dialect = makeDialect(*definition);

	// The state machine writes finished, glued row strings straight into `rowStr`. There is
	// no intermediate vector-of-fields and no merge pass.
	std::string   rowStr;
	MergedRowSink sink(&rowStr, CsvDataStorage::internalGlue());

	// Local, NOT a member: each call parses a complete input. CsvApplication::guessDefinition
	// reuses one CsvParser across its eight dialect probes, and carrying "still inside a
	// quoted field" from one probe into the next corrupted the next probe's field counts —
	// and so, potentially, the dialect chosen for the file.
	bool enclosed = false;

	// skip bomBytes
	input->ignore(definition->bomBytes);

	// read lines from istream `input` into `line`
	while( myGetlineEncodings( *input, line, definition->encoding) ) {
		if( !enclosed )
			sink.startRecord(line.size());

		// translate encodings to valid UTF-8
		switch( definition->encoding ) {
			case CsvDefinition::ENC_NONE:
			case CsvDefinition::ENC_UTF8:
				Helper::fixUtf8(line);				// replaces invalid chars with replacement char
			break;
			case CsvDefinition::ENC_Latin1:
			case CsvDefinition::ENC_Win1252:
				line = transcodeToUtf8(definition->encoding, line);
			break;
			case CsvDefinition::ENC_UTF16LE:			// translation happens in myGetlineEncodings()
			case CsvDefinition::ENC_UTF16BE:			// translation happens in myGetlineEncodings()
			default:
			break;
		}

		// run the shared state machine over this physical line
		csvScanLine( line.data(), line.data() + line.size(), dialect, enclosed, sink );

		const bool isLastLine = ( input->rdstate() == std::ios_base::eofbit );

		if( enclosed ) {
			// still inside a quoted field: the record continues on the next physical line
			sink.lineContinuation();
		} else {
			sink.endField();
			if( !commitRecord(storage, rowStr, (long) sink.fields(), isLastLine, st) )
				break;
		}

		// stop parsing if input has been fully consumed
		if( isLastLine )
			break;
	} // end of while( myGetlineEncodings() )

	// Delete last row if storage is not empty
	if( storage.rows() )
		storage.deleteRows( storage.rows() - 1, storage.rows() - 1 );

	// keys in flatHisto are unique by construction, so this is a faithful conversion
	return std::map<long,long>(st.flatHisto.begin(), st.flatHisto.end());
}


/*
 *	Which encodings the flat-buffer path can handle. UTF-16 and UTF-32 are read a code unit
 *	at a time and transcoded while splitting lines (myGetlineEncodings), so they keep using
 *	the stream path.
 */
bool CsvParser::bufferPathSupports(CsvDefinition::Encodings enc) {
	switch( enc ) {
		case CsvDefinition::ENC_NONE:
		case CsvDefinition::ENC_UTF8:
		case CsvDefinition::ENC_Latin1:
		case CsvDefinition::ENC_Latin9:
		case CsvDefinition::ENC_Win1252:
			return true;
		default:
			return false;
	}
}


/*
 *	Transcodes one 8-bit-encoded line to UTF-8. Latin-9 and everything else pass through, as
 *	they always have.
 */
std::string CsvParser::transcodeToUtf8(CsvDefinition::Encodings enc, const std::string& line) {
	switch( enc ) {
		case CsvDefinition::ENC_Latin1:  return Helper::latin1toutf8(line);
		case CsvDefinition::ENC_Win1252: return Helper::win1252toutf8(line);
		default:                         return line;
	}
}


/*
 *	Parses the whole buffer. Adds the BOM skip and the trailing-row delete on top of
 *	parseCsvRange(), which does the actual work.
 */
std::map<long,long> CsvParser::parseCsvBuffer( const char* data, uint64_t len, CsvDataStorage &storage,
                                               CsvDefinition *definition, int maxLines, bool resizeRows,
                                               bool assumeValidUtf8 ) {
	RangeOptions opt;
	opt.from            = std::min<uint64_t>((uint64_t) definition->bomBytes, len);
	opt.to              = len;
	opt.finalRange      = true;
	opt.maxLines        = maxLines;
	opt.resizeRows      = resizeRows;
	opt.assumeValidUtf8 = assumeValidUtf8;

	std::map<long,long> histogram = parseCsvRange(data, opt, storage, definition);

	// Delete last row if storage is not empty
	if( storage.rows() )
		storage.deleteRows( storage.rows() - 1, storage.rows() - 1 );

	return histogram;
}


/*
 *	Parses one slice of a flat buffer.
 *
 *	Reproducing the stream reader exactly matters more than anything else here. CsvLineCursor
 *	handles the line splitting (LF / CRLF / lone CR); on top of that this loop reproduces:
 *	  - NUL bytes dropped from the line BEFORE the state machine sees it, which shifts both
 *	    character adjacency and the "last character of the line" test
 *	  - reaching the end of input yielding one final line, with eofbit set only when that line
 *	    is empty, which is what produces (or suppresses) the trailing phantom row
 *
 *	This is the single implementation the serial buffer path and every parallel worker share,
 *	so the two cannot drift apart.
 */
std::map<long,long> CsvParser::parseCsvRange( const char* data, const RangeOptions& opt, CsvDataStorage &storage,
                                              CsvDefinition *definition, bool* endedEnclosed ) {
	CommitState st;
	st.maxLines   = opt.maxLines;
	st.resizeRows = opt.resizeRows;

	const CsvDialect dialect = makeDialect(*definition);
	const CsvDefinition::Encodings enc = definition->encoding;

	std::string   rowStr;
	MergedRowSink sink(&rowStr, CsvDataStorage::internalGlue());
	std::string   scratch;					// only touched when a line must be materialised

	// One check for the whole range beats one per line – but only for a full parse. A dialect
	// probe stops after ten lines, so scanning the whole buffer up front would cost far more
	// than the per-line checks it saves (and guessDefinition() runs eight of them).
	const bool checkNulPerLine = ( opt.maxLines != 0 );
	const bool rangeHasNul     = checkNulPerLine || csvRangeHasNul(data, opt.from, opt.to);

	CsvLineCursor cursor(data, opt.from, opt.to);
	bool enclosed = false;

	for(;;) {
		const char* lineBegin = data + opt.to;
		const char* lineEnd   = lineBegin;
		bool        reachedEnd = false;

		if( !cursor.next(lineBegin, lineEnd, reachedEnd) ) {
			if( !opt.finalRange )
				break;							// a non-final slice just stops at its boundary
			// the reader still yields one final, empty line and sets eofbit
			reachedEnd = true;
		}

		// == "the stream reader would have set eofbit here": end of input reached, and the
		// line is empty once its NUL bytes have been dropped
		bool eofNow = false;
		if( reachedEnd && opt.finalRange ) {
			eofNow = true;
			for( const char* q = lineBegin; q < lineEnd; ++q ) {
				if( *q != '\0' ) { eofNow = false; break; }
			}
		}

		if( !enclosed )
			sink.startRecord((size_t)(lineEnd - lineBegin) + 1);

		//
		//	Produce the bytes the state machine should see. The fast path – valid UTF-8, no
		//	NULs – hands it a view straight into the mapping and copies nothing at all.
		//
		const char* p = lineBegin;
		const char* e = lineEnd;
		const bool  hasNul = rangeHasNul && csvStripNuls(lineBegin, lineEnd, scratch, p, e);

		switch( enc ) {
			case CsvDefinition::ENC_NONE:
			case CsvDefinition::ENC_UTF8:
				if( hasNul ) {
					Helper::fixUtf8(scratch);
					p = scratch.data(); e = p + scratch.size();
				} else if( !opt.assumeValidUtf8 && !utf8::is_valid(lineBegin, lineEnd) ) {
					scratch.assign(lineBegin, lineEnd);
					Helper::fixUtf8(scratch);
					p = scratch.data(); e = p + scratch.size();
				}
			break;
			case CsvDefinition::ENC_Latin1:
			case CsvDefinition::ENC_Win1252:
				scratch = transcodeToUtf8(enc, hasNul ? scratch : std::string(lineBegin, lineEnd));
				p = scratch.data(); e = p + scratch.size();
			break;
			default:
				// Latin-9 and anything else: no transcoding, exactly as on the stream path
			break;
		}

		csvScanLine( p, e, dialect, enclosed, sink );

		if( enclosed ) {
			sink.lineContinuation();
		} else {
			sink.endField();
			if( !commitRecord(storage, rowStr, (long) sink.fields(), eofNow, st) )
				break;
		}

		if( eofNow )
			break;
	}

	if( endedEnclosed )
		*endedEnclosed = enclosed;

	return std::map<long,long>(st.flatHisto.begin(), st.flatHisto.end());
}


/*
 *	Appends one finished record to `storage`, maintaining the histogram, the column count and
 *	the progress/cancellation bookkeeping. Shared by both parse loops.
 *
 *	`isLastLine` mirrors the stream reader's eofbit: the last record's length is deliberately
 *	left out of the histogram.
 *
 *	@return		false when parsing must stop (row cap, maxLines, or a cancellation request)
 */
bool CsvParser::commitRecord(CsvDataStorage &storage, std::string &rowStr, long fieldCount,
                             bool isLastLine, CommitState &st) {
	// calculate histogram data
	if( !isLastLine ) {
		bool seen = false;
		for( auto& kv : st.flatHisto ) {
			if( kv.first == fieldCount ) { ++kv.second; seen = true; break; }
		}
		if( !seen ) {
			st.flatHisto.emplace_back( fieldCount, 1L );
		}
	}

	// resize rows
	if( st.resizeRows && fieldCount ) {
		if( st.act_cols < fieldCount ) {
			// parsed line is longer than columns(): resize storage
			storage.resize(0, fieldCount);
		} else if( st.act_cols > fieldCount ) {
			// this row is shorter than the ones before it: pad it out with empty fields
			rowStr.append( (size_t)(st.act_cols - fieldCount), CsvDataStorage::internalGlue() );
		}
		st.act_cols = std::max( st.act_cols, fieldCount );
	}

	// add parsed line back to table storage
	storage.push_back( std::move(rowStr) );
	++st.act_rows;

	// publish progress / poll for cancellation in coarse batches so that the counter's cache
	// line doesn't ping-pong between threads
	if( (st.act_rows & 0x0FFF) == 0 ) {
		if( sharedRowCounter )
			sharedRowCounter->fetch_add(4096, std::memory_order_relaxed);
		if( cancelRequested && cancelRequested->load(std::memory_order_relaxed) )
			return false;
	}
	// show an update every 25,000 rows in the status bar – very expensive!
	// (only the owning thread ever installs onProgress, so workers pay nothing here)
	if( onProgress && st.act_rows % 25000 == 0 ) {
		onProgress(st.act_rows);
	}
	if( st.maxLines && st.act_rows >= st.maxLines ) {
		return false;
	}
	if( st.act_rows >= (long) INT_MAX ) {
		// table_index_t is int – one more row and rows() would silently wrap
		rowLimitExceeded = true;
		return false;
	}
	return true;
}


/*
 *	Reduces a CsvDefinition to what the state machine actually branches on.
 */
CsvDialect CsvParser::makeDialect(const CsvDefinition& definition) {
	CsvDialect d;
	d.delimiter    = definition.delimiter;
	d.quote        = definition.quote;
	d.escape       = definition.escape;
	d.escapeActive = ( definition.quote != definition.escape );
	d.buildSpecialTable();
	return d;
}


/*
 *	Estimates how many rows a buffer holds, by sampling the average line length over the
 *	first 64 KB. Used only to reserve storage up front – being wrong costs a little memory or
 *	a little reallocation, never correctness.
 */
size_t CsvParser::estimateRowCount(const char* data, uint64_t len) {
	const uint64_t SAMPLE = 64 * 1024;
	if( len == 0 || data == nullptr )
		return 0;

	const uint64_t got = std::min<uint64_t>(SAMPLE, len);

	// count line ends the way myGetline() does: LF, CRLF and a lone CR all end one line
	uint64_t lines = 0;
	for( uint64_t i = 0; i < got; ++i ) {
		if( data[i] == '\n' )
			++lines;
		else if( data[i] == '\r' && (i + 1 >= got || data[i+1] != '\n') )
			++lines;
	}
	if( lines == 0 )
		return 1;

	double avgLineLen = (double) got / (double) lines;
	return clampRowEstimate(((double) len / avgLineLen) * 1.05, len);	// small headroom
}


/*
 *	Keeps a row-count estimate sane: at least one row, and never more than fits in
 *	max(256 MB, bytes/4) of row vector.
 */
size_t CsvParser::clampRowEstimate(double estimate, uint64_t bytes) {
	if( estimate < 1 )
		return 1;
	const double memBudget = std::max(256.0 * 1024 * 1024, (double) bytes / 4.0);
	const double maxRows   = memBudget / (double) sizeof(std::string);
	return (size_t) std::min(estimate, maxRows);
}


/*
 *	Same, for an istream. Leaves the stream rewound to 0.
 */
size_t CsvParser::estimateRowCount(std::istream& input, int64_t fileLength) {
	const size_t SAMPLE = 64 * 1024;
	if( fileLength <= 0 )
		return 0;

	std::vector<char> buf(SAMPLE);
	input.clear();
	input.seekg(0);
	input.read(buf.data(), SAMPLE);
	size_t got = (size_t) input.gcount();
	input.clear();
	input.seekg(0);
	if( got == 0 )
		return 0;

	size_t rows = estimateRowCount(buf.data(), got);
	if( rows == 0 )
		return 0;
	// scale the sample's density up to the whole file
	return clampRowEstimate((double) rows * ((double) fileLength / (double) got), (uint64_t) fileLength);
}


//
/*
	myGetline()

	Used by myGetlineEncodings()

	http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
	Just works for ASCII-based 8-bit encodings like ASCII, Latin1, Latin9, Win1252 and UTF8
 */
std::istream& CsvParser::myGetline(std::istream& is, std::string& t) {
	t.clear();

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case EOF:
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
		case '\0':
			// skip NULL bytes
			break;
        default:
            t += (char)c;
        }
    }
}


/*
 *	For UTF16 and UTF32 encodings with LF or CRLF newline – doesn't work with CR only newline
 *	https://stackoverflow.com/a/50714844/2771733
 *	TODO UTF-32LE and UTF-32BE
 */
std::istream& CsvParser::myGetlineEncodings(std::istream& is, std::string& t, CsvDefinition::Encodings enc) {
	if( enc == CsvDefinition::ENC_NONE ||					// treated as UTF-8, exactly as parseCsvStream() does
		enc == CsvDefinition::ENC_UTF8 ||
		enc == CsvDefinition::ENC_Latin1 ||
		enc == CsvDefinition::ENC_Latin9 ||
		enc == CsvDefinition::ENC_Win1252
	 ) {
		return myGetline(is, t);
	}
	// Anything below reads fixed-width code units. ENC_NONE used to land here and, with
	// unitLength defaulting to 1, appended nothing at all – every ENC_NONE file parsed as an
	// empty table. setTypeByUser() maps ENC_NONE to ENC_UTF8 before the load, so the app
	// never hit it, but the two parse paths have to agree.
	codeUnitReturn_t nextCodeUnit;
	int unitLength = 1;
	bool bigEndian = true;
	
#ifdef DEBUG
	std::cerr << "myGetlineEncodings" << std::endl;
#endif
	
	// get length of used code unit
	if( enc == CsvDefinition::ENC_UTF16BE || enc == CsvDefinition::ENC_UTF16LE )
		unitLength = 2;
	if( enc == CsvDefinition::ENC_UTF32BE || enc == CsvDefinition::ENC_UTF32LE )
		unitLength = 4;
	// get endian order
	if( enc == CsvDefinition::ENC_UTF16LE || enc == CsvDefinition::ENC_UTF32LE )
		bigEndian = false;

	t.clear();
    std::istream::sentry se(is, true);
    std::streambuf *sb = is.rdbuf();

    for(;;) {
		nextCodeUnit = getNextCodeUnit(sb, unitLength, bigEndian);
		if( enc == CsvDefinition::ENC_UTF16BE || enc == CsvDefinition::ENC_UTF16LE ) {
			if( nextCodeUnit.codeUnit16 == 0x000A ) {
				break;
			}
		}
		if( nextCodeUnit.octet == EOF ) {
		    if(t.empty()) {
				is.setstate(std::ios::eofbit);
		    }
			break;
		}
		if( unitLength == 2 ) {
			if( nextCodeUnit.codeUnit16 >= 0xD800 && nextCodeUnit.codeUnit16 <= 0xDBFF ) {
				codeUnitReturn_t temp = getNextCodeUnit(sb, unitLength, bigEndian);
				t += Helper::utf16_utf8(nextCodeUnit.codeUnit16, temp.codeUnit16);
			} else {
				t += Helper::utf16_utf8(nextCodeUnit.codeUnit16,0);
			}
		}
		
    }
	
	if( t.length() && t.back() == '\r') {
		t.pop_back();
	}

	return is;
}




// returns a code unit (and the value of the last octet so that calling methods can see an EOF marker)
CsvParser::codeUnitReturn_t CsvParser::getNextCodeUnit(std::streambuf *sb, int unitLength, bool bigEndian) {
	// MUST be zero-initialised: on EOF the loop below breaks early and leaves the remaining
	// bytes unwritten. Reading them back was undefined behaviour, and the resulting garbage
	// could compare equal to the 0x000A line terminator – which made UTF-16 line splitting
	// depend on whatever happened to be on the stack.
	uint8_t streamBytes[4] = {0, 0, 0, 0};
	codeUnitReturn_t ret;
	int octet = 0;
	// write unitLength bytes into codeUnit32 in given order (BE or LE)
	for(int i=0; i<unitLength; ++i) {
		octet = sb->sbumpc();
		if( octet == EOF )
			break;
		streamBytes[i] = (uint8_t) octet;
	}
	ret.octet = octet;
	if( unitLength == 4 ) {
		if( bigEndian )
			ret.codeUnit32 = Helper::pack32(streamBytes[3],streamBytes[2],streamBytes[1],streamBytes[0]);	// Big Endian
		else
			ret.codeUnit32 = Helper::pack32(streamBytes[0],streamBytes[1],streamBytes[2],streamBytes[3]);	// Little Endian
	} else if( unitLength == 2 ) {
		if( bigEndian )
			ret.codeUnit16 = Helper::pack16(streamBytes[1],streamBytes[0]);						// Big Endian
		else
			ret.codeUnit16 = Helper::pack16(streamBytes[0],streamBytes[1]);						// Little Endian
	}
	return ret;
}








