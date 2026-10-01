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


#ifndef _SIMPLEFIFO_HH
#define _SIMPLEFIFO_HH


#include <string>
#include <deque>
#include <stdexcept>


class SimpleFifo {

public:
	SimpleFifo(size_t max_elements = 0) {
        setSize(max_elements);
    }

    void setSize(size_t max_elements) {
        MAX_ELEMENTS = max_elements;
    }
  
    // Clear storage
    void clear() {
        storage.clear();
    }

    size_t size() {
        return storage.size();
    }

    // true if item is in storage
    bool contains(std::string item) {
        bool found = false;
        printf("storage: %p\n", &storage);
        for( auto &e : storage ) {
            if( item == e ) {
                found = true;
                break;
            }
        }
        return found;
    }


    // Push new item to front if not present
	void store(std::string item) {
        if( !contains(item) ) {
            // only insert if not present
            storage.push_front(item);
            if( storage.size() > MAX_ELEMENTS ) {
                // store only MAX_ELEMENTS items
                storage.resize(MAX_ELEMENTS);
            }
        }
    }

    std::string at( size_t seek_index ) {
        size_t my_index = 0;
        for( std::deque<std::string>::iterator itr = begin(); itr < end(); ++itr ) {
            if( my_index++ == seek_index ) {
                return *itr;
            }
        }
        throw std::out_of_range("Out of range error");
    }

    std::deque<std::string>::iterator begin() {
        return storage.begin();
    }

    std::deque<std::string>::iterator end() {
        return storage.end();
    }
  

private:
	size_t MAX_ELEMENTS;
	std::deque<std::string> storage;

};


#endif
