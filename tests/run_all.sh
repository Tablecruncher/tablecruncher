#!/bin/sh
#
# Runs the whole parser test suite. Point BIN at a tc_parsecheck build
# (a sanitizer build works too, just slower).
#
#   ./tests/run_all.sh
#   BIN=./build/tsan/tc_parsecheck ./tests/run_all.sh
#
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build/tests/tc_parsecheck}"
DATA="$ROOT/tests/data"

[ -f "$DATA/manifest.tsv" ] || "$ROOT/tests/gen_corpus.sh"

echo "== goldens (serial path regression)"
"$BIN" --golden-check "$DATA"
echo "== istream vs mmap"
"$BIN" --diff-buffer "$DATA"
echo "== serial vs parallel, 7 chunk counts"
"$BIN" --diff-parallel "$DATA" | tail -1
echo "== chunk boundary sweep (every byte offset, plus pairs and triples)"
"$BIN" --sweep "$DATA" | tail -1
echo "== differential fuzz"
"$BIN" --fuzz "${FUZZ:-100000}"
echo "== parallel UTF-8 validation"
"$BIN" --utf8check "$DATA"
"$BIN" --utf8fuzz "${UTF8FUZZ:-10000}" | head -1
echo "== column width scan"
"$BIN" --colcheck "$DATA"
echo "== parser state does not leak between calls"
"$BIN" --reusecheck "$DATA"
echo "== dialect guessing (decides silently how every file opens)"
"$BIN" --guess-check "$DATA"
echo
echo "all green"
