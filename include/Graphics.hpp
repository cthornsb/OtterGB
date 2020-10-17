#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "colors.hpp"

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

class GPU;

#ifndef USE_OPENGL
	class SDL_Renderer;
	class SDL_Window;
	class SDL_Rect;

	class SDL_KeyboardEvent;
	class SDL_MouseButtonEvent;
	class SDL_MouseMotionEvent;

	class KeypressEvent{
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

		KeypressEvent() : key(0x0), 
			            down(false), none(true), 
			            lshift(false), rshift(false), 
			            lctrl(false), rctrl(false), 
			            lalt(false), ralt(false), 
			            lgui(false), rgui(false), 
			            num(false), caps(false), mode(false) { }
		
		void decode(const SDL_KeyboardEvent* evt, const bool &isDown);
	};

	class MouseEvent{
	public:
		unsigned char clicks;
		
		int x;
		int y;
		int xrel;
		int yrel;
		
		bool down;
		bool lclick;
		bool mclick;
		bool rclick;
		bool x1;
		bool x2;
		
		MouseEvent() : clicks(0x0),
			              x(0), y(0), xrel(0), yrel(0),
			              down(false), lclick(false), mclick(false), rclick(false),
			              x1(false), x2(false) { }
			              
		void decode(const SDL_MouseButtonEvent* evt, const bool &isDown);
		
		void decode(const SDL_MouseMotionEvent* evt);
	};

	class Window{
	public:
		/** Default constructor
		  */
		Window() : renderer(NULL), window(NULL), W(DEFAULT_WINDOW_WIDTH), H(DEFAULT_WINDOW_HEIGHT), nMult(1), init(false) { }
		
		/** Constructor taking the width and height of the window
		  */
		Window(const int &width, const int &height) : renderer(NULL), window(NULL), W(width), H(height), nMult(1), init(false) { }

		/** Destructor
		  */
		~Window();

		/** Get the width of the window (in pixels)
		  */
		int getWidth() const { return W; }
		
		/** Get the height of the window (in pixels)
		  */
		int getHeight() const { return H; }

		/** Get a pointer to the last user keypress event
		  */
		KeypressEvent* getKeypress(){ return &lastKey; }
		
		/** Get a pointer to the last user mouse event
		  */
		MouseEvent* getMouse(){ return &lastMouse; }

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
		void setDrawColor(const ColorRGB &color, const float &alpha=1);

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

		KeypressEvent lastKey; ///< The last key which was pressed by the user
		MouseEvent lastMouse; ///< The last mouse event which was performed by the user
		
		SDL_Rect *rectangle; ///< A rectangle used for drawing chunky pixels
	};
#else
	#include <vector>

	#include "vector3.hpp"

	class KeypressEvent{
	public:
		unsigned char key;

		bool down;

		KeypressEvent() : key(0x0), down(false) { }
	};

	class Window{
	public:
		/** Default constructor
		  */
		Window() : W(DEFAULT_WINDOW_WIDTH), H(DEFAULT_WINDOW_HEIGHT), nMult(1), init(false) { }
		
		/** Constructor taking the width and height of the window
		  */
		Window(const int &width, const int &height) : W(width), H(height), nMult(1), init(false) { }

		/** Destructor
		  */
		~Window();
		
		void close();

		void processEvents();

		GPU *getGPU(){ return gpu; }

		/** Get the width of the window (in pixels)
		  */
		int getWidth() const { return W; }
		
		/** Get the height of the window (in pixels)
		  */
		int getHeight() const { return H; }

		/** Get a pointer to the last user keypress event
		  */
		KeypressEvent* getKeypress(){ return &lastKey; }
		
		/** Get a pointer to the last user mouse event
		  */
		//MouseEvent* getMouse(){ return &lastMouse; }

		void setGPU(GPU *ptr){ gpu = ptr; }

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
		void setDrawColor(const ColorRGB &color, const float &alpha=1);

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

		/** Draw an N-sided polygon
		  * @param vertices Vector of vertices to draw
		  */ 
		void drawPolygon(const std::vector<vector3> &vertices);

		/** Render the current frame
		  */
		void render();

		/** Return true if the window has been closed and return false otherwise
		  */
		bool status();

		/** Initialize OpenGL and open the window
		  */
		void initialize();

	private:
		int W; ///< Width of the window (in pixels)
		int H; ///< Height of the window (in pixels)
		int nMult; ///< Integer multiplier for window scaling

		bool init; ///< Flag indicating that the window has been initialized

		KeypressEvent lastKey; ///< The last key which was pressed by the user
		//MouseEvent lastMouse; ///< The last mouse event which was performed by the user

		GPU *gpu; ///< Pointer to the graphics processor

		void addVertex(const int &x, const int &y);
	};
#endif

#endif
