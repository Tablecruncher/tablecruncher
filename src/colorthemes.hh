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


 #ifndef _COLORTHEMES_HH
#define _COLORTHEMES_HH

#include <map>
#include <string>
#include <stdexcept>


class ColorThemes {
private:
    static const int darkBg;
    static const int darkTableBg;
    static const int darkFg;
    static const int brightSignatureBlue;
    static const int brightSignatureBlueBorder;
    static const int brightSignatureBlueText;
    static const int brightSignatureBlueBorderWidth;
    static const int brightDefaultButtonBg;
    static const std::map<std::string, std::map<std::string, int> > colors;
public:
    static unsigned int getColor(std::string theme, std::string color) {
        try {
            return colors.at(theme).at(color);
        } catch(std::out_of_range& e) {
#ifdef DEBUG
std::cerr << "Undefined color!" << std::endl;
#endif
            return 0x66666600;
        }
    }
    static bool isTheme(std::string theme) {
        if( ColorThemes::colors.count(theme) == 0 ) {
            return false;
        }
        return true;
    }
};


#endif
