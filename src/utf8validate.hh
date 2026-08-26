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

#ifndef _UTF8VALIDATE_HH
#define _UTF8VALIDATE_HH

//
//	Multi-threaded UTF-8 validation over a flat buffer.
//
//	Answers three questions in one pass, because the load path needs all three and they only
//	differ in where they start:
//	  - validFrom0   is the whole buffer valid?            (nothing depends on this today,
//	                                                        but it is the honest answer)
//	  - validFrom4   is [4, len) valid?                    the ENCODING DECISION, chosen to
//	                                                        stay bit-compatible with the
//	                                                        historical istream code path
//	  - validFromBom is [bomBytes, len) valid?             the region actually parsed, and
//	                                                        therefore the gate that proves
//	                                                        Helper::fixUtf8() is the identity
//
//	No FLTK, no iostream – this runs on worker threads.
//

#include <cstdint>

struct Utf8ValidationResult {
	bool validFrom0   = false;
	bool validFrom4   = false;
	bool validFromBom = false;
};

//
//	`threads` is a hint; 0 means "pick something sensible". The result is exactly what
//	utf8::is_valid() would return for each of the three regions – see the note on chunk
//	stitching in utf8validate.cpp for why that is not merely approximately true.
//
//	`forceSplit` is a TEST HOOK: it skips the "too small to bother" shortcut so that a tiny
//	buffer still gets carved into `threads` ranges. That is what lets the harness sweep range
//	boundaries across every byte offset cheaply.
Utf8ValidationResult validateUtf8Parallel(const char* data, uint64_t len, int bomBytes,
                                          unsigned threads = 0, bool forceSplit = false);

#endif
