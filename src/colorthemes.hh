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
