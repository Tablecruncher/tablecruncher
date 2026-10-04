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



Fl_Color toolbarHoverColor();

/** Adds hover highlight and hand cursor to a toolbar button class (Fl_Button, Fl_Light_Button, ...). */
template<class Base>
class My_Toolbar_Hover : public Base {
  public:
	template<class... Args> My_Toolbar_Hover(Args... args) : Base(args...) {}
	int handle(int event) {
		if( event == FL_ENTER || event == FL_LEAVE ) {
			hover = (event == FL_ENTER) && this->active();
			if( this->window() ) {
				this->window()->cursor(hover ? FL_CURSOR_HAND : FL_CURSOR_DEFAULT);
				// FL_NO_BOX doesn't clear its area, so repaint the window region behind the button
				this->window()->damage(FL_DAMAGE_ALL, this->x(), this->y(), this->w(), this->h());
			}
		}
		return Base::handle(event);
	}
	void draw() {
		if( !hover ) {
			Base::draw();
		} else if( this->box() == FL_NO_BOX ) {
			fl_rectf(this->x(), this->y(), this->w(), this->h(), toolbarHoverColor());
			Base::draw();
		} else {
			Fl_Color c = this->color();
			this->color(toolbarHoverColor());
			Base::draw();
			this->color(c);
		}
	}
  private:
	bool hover = false;
};
typedef My_Toolbar_Hover<Fl_Button> My_Toolbar_Button;
typedef My_Toolbar_Hover<Fl_Light_Button> My_Toolbar_Check_Button;


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
	  using Fl_Window::show;
	  void show() override;
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



