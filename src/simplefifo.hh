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
