#include <iostream>
#include <map>

#include <GL/freeglut.h>

#include "GraphicsOpenGL.hpp"

std::map<int, Window*> listOfWindows;

Window *getCurrentWindow(){
	return listOfWindows[glutGetWindow()];
}

/** Handle OpenGL keyboard presses.
  * @param key The keyboard character which was pressed.
  * @param x X coordinate of the mouse when the key was pressed (not used).
  * @param y Y coordinate of the mouse when the key was pressed (not used).
  */
void handleKeys(unsigned char key, int x, int y){
	Window *currentWindow = getCurrentWindow();
	if(key == 0x1B){ // Escape
		currentWindow->close();
		return;
	}
	currentWindow->getKeypress()->keyDown(key);
}

/** Handle OpenGL keyboard key releases.
  * @param key The keyboard character which was pressed.
  * @param x X coordinate of the mouse when the key was released (not used).
  * @param y Y coordinate of the mouse when the key was released (not used).
  */
void handleKeysUp(unsigned char key, int x, int y){
	getCurrentWindow()->getKeypress()->keyUp(key);
}

/** Handle OpenGL special key presses.
  * Redirect special keys to a more convenient format.
  * @param key The special key which was pressed
  * @param x X coordinate of the mouse when the key was pressed (not used)
  * @param y Y coordinate of the mouse when the key was pressed (not used)
  */
void handleSpecialKeys(int key, int x, int y){
	unsigned char ckey = 0x0;
	switch (key) {
		case GLUT_KEY_UP:
			ckey = 0x52;
			break;
		case GLUT_KEY_LEFT:
			ckey = 0x50;
			break;
		case GLUT_KEY_DOWN:
			ckey = 0x51;
			break;
		case GLUT_KEY_RIGHT:
			ckey = 0x4F;
			break;
		default:
			if (key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12) { // Function keys
				ckey = 0xF0;
				ckey |= (key & 0x0F);
			}
			else { return; }
			break;
	}
	if(ckey)
		handleKeys(ckey, x, y);
}

/** Handle OpenGL special key releases.
  * Redirect special keys to match the ones used by SDL.
  * @param key The special key which was pressed
  * @param x X coordinate of the mouse when the key was released (not used)
  * @param y Y coordinate of the mouse when the key was released (not used)
  */
void handleSpecialKeysUp(int key, int x, int y){
	unsigned char ckey = 0x0;
	switch (key) {
		case GLUT_KEY_UP:
			ckey = 0x52;
			break;
		case GLUT_KEY_LEFT:
			ckey = 0x50;
			break;
		case GLUT_KEY_DOWN:
			ckey = 0x51;
			break;
		case GLUT_KEY_RIGHT:
			ckey = 0x4F;
			break;
		default:
			return;
	}
	if(ckey)
		handleKeysUp(ckey, x, y);
}

/** Handle window resizes.
  * @param width The new width of the window after the user has resized it.
  * @param height The new height of the window after the user has resized it.
  */
void reshapeScene(GLint width, GLint height){
	Window *currentWindow = getCurrentWindow();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	int wprime = width;
	int hprime = height;
	float aspect = float(wprime)/hprime;

	if(aspect > currentWindow->getAspectRatio()){ // Wider window (height constrained)
		wprime = (int)(currentWindow->getAspectRatio() * hprime);
		glPointSize(float(hprime)/currentWindow->getHeight());
	}
	else{ // Taller window (width constrained)
		hprime = (int)(wprime / currentWindow->getAspectRatio());
		glPointSize(float(wprime)/currentWindow->getWidth());
	}

	glViewport(0, 0, wprime, hprime); // Update the viewport
	gluOrtho2D(0, currentWindow->getWidth(), currentWindow->getHeight(), 0);
	glMatrixMode(GL_MODELVIEW);

	// Update window size
	currentWindow->setWidth(wprime);
	currentWindow->setHeight(hprime);

	glutPostRedisplay();
	
	// Clear the window.
	currentWindow->clear();
}

void displayFunction() {
	// This callback does nothing, but is required on Windows
}

/////////////////////////////////////////////////////////////////////
// class KeyStates
/////////////////////////////////////////////////////////////////////

KeyStates::KeyStates() : count(0), streamMode(false) {
	reset();
}

void KeyStates::enableStreamMode(){
	streamMode = true;
	reset();
}

void KeyStates::disableStreamMode(){
	streamMode = false;
	reset();
}

bool KeyStates::poll(const unsigned char &key){ 
	if(states[key]){
		states[key] = false;
		return true;
	}
	return false;
}

void KeyStates::keyDown(const unsigned char &key){
	if(!streamMode){
		if(!states[key]){
			states[key] = true;
			count++;
		}
	}
	else{
		buffer.push((char)key);
	}
}

void KeyStates::keyUp(const unsigned char &key){
	if(!streamMode){
		if(states[key]){
			states[key] = false;
			count--;
		}
	}
}

bool KeyStates::get(char& key){
	if(buffer.empty())
		return false;
	key = buffer.front();
	buffer.pop();
	return true;
}

void KeyStates::reset(){
	for(int i = 0; i < 256; i++)
		states[i] = false;
	while(!buffer.empty())
		buffer.pop();
	count = 0;
}

/////////////////////////////////////////////////////////////////////
// class Window
/////////////////////////////////////////////////////////////////////

Window::~Window(){
	close();
}

void Window::close(){
	glutDestroyWindow(winID);
	init = false;
}

void Window::processEvents(){
	glutMainLoopEvent();
}

void Window::setScalingFactor(const int &scale){ 
	nMult = scale; 
	glutReshapeWindow(W*scale, H*scale);
}
	
void Window::setDrawColor(ColorRGB *color, const float &alpha/*=1*/){
	glColor3f(color->r, color->g, color->b);
}

void Window::setDrawColor(const ColorRGB &color, const float &alpha/*=1*/){
	glColor3f(color.r, color.g, color.b);
}

void Window::setCurrent(){
	glutSetWindow(winID);
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

/** Draw multiple lines to the screen
  * @param x1 X coordinate of the upper left corner
  * @param y1 Y coordinate of the upper left corner
  * @param x2 X coordinate of the bottom right corner
  * @param y2 Y coordinate of the bottom right corner
  */
void Window::drawRectangle(const int &x1, const int &y1, const int &x2, const int &y2){
	drawLine(x1, y1, x2, y1); // Top
	drawLine(x2, y1, x2, y2); // Right
	drawLine(x2, y2, x1, y2); // Bottom
	drawLine(x1, y2, x1, y1); // Left
}

void Window::render(){
	glFlush();
}

bool Window::status(){
	return init;
}

void Window::initialize(){
	if(init) return;

	// Dummy command line arguments
	int dummyArgc = 1;
	
	nMult = 2;

	// Open the SDL window
	static bool firstInit = true;
	if(firstInit){ // Stupid glut
		glutInit(&dummyArgc, NULL);
		firstInit = false;
	}
	glutInitDisplayMode(GLUT_SINGLE);
	glutInitWindowSize(W*nMult, H*nMult);
	glutInitWindowPosition(100, 100);
	winID = glutCreateWindow("gbc");
	listOfWindows[winID] = this;

	// Set window size handler
	glutReshapeFunc(reshapeScene);

	// Set the display function callback (required for Windows)
	glutDisplayFunc(displayFunction);

	init = true;
}

void Window::setKeyboardStreamMode(){
	// Enable keyboard repeat
	glutIgnoreKeyRepeat(0);
	keys.enableStreamMode();
}

void Window::setKeyboardToggleMode(){
	// Disable keyboard repeat
	glutIgnoreKeyRepeat(1);
	keys.disableStreamMode();
}

void Window::setupKeyboardHandler(){
	// Set keyboard handler
	glutKeyboardFunc(handleKeys);
	glutSpecialFunc(handleSpecialKeys);

	// Keyboard up handler
	glutKeyboardUpFunc(handleKeysUp);
	glutSpecialUpFunc(handleSpecialKeysUp);

	// Set window size handler
	glutReshapeFunc(reshapeScene);

	// Set keyboard keys to behave as button toggles
	setKeyboardToggleMode();
}

void Window::paintGL(){
	this->render();
}

void Window::initializeGL(){
	this->initialize();
}

void Window::resizeGL(int width, int height){
	reshapeScene(width, height);
}
