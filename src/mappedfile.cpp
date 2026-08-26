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

#include "mappedfile.hh"

#include <cstdio>

#ifdef _WIN64
	#include <windows.h>
	#include "helper.hh"				// utf8_to_ws() for the wide path
#else
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
#endif


MappedFile::~MappedFile() {
	close();
}


MappedFile::MappedFile(MappedFile&& other) noexcept {
	*this = std::move(other);
}


MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
	if( this == &other )
		return *this;
	close();
	data_   = other.data_;
	size_   = other.size_;
	mapped_ = other.mapped_;
	heap_   = std::move(other.heap_);
#ifdef _WIN64
	fileHandle_ = other.fileHandle_;
	mapHandle_  = other.mapHandle_;
	other.fileHandle_ = nullptr;
	other.mapHandle_  = nullptr;
#else
	fd_ = other.fd_;
	other.fd_ = -1;
#endif
	// if the source was a heap fallback, data_ pointed into its vector – re-point it at ours
	if( !mapped_ && !heap_.empty() )
		data_ = heap_.data();
	other.data_   = nullptr;
	other.size_   = 0;
	other.mapped_ = false;
	return *this;
}


void MappedFile::close() {
#ifdef _WIN64
	if( mapped_ && data_ )
		UnmapViewOfFile((LPCVOID) data_);
	if( mapHandle_ ) {
		CloseHandle((HANDLE) mapHandle_);
		mapHandle_ = nullptr;
	}
	if( fileHandle_ && fileHandle_ != INVALID_HANDLE_VALUE ) {
		CloseHandle((HANDLE) fileHandle_);
		fileHandle_ = nullptr;
	}
#else
	if( mapped_ && data_ && size_ > 0 )
		munmap((void*) data_, (size_t) size_);
	if( fd_ >= 0 ) {
		::close(fd_);
		fd_ = -1;
	}
#endif
	heap_.clear();
	heap_.shrink_to_fit();
	data_   = nullptr;
	size_   = 0;
	mapped_ = false;
}


#ifdef _WIN64

bool MappedFile::open(const std::string& path) {
	close();

	std::wstring wpath = Helper::utf8_to_ws(path);
	fileHandle_ = (void*) CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
	                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if( fileHandle_ == INVALID_HANDLE_VALUE ) {
		fileHandle_ = nullptr;
		return false;
	}

	LARGE_INTEGER li;
	if( !GetFileSizeEx((HANDLE) fileHandle_, &li) ) {
		close();
		return false;
	}
	size_ = (uint64_t) li.QuadPart;

	if( size_ == 0 ) {
		// CreateFileMapping refuses a zero-length file; an empty buffer is a valid result
		static const char emptyByte = 0;
		data_   = &emptyByte;
		mapped_ = false;
		return true;
	}

	mapHandle_ = (void*) CreateFileMappingW((HANDLE) fileHandle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if( mapHandle_ ) {
		void* view = MapViewOfFile((HANDLE) mapHandle_, FILE_MAP_READ, 0, 0, 0);
		if( view ) {
			data_   = (const char*) view;
			mapped_ = true;
			return true;
		}
		CloseHandle((HANDLE) mapHandle_);
		mapHandle_ = nullptr;
	}

	// mapping failed – fall back to a plain read for files small enough to justify it
	if( size_ > MAX_HEAP_FALLBACK ) {
		close();
		return false;
	}
	heap_.resize((size_t) size_);
	DWORD got = 0;
	if( !ReadFile((HANDLE) fileHandle_, heap_.data(), (DWORD) size_, &got, nullptr) || got != size_ ) {
		close();
		return false;
	}
	data_   = heap_.data();
	mapped_ = false;
	return true;
}


void MappedFile::adviseSequential() {
	// no direct equivalent; FILE_FLAG_SEQUENTIAL_SCAN would have to be set at CreateFile time
}

#else

bool MappedFile::open(const std::string& path) {
	close();

	fd_ = ::open(path.c_str(), O_RDONLY);
	if( fd_ < 0 )
		return false;

	struct stat st;
	if( fstat(fd_, &st) != 0 || !S_ISREG(st.st_mode) ) {
		close();
		return false;
	}
	size_ = (uint64_t) st.st_size;

	if( size_ == 0 ) {
		// mmap of length 0 is an error; an empty buffer is a perfectly valid result
		static const char emptyByte = 0;
		data_   = &emptyByte;
		mapped_ = false;
		return true;
	}

	void* p = mmap(nullptr, (size_t) size_, PROT_READ, MAP_PRIVATE, fd_, 0);
	if( p != MAP_FAILED ) {
		data_   = (const char*) p;
		mapped_ = true;
		return true;
	}

	// mmap failed – fall back to a plain read for files small enough to justify it
	if( size_ > MAX_HEAP_FALLBACK ) {
		close();
		return false;
	}
	heap_.resize((size_t) size_);
	ssize_t got = pread(fd_, heap_.data(), (size_t) size_, 0);
	if( got < 0 || (uint64_t) got != size_ ) {
		close();
		return false;
	}
	data_   = heap_.data();
	mapped_ = false;
	return true;
}


void MappedFile::adviseSequential() {
	if( !mapped_ || size_ == 0 )
		return;
#if defined(POSIX_MADV_SEQUENTIAL)
	posix_madvise((void*) data_, (size_t) size_, POSIX_MADV_SEQUENTIAL);
#elif defined(MADV_SEQUENTIAL)
	madvise((void*) data_, (size_t) size_, MADV_SEQUENTIAL);
#endif
}

#endif
