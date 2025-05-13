#ifndef _CSVWIDGETS_HH
#define _CSVWIDGETS_HH

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Pixmap.H>


#include "globals.hh"

/** \file csvwidgets.hh
 * \brief Contains several classes that override FLTK widgets
 * 
 */



class My_Toolbar : public Fl_Pack {
  public:
	My_Toolbar(int X,int Y,int W,int H);
	Fl_Button *AddButton(const char *name, Fl_RGB_Image *img=0, Fl_Callback *cb=0, void *data=0, int width=0, std::string shortName="", int fontSize = 0);
	Fl_Light_Button *AddCheckButton(const char *name, Fl_Callback *cb=0, void *data=0, int width=0);
};



class My_Fl_Double_Window : public Fl_Double_Window {
  public:
	  My_Fl_Double_Window(int, int, const char *);
	  My_Fl_Double_Window(int, int, int, int, const char *);
	  ~My_Fl_Double_Window();
	  int handle(int);
  private:
	
};


class My_Fl_Small_Window : public Fl_Window {
  public:
	  My_Fl_Small_Window(int W,int H);
	  My_Fl_Small_Window(int, int, const char *);
	  My_Fl_Small_Window(int, int, int, int, const char *);
	  int handle(int);
	  virtual void gotFocus();
	  int32_t dataExchange;
};


class My_Fl_Search_Window : public My_Fl_Small_Window {
  public:
	My_Fl_Search_Window(int W,int H);
	void gotFocus();
};



class My_Fl_Button : public Fl_Button {
  public:
	enum ButtonStyle {
		DEFAULT,
		HIGHLIGHT
	};
	My_Fl_Button(int, int, int, int, const char *label = 0);
	void draw();
	struct buttonColorStruct {
		Fl_Color background;
		Fl_Color label;
		Fl_Color border;
		Fl_Color windowBg;
		int borderWidth;
	} colors;
	void set_colors(struct My_Fl_Button::buttonColorStruct c);
  private:
	static void rbox(struct buttonColorStruct, int x, int y, int w, int h, bool rounded=true);
};




#endif



