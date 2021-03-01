#include <iostream>

#include "GraphicsOpenGL.hpp"

void handleErrors(int error, const char* description) {
	std::cout << " [glfw] Error! id=" << error << " : " << description << std::endl;
}

/** Handle window resizes.
  * @param width The new width of the window after the user has resized it.
  * @param height The new height of the window after the user has resized it.
  */
void reshapeScene(GLFWwindow* window, int width, int height){
	Window* currentWindow = ActiveWindows::get().find(window);
	if(!currentWindow)
		return;
	
	// Update window size
	currentWindow->updateWindowSize(width, height);
}

void windowFocus(GLFWwindow* window, int focused){
	if (focused){ // The window gained input focus
	}
	else{ // The window lost input focus
	}
}

/////////////////////////////////////////////////////////////////////
// class Window
/////////////////////////////////////////////////////////////////////

Window::~Window(){
	// glfw window will automatically be destroyed by its unique_ptr destructor
}

void Window::close(){
	glfwSetWindowShouldClose(win.get(), GLFW_TRUE);
}

bool Window::processEvents(){
	setCurrent();
	if(!status())
		return false;
	glfwPollEvents();
	return true;
}

void Window::updateWindowSize(const int& w, const int& h){
	width = w;
	height = h;
	aspect = float(w) / float(h);
	if(init){
		glfwSetWindowSize(win.get(), width, height); // Update physical window size
		reshape();
	}
}

void Window::updateWindowSize(const int& scale){
	updateWindowSize(nNativeWidth * scale, nNativeHeight * scale);
}

void Window::setDrawColor(ColorRGB *color, const float &alpha/*=1*/){
	glColor3f(color->r, color->g, color->b);
}

void Window::setDrawColor(const ColorRGB &color, const float &alpha/*=1*/){
	glColor3f(color.r, color.g, color.b);
}

void Window::setCurrent(){
	glfwMakeContextCurrent(win.get());
}

void Window::clear(const ColorRGB &color/*=Colors::BLACK*/){
	glClear(GL_COLOR_BUFFER_BIT);
}

void Window::drawPixel(const int &x, const int &y){
	glBegin(GL_POINTS);
		glVertex2i(x, y);
	glEnd();
}

void Window::drawPixel(const int *x, const int *y, const size_t &N){
	for(size_t i = 0; i < N; i++) // Draw N pixels
		drawPixel(x[i], y[i]);
}

void Window::drawLine(const int &x1, const int &y1, const int &x2, const int &y2){
	glBegin(GL_LINES);
		glVertex2i(x1, y1);
		glVertex2i(x2, y2);
	glEnd();
}

void Window::drawLine(const int *x, const int *y, const size_t &N){
	if(N < 2) // Nothing to draw
		return;
	for(size_t i = 0; i < N-1; i++)
		drawLine(x[i], y[i], x[i+1], y[i+1]);
}

void Window::drawRectangle(const int &x1, const int &y1, const int &x2, const int &y2){
	drawLine(x1, y1, x2, y1); // Top
	drawLine(x2, y1, x2, y2); // Right
	drawLine(x2, y2, x1, y2); // Bottom
	drawLine(x1, y2, x1, y1); // Left
}

void Window::drawBitmap(const unsigned int& width, const unsigned int& height, const float& x0, const float& y0, const unsigned char* data){
	glRasterPos2i(x0, y0);
	glBitmap(width, height, 0, 0, 0, 0, data);
}

void Window::drawPixels(const unsigned int& width, const unsigned int& height, const float& x0, const float& y0, const ImageBuffer* data){
	glRasterPos2i(x0, y0);
	glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data->get());
}

void Window::buffWrite(const unsigned short& x, const unsigned short& y, const ColorRGB& color){
	buffer.setPixel(x, y, color);
}

void Window::buffWriteLine(const unsigned short& y, const ColorRGB& color){
	buffer.setPixelRow(y, color);
}

void Window::render(){
	glFlush();
	glfwSwapBuffers(win.get());
}

void Window::drawBuffer(){
	drawPixels(nNativeWidth, nNativeHeight ,0, nNativeHeight, &buffer);
}

void Window::renderBuffer(){
	drawBuffer();
	render();
}

bool Window::status(){
	return (init && !glfwWindowShouldClose(win.get()));
}

void Window::initialize(){
	if(init || nNativeWidth == 0 || nNativeHeight == 0) 
		return;

	// Set the GLFW error callback
	glfwSetErrorCallback(handleErrors);

	// Open the graphics window
	static bool firstInit = true;
	if(firstInit){
		glfwInit();
		firstInit = false;
	}
	
	// Create the window
	win.reset(glfwCreateWindow(nNativeWidth, nNativeHeight, "ottergb", NULL, NULL)); // Windowed
	//win.reset(glfwCreateWindow(nNativeWidth, nNativeHeight, "ottergb", glfwGetPrimaryMonitor(), NULL)); // Fullscreen
	
	// Add window to the map
	ActiveWindows::get().add(win.get(), this);

	// Setup frame buffer
	buffer.resize(nNativeWidth, nNativeHeight);

	// Set initialization flag
	init = true;

	// Update projection matrix
	updateWindowSize(width, height);

	// Set window size handler
	glfwSetWindowSizeCallback(win.get(), reshapeScene);

	// Set window focus callback
	glfwSetWindowFocusCallback(win.get(), windowFocus);
}

void Window::updatePixelZoom(){
	glPixelZoom((float)width / nNativeWidth, (float)height / nNativeHeight);
}

void Window::setKeyboardStreamMode(){
	// Enable keyboard repeat
	keys.enableStreamMode();
}

void Window::setKeyboardToggleMode(){
	// Disable keyboard repeat
	keys.disableStreamMode();
}

void Window::setupKeyboardHandler(){
	// Set keyboard callback
	glfwSetKeyCallback(win.get(), handleKeys);

	// Set keyboard keys to behave as button toggles
	setKeyboardToggleMode();

	// Disable key repeat
	glfwSetInputMode(win.get(), GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetInputMode(win.get(), GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

	// Set cursor properties
	//glfwSetInputMode(win.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//glfwSetInputMode(win.get(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	//glfwSetInputMode(win.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::reshape(){
	setCurrent();
	
	updatePixelZoom();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Update viewport
	glViewport(0, 0, width, height);
	glOrtho(0, nNativeWidth, nNativeHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	// Clear the window.
	clear();
}

ActiveWindows& ActiveWindows::get(){
	static ActiveWindows instance;
	return instance;
}

void ActiveWindows::add(GLFWwindow* glfw, Window* win){
	listOfWindows[glfw] = win;
}

Window* ActiveWindows::find(GLFWwindow* glfw){
	Window* retval = 0x0;
	auto win = listOfWindows.find(glfw);
	if(win != listOfWindows.end())
		retval = win->second;
	return retval;
}
	
