/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tc_parsecheck — headless CSV parser test harness for Tablecruncher.
 *
 * Links WITHOUT FLTK. If this target compiles, the data layer is still properly
 * decoupled from the UI layer (see docs/dev/parallel-loading-plan.md §7).
 *
 * Modes:
 *   --serial FILE           parse serially, print rows/cols/histogram/row-hash
 *   --golden-write DIR      parse every entry of DIR/manifest.tsv, write DIR/goldens.tsv
 *   --golden-check DIR      re-parse and compare against DIR/goldens.tsv
 *   --bench FILE            time the serial parse
 *
 * Dialect flags (used by --serial / --bench):
 *   --delim T --quote T --escape T --enc NAME --bom N
 *   where T is a single character or one of TAB COMMA SEMI PIPE COLON ASTER DQUOTE BSLASH
 *   and NAME is one of AUTO UTF8 NONE UTF16LE UTF16BE UTF32LE UTF32BE LATIN1 LATIN9 WIN1252
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <string>
#include <vector>

#include "csvparser.hh"
#include "csvdatastorage.hh"
#include "globals.hh"
#include "helper.hh"
#include "mappedfile.hh"
#include "utf8validate.hh"
#include "csvloader.hh"
#include "utf8.h"


/* ------------------------------------------------------------------ helpers */

static uint64_t fnv1a64(const std::string& s, uint64_t h = 1469598103934665603ULL) {
	for( unsigned char c : s ) {
		h ^= (uint64_t) c;
		h *= 1099511628211ULL;
	}
	return h;
}


static bool parseCharToken(const std::string& tok, char& out) {
	if( tok.size() == 1 ) { out = tok[0]; return true; }
	if( tok == "TAB" )    { out = '\t'; return true; }
	if( tok == "COMMA" )  { out = ',';  return true; }
	if( tok == "SEMI" )   { out = ';';  return true; }
	if( tok == "PIPE" )   { out = '|';  return true; }
	if( tok == "COLON" )  { out = ':';  return true; }
	if( tok == "ASTER" )  { out = '*';  return true; }
	if( tok == "DQUOTE" ) { out = '"';  return true; }
	if( tok == "BSLASH" ) { out = '\\'; return true; }
	return false;
}


static bool parseEncodingToken(const std::string& tok, CsvDefinition::Encodings& out, bool& autoDetect) {
	autoDetect = false;
	if( tok == "AUTO" )     { autoDetect = true; out = CsvDefinition::ENC_UTF8; return true; }
	if( tok == "UTF8" )     { out = CsvDefinition::ENC_UTF8;     return true; }
	if( tok == "NONE" )     { out = CsvDefinition::ENC_NONE;     return true; }
	if( tok == "UTF16LE" )  { out = CsvDefinition::ENC_UTF16LE;  return true; }
	if( tok == "UTF16BE" )  { out = CsvDefinition::ENC_UTF16BE;  return true; }
	if( tok == "UTF32LE" )  { out = CsvDefinition::ENC_UTF32LE;  return true; }
	if( tok == "UTF32BE" )  { out = CsvDefinition::ENC_UTF32BE;  return true; }
	if( tok == "LATIN1" )   { out = CsvDefinition::ENC_Latin1;   return true; }
	if( tok == "LATIN9" )   { out = CsvDefinition::ENC_Latin9;   return true; }
	if( tok == "WIN1252" )  { out = CsvDefinition::ENC_Win1252;  return true; }
	return false;
}


/*
 *	Mirrors CsvApplication::guessEncoding(std::istream*) — which lives behind FLTK and so
 *	cannot be linked here — including its two quirks:
 *	  - the UTF-8 validity test starts at byte offset 4, because four read() calls have
 *	    already consumed 4 bytes when utf8::is_valid() runs
 *	  - files >= TCRUNCHER_NUM_UTF8_TEST_BYTES are never validated, so they stay ENC_NONE
 *	Only the validation quirks are re-stated here; BOM detection comes from the shared
 *	CsvDefinition::fromBom() so there is no second copy of that table.
 */
static std::pair<CsvDefinition::Encodings,int> detectEncoding(const std::string& path, long fileLength) {
	CsvDefinition::Encodings enc = CsvDefinition::ENC_NONE;
	int bomBytes = 0;
	unsigned char octet[4] = {0,0,0,0};

	std::ifstream in(path, std::ios::binary);
	if( !in ) return {enc, bomBytes};
	in.read((char*)octet, 4);
	std::streamsize got = in.gcount();
	for( std::streamsize i = got; i < 4; ++i ) octet[i] = 0;

	enc = CsvDefinition::fromBom(octet, bomBytes);

	// the iterator was constructed before the 4 read() calls, but reads from the same
	// streambuf – so validation effectively starts at offset 4
	std::istreambuf_iterator<char> it(in.rdbuf());
	std::istreambuf_iterator<char> eos;
	if( fileLength < TCRUNCHER_NUM_UTF8_TEST_BYTES && utf8::is_valid(it, eos) ) {
		enc = CsvDefinition::ENC_UTF8;
	}
	return {enc, bomBytes};
}


struct Summary {
	long rows = 0;
	long cols = 0;
	std::string histo;
	uint64_t hash = 0;
	bool ok = false;
	bool refused = false;				// planLoad() declined the parallel path
	std::string refusedReason;
};


static std::string histoToString(const std::map<long,long>& h) {
	std::string s;
	for( auto const& kv : h ) {
		if( !s.empty() ) s += ",";
		s += std::to_string(kv.first) + ":" + std::to_string(kv.second);
	}
	if( s.empty() ) s = "-";
	return s;
}


static bool g_skipHash = false;			// benchmarks measure parsing, not FNV over the result

static Summary summarise(CsvDataStorage& storage, const std::map<long,long>& histo,
                         std::vector<std::string>* rowsOut) {
	Summary sum;
	sum.rows = storage.rows();
	sum.cols = storage.columns();
	sum.histo = histoToString(histo);
	uint64_t h = 1469598103934665603ULL;
	for( table_index_t r = 0; !g_skipHash && r < storage.rows(); ++r ) {
		std::string row = storage.getRow(r);
		h = fnv1a64(row, h);
		h ^= 0xFF; h *= 1099511628211ULL;			// row separator, so row splits are hashed distinctly
		if( rowsOut ) rowsOut->push_back(row);
	}
	sum.hash = h;
	sum.ok = true;
	return sum;
}


static Summary runSerial(const std::string& path, CsvDefinition def, std::vector<std::string>* rowsOut = nullptr) {
	std::ifstream input(path, std::ios::binary);
	if( !input ) {
		fprintf(stderr, "cannot open %s\n", path.c_str());
		return Summary();
	}
	CsvDataStorage storage;
	CsvParser parser;
	// mirror what CsvWindow::loadFile() does, so benchmarks reflect the real load path
	storage.reserveRows( CsvParser::estimateRowCount(input, Helper::getFileSize(path)) );
	std::map<long,long> histo = parser.parseCsvStream(&input, storage, &def);
	return summarise(storage, histo, rowsOut);
}


static Summary runBufferMem(const char* data, uint64_t len, CsvDefinition def,
                            std::vector<std::string>* rowsOut = nullptr) {
	CsvDataStorage storage;
	CsvParser parser;
	storage.reserveRows( CsvParser::estimateRowCount(data, len) );
	std::map<long,long> histo = parser.parseCsvBuffer(data, len, storage, &def);
	return summarise(storage, histo, rowsOut);
}


static Summary runBuffer(const std::string& path, CsvDefinition def, std::vector<std::string>* rowsOut = nullptr) {
	MappedFile mf;
	if( !mf.open(path) ) {
		fprintf(stderr, "cannot map %s\n", path.c_str());
		return Summary();
	}
	mf.adviseSequential();
	return runBufferMem(mf.data(), mf.size(), def, rowsOut);
}


static void printHexRow(const std::string& row) {
	for( size_t i = 0; i < row.size(); ++i ) {
		printf("%02X ", (unsigned char) row[i]);
		if( (i % 16) == 15 ) printf("\n");
	}
	printf("\n");
}


/* ------------------------------------------------------------------ manifest */

struct ManifestEntry {
	std::string name;
	CsvDefinition def;
	bool autoDetect = false;
};


static bool readManifest(const std::string& dir, std::vector<ManifestEntry>& out) {
	std::string mpath = dir + "/manifest.tsv";
	std::ifstream in(mpath);
	if( !in ) {
		fprintf(stderr, "cannot read manifest %s\n", mpath.c_str());
		return false;
	}
	std::string line;
	int lineNo = 0;
	while( std::getline(in, line) ) {
		++lineNo;
		if( line.empty() || line[0] == '#' ) continue;
		std::vector<std::string> f = Helper::splitString("\t", line);
		if( f.size() < 6 ) {
			fprintf(stderr, "%s:%d: expected 6 tab-separated fields\n", mpath.c_str(), lineNo);
			return false;
		}
		ManifestEntry e;
		e.name = f[0];
		if( !parseCharToken(f[1], e.def.delimiter) ||
		    !parseCharToken(f[2], e.def.quote) ||
		    !parseCharToken(f[3], e.def.escape) ||
		    !parseEncodingToken(f[4], e.def.encoding, e.autoDetect) ) {
			fprintf(stderr, "%s:%d: bad dialect token\n", mpath.c_str(), lineNo);
			return false;
		}
		e.def.bomBytes = std::atoi(f[5].c_str());
		out.push_back(e);
	}
	return true;
}


/*
 *	Compares two summaries and, when rows were captured, reports the first differing row.
 */
static bool compareRuns(const std::string& label, const Summary& a, const Summary& b,
                        const std::vector<std::string>& ra, const std::vector<std::string>& rb) {
	if( !a.ok || !b.ok ) {
		fprintf(stderr, "%s: a run failed\n", label.c_str());
		return false;
	}
	if( a.rows == b.rows && a.cols == b.cols && a.histo == b.histo && a.hash == b.hash )
		return true;

	fprintf(stderr, "DIFF %s\n", label.c_str());
	fprintf(stderr, "  rows  %ld / %ld\n", a.rows, b.rows);
	fprintf(stderr, "  cols  %ld / %ld\n", a.cols, b.cols);
	fprintf(stderr, "  histo %s / %s\n", a.histo.c_str(), b.histo.c_str());
	fprintf(stderr, "  hash  %016llx / %016llx\n", (unsigned long long) a.hash, (unsigned long long) b.hash);
	size_t n = std::min(ra.size(), rb.size());
	for( size_t i = 0; i < n; ++i ) {
		if( ra[i] != rb[i] ) {
			fprintf(stderr, "  first differing row %zu:\n    A: ", i);
			printHexRow(ra[i]);
			fprintf(stderr, "    B: ");
			printHexRow(rb[i]);
			break;
		}
	}
	return false;
}


/*
 *	Runs both the stream path and the flat-buffer path over the whole manifest and demands
 *	byte-for-byte agreement. Encodings the buffer path does not claim to handle are skipped.
 */
static int diffBuffer(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;

	int bad = 0, ran = 0, skipped = 0;
	for( auto& e : entries ) {
		std::string fpath = dir + "/" + e.name;
		CsvDefinition def = e.def;
		if( e.autoDetect ) {
			auto d = detectEncoding(fpath, Helper::getFileSize(fpath));
			def.encoding = d.first;
			def.bomBytes = d.second;
		}
		if( !CsvParser::bufferPathSupports(def.encoding) ) { ++skipped; continue; }
		std::vector<std::string> ra, rb;
		Summary a = runSerial(fpath, def, &ra);
		Summary b = runBuffer(fpath, def, &rb);
		++ran;
		if( !compareRuns(e.name + " [" + CsvDefinition::getEncodingName(def.encoding) + "]", a, b, ra, rb) )
			++bad;
	}
	if( bad ) { fprintf(stderr, "%d of %d differ\n", bad, ran); return 1; }
	printf("stream == buffer on all %d entries (%d skipped: encoding not on the buffer path)\n", ran, skipped);
	return 0;
}


static Summary runParallelMem(const char* data, uint64_t len, CsvDefinition def,
                              const std::vector<uint64_t>& forcedOffsets, size_t chunkCount,
                              unsigned threads, std::vector<std::string>* rowsOut = nullptr) {
	Utf8ValidationResult v = validateUtf8Parallel(data, len, def.bomBytes);
	LoadPlan plan = CsvLoader::planLoad(def, len, v, true, 0, true, /*forceParallel*/ true);
	if( plan.path != LoadPath::Parallel ) {
		Summary s;
		s.refused = true;
		s.refusedReason = plan.reason;
		return s;
	}
	if( chunkCount ) plan.chunkCount = chunkCount;
	if( threads )    plan.threads    = threads;
	plan.forcedNominalOffsets = forcedOffsets;

	CsvDataStorage storage;
	LoadProgress   progress;
	LoadResult     r = CsvLoader::loadParallel(data, len, def, plan, storage, progress);
	if( r.cancelled ) return Summary();
	return summarise(storage, r.histogram, rowsOut);
}


static Summary runParallel(const std::string& path, CsvDefinition def,
                           const std::vector<uint64_t>& forcedOffsets, size_t chunkCount,
                           unsigned threads, std::vector<std::string>* rowsOut = nullptr) {
	MappedFile mf;
	if( !mf.open(path) ) {
		fprintf(stderr, "cannot map %s\n", path.c_str());
		return Summary();
	}
	return runParallelMem(mf.data(), mf.size(), def, forcedOffsets, chunkCount, threads, rowsOut);
}


/*
 *	Resolves a manifest entry's dialect, applying AUTO detection when asked for.
 */
static CsvDefinition resolveDef(const std::string& fpath, const ManifestEntry& e) {
	CsvDefinition def = e.def;
	if( e.autoDetect ) {
		auto d = detectEncoding(fpath, Helper::getFileSize(fpath));
		def.encoding = d.first;
		def.bomBytes = d.second;
	}
	return def;
}


/*
 *	Runs the whole corpus through the parallel loader at a range of chunk counts and demands
 *	byte-for-byte agreement with the serial buffer path.
 */
static int diffParallel(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;

	const size_t chunkCounts[] = { 1, 2, 3, 5, 8, 17, 64 };
	int bad = 0, ran = 0, refused = 0;
	for( auto& e : entries ) {
		std::string fpath = dir + "/" + e.name;
		CsvDefinition def = resolveDef(fpath, e);

		std::vector<std::string> ra;
		Summary a = runBuffer(fpath, def, &ra);

		for( size_t k : chunkCounts ) {
			std::vector<std::string> rb;
			Summary b = runParallel(fpath, def, {}, k, 0, &rb);
			if( b.refused ) {
				if( k == chunkCounts[0] ) {
					++refused;
					printf("  refused: %-36s %s\n", e.name.c_str(), b.refusedReason.c_str());
				}
				break;
			}
			++ran;
			char label[256];
			snprintf(label, sizeof(label), "%s [%s, %zu chunks]",
			         e.name.c_str(), CsvDefinition::getEncodingName(def.encoding).c_str(), k);
			if( !compareRuns(label, a, b, ra, rb) ) ++bad;
		}
	}
	if( bad ) { fprintf(stderr, "%d of %d parallel runs differ\n", bad, ran); return 1; }
	printf("parallel == serial on all %d runs (%d files refused the parallel path)\n", ran, refused);
	return 0;
}


/*
 *	The highest-value test in the suite.
 *
 *	Forces a chunk boundary at EVERY byte offset of the file and diffs each parallel parse
 *	against the serial one, then sweeps pairs and triples of simultaneous boundaries to
 *	exercise carry propagation and the "chunk contains no record start" absorption. This is
 *	what proves boundaries landing mid-quoted-field, between the CR and LF of a CRLF, inside a
 *	multi-byte sequence, on a NUL, right after an escape character and inside a doubled quote
 *	all produce identical output.
 */
static int sweep(const std::string& path, CsvDefinition def, bool quiet) {
	int64_t sz = Helper::getFileSize(path);
	if( sz < 0 ) { fprintf(stderr, "cannot stat %s\n", path.c_str()); return 2; }
	const uint64_t len = (uint64_t) sz;

	std::vector<std::string> ra;
	Summary a = runBuffer(path, def, &ra);
	if( !a.ok ) return 2;

	{	// make sure the parallel path is even eligible before claiming the sweep proves anything
		Summary probe = runParallel(path, def, {}, 1, 1);
		if( probe.refused ) {
			if( !quiet )
				printf("  sweep skipped %-30s (%s)\n", path.c_str(), probe.refusedReason.c_str());
			return 0;
		}
	}

	int bad = 0;
	long cases = 0;
	for( uint64_t off = 0; off <= len; ++off ) {
		std::vector<std::string> rb;
		Summary b = runParallel(path, def, { off }, 0, 1, &rb);
		++cases;
		char label[256];
		snprintf(label, sizeof(label), "%s @1 boundary %llu", path.c_str(), (unsigned long long) off);
		if( !compareRuns(label, a, b, ra, rb) ) { ++bad; if( bad > 3 ) break; }
	}

	// pairs and triples, on a coarser grid so the run stays quick
	const uint64_t stride = std::max<uint64_t>(1, len / 60);
	for( uint64_t o1 = 0; o1 <= len && bad <= 3; o1 += stride ) {
		for( uint64_t o2 = o1; o2 <= len && bad <= 3; o2 += stride ) {
			std::vector<std::string> rb;
			Summary b = runParallel(path, def, { o1, o2 }, 0, 1, &rb);
			++cases;
			char label[256];
			snprintf(label, sizeof(label), "%s @2 boundaries %llu,%llu",
			         path.c_str(), (unsigned long long) o1, (unsigned long long) o2);
			if( !compareRuns(label, a, b, ra, rb) ) { ++bad; continue; }

			for( uint64_t o3 = o2; o3 <= len && bad <= 3; o3 += stride * 7 ) {
				std::vector<std::string> rc;
				Summary c = runParallel(path, def, { o1, o2, o3 }, 0, 1, &rc);
				++cases;
				snprintf(label, sizeof(label), "%s @3 boundaries %llu,%llu,%llu",
				         path.c_str(), (unsigned long long) o1, (unsigned long long) o2,
				         (unsigned long long) o3);
				if( !compareRuns(label, a, c, ra, rc) ) ++bad;
			}
		}
	}

	if( bad ) { fprintf(stderr, "%s: %d of %ld sweep cases differ\n", path.c_str(), bad, cases); return 1; }
	if( !quiet ) printf("  %-40s %ld boundary placements, all identical\n", path.c_str(), cases);
	return 0;
}


static int sweepCorpus(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;
	int bad = 0;
	for( auto& e : entries ) {
		std::string fpath = dir + "/" + e.name;
		if( Helper::getFileSize(fpath) > 20000 ) continue;		// sweeping is O(len^2)-ish
		if( sweep(fpath, resolveDef(fpath, e), false) ) ++bad;
	}
	if( bad ) { fprintf(stderr, "%d corpus files failed the boundary sweep\n", bad); return 1; }
	printf("boundary sweep clean over the corpus\n");
	return 0;
}


/*
 *	Differential fuzz: random content over the bytes that actually drive the state machine,
 *	random dialects, random chunk counts. The hand-written corpus only catches what we already
 *	thought of; this is what would catch an FSM/prescan divergence nobody anticipated.
 */
static int fuzzParallel(int iterations) {
	unsigned seed = 987654321u;
	auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 8); };

	const char delims[6]  = { ',', ';', '\t', '|', ':', '*' };
	const char escapes[3] = { '"', '\\', '*' };

	int bad = 0;
	long compared = 0, refused = 0;
	for( int it = 0; it < iterations; ++it ) {
		CsvDefinition def;
		def.delimiter = delims[rnd() % 6];
		def.quote     = '"';
		def.escape    = escapes[rnd() % 3];
		def.encoding  = CsvDefinition::ENC_UTF8;
		def.bomBytes  = 0;

		const size_t target = 40 + (rnd() % 900);
		std::string buf;
		buf.reserve(target + 8);
		while( buf.size() < target ) {
			switch( rnd() % 16 ) {
				case 0:  buf.push_back(def.delimiter);      break;
				case 1:  buf.push_back(def.quote);          break;
				case 2:  buf.push_back(def.escape);         break;
				case 3:  buf.push_back(',');                break;
				case 4:  buf.push_back('"');                break;
				case 5:  buf.push_back('\\');               break;
				case 6:  buf.push_back('\r');               break;
				case 7:  case 8:
				         buf.push_back('\n');               break;
				case 9:  buf.push_back('\0');               break;
				case 10: buf.append("\xC3\xA9");             break;   // 2-byte
				case 11: buf.append("\xE2\x82\xAC");         break;   // 3-byte
				case 12: buf.append("\xF0\x9D\x84\x9E");     break;   // 4-byte
				default: buf.push_back((char)(0x20 + (rnd() % 0x5F)));   // printable ASCII
			}
		}

		std::vector<std::string> ra;
		Summary a = runBufferMem(buf.data(), buf.size(), def, &ra);

		for( int rep = 0; rep < 3; ++rep ) {
			size_t chunks = 1 + (rnd() % 8);
			std::vector<std::string> rb;
			Summary b = runParallelMem(buf.data(), buf.size(), def, {}, chunks, 1, &rb);
			if( b.refused ) { ++refused; break; }
			++compared;
			char label[160];
			snprintf(label, sizeof(label), "fuzz#%d delim=%02X esc=%02X chunks=%zu",
			         it, (unsigned char) def.delimiter, (unsigned char) def.escape, chunks);
			if( !compareRuns(label, a, b, ra, rb) ) {
				++bad;
				// dump the offending input so the case can be reproduced
				fprintf(stderr, "  input (%zu bytes):", buf.size());
				for( size_t i = 0; i < buf.size(); ++i ) fprintf(stderr, " %02X", (unsigned char) buf[i]);
				fprintf(stderr, "\n");
				break;
			}
		}
		if( bad >= 3 ) break;
	}
	if( bad ) { fprintf(stderr, "%d fuzz divergences\n", bad); return 1; }
	printf("differential fuzz: %ld parallel/serial comparisons identical (%ld inputs refused)\n",
	       compared, refused);
	return 0;
}


/*
 *	Every parseCsvStream() call must be self-contained.
 *
 *	CsvApplication::guessDefinition reuses ONE CsvParser across its eight dialect probes, so
 *	any state a parser keeps between calls leaks from one probe into the next and can change
 *	the dialect chosen for the file. This parses each corpus file twice through a single
 *	parser and demands the second result match a fresh one.
 */
static int reuseCheck(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;

	int bad = 0, ran = 0;
	for( auto& e : entries ) {
		const std::string fpath = dir + "/" + e.name;
		if( Helper::getFileSize(fpath) > 200000 ) continue;
		CsvDefinition def = resolveDef(fpath, e);

		// the reference: a parser that has never been used before
		std::vector<std::string> ra;
		Summary fresh = runSerial(fpath, def, &ra);

		// the same file, through a parser that has already parsed something else
		CsvParser parser;
		for( const char* warmup : { "a,\"unterminated\n", "x,y,z\n" } ) {
			std::istringstream warm(warmup);
			CsvDataStorage scratchStorage;
			CsvDefinition  warmDef = def;
			parser.parseCsvStream(&warm, scratchStorage, &warmDef, 10, false);
		}
		std::ifstream in(fpath, std::ios::binary);
		if( !in ) continue;
		CsvDataStorage storage;
		CsvDefinition  again = def;
		std::map<long,long> histo = parser.parseCsvStream(&in, storage, &again);
		std::vector<std::string> rb;
		Summary reused = summarise(storage, histo, &rb);

		++ran;
		if( !compareRuns(e.name + " [reused parser]", fresh, reused, ra, rb) )
			++bad;
	}
	if( bad ) { fprintf(stderr, "%d of %d files parse differently through a reused parser\n", bad, ran); return 1; }
	printf("parseCsvStream is self-contained on all %d files\n", ran);
	return 0;
}


/* ------------------------------------------------------- column content lengths */

/*
 *	CsvDataStorage::columnContentLengths() replaces a per-cell get(R,C) loop that rescanned
 *	each row from byte zero for every column. This checks the two agree exactly – including
 *	the integer-division average and the ""-for-a-missing-field rule.
 */
static int colCheck(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;

	int bad = 0, ran = 0;
	for( auto& e : entries ) {
		std::string fpath = dir + "/" + e.name;
		CsvDefinition def = resolveDef(fpath, e);

		MappedFile mf;
		if( !mf.open(fpath) ) continue;
		CsvDataStorage storage;
		CsvParser parser;
		if( CsvParser::bufferPathSupports(def.encoding) )
			parser.parseCsvBuffer(mf.data(), mf.size(), storage, &def);
		else {
			std::ifstream in(fpath, std::ios::binary);
			parser.parseCsvStream(&in, storage, &def);
		}

		for( table_index_t probe : { (table_index_t) 0, (table_index_t) 1, (table_index_t) 3, (table_index_t) 100000 } ) {
			std::vector<std::pair<int,int>> fast = storage.columnContentLengths(probe);

			// the naive reference: exactly what CsvTable::maximumContentLength used to do
			const table_index_t C = storage.columns();
			table_index_t probeRows = storage.rows();
			if( probe > 0 ) probeRows = std::min(probeRows, probe);
			std::vector<std::pair<int,int>> slow;
			if( C > 0 && probeRows > 0 ) {
				for( table_index_t c = 0; c < C; ++c ) {
					int maxLen = 0;
					uint64_t sum = 0;
					for( table_index_t r = 0; r < probeRows; ++r ) {
						int l = (int) storage.get(r, c).length();
						if( l > maxLen ) maxLen = l;
						sum += (uint64_t) l;
					}
					slow.push_back({ maxLen, (int)(sum / (uint64_t) probeRows) });
				}
			} else if( C > 0 ) {
				slow.assign((size_t) C, {0,0});
			}

			++ran;
			if( fast != slow ) {
				fprintf(stderr, "COLUMN MISMATCH %s (probe %d): %zu vs %zu entries\n",
				        e.name.c_str(), (int) probe, fast.size(), slow.size());
				for( size_t c = 0; c < std::min(fast.size(), slow.size()); ++c ) {
					if( fast[c] != slow[c] )
						fprintf(stderr, "  col %zu: (%d,%d) vs (%d,%d)\n",
						        c, fast[c].first, fast[c].second, slow[c].first, slow[c].second);
				}
				++bad;
			}
		}
	}
	if( bad ) { fprintf(stderr, "%d of %d column-length cases differ\n", bad, ran); return 1; }
	printf("columnContentLengths matches the per-cell reference on all %d cases\n", ran);
	return 0;
}


/*
 *	Times the one-pass column scan against the per-cell formulation it replaces.
 */
static int benchCols(const std::string& path, CsvDefinition def) {
	MappedFile mf;
	if( !mf.open(path) ) { fprintf(stderr, "cannot map %s\n", path.c_str()); return 2; }
	CsvDataStorage storage;
	CsvParser parser;
	storage.reserveRows( CsvParser::estimateRowCount(mf.data(), mf.size()) );
	parser.parseCsvBuffer(mf.data(), mf.size(), storage, &def);

	const table_index_t probe = std::min<table_index_t>(storage.rows(), TCRUNCHER_MAX_PROBE_ROWS_ARRANGE_COLS);
	printf("table: %d rows x %d cols, probing %d rows\n", (int) storage.rows(), (int) storage.columns(), (int) probe);

	auto t0 = std::chrono::steady_clock::now();
	std::vector<std::pair<int,int>> fast = storage.columnContentLengths(probe);
	auto t1 = std::chrono::steady_clock::now();
	printf("one-pass : %8.1f ms\n", std::chrono::duration<double,std::milli>(t1 - t0).count());

	t0 = std::chrono::steady_clock::now();
	uint64_t sink = 0;
	for( table_index_t c = 0; c < storage.columns(); ++c )
		for( table_index_t r = 0; r < probe; ++r )
			sink += storage.get(r, c).length();
	t1 = std::chrono::steady_clock::now();
	printf("per-cell : %8.1f ms  (checksum %llu, %zu cols measured)\n",
	       std::chrono::duration<double,std::milli>(t1 - t0).count(),
	       (unsigned long long) sink, fast.size());
	return 0;
}


/* -------------------------------------------------------------- utf8 validation */

/*
 *	Checks validateUtf8Parallel() against utf8::is_valid() for one buffer, sweeping the
 *	thread count so that the range boundaries land on many different byte offsets.
 */
static int utf8CheckBuffer(const std::string& label, const char* data, uint64_t len, int bomBytes) {
	auto oracle = [&](uint64_t from) {
		uint64_t f = std::min<uint64_t>(from, len);
		return utf8::is_valid(data + f, data + len);
	};
	const bool want0   = oracle(0);
	const bool want4   = oracle(4);
	const bool wantBom = oracle(bomBytes < 0 ? 0 : (uint64_t) bomBytes);

	int bad = 0;
	for( unsigned t = 1; t <= 17; ++t ) {
		Utf8ValidationResult v = validateUtf8Parallel(data, len, bomBytes, t, true);
		if( v.validFrom0 != want0 || v.validFrom4 != want4 || v.validFromBom != wantBom ) {
			fprintf(stderr, "UTF8 MISMATCH %s (len %llu, bom %d, threads %u)\n",
			        label.c_str(), (unsigned long long) len, bomBytes, t);
			fprintf(stderr, "  from0   got %d want %d\n", (int) v.validFrom0,   (int) want0);
			fprintf(stderr, "  from4   got %d want %d\n", (int) v.validFrom4,   (int) want4);
			fprintf(stderr, "  fromBom got %d want %d\n", (int) v.validFromBom, (int) wantBom);
			++bad;
			break;
		}
	}
	return bad;
}


static int utf8Check(const std::string& dir) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;
	int bad = 0, ran = 0;
	for( auto& e : entries ) {
		std::ifstream in(dir + "/" + e.name, std::ios::binary);
		if( !in ) continue;
		std::string buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		for( int bom = 0; bom <= 4; ++bom ) {
			bad += utf8CheckBuffer(e.name, buf.data(), buf.size(), bom);
			++ran;
		}
	}
	if( bad ) { fprintf(stderr, "%d utf8 mismatches over %d cases\n", bad, ran); return 1; }
	printf("utf8 validation matches utf8::is_valid on all %d corpus cases\n", ran);
	return 0;
}


/*
 *	Random buffers built from a mix of ASCII, well-formed multibyte sequences and every way a
 *	sequence can be malformed – then validated with forced range splitting so boundaries land
 *	inside sequences, inside orphan continuation runs, and on the buffer's edges.
 */
static int utf8Fuzz(int iterations) {
	unsigned seed = 12345;
	auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 8); };

	int bad = 0;
	int validCases = 0;
	for( int it = 0; it < iterations; ++it ) {
		size_t len = 65 + (rnd() % 3000);
		// half the buffers are built from well-formed pieces only, so the "valid" verdict is
		// exercised as hard as the "invalid" one
		const bool wellFormedOnly = ((it & 1) == 0);
		std::string buf;
		buf.reserve(len + 8);
		while( buf.size() < len ) {
			switch( rnd() % (wellFormedOnly ? 7u : 12u) ) {
				case 0: case 1: case 2: case 3:
					buf.push_back((char) (0x20 + (rnd() % 0x5F)));            break;   // ASCII
				case 4:
					buf.append("\xC3\xA9");                                   break;   // 2-byte
				case 5:
					buf.append("\xE2\x82\xAC");                               break;   // 3-byte
				case 6:
					buf.append("\xF0\x9D\x84\x9E");                           break;   // 4-byte
				case 7:
					buf.push_back((char) (0x80 + (rnd() % 0x40)));            break;   // orphan continuation
				case 8:
					buf.append("\xC0\xAF");                                   break;   // overlong
				case 9:
					buf.append("\xED\xA0\x80");                               break;   // surrogate
				case 10:
					buf.push_back((char) 0xF0);                               break;   // truncated lead
				default:
					buf.push_back((char) 0x00);                               break;   // NUL
			}
		}
		int bom = (int) (rnd() % 5);
		if( utf8::is_valid(buf.data(), buf.data() + buf.size()) ) ++validCases;
		bad += utf8CheckBuffer("fuzz#" + std::to_string(it), buf.data(), buf.size(), bom);
		if( bad > 5 ) break;
	}

	// one buffer past the real PARALLEL_THRESHOLD, without the forceSplit hook, so the
	// production code path (actual std::threads) is exercised too
	{
		std::string big;
		big.reserve(3u << 20);
		while( big.size() < (3u << 20) ) big.append("héllo,wörld,\xF0\x9D\x84\x9E,plain\n");
		Utf8ValidationResult v = validateUtf8Parallel(big.data(), big.size(), 0);
		if( !v.validFrom0 || !v.validFrom4 || !v.validFromBom ) {
			fprintf(stderr, "UTF8 MISMATCH: valid 3 MB buffer rejected on the threaded path\n");
			++bad;
		}
		// A lead byte followed by a non-continuation byte is invalid no matter where it
		// lands – either on its own, or by breaking the sequence it was written into.
		big[2u << 20]     = (char) 0xC3;
		big[(2u << 20)+1] = (char) 0x41;
		const bool wantBig = utf8::is_valid(big.data(), big.data() + big.size());
		v = validateUtf8Parallel(big.data(), big.size(), 0);
		if( wantBig ) {
			fprintf(stderr, "UTF8 TEST BROKEN: corrupted 3 MB buffer is still valid\n");
			++bad;
		} else if( v.validFrom0 ) {
			fprintf(stderr, "UTF8 MISMATCH: corrupted 3 MB buffer accepted on the threaded path\n");
			++bad;
		}
	}

	if( bad ) { fprintf(stderr, "%d utf8 fuzz mismatches\n", bad); return 1; }
	printf("utf8 fuzz: %d random buffers x 17 thread counts (%d of them valid), all match utf8::is_valid;\n"
	       "           threaded path over a 3 MB buffer OK both ways\n", iterations, validCases);
	return 0;
}


static int goldens(const std::string& dir, bool write) {
	std::vector<ManifestEntry> entries;
	if( !readManifest(dir, entries) ) return 2;

	std::string gpath = dir + "/goldens.tsv";
	std::vector<std::string> lines;
	for( auto& e : entries ) {
		std::string fpath = dir + "/" + e.name;
		CsvDefinition def = e.def;
		if( e.autoDetect ) {
			auto d = detectEncoding(fpath, Helper::getFileSize(fpath));
			def.encoding = d.first;
			def.bomBytes = d.second;
		}
		Summary s = runSerial(fpath, def);
		if( !s.ok ) return 2;
		char buf[256];
		snprintf(buf, sizeof(buf), "%ld\t%ld\t%s\t%016llx",
		         s.rows, s.cols, s.histo.c_str(), (unsigned long long) s.hash);
		lines.push_back(e.name + "\t" + buf);
	}

	if( write ) {
		std::ofstream out(gpath, std::ios::trunc);
		if( !out ) { fprintf(stderr, "cannot write %s\n", gpath.c_str()); return 2; }
		for( auto& l : lines ) out << l << "\n";
		printf("wrote %zu goldens to %s\n", lines.size(), gpath.c_str());
		return 0;
	}

	std::ifstream in(gpath);
	if( !in ) { fprintf(stderr, "cannot read %s (run --golden-write first)\n", gpath.c_str()); return 2; }
	std::vector<std::string> expected;
	std::string l;
	while( std::getline(in, l) ) if( !l.empty() ) expected.push_back(l);

	int bad = 0;
	if( expected.size() != lines.size() ) {
		fprintf(stderr, "golden count mismatch: have %zu, expected %zu\n", lines.size(), expected.size());
		++bad;
	}
	size_t n = std::min(expected.size(), lines.size());
	for( size_t i = 0; i < n; ++i ) {
		if( expected[i] != lines[i] ) {
			fprintf(stderr, "MISMATCH\n  expected: %s\n  actual:   %s\n", expected[i].c_str(), lines[i].c_str());
			++bad;
		}
	}
	if( bad ) { fprintf(stderr, "%d golden mismatch(es)\n", bad); return 1; }
	printf("all %zu goldens match\n", lines.size());
	return 0;
}


/* ---------------------------------------------------------------------- main */

static void usage() {
	printf(
		"tc_parsecheck — headless Tablecruncher parser tests\n"
		"\n"
		"  --serial FILE [--dump-rows]   parse via the istream path and print a summary\n"
		"  --buffer FILE [--dump-rows]   parse via the mmap path and print a summary\n"
		"  --diff-buffer DIR             demand byte-for-byte agreement of both paths over the corpus\n"
		"  --golden-write DIR            write DIR/goldens.tsv from DIR/manifest.tsv\n"
		"  --golden-check DIR            compare against DIR/goldens.tsv\n"
		"  --bench FILE [--repeat N]     time the istream parse\n"
		"  --bench-buffer FILE           time the mmap parse\n"
		"  --utf8check DIR               parallel UTF-8 validation vs utf8::is_valid over the corpus\n"
		"  --utf8fuzz N                  same, over N random malformed buffers\n"
		"  --parallel FILE [--dump-rows] parse via the parallel loader (forced on)\n"
		"  --diff-parallel DIR           parallel vs serial over the corpus at 7 chunk counts\n"
		"  --sweep FILE|DIR              force a chunk boundary at every byte offset and diff\n"
		"  --bench-parallel FILE         time the parallel load\n"
		"  --fuzz N                      differential fuzz: N random inputs, serial vs parallel\n"
		"  --colcheck DIR                columnContentLengths vs the per-cell reference\n"
		"  --reusecheck DIR              parseCsvStream must be self-contained across calls\n"
		"  --bench-cols FILE             time the column-width scan both ways\n"
		"\n"
		"dialect: --delim T --quote T --escape T --enc NAME --bom N\n"
		"  T    single char | TAB COMMA SEMI PIPE COLON ASTER DQUOTE BSLASH\n"
		"  NAME AUTO UTF8 NONE UTF16LE UTF16BE UTF32LE UTF32BE LATIN1 LATIN9 WIN1252\n");
}


int main(int argc, char** argv) {
	std::string mode, arg;
	CsvDefinition def;
	bool autoDetect = true;			// default: behave like the app and sniff the encoding
	bool dumpRows = false;
	int repeat = 1;

	for( int i = 1; i < argc; ++i ) {
		std::string a = argv[i];
		auto need = [&](const char* what) -> std::string {
			if( i + 1 >= argc ) { fprintf(stderr, "%s needs an argument\n", what); exit(2); }
			return argv[++i];
		};
		if( a == "--serial" || a == "--buffer" || a == "--bench" || a == "--bench-buffer"
		    || a == "--golden-write" || a == "--golden-check" || a == "--diff-buffer"
		    || a == "--utf8check" || a == "--utf8fuzz"
		    || a == "--parallel" || a == "--diff-parallel" || a == "--sweep" || a == "--bench-parallel"
		    || a == "--fuzz" || a == "--colcheck" || a == "--bench-cols" || a == "--reusecheck" ) {
			mode = a; arg = need(a.c_str());
		} else if( a == "--delim" )  { if(!parseCharToken(need("--delim"),  def.delimiter)) { fprintf(stderr,"bad --delim\n"); return 2; } }
		else if( a == "--quote" )  { if(!parseCharToken(need("--quote"),  def.quote))     { fprintf(stderr,"bad --quote\n"); return 2; } }
		else if( a == "--escape" ) { if(!parseCharToken(need("--escape"), def.escape))    { fprintf(stderr,"bad --escape\n"); return 2; } }
		else if( a == "--enc" )    { if(!parseEncodingToken(need("--enc"), def.encoding, autoDetect)) { fprintf(stderr,"bad --enc\n"); return 2; } }
		else if( a == "--bom" )    { def.bomBytes = std::atoi(need("--bom").c_str()); autoDetect = false; }
		else if( a == "--repeat" ) { repeat = std::atoi(need("--repeat").c_str()); }
		else if( a == "--dump-rows" ) { dumpRows = true; }
		else if( a == "--help" || a == "-h" ) { usage(); return 0; }
		else { fprintf(stderr, "unknown option: %s\n", a.c_str()); usage(); return 2; }
	}

	if( mode.empty() ) { usage(); return 2; }

	if( mode == "--golden-write" ) return goldens(arg, true);
	if( mode == "--golden-check" ) return goldens(arg, false);
	if( mode == "--diff-buffer" )  return diffBuffer(arg);
	if( mode == "--utf8check" )    return utf8Check(arg);
	if( mode == "--utf8fuzz" )     return utf8Fuzz(std::atoi(arg.c_str()));
	if( mode == "--diff-parallel" ) return diffParallel(arg);
	if( mode == "--fuzz" )          return fuzzParallel(std::atoi(arg.c_str()));
	if( mode == "--colcheck" )      return colCheck(arg);
	if( mode == "--reusecheck" )    return reuseCheck(arg);

	if( autoDetect ) {
		auto d = detectEncoding(arg, Helper::getFileSize(arg));
		def.encoding = d.first;
		def.bomBytes = d.second;
	}

	if( mode == "--sweep" ) {
		struct stat stbuf;
		if( stat(arg.c_str(), &stbuf) == 0 && S_ISDIR(stbuf.st_mode) )
			return sweepCorpus(arg);
		return sweep(arg, def, false);
	}

	if( mode == "--bench-cols" ) return benchCols(arg, def);

	if( mode == "--serial" || mode == "--buffer" || mode == "--parallel" ) {
		std::vector<std::string> rows;
		Summary s;
		if( mode == "--buffer" )        s = runBuffer(arg, def, dumpRows ? &rows : nullptr);
		else if( mode == "--parallel" ) s = runParallel(arg, def, {}, 0, 0, dumpRows ? &rows : nullptr);
		else                            s = runSerial(arg, def, dumpRows ? &rows : nullptr);
		if( s.refused ) { printf("parallel refused: %s\n", s.refusedReason.c_str()); return 3; }
		if( !s.ok ) return 2;
		printf("file:  %s\n", arg.c_str());
		printf("enc:   %s (bom %d)\n", CsvDefinition::getEncodingName(def.encoding).c_str(), def.bomBytes);
		printf("rows:  %ld\n", s.rows);
		printf("cols:  %ld\n", s.cols);
		printf("histo: %s\n", s.histo.c_str());
		printf("hash:  %016llx\n", (unsigned long long) s.hash);
		if( dumpRows ) {
			for( size_t r = 0; r < rows.size(); ++r ) {
				printf("--- row %zu (%zu bytes)\n", r, rows[r].size());
				printHexRow(rows[r]);
			}
		}
		return 0;
	}

	if( mode == "--bench" || mode == "--bench-buffer" || mode == "--bench-parallel" ) {
		g_skipHash = true;
		double best = 1e30;
		Summary s;
		for( int i = 0; i < repeat; ++i ) {
			auto t0 = std::chrono::steady_clock::now();
			if( mode == "--bench-buffer" )        s = runBuffer(arg, def);
			else if( mode == "--bench-parallel" ) s = runParallel(arg, def, {}, 0, 0);
			else                                  s = runSerial(arg, def);
			auto t1 = std::chrono::steady_clock::now();
			double secs = std::chrono::duration<double>(t1 - t0).count();
			if( secs < best ) best = secs;
		}
		if( s.refused ) { printf("parallel refused: %s\n", s.refusedReason.c_str()); return 3; }
		if( !s.ok ) return 2;
		int64_t bytes = Helper::getFileSize(arg);
		const char* label = (mode == "--bench-buffer") ? "buffer:"
		                  : (mode == "--bench-parallel") ? "parallel:" : "serial:";
		printf("%-10s %.3f s  %.1f MB/s  %.0f rows/s  (%ld rows x %ld cols)\n",
		       label, best, (bytes / (1024.0*1024.0)) / best, s.rows / best, s.rows, s.cols);
		return 0;
	}

	usage();
	return 2;
}
