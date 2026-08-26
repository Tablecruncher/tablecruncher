#!/bin/sh
#
# Generates the test corpus into tests/data/ (gitignored).
#
#   ./tests/gen_corpus.sh            small correctness corpus + manifest
#   ./tests/gen_corpus.sh --big      also the ~120-160 MB benchmark files (bench_*)
#
set -e
DIR="$(cd "$(dirname "$0")" && pwd)/data"
mkdir -p "$DIR"
BIG=0
[ "$1" = "--big" ] && BIG=1
DIR="$DIR" BIG="$BIG" python3 - <<'PY'
import os, random

D   = os.environ["DIR"]
BIG = os.environ["BIG"] == "1"
manifest = []


def w(name, data, delim=",", quote='"', escape='"', enc="AUTO", bom=0, in_manifest=True):
    if isinstance(data, str):
        data = data.encode("utf-8")
    with open(os.path.join(D, name), "wb") as f:
        f.write(data)
    if in_manifest:
        manifest.append((name, delim, quote, escape, enc, bom))


def alias(name, delim=",", quote='"', escape='"', enc="AUTO", bom=0):
    """Add another manifest row for an existing file, under a different dialect."""
    manifest.append((name, delim, quote, escape, enc, bom))


# ---------------------------------------------------------------- line endings
w("plain_lf",                "a,b,c\n1,2,3\n4,5,6\n")
w("plain_lf_no_trailing_nl", "a,b,c\n1,2,3\n4,5,6")
w("plain_crlf",              "a,b,c\r\n1,2,3\r\n4,5,6\r\n")
w("plain_cr_only",           "a,b,c\r1,2,3\r4,5,6\r")
w("mixed_endings",           "a,b,c\n1,2,3\r\n4,5,6\r7,8,9\n")

# -------------------------------------------------------------------- quoting
w("quoted_embedded_lf",        'a,"line1\nline2",c\nd,e,f\n')
w("quoted_embedded_crlf",      'a,"line1\r\nline2",c\r\nd,e,f\r\n')
w("doubled_quotes",            'a,"he said ""hi""",c\n1,2,3\n')
w("doubled_quotes_unenclosed", 'a,""x,c\n1,2,3\n')          # "" opens a quote, consumes ONE char
w("quote_midfield",            'a,mid"quote,c\n1,2,3\n')
w("quote_after_space",         'a, "spaced",c\n1,2,3\n')
w("quote_at_eol",              'a,b,c"\n1,2,3\n')
w("unterminated_quote_at_eof", 'a,b,c\n1,"unterminated\n')  # deleteRows() eats the last REAL row
w("all_quotes",                '""""""""""""\n""""""\n')    # the non-converging dual-hypothesis case
w("quote_then_eof_no_nl",      'a,"open')

# --------------------------------------------------------- escape dialect (\)
w("bs_escaped_delim",  "a,b\\,c,d\n1,2,3\n",     escape="BSLASH")
w("bs_escaped_quote",  'a,b\\"c,d\n1,2,3\n',     escape="BSLASH")
w("bs_at_eol",         "a,b,c\\\n1,2,3\n",       escape="BSLASH")   # trailing \ is NOT an escape
w("bs_before_crlf",    "a,b,c\\\r\n1,2,3\r\n",   escape="BSLASH")
w("bs_double",         "a,b\\\\c,d\n1,2,3\n",    escape="BSLASH")
w("bs_before_nul",     b"a,b\\\x00\"c,d\n1,2,3\n", escape="BSLASH") # NUL removed first -> \" 

# ------------------------------------------------------------------ raggedness
w("ragged_growing",     "a\na,b\na,b,c\na,b,c,d\n")
w("ragged_shrinking",   "a,b,c,d\na,b,c\na,b\na\n")
w("ragged_widest_last", "a,b\na,b\na,b\na,b,c,d,e\n")
w("ragged_widest_first","a,b,c,d,e\na,b\na,b\na,b\n")

# ------------------------------------------------------------------------ NULs
w("nul_in_field",       b"a,b\x00c,d\n1,2,3\n")
w("nul_in_quotes",      b'a,"b\x00c",d\n1,2,3\n')
w("nul_adjacent_delim", b"a,\x00,b\n1,2,3\n")
w("bs_nul_quote",       b'a,\\\x00"b,c\n1,2,3\n', escape="BSLASH")

# ---------------------------------------------------------------- invalid UTF-8
w("invalid_lone_continuation",        b"a,b\x80c,d\n1,2,3\n")
w("invalid_truncated_lead_eats_delim", b'a,"abc\xf0"\nx,y\n')   # NOT_ENOUGH_ROOM drops the closing quote
w("invalid_overlong",                 b"a,\xc0\xaf,c\n1,2,3\n")
w("invalid_surrogate",                b"a,\xed\xa0\x80,c\n1,2,3\n")

# ------------------------------------------------------------------ 8-bit encodings
w("latin1",          b"a,caf\xe9,c\n1,2,3\n",              enc="LATIN1")
w("latin1_c1_bytes", b"a,x\x80\x9fy,c\n1,2,3\n",           enc="LATIN1")  # 0x80-0x9F are DROPPED
w("win1252",         b"a,caf\xe9 \x80,c\n1,2,3\n",         enc="WIN1252")
w("latin9",          b"a,caf\xe9,c\n1,2,3\n",              enc="LATIN9")

# ------------------------------------------------------------------- UTF-16/32
u16 = "a,b,c\n1,2,3\n"
w("utf16le_bom",       b"\xff\xfe" + u16.encode("utf-16-le"), enc="AUTO")
w("utf16be_bom",       b"\xfe\xff" + u16.encode("utf-16-be"), enc="AUTO")
w("utf16le_nobom",     u16.encode("utf-16-le"),              enc="UTF16LE")
# the preserved quirk: ASCII UTF-16LE WITH a BOM is detected as UTF-8 and parsed via NUL stripping
w("utf16le_ascii_bom", b"\xff\xfe" + u16.encode("utf-16-le"), enc="AUTO", in_manifest=False)
alias("utf16le_ascii_bom", enc="AUTO")
w("utf16_nonascii",    b"\xff\xfe" + "a,café,€\n1,2,3\n".encode("utf-16-le"), enc="AUTO")
# a UTF-16LE file whose first character is U+xx00: the low byte comes first, so the BOM test
# sees FF FE 00 00 and used to report UTF-32LE, which the reader cannot decode at all
w("utf16le_cjk",       b"\xff\xfe" + "\u4e00,\u4e8c,\u4e09\n1,2,3\n".encode("utf-16-le"), enc="AUTO")
w("utf32le_bom",       b"\xff\xfe\x00\x00" + u16.encode("utf-32-le")[4:], enc="AUTO")
w("utf8_bom",          b"\xef\xbb\xbf" + u16.encode("utf-8"), enc="AUTO")

# ------------------------------------------------------------------- degenerate
w("empty",            "")
w("one_line",         "a,b,c\n")
w("one_line_no_nl",   "a,b,c")
w("only_newlines",    "\n\n\n\n")
w("single_column",    "a\nb\nc\n")
w("single_delimiter", ",")
w("header_only",      "name,age,city\n")

# ------------------------------------------------------ JSON inside a CSV column
# RFC-quoted JSON payloads. The JSON is full of ':' and ',', so a dialect guesser that just
# prefers "whichever delimiter yields the most columns" picks ':' and shreds every row. The
# giveaway is that ':' leaves the header as ONE field while exploding the data rows.
_req  = '{""user"": {""id"": ""566674135""}, ""amount"": """", ""list"": [{""code"": ""ABC123""}], ""skip"": true}'
_resp = '{""errors"": [{""code"": 759, ""message"": ""redeem failed""}], ""customer"": {""id"": 566674135, ""profiles"": [{""fields"": {}, ""name"": ""chi que""}]}, ""status"": {""code"": 400, ""message"": ""series has expired""}}'
_err  = '{""error"": ""ABC123:series has expired""}'
_rows = ["id,tenant_id,trace_id,req_body,resp_body,created_at,err_body,version"]
for _i in range(6):
    _rows.append('%d,101,f4bce9098bf89a62,"%s","%s",2026-06-06 00:55:38.534355+00,"%s",v2'
                 % (5226 + _i, _req, _resp, _err))
w("json_in_column", "\n".join(_rows) + "\n")

# the same payloads in a semicolon-delimited file
_semi = ["id;req_body;created_at"]
for _i in range(6):
    _semi.append('%d;"%s";2026-06-06 00:55:38+00' % (5226 + _i, _req))
w("json_semicolon", "\n".join(_semi) + "\n", delim="SEMI")


# ------------------------------------------------------------------------ shape
# a single quoted field spanning many lines -> a chunk fully inside it yields NONE
giant = 'a,"' + ("filler line inside one giant quoted field\n" * 400) + '",z\nnext,row,here\n'
w("one_giant_field", giant)

# moderately wide table, exercises arrangeColumns / per-row glue
rows = []
rows.append(",".join("c%d" % c for c in range(300)))
for r in range(500):
    rows.append(",".join("v%d_%d" % (r, c) for c in range(300)))
w("wide_300cols_500rows", "\n".join(rows) + "\n")

# a deterministic pseudo-random soup, standard dialect
random.seed(20260826)
alphabet = ['a', 'b', 'Z', ',', '"', '\\', '\r', '\n', '\x00', ';', '\t', 'é', '€', '𝄞', ' ']
soup = "".join(random.choice(alphabet) for _ in range(20000))
w("random_soup", soup)
alias("random_soup", delim="SEMI")
alias("random_soup", delim="TAB", escape="BSLASH")

# a few extra dialect passes over existing files, for breadth
alias("plain_lf",       delim="SEMI")
alias("doubled_quotes", escape="BSLASH")
alias("all_quotes",     escape="BSLASH")
alias("nul_in_quotes",  delim="PIPE")

# ------------------------------------------------------------------ benchmarks
if BIG:
    # (A) ~120 MB, 6 cols, ~15% quoted, some embedded newlines and multibyte UTF-8
    random.seed(7)
    with open(os.path.join(D, "bench_mid.csv"), "w", encoding="utf-8") as f:
        for i in range(2_500_000):
            c = ["id%d" % i, "name-%d" % (i % 9973), "%.4f" % (i * 0.37)]
            r = i % 20
            if r == 0:
                c.append('"quoted, with comma"')
            elif r == 1:
                c.append('"two\nlines"')
            elif r == 2:
                c.append("caf\u00e9-\u20ac")
            else:
                c.append("plain%d" % r)
            c.append("%d" % (i % 7))
            c.append("tail")
            f.write(",".join(c) + "\n")

    # (B) no-quote TSV of comparable size – the best case, where the prescan is pure memchr
    with open(os.path.join(D, "bench_tsv.tsv"), "w", encoding="utf-8") as f:
        for i in range(2_500_000):
            f.write("id%d\tname-%d\t%.4f\tplain\t%d\ttail\n" % (i, i % 9973, i * 0.37, i % 7))

    # (C) 1000 columns x 20k rows – exercises the arrangeColumns scan and per-row glue cost
    with open(os.path.join(D, "bench_wide.csv"), "w", encoding="utf-8") as f:
        f.write(",".join("col%d" % c for c in range(1000)) + "\n")
        for r in range(20_000):
            f.write(",".join("v%d_%d" % (r % 97, c) for c in range(1000)) + "\n")

    manifest.append(("bench_mid.csv",  ",",   '"', '"', "AUTO", 0))
    manifest.append(("bench_tsv.tsv",  "TAB", '"', '"', "AUTO", 0))
    manifest.append(("bench_wide.csv", ",",   '"', '"', "AUTO", 0))

with open(os.path.join(D, "manifest.tsv"), "w") as f:
    f.write("# name\tdelim\tquote\tescape\tencoding\tbomBytes\n")
    for row in manifest:
        f.write("\t".join(str(x) for x in row) + "\n")

print("corpus: %d files, %d manifest rows -> %s" % (
    len([n for n in os.listdir(D) if n not in ("manifest.tsv", "goldens.tsv")]),
    len(manifest), D))
PY
