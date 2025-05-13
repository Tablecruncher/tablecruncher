#ifndef _CSVAPPLICATION_HH
#define _CSVAPPLICATION_HH


#include "globals.hh"
#include "colorthemes.hh"
#include "csvdatastorage.hh"
#include "csvwindow.hh"
#include "csvtable.hh"
#include "csvgrid.hh"
#include "csvmenu.hh"
#include "macro.hh"


#include <vector>
#include <tuple>
#include <fstream>
#include <map>
#include <string>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <queue>
#include <deque>

#ifdef _WIN64
#include <direct.h>
#endif



#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/platform.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Help_View.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Tooltip.H>


#include "utf8.h"
#include "recentfiles.hh"


namespace ui_icons {
	#ifndef _WIN64
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#endif
	#include "icons/icon-alert.xpm"
	#include "icons/icon-minus.xpm"
	#include "icons/icon-plus.xpm"
	#include "icons/icon-question.xpm"
	#ifndef _WIN64
	#pragma GCC diagnostic pop
	#endif
}





// Avoid errors with names included from X11 on Linux:
#ifdef __linux__
	#ifdef Success
	#undef Success
	#endif
	#ifdef None
	#undef None
	#endif
#endif
#include "httplib.h"


#ifndef MY_VERSION
#define MY_VERSION 0.0.0.0
#endif




// I think this global function is very bad style ... TODO XXX
void updateMacroLogBuffer(void *logDisplay, std::string str);			// Used by Macro::... to show infos to user


/**
 * \brief The singleton application object handling all user interactions.
 * 
 * CsvApplication is responsible for handling menu calls and keyboard shortcuts. It manages
 * the global helper windows like `searchWin` (class `My_Fl_Search_Window`).
 * 
 * This object gets instantiated in `main.cpp` and is stored in the global variable `app`.
 * 
 */
class CsvApplication {
public:
	enum SaveType {
		CSV,			// Save as CSV
		JSON			// Save as JSON
	};
	enum ReplaceAllType: long {
		REPLACE,
		FLAG,
		UNFLAG
	};
	enum FontUsage {
		TEXT,
		COL_HEADER,
		ROW_HEADER
	};
	
	bool nfcIsOpen = false;
		
	CsvApplication();
	~CsvApplication();
	int createNewWindow();								// Opens a new window, returns slut number
	size_t getCreatedWindowCount();
	static void openFileCB(Fl_Widget *, void *);
	static void saveFileCB(Fl_Widget *, void *);
	static void saveFileAsCB(Fl_Widget *, void *);
	static void splitCsvCB(Fl_Widget *, void *);
	static void exportJsonCB(Fl_Widget *, void *);
	static void exportFlaggedCB(Fl_Widget *, void *);
	static void checkDataConsistencyCB(Fl_Widget *, void *);
	static void showInfoWindowCB(Fl_Widget *, void *);
	static void addColBeforeCB(Fl_Widget *, void *);
	static void addColCB(Fl_Widget *, void *);
	static void addRowAboveCB(Fl_Widget *, void *);
	static void addRowCB(Fl_Widget *, void *);
	static void delColsCB(Fl_Widget *, void *);
	static void delRowsCB(Fl_Widget *, void *);
	static void undoCB(Fl_Widget *, void *);
	static void setCsvPropertiesCB(Fl_Widget *, void *data);
	static void quitApplicationCB(Fl_Widget *, void *);
	static void closeRemainOpenWindow(Fl_Widget *, void *);
	static void disableUndoCB(Fl_Widget *, void *, bool confirmDialog = true);
	static void enableUndoCB(Fl_Widget *, void *);
	void openFile(bool askUser, bool reopen=false);		// Asks for a filename and opens that file. askUser => should the user choose the CSV format?
	void openFile(std::string path, bool askUser);		// Opens the given file.
	void openRecentFile(size_t index);					// Opens the index-th item in the Open Recent File menu
	bool splitCsvFiles();
	std::string splittedFileName(std::string pathWithoutExtension, std::string extension, int num, int digitalExtensionLength);
	bool saveFile(bool saveAs = false, SaveType type = CSV, bool flaggedOnly = false);
	void closeWindow(int windowIndex=-1, bool forceClose=false);				// closes a window, stated by windowIndex
	void quitApplication(bool forceClose=false);								// quits application by closing all windows
	int getWindowByPointer(Fl_Widget *widget);
	void setTopWindow(int topWindow=-1);				// sets a new value for the topWindow – gets called via Fl_Window::handle() => FL_FOCUS
	int getTopWindow();
	void setFocusByWinIndex(int winIndex);
	void copySelection();								// copies the selected cells as CSV, using the chosen definition
	void paste(bool askUser=false, bool fillSelection=false);		// pastes into the top-most window
	void find();
	void updateSearchWindow();
	void sort(unsigned int column=0);
	void flagSelectedRowsCB();
	void unflagRowsCB();
	void invertFlaggedCB();
	void deleteFlaggedCB(bool deleteFlagged = true);
	void executeMacro();
	static void executeMacroListCB(Fl_Widget *widget, void *data);		// TODO public?
	static void executeMacroCB(Fl_Widget *, void *data);				// TODO public?
	static void insertLoopButtonCB(Fl_Widget *, void *data);			// TODO public?
	static void doSortCB(Fl_Widget *, void *);
	static void doSortWinCB(Fl_Widget *, long data);
	std::string getPreference(Fl_Preferences *pref, std::string key, std::string def);
	void moveCols(bool right);
	void addCol(bool right);
	void addRow(bool below);
	void delCols();
	void delRows();
	void splitCol();
	static void splitColumn_CB(Fl_Widget *widget, void *data);
	static void splitColumn_Cancel_CB(Fl_Widget *widget, void *data);
	void mergeCols();
	static void mergeColumn_CB(Fl_Widget *widget, void *data);
	static void mergeColumn_Cancel_CB(Fl_Widget *widget, void *data);
	static void switchHeaderRowCB(Fl_Widget *, void *);
    static void arrangeColumnsCB(Fl_Widget *, void *);
	static void aboutCB(Fl_Widget *, void *);
	void changeFontSize(int changeMode);
	void setUndoMenuItem(bool );
	static std::pair<CsvDefinition, float> guessDefinition(std::istream *input);		// guesses the CSV definition
	static std::pair<CsvDefinition::Encodings, int> guessEncoding(std::istream *input, long streamLength=0);
	static CsvDefinition setTypeByUser(CsvDefinition guessedDefinition, std::istream *input, std::string buttonText = "Open");
	bool isAlreadyOpened(std::string path);
	static void droppedFileCB(const char *path);
	void setWorkDir(std::string workdDir);
	std::string getWorkDir();
	void setTheme(std::string theme);
	std::string getTheme();
	void setFont(std::string font);
	std::string getFont();
	static int myFlChoice(std::string title, std::string message, std::vector<std::string> options, int buttonWidth=100, int windowHeight=140);
	static std::pair<int,std::string> myFlAskString(std::string title, std::string buttonText, int buttonWidth=80);
	std::tuple<int, int> lastFoundPosition();
	void jumpToRow();
	void jumpToFlaggedRow(bool direction = true);
	void checkUpdate(bool ignorePref = false);
	void setUpdateCheck(bool allowed);
	bool isUpdateCheckAllowed();
	int64_t getPhysMemSize();
	void updateMenu(int windowIndex);
	bool showOnboardingProcess();
	void showOnboardingNextCB(Fl_Widget *w, void *data);
	void showOnboardingOkCB(Fl_Widget *w, void *data);
	void showOnboardingCancelCB(Fl_Widget *w, void *data);
	void showImWorkingWindow(std::string message, bool showAlways = false);
	void hideImWorkingWindow();
	void editSingleCell();
	static void editSingleCellCB(Fl_Widget *, void *);
	static void editSingleCellSaveCB(Fl_Widget *widget, void *data);
	int getEditorTabDistance();
	int getCustomFont(CsvApplication::FontUsage usage);
	void checkDataConsistency();
	std::map<std::string,int> getFontMapping();
	
	static void AppMenuPreferencesCB(Fl_Widget *, void *);
	static void updateCheckButtonCB(Fl_Widget *w, void *);
	static void checkUpdateCB(Fl_Widget *, void *);

	static struct My_Fl_Button::buttonColorStruct createButtonColor(My_Fl_Button::ButtonStyle);
	static Fl_RGB_Image *xpmResizer(const char* const* xpm_data, int size);

	CsvMenu *getAppMenuBar();
	
	
private:
	typedef std::tuple<Fl_Hold_Browser *, Fl_Text_Editor *> addDelParamType;
	
	CsvMenu *appMenuBar;								// Holds the pointer to the menu bar
	// Widgets for Search and Replace Window
	My_Fl_Search_Window *searchWin;
	Fl_Input *searchInput;
	My_Fl_Button *findButton;
	Fl_Input *replaceInput;
	My_Fl_Button *replaceButton;
	My_Fl_Button *replaceFindButton;
	My_Fl_Button *replaceAllButton;
	My_Fl_Button *flagMatchingButton;
	My_Fl_Button *unflagMatchingButton;
	Fl_Check_Button *ignoreCase;
	Fl_Check_Button *useRegex;
	Fl_Box *searchWinLabel;
	Fl_Box *searchWinScope;
	#ifdef __APPLE__
	My_Fl_Small_Window *remainOpenWin;					// Small window that is opened as a placeholder when the last application window is closed
	My_Fl_Button *remainQuitButton;
	#endif
	My_Fl_Small_Window *sortWin;
	My_Fl_Small_Window *setTypeByUserWin;				// Window to let user choose encoding and delimiters
	Fl_Choice *encChoice;								// ... encoding dropdown
	bool setTypeByUserCancelled;						// ... true indicates that paste or open has been cancelled (ESC, CMD-W, red button)
	struct previewTableStruct {
		std::istream *input;
		CsvTable *table;
		CsvGrid *grid;
		CsvDefinition *definition;
	};
	int topWindow = 0;									// Index of the top-most application window
	size_t createdWindowCount = 1;						// incremented on every new created window, used for generic window names
	std::string workDir;
	std::string theme = "Bright";
	std::string gridFont = "Helvetica";
	std::tuple<int, int> lastFound = {-1,-1};			// last cell a search has been succesful
	// macroWin
	int lastMacroWinX = -1;								// stores the position of the macro window
	int lastMacroWinY = -1;
	bool macroWinOpened = false;
	std::string lastChoosenMacroName = "";				// name of the most recently selected macro (used to calculate lastSelectedMacro)
	int lastSelectedMacro = 0;							// index within macroList (Fl_Browser) that was previously selected (used to update previously selected marco source)
	bool myFlAskStringOk;								// signals a myFlAskString window has not been cancelled
	int64_t physMemSize = 0;
	bool checkUpdateAllowed = false;
	My_Fl_Small_Window *imWorkingWindow;
	Fl_Button *imWorkingButton;
	int lastSingleEditWinWidth = 0;
	int lastSingleEditWinHeight = 0;
	std::string lastSplitString = "";
	std::string lastGlueString = "";
	Fl_RGB_Image *plusPng;
	Fl_RGB_Image *minusPng;
	const int macroWinImgButtonSize = 15;				// size of the image button (plus, minus) in the macro window
	struct customFonts_s {
		int gridText = FL_HELVETICA;
		int gridColHeader = FL_HELVETICA;
		int gridRowHeader = FL_HELVETICA;
	} customFonts;
	std::map<std::string, int> fontMapping;
	RecentFiles recentFiles;


	static std::pair<table_index_t, table_index_t> tableStatistics(CsvDataStorage localStorage);	// Calculates the maximum number of columns and the variance of columns
	static void showPreview(struct previewTableStruct);		// parses input and shows data
	// Callbacks for setTypeByUser()
	static void setTypeByUser_Done_CB(Fl_Widget *, long data);
	static void setTypeByUser_Type_CB(Fl_Widget *widget, void *data);
	static void setTypeByUser_Esc_CB(Fl_Widget *widget, void *data);
	static void setTypeByUser_Enc_CB(Fl_Widget *widget, void *data);
	static void setTypeByUser_Quote_CB(Fl_Widget *widget, void *data);
	static void find_substring_CB(Fl_Widget*, long data);
	static int find_replace(bool callFindNext);
	static void find_replace_CB(Fl_Widget *, void *);
	static void find_replaceFind_CB(Fl_Widget *, void *);
	static void find_replaceAll_CB(Fl_Widget *, long data);
	static void myFlChoice_CB(Fl_Widget *widget, void *data);
	static void myFlChoiceWin_CB(Fl_Widget *widget, long data=0);
	static void myFlAskStringCB(Fl_Widget *widget, long data);
	static void myFlAskStringOkCB(Fl_Widget *widget, void *);
	static void addNewMacroCB(Fl_Widget *, void *data);
	static void deleteMacroCB(Fl_Widget *, void *data);
	static void updateMacroBrowserList(Fl_Browser *macroList, int selected=0);
	static int selectMacroBrowserEntry(Fl_Browser *macroList, std::string );
	static void showImWorkingWindowCB(Fl_Widget *, long );
	static void dumpWindows();
};



#endif


