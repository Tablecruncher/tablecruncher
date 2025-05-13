#ifndef _RECENTFILES_HH
#define _RECENTFILES_HH

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "globals.hh"



class RecentFiles {

public:
    void add(std::string filepath) {
        // remove `filepath` from `recentFullPaths` if present
        recentFullPaths.erase( std::remove(recentFullPaths.begin(), recentFullPaths.end(), filepath), recentFullPaths.end() );
        // insert at front
        if( recentFullPaths.size() == 0 ) {
            recentFullPaths.push_back(filepath);
        } else {
            recentFullPaths.insert(recentFullPaths.begin(), filepath);
        }
        // only keep the first `TCRUNCHER_PREF_RECENT_FILES_NUM` items
        if( recentFullPaths.size() > TCRUNCHER_PREF_RECENT_FILES_NUM ) {
            recentFullPaths.resize(TCRUNCHER_PREF_RECENT_FILES_NUM);
        }
        
    }

    
    std::string get(size_t index) {
        // std::cerr << "Trying to get " << index << "-th item of Recent Menu" << std::endl;
        if( index < recentFullPaths.size() ) {
            return recentFullPaths.at(index);
        }
        return "";
}


    std::vector<std::string> getRecentFiles() {
        return recentFullPaths;
    };


private:
	std::vector<std::string> recentFullPaths;

    
};




#endif
