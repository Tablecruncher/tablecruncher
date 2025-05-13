#include "csvwidgets.hh"
#include "csvwindow.hh"
#include "csvapplication.hh"


extern CsvApplication app;
extern CsvWindow windows[];


/************************************************************************************
*
*	My_Toolbar
*
************************************************************************************/


/**
	Creates a toolbar (Fl_Pack) to use in a single window.
	@param	X,Y		toolbar position
	@param	W,H		toolbar size
*/
My_Toolbar::My_Toolbar(int X,int Y,int W,int H) : Fl_Pack(X,Y,W,H) {
	type(Fl_Pack::HORIZONTAL);			// horizontal packing of buttons
	box();
	spacing(6);							// spacing between buttons
	end();
}

/**
	Adds a standard button to the toolbar.
	@param	name		C-String containing the label for the Button
	@param	img			Image (Fl_PNG_Image) for the button (optional)
	@param	cb			Callback function, activated when the button is pressed
	@param	data		Data that should get sent to the callback
	@param	width		Width (and height) of the button. (TODO Not working!?)
	@param	shortName	If given, it is used as the button text (if no image is present)
	@return				Newly created button.
*/
Fl_Button *My_Toolbar::AddButton(const char *name, Fl_RGB_Image *img, Fl_Callback *cb, void *data, int width, std::string shortName, int fontSize) {
	begin();
	if( !width ) {
		width = 40;
	}
	Fl_Button *b = new Fl_Button(0,0,width,width);
	b->box(FL_NO_BOX);
	b->clear_visible_focus();
	if( name )
		b->tooltip(name);
	if( img ) {
		b->image(img);
	} else {
		b->labelcolor(ColorThemes::getColor(app.getTheme(), "toolbar_text"));
		if( fontSize > 0 )
			b->labelsize(fontSize);
		if( shortName != "" ) {
			b->copy_label(shortName.c_str());
		} else {
			b->copy_label(name);
		}
	}
	if( cb )
		b->callback(cb,data);
	if( width )
		b->resize(0,0,width,width);
	end();
	return b;
}

/**
	Adds a checkbox to the toolbar.
	@param	name		C-String containing the label for the Button
	@param	cb			Callback function, activated when the button is pressed
	@param	data		Data that should get sent to the callback
	@param	width		Width (and height) of the button. (TODO Not working!?)
	@return				Newly created checkbox.
*/
Fl_Light_Button *My_Toolbar::AddCheckButton(const char *name, Fl_Callback *cb, void *data, int width) {
	begin();
	Fl_Light_Button *b = new Fl_Light_Button(0,0,width,TCRUNCHER_ICON_BAR_HEIGHT-18, name);
	b->box(FL_FLAT_BOX);
	b->color(ColorThemes::getColor(app.getTheme(), "toolbar_bg"));
	b->labelcolor(ColorThemes::getColor(app.getTheme(), "toolbar_text"));
	b->selection_color(ColorThemes::getColor(app.getTheme(), "header_button_on"));
	b->clear_visible_focus();
	if( name )
		b->tooltip(name);
	if( cb )
		b->callback(cb,data);
	if( width )
		b->resize(0,0,width,32);
	end();
	return b;
}



/************************************************************************************
*
*	My_Fl_Small_Window: used for 
*
************************************************************************************/


My_Fl_Small_Window::My_Fl_Small_Window(int W,int H) : Fl_Window (W, H) {}
My_Fl_Small_Window::My_Fl_Small_Window(int W,int H, const char* title) : Fl_Window (W, H, title) {}
My_Fl_Small_Window::My_Fl_Small_Window(int X,int Y,int W,int H, const char* title) : Fl_Window (X, Y, W, H, title) {}

/**
 *	Calls window callback function with 0 (for ESC or red Close Win Button) or TCRUNCHER_MYFLCHOICE_MAGICAL for pressing ENTER
 */
int My_Fl_Small_Window::handle(int event) {
	switch( event ) {
		case FL_KEYBOARD:
			if( Fl::event_command() && Fl::event_key() == 'w' && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				do_callback(this, -1);		// the same as pressing ESC
				return 1; // TODO needed?
			}
			if( Fl::event_key() == FL_Enter && !Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// https://www.fltk.org/doc-1.3/classFl__Widget.html#a8bf5537fae51a9891a330d4c6b6e499b
				// Call the callback with the magical number => Enter has been pressed
				do_callback(this, TCRUNCHER_MYFLCHOICE_MAGICAL);
				return 1; // TODO needed?
			}
			if( Fl::event_key() == '+' && Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// override CMD plus
				return 1;
			}
			if( Fl::event_key() == '-' && Fl::event_command() && !Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() ) {
				// override CMD minus
				return 1;
			}
		break;
		case FL_FOCUS:
			gotFocus();
		break;
	}
	if( !windows[app.getTopWindow()].grid->areEventsAllowed() ) {
		return 1;
	} else {
		return(Fl_Window::handle(event));
	}
}
void My_Fl_Small_Window::gotFocus() {
	// can be overwritten for single windows
	// printf("A Small Window got focus!\n");
}


/************************************************************************************
*
*	My_Fl_Search_Window
*
************************************************************************************/

My_Fl_Search_Window::My_Fl_Search_Window(int W,int H) : My_Fl_Small_Window (W, H) {}
void My_Fl_Search_Window::gotFocus() {
	// printf("Search Window got focus!\n");
	//app.updateSearchWindow();
}



/************************************************************************************
*
*	My_Fl_Button
*
************************************************************************************/

My_Fl_Button::My_Fl_Button(int x, int y, int w, int h, const char *label) : Fl_Button(x, y, w, h, label) {
}

/*
 *	Draws the Button
 *	col		Background color
 *	c		Label color
 */
void My_Fl_Button::draw() {
	if (type() == FL_HIDDEN_BUTTON) return;
	#ifdef __APPLE__
	rbox(colors, x(),y(), w(),h());
	#else
	rbox(colors, x(),y(), w(),h(), false);
	#endif
	Fl_Color c = labelcolor();
	labelcolor(colors.label);
	draw_label();
	labelcolor(c);
	if (Fl::focus() == this)
		draw_focus();
}



/*
 *
 *
 */
void My_Fl_Button::rbox(struct buttonColorStruct colors, int x, int y, int w, int h, bool rounded) {

	fl_color(colors.background);			// draw background
	fl_rectf(x,y,w,h);
	fl_color(colors.border);				// draw rectangle border
	if( colors.borderWidth > 1 ) {
		fl_rect(x+1,y+1,w-2,h-2);
	}
	fl_rect(x,y,w,h);

	if( rounded ) {
		//
		// TOP-LEFT-CORNER
		fl_color(colors.border);			// 4x4 border color
		fl_rectf(x,y,4,4);
		fl_color(colors.background);		// 2x2 button bg color
		fl_rectf(x+2,y+2,2,2);
		fl_color(colors.windowBg);			// corner to window bg
		fl_point(x,y);
		// antialias
		fl_color(fl_color_average(colors.windowBg,colors.border,0.8));
		fl_point(x+1,y); fl_point(x,y+1);
		fl_color(fl_color_average(colors.windowBg,colors.border,0.6));
		fl_point(x+2,y); fl_point(x,y+2);
		fl_color(fl_color_average(colors.background,colors.border,0.7));
		fl_point(x+3,y+1); fl_point(x+1,y+3);

		//
		// TOP-RIGHT-CORNER
		fl_color(colors.border);			// 4x4 border color
		fl_rectf(x+w-4,y,4,4);
		fl_color(colors.background);		// 2x2 button bg color
		fl_rectf(x+w-4,y+2,2,2);
		fl_color(colors.windowBg);			// corner to window bg
		fl_point(x+w-1,y);
		// antialias
		fl_color(fl_color_average(colors.windowBg,colors.border,0.8));
		fl_point(x+w-2,y); fl_point(x+w-1,y+1);
		fl_color(fl_color_average(colors.windowBg,colors.border,0.6));
		fl_point(x+w-3,y); fl_point(x+w-1,y+2);
		fl_color(fl_color_average(colors.background,colors.border,0.7));
		fl_point(x+w-4,y+1); fl_point(x+w-2,y+3);
		
		//
		// BOTTOM-LEFT-CORNER
		fl_color(colors.border);			// 4x4 border color
		fl_rectf(x,y+h-4,4,4);
		fl_color(colors.background);		// 2x2 button bg color
		fl_rectf(x+2,y+h-4,2,2);
		fl_color(colors.windowBg);			// corner to window bg
		fl_point(x,y+h-1);
		// antialias
		fl_color(fl_color_average(colors.windowBg,colors.border,0.8));
		fl_point(x+1,y+h-1); fl_point(x,y+h-2);
		fl_color(fl_color_average(colors.windowBg,colors.border,0.6));
		fl_point(x+2,y+h-1); fl_point(x,y+h-3);
		fl_color(fl_color_average(colors.background,colors.border,0.7));
		fl_point(x+3,y+h-2); fl_point(x+1,y+h-4);

		//
		// BOTTOM-RIGHT-CORNER
		fl_color(colors.border);			// 4x4 border color
		fl_rectf(x+w-4,y+h-4,4,4);
		fl_color(colors.background);		// 2x2 button bg color
		fl_rectf(x+w-4,y+h-4,2,2);
		fl_color(colors.windowBg);			// corner to window bg
		fl_point(x+w-1,y+h-1);
		// antialias
		fl_color(fl_color_average(colors.windowBg,colors.border,0.8));
		fl_point(x+w-2,y+h-1); fl_point(x+w-1,y+h-2);
		fl_color(fl_color_average(colors.windowBg,colors.border,0.6));
		fl_point(x+w-3,y+h-1); fl_point(x+w-1,y+h-3);
		fl_color(fl_color_average(colors.background,colors.border,0.7));
		fl_point(x+w-4,y+h-2); fl_point(x+w-2,y+h-4);
	}
}

void My_Fl_Button::set_colors(struct My_Fl_Button::buttonColorStruct c) {
	colors.background = c.background;
	colors.label = c.label;
	colors.border = c.border;
	colors.windowBg = c.windowBg;
	colors.borderWidth = c.borderWidth;
}






