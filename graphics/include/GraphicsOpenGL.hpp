#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <vector>
#include <queue>
#include <memory>
#include <map>

#include <GLFW/glfw3.h>

#include "colors.hpp"
#include "KeyStates.hpp"
#include "ImageBuffer.hpp"

class GPU;

struct DestroyGLFWwindow {
	void operator()(GLFWwindow* ptr) {
		glfwDestroyWindow(ptr);
	}
};

class Window{
public:
	/** Default constructor
	  */
	Window() = delete;
	
	/** Constructor taking the width and height of the window
	  */
	Window(const int &w, const int &h, const int& scale=1) : 
		win(),
		nNativeWidth(w), 
		nNativeHeight(h),
		fNativeAspect(float(w)/h),
		width(nNativeWidth * scale),
		height(nNativeHeight * scale),
		aspect(fNativeAspect),
		init(false),
		gpu(0x0),
		keys(),
		buffer()
	{
	}

	/** Copy constructor
	  */
	Window(const Window&) = delete;
	
	/** Assignment operator
	  */
	Window& operator = (const Window&) = delete;

	/** Destructor
	  */
	~Window();
	
	void close();

	bool processEvents();

	GPU *getGPU(){
		return gpu; 
	}

	/** Get the width of the window (in pixels)
	  */
	int getNativeWidth() const {
		return nNativeWidth; 
	}
	
	/** Get the height of the window (in pixels)
	  */
	int getNativetHeight() const {
		return nNativeHeight;
	}

	/** Get the aspect ratio of the window (W/H)
	  */
	float getNativeAspectRatio() const {
		return fNativeAspect;
	}

	/** Get the current width of the window (in pixels)
	  */
	int getWidth() const {
		return width;
	}
	
	/** Get the current height of the window (in pixels)
	  */
	int getHeight() const {
		return height;
	}

	/** Get the aspect ratio of the window (W/H)
	  */
	float getAspectRatio() const {
		return aspect;
	}

	/** Get a pointer to the last user keypress event
	  */
	KeyStates* getKeypress(){
		return &keys;
	}

	/** Set pointer to the pixel processor
	  */
	void setGPU(GPU *ptr){
		gpu = ptr;
	}

	/** Set the size of graphical window
	  * Has no effect if window has not been initialized.
	  */
	void updateWindowSize(const int& w, const int& h);

	/** Set the size of graphical window
	  * Has no effect if window has not been initialized.
	  */
	void updateWindowSize(const int& scale);

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

	/** Draw multiple lines to the screen
	  * @param x1 X coordinate of the upper left corner
	  * @param y1 Y coordinate of the upper left corner
	  * @param x2 X coordinate of the bottom right corner
	  * @param y2 Y coordinate of the bottom right corner
	  */
	static void drawRectangle(const int &x1, const int &y1, const int &x2, const int &y2);

	/** Write pixel data from an array to the GPU frame buffer
	  */
	static void drawBitmap(const unsigned int& width, const unsigned int& height, const float& x0, const float& y0, const unsigned char* data);

	/** Write pixel data from an image buffer to the GPU frame buffer
	  */
	static void drawPixels(const unsigned int& width, const unsigned int& height, const float& x0, const float& y0, const ImageBuffer* data);

	/** Write to CPU frame buffer
	  */
	void buffWrite(const unsigned short& x, const unsigned short& y, const ColorRGB& color);

	/** Draw a horizontal line to CPU frame buffer
	  */
	void buffWriteLine(const unsigned short& y, const ColorRGB& color);

	/** Render the current frame
	  */
	void render();

	/** Draw the image buffer but do not render the frame
	  * Useful if you wish to do additional processing on top of the image buffer.
	  */
	void drawBuffer();

	/** Draw the image buffer and render the frame
	  */
	void renderBuffer();

	/** Return true if the window has been closed and return false otherwise
	  */
	bool status();

	/** Initialize OpenGL and open the window
	  */
	void initialize();

	/** Set pixel scaling factor
	  */
	void updatePixelZoom();

	void setKeyboardStreamMode();

	void setKeyboardToggleMode();

	void setupKeyboardHandler();

private:
	std::unique_ptr<GLFWwindow, DestroyGLFWwindow> win;

	int nNativeWidth; ///< Original width of the window (in pixels)
	
	int nNativeHeight; ///< Original height of the window (in pixels)
	
	float fNativeAspect; ///< Original aspect ratio of window

	int width; ///< Current width of window
	
	int height; ///< Current height of window
	
	float aspect; ///< Current aspect ratio of window
	
	bool init; ///< Flag indicating that the window has been initialized

	GPU *gpu; ///< Pointer to the graphics processor

	KeyStates keys; ///< The last key which was pressed by the user
	
	ImageBuffer buffer; ///< CPU-side frame buffer
	
	/** Update viewport and projection matrix after resizing window
	  */
	void reshape();
};

class ActiveWindows{
public:
	/** Copy constructor
	  */
	ActiveWindows(const ActiveWindows&) = delete;

	/** Assignment operator
	  */
	ActiveWindows& operator = (const ActiveWindows&) = delete;
	
	/** Get reference to the singleton
	  */
	static ActiveWindows& get();
	
	/** Add a GLFW window to the map
	  */
	void add(GLFWwindow* glfw, Window* win);
	
	/** Find a pointer to an opengl window object
	  */
	Window* find(GLFWwindow* glfw);
	
private:
	std::map<GLFWwindow*, Window*> listOfWindows; ///< Map of GLFW windows

	/** Default constructor (private)
	  */
	ActiveWindows() :
		listOfWindows()
	{ 
	}
};

#endif

