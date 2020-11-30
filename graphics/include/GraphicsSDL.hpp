#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "colors.hpp"

// Constants, do not adjust. Adjust the scaling factor instead.
const unsigned int SCREEN_WIDTH = 160;
const unsigned int SCREEN_HEIGHT = 144;
const float ASPECT_RATIO = SCREEN_WIDTH/SCREEN_HEIGHT;

class GPU;

class SDL_Renderer;
class SDL_Window;
class SDL_Rect;

class SDL_KeyboardEvent;
class SDL_MouseButtonEvent;
class SDL_MouseMotionEvent;

class KeyStates{
public:
	unsigned char key;

	bool down;
	bool none;
	bool lshift;
	bool rshift;
	bool lctrl;
	bool rctrl;
	bool lalt;
	bool ralt;
	bool lgui;
	bool rgui;
	bool num;
	bool caps;
	bool mode;

	KeyStates() : key(0x0), 
		            down(false), none(true), 
		            lshift(false), rshift(false), 
		            lctrl(false), rctrl(false), 
		            lalt(false), ralt(false), 
		            lgui(false), rgui(false), 
		            num(false), caps(false), mode(false) { }
	
	void decode(const SDL_KeyboardEvent* evt, const bool &isDown);
};

class Window{
public:
	/** Default constructor
	  */
	Window() : renderer(NULL), window(NULL), W(SCREEN_WIDTH), H(SCREEN_HEIGHT), nMult(2), init(false) { }
	
	/** Constructor taking the width and height of the window
	  */
	Window(const int &width, const int &height) : renderer(NULL), window(NULL), W(width), H(height), nMult(2), init(false) { }

	/** Destructor
	  */
	~Window();

	void processEvents(){ }

	/** Get the width of the window (in pixels)
	  */
	int getWidth() const { return W; }
	
	/** Get the height of the window (in pixels)
	  */
	int getHeight() const { return H; }

	/** Get a pointer to the last user keypress event
	  */
	KeyStates* getKeypress(){ return &lastKey; }
	
	/** Set the width of the window (in pixels)
	  */
	void setWidth(const int &width){ W = width; }
	
	/** Set the height of the window (in pixels)
	  */
	void setHeight(const int &height){ H = height; }

	/** Set the integer pixel scaling multiplier (default = 1)
	  */
	void setScalingFactor(const int &scale){ nMult = scale; }

	/** Set the current draw color
	  */
	void setDrawColor(ColorRGB *color, const float &alpha=1);

	/** Clear the screen with a given color
	  */
	void clear(const ColorRGB &color=Colors::BLACK);

	/** Draw a single pixel at position (x, y)
	  */
	void drawPixel(const int &x, const int &y);

	/** Draw multiple pixels at positions (x1, y1) (x2, y2) ... (xN, yN)
	  * @param x Array of X pixel coordinates
	  * @param y Array of Y pixel coordinates
	  * @param N The number of elements in the arrays and the number of pixels to draw
	  */
	void drawPixel(const int *x, const int *y, const size_t &N);
	
	/** Draw a single line to the screen between points (x1, y1) and (x2, y2)
	  */
	void drawLine(const int &x1, const int &y1, const int &x2, const int &y2);

	/** Draw multiple lines to the screen
	  * @param x Array of X pixel coordinates
	  * @param y Array of Y pixel coordinates
	  * @param N The number of elements in the arrays. Since it is assumed that the number of elements 
		       in the arrays is equal to @a N, the total number of lines which will be drawn is equal to N-1
	  */
	void drawLine(const int *x, const int *y, const size_t &N);

	/** Render the current frame
	  */
	void render();

	/** Return true if the window has been closed and return false otherwise
	  */
	bool status();

	/** Initialize SDL and open the window
	  */
	void initialize();

private:
	SDL_Renderer *renderer; ///< Pointer to the SDL renderer
	SDL_Window *window; ///< Pointer to the SDL window

	int W; ///< Width of the window (in pixels)
	int H; ///< Height of the window (in pixels)
	int nMult; ///< Integer multiplier for window scaling

	bool init; ///< Flag indicating that the window has been initialized

	KeyStates lastKey; ///< The last key which was pressed by the user
	
	SDL_Rect *rectangle; ///< A rectangle used for drawing chunky pixels
};
#endif
