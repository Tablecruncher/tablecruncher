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

#ifndef _MAPPEDFILE_HH
#define _MAPPEDFILE_HH

//
//	A whole file as one flat, read-only byte range.
//
//	Every consumer of the file – dialect guessing, encoding detection, the parse – reads the
//	same bytes, which removes the repeated seekg(0) re-reads and the separate streaming pass
//	that UTF-8 validation used to make. mmap is preferred over reading into a std::string
//	because the mapped pages are clean and evictable: the parsed representation of a 1 GB CSV
//	is already ~1.3 GB, and adding a 1 GB read buffer on top would hurt exactly the users who
//	care about large files.
//
//	No FLTK, no iostream – this is used from worker threads.
//

#include <cstdint>
#include <string>
#include <vector>


class MappedFile {
public:
	MappedFile() = default;
	~MappedFile();

	MappedFile(const MappedFile&)            = delete;
	MappedFile& operator=(const MappedFile&) = delete;
	MappedFile(MappedFile&& other) noexcept;
	MappedFile& operator=(MappedFile&& other) noexcept;

	// Maps `path` read-only. Falls back to reading the whole file onto the heap when the
	// mapping fails and the file is small enough to make that sane. Returns false if neither
	// worked; an empty file counts as opened, with size 0. data() is null until it succeeds.
	bool open(const std::string& path);
	void close();

	const char* data()     const { return data_; }
	uint64_t    size()     const { return size_; }

	void adviseSequential();			// hint the kernel: we will read this front to back

	// A heap fallback larger than this is refused – at that point streaming is the better
	// answer than doubling peak memory.
	static const uint64_t MAX_HEAP_FALLBACK = 256ull * 1024 * 1024;

private:
	const char*       data_   = nullptr;
	uint64_t          size_   = 0;
	bool              mapped_ = false;
	std::vector<char> heap_;

#ifdef _WIN64
	void* fileHandle_ = nullptr;
	void* mapHandle_  = nullptr;
#else
	int   fd_ = -1;
#endif
};

#endif
