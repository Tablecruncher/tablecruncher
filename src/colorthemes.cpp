#include "colorthemes.hh"


/************************************************************************************
*
*	ColorThemes
*
************************************************************************************/

const int ColorThemes::darkBg = 0x32323200;			// 0x35373900
const int ColorThemes::darkTableBg = 0x47474700;	// 0x42393100
const int ColorThemes::darkFg = 0xAFB2B400;			// 0xAFB2B400

// Blue (on Apple) highlight buttons
#ifdef __APPLE__
const int ColorThemes::brightSignatureBlue = 0x1D8AFF00;			// Apple: 4E98FD	Sonoma: 1D8AFF
const int ColorThemes::brightSignatureBlueBorder = 0x1D8AFF00;		// Apple: 4382D8
const int ColorThemes::brightSignatureBlueText = 0xFFFFFF00;		// Apple: FFFFFF
const int ColorThemes::brightSignatureBlueBorderWidth = 1;			// Apple: 1
#else
const int ColorThemes::brightSignatureBlue = 0xE1E1E100;			// Win: E1E1E1
const int ColorThemes::brightSignatureBlueBorder = 0x0078D700;		// Win: 0078D7
const int ColorThemes::brightSignatureBlueText = 0x00000000;		// Win: 000000
const int ColorThemes::brightSignatureBlueBorderWidth = 2;			// Win: 2
#endif


// Default buttons (non highlighted)
#ifdef __APPLE__
const int ColorThemes::brightDefaultButtonBg = 0xFFFFFF00;			// Apple: FFFFFF
#else
const int ColorThemes::brightDefaultButtonBg = 0xE1E1E100;			// Win: E1E1E1
#endif




const std::map<std::string, std::map<std::string, int> > ColorThemes::colors = {
	{ 	"Bright", {
			{"win_bg", 0xF0F0F000},
			{"win_text", 0x00000000},
			// {"table_bg", 0xFFFFFF00},
			{"table_bg", 0xF8F8F800},
			{"toolbar_bg", 0xF0F0F000},
			{"toolbar_text", 0x00000000},
			{"statusbar_bg", 0xF0F0F000},
			{"statusbar_text", 0x00000000},
			{"cell_bg", 0xFFFFFF00},
			{"cell_text", 0x00000000},
			{"cell_flagged_bg", 0xDC999900},
			{"cell_flagged_text", 0x00000000},
			{"cell_found_bg", 0xCBE89600},
			{"button_bg", ColorThemes::brightDefaultButtonBg},
			{"button_text", 0x00000000},
			{"button_border", 0xDDDDDD00},
			{"button_border_width", 1},
			{"hightlight_button_bg", ColorThemes::brightSignatureBlue},
			{"hightlight_button_text", ColorThemes::brightSignatureBlueText},
			{"hightlight_button_border", ColorThemes::brightSignatureBlueBorder},
			{"highlight_button_border_width", ColorThemes::brightSignatureBlueBorderWidth},
			// {"header_row_bg", 0xE6F0FF00},
			{"header_row_bg", 0xF8F8F800},
			{"header_row_text", 0x59575300},  			// 0x0D679300 0x2978A000
			{"grid_border", 0xDCE6FF00},
			{"selection_bg", 0xF0F0F000},
			{"selection_flagged_text", 0x50505000},
			{"selection_flagged_bg", 0xFFD1D100},
			{"selection_text", 0x50505000},
			{"deletion_bg", 0xFFD3D300},
			{"deletion_text", 0x00000000},
			{"header_button_on", 0xFF965D00},			// 0xF3833700 0xCBE89600
			{"cell_ellipsis", 0xF3833700}				// 0xDB9B9B00
		}
	},
	{ 	"Dark", {
			{"win_bg", ColorThemes::darkBg},
			{"win_text", ColorThemes::darkFg},
			{"table_bg", ColorThemes::darkTableBg},
			{"toolbar_bg", ColorThemes::darkBg},
			{"toolbar_text", ColorThemes::darkFg},
			{"statusbar_bg", ColorThemes::darkBg},
			{"statusbar_text", ColorThemes::darkFg},
			{"cell_bg", ColorThemes::darkBg},
			{"cell_text", ColorThemes::darkFg},
			{"cell_flagged_bg", 0xB8676700},
			{"cell_flagged_text", ColorThemes::darkFg},
			{"cell_found_bg", 0x73982F00},
			{"button_bg", 0xC0C0C000},
			{"button_text", 0x00000000},
			{"button_border", 0xA9A9A900},
			{"button_border_width", 1},
			{"hightlight_button_bg", 0x195FD900},
			{"hightlight_button_text", 0xFFFFFF00},
			{"hightlight_button_border", 0x1653BE00},
			{"highlight_button_border_width", 1},
			{"header_row_bg", 0x65676800},
			{"header_row_text", 0xFFFFFF00},
			{"grid_border", 0x595B5C00},
			{"selection_bg", 0x84878800},
			{"selection_flagged_text", 0x30303000},
			{"selection_flagged_bg", 0xDC999900},
			{"selection_text", 0x30303000},
			{"deletion_bg", 0xFF696900},
			{"deletion_text", 0xEAEAEA00},
			{"header_button_on", 0xF3833700},				// 0xCBE89600
			{"cell_ellipsis", 0xF3833700}					// 0xDB9B9B00
		}
	},
	{ 	"Solarized Bright", {
			{"win_bg", 0xFDF6E300},
			{"win_text", 0x00000000},
			{"table_bg", 0xEEE8D500},
			{"toolbar_bg", 0xFDF6E300},
			{"toolbar_text", 0x00000000},
			{"statusbar_bg", 0xFDF6E300},
			{"statusbar_text", 0x00000000},
			{"cell_bg", 0xFDF6E300},
			{"cell_text", 0x657B8300},
			{"cell_flagged_bg", 0xF3767200},
			{"cell_flagged_text", 0xFFFFFF00},
			{"cell_found_bg", 0xCBE89600},
			{"button_bg", ColorThemes::brightDefaultButtonBg},
			{"button_text", 0x00000000},
			{"button_border", 0xDDDDDD00},
			{"button_border_width", 1},
			{"hightlight_button_bg", 0x4E98FD00},
			{"hightlight_button_text", 0xFFFFFF00},
			{"hightlight_button_border", 0x4382D800},
			{"highlight_button_border_width", 1},
			{"header_row_bg", 0xEEE8D500},
			{"header_row_text", 0x586E7500},
			{"grid_border", 0xF4EEDB00},
			{"selection_bg", 0xF0F0F000},
			{"selection_flagged_text", 0x40404000},
			{"selection_flagged_bg", 0xFFA09E00},
			{"selection_text", 0x40404000},
			{"deletion_bg", 0xD3368200},
			{"deletion_text", 0x93A1A100},
			{"header_button_on", 0x85990000},
			{"cell_ellipsis", 0xdc322f00}
		}
	},
	{ 	"Solarized Dark", {
			{"win_bg", 0x002b3600},
			{"win_text", 0x83949600},
			{"table_bg", 0x07364200},
			{"toolbar_bg", 0x002b3600},
			{"toolbar_text", 0x83949600},
			{"statusbar_bg", 0x002b3600},
			{"statusbar_text", 0x83949600},
			{"cell_bg", 0x07364200},
			{"cell_text", 0x83949600},
			{"cell_flagged_bg", 0xD3368200},
			{"cell_flagged_text", 0xEEE8D500},
			{"cell_found_bg", 0x268bd200},
			{"button_bg", 0x93a1a100},
			{"button_text", 0x07364200},
			{"button_border", 0x07364200},
			{"button_border_width", 1},
			{"hightlight_button_bg", 0x268bd200},
			{"hightlight_button_text", 0xfdf6e300},
			{"hightlight_button_border", 0x07364200},
			{"highlight_button_border_width", 1},
			{"header_row_bg", 0x586E7500},
			{"header_row_text", 0xEEE8D500},
			{"grid_border", 0x595B5C00},
			{"selection_bg", 0x586E7500},
			{"selection_flagged_text", 0xFDF6E300},
			{"selection_flagged_bg", 0xEA83B600},
			{"selection_text", 0xFDF6E300},
			{"deletion_bg", 0xDC322F00},
			{"deletion_text", 0xeee8d500},
			{"header_button_on", 0x85990000},
			{"cell_ellipsis", 0xdc322f00}
		}
	}
};

