#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <vector>

#ifdef USE_QT_DEBUGGER
#include <QtOpenGL/QGLWidget>
#endif

#include "colors.hpp"

class GPU;

class KeyStates{
public:
	KeyStates();
	
	bool empty() const { return (count == 0); }
	
	bool check(const unsigned char &key) const { return states[key]; }
	
	bool poll(const unsigned char &key);
	
	void keyDown(const unsigned char &key);
	
	void keyUp(const unsigned char &key);

private:
	unsigned short count; ///< Number of standard keyboard keys which are currently pressed

	bool states[256]; ///< States of keyboard keys (true indicates key is down) 
};

#ifndef USE_QT_DEBUGGER
class Window{
#else
class Window : public QGLWidget {
#endif
public:
	/** Default constructor
	  */
	Window() : 
#ifdef USE_QT_DEBUGGER
		QGLWidget(),
#endif
		W(0), 
		H(0), 
		A(1),
		width(0),
		height(0),
		aspect(1),
		nMult(1), 
		winID(0), 
		init(false)
	{ 
	}
	
	/** Constructor taking the width and height of the window
	  */
	Window(const int &w, const int &h) : 
#ifdef USE_QT_DEBUGGER
		QGLWidget(),
#endif
		W(w), 
		H(h),
		A(float(w)/h),
		width(w),
		height(h),
		aspect(float(w)/h),
		nMult(1), 
		init(false)
	{
	}

	/** Destructor
	  */
	~Window();
	
	void close();

	void processEvents();

	GPU *getGPU(){ return gpu; }

	/** Get the width of the window (in pixels)
	  */
	int getCurrentWidth() const { return W; }
	
	/** Get the height of the window (in pixels)
	  */
	int getCurrentHeight() const { return H; }

	/** Get the width of the window (in pixels)
	  */
	int getWidth() const { return W; }
	
	/** Get the height of the window (in pixels)
	  */
	int getHeight() const { return H; }

	/** Get screen scale multiplier.
	  */
	int getScale() const { return nMult; }

	/** Get the aspect ratio of the window (W/H)
	  */
	float getCurrentAspectRatio() const { return aspect; }

	/** Get the aspect ratio of the window (W/H)
	  */
	float getAspectRatio() const { return A; }

	/** Get the GLUT window ID number
	  */
	int getWindowID() const { return winID; }

	/** Get a pointer to the last user keypress event
	  */
	KeyStates* getKeypress(){ return &keys; }

	/** Set pointer to the pixel processor
	  */
	void setGPU(GPU *ptr){ gpu = ptr; }

	/** Set the width of the window (in pixels)
	  */
	void setWidth(const int &w){ width = w; }
	
	/** Set the height of the window (in pixels)
	  */
	void setHeight(const int &h){ height = h; }

	/** Set the integer pixel scaling multiplier (default = 1)
	  */
	void setScalingFactor(const int &scale);

	/** Set the current draw color
	  */
	static void setDrawColor(ColorRGB *color, const float &alpha=1);

	/** Set the current draw color
	  */
	static void setDrawColor(const ColorRGB &color, const float &alpha=1);

	/** Set this window as the current GLUT window
	  */
	void setCurrent();

	/** Clear the screen with a given color
	  */
	static void clear(const ColorRGB &color=Colors::BLACK);

	/** Draw a single pixel at position (x, y)
	  */
	static void drawPixel(const int &x, const int &y);

	/** Draw multiple pixels at positions (x1, y1) (x2, y2) ... (xN, yN)
	  * @param x Array of X pixel coordinates
	  * @param y Array of Y pixel coordinates
	  * @param N The number of elements in the arrays and the number of pixels to draw
	  */
	static void drawPixel(const int *x, const int *y, const size_t &N);
	
	/** Draw a single line to the screen between points (x1, y1) and (x2, y2)
	  */
	static void drawLine(const int &x1, const int &y1, const int &x2, const int &y2);

	/** Draw multiple lines to the screen
	  * @param x Array of X pixel coordinates
	  * @param y Array of Y pixel coordinates
	  * @param N The number of elements in the arrays. Since it is assumed that the number of elements 
		       in the arrays is equal to @a N, the total number of lines which will be drawn is equal to N-1
	  */
	static void drawLine(const int *x, const int *y, const size_t &N);

	/** Render the current frame
	  */
	static void render();

	/** Return true if the window has been closed and return false otherwise
	  */
	bool status();

	/** Initialize OpenGL and open the window
	  */
	void initialize();

	void setupKeyboardHandler();

	virtual void paintGL();
	
	virtual void initializeGL();

	virtual void resizeGL(int width, int height);

private:
	int W; ///< Original width of the window (in pixels)
	int H; ///< Original height of the window (in pixels)
	float A; ///< Original aspect ratio of window

	int width; ///< Current width of window
	int height; ///< Current height of window
	float aspect; ///< Current aspect ratio of window
	
	int nMult; ///< Integer multiplier for window scaling
	int winID; ///< GLUT window identifier

	bool init; ///< Flag indicating that the window has been initialized

	GPU *gpu; ///< Pointer to the graphics processor

	KeyStates keys; ///< The last key which was pressed by the user
};
#endif
