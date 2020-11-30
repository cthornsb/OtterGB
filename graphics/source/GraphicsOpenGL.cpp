#include <iostream>

#include <GL/freeglut.h>

#include "GraphicsOpenGL.hpp"
#include "SystemGBC.hpp"
#include "GPU.hpp"

Window *MAIN_WINDOW = 0x0;

/** Handle OpenGL keyboard presses.
  * @param key The keyboard character which was pressed.
  * @param x X coordinate of the mouse when the key was pressed (not used).
  * @param y Y coordinate of the mouse when the key was pressed (not used).
  */
void handleKeys(unsigned char key, int x, int y){
	if(key == 0x1B){ // Escape
		MAIN_WINDOW->close();
		return;
	}
	MAIN_WINDOW->getKeypress()->keyDown(key);
}

/** Handle OpenGL keyboard key releases.
  * @param key The keyboard character which was pressed.
  * @param x X coordinate of the mouse when the key was released (not used).
  * @param y Y coordinate of the mouse when the key was released (not used).
  */
void handleKeysUp(unsigned char key, int x, int y){
	MAIN_WINDOW->getKeypress()->keyUp(key);
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
		case GLUT_KEY_F1 ... GLUT_KEY_F12: // Function keys
			ckey = 0xF0;
			ckey |= (key & 0x0F);
			break;
		default:
			return;
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

#ifndef BACKGROUND_DEBUG
	const int WIDTH = 160;		
	const int HEIGHT = 144;
	const float ASPECT = float(WIDTH)/HEIGHT;
#else		
	const int WIDTH = 256;		
	const int HEIGHT = 256;
	const float ASPECT = float(WIDTH)/HEIGHT;
#endif
	
	int wprime = width;
	int hprime = height;

	if(width > height){
		wprime = height * ASPECT; // Adjust width for desired aspect ratio
		glPointSize(hprime/HEIGHT); // Set chunky pixel size
	}
	else{
		hprime = width / ASPECT;
		glPointSize(wprime/WIDTH); // Set chunky pixel size
	}

	MAIN_WINDOW->setWidth(wprime);
	MAIN_WINDOW->setHeight(hprime);

	glViewport(0, 0, wprime, hprime); // Update the viewport
	gluOrtho2D(0, WIDTH, HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

/////////////////////////////////////////////////////////////////////
// class KeyStates
/////////////////////////////////////////////////////////////////////

KeyStates::KeyStates(){
	for(int i = 0; i < 256; i++)
		states[i] = false;
}

bool KeyStates::poll(const unsigned char &key){ 
	if(states[key]){
		states[key] = false;
		return true;
	}
	return false;
}

void KeyStates::keyDown(const unsigned char &key){
	if(!states[key]){
		states[key] = true;
		count++;
	}
}

void KeyStates::keyUp(const unsigned char &key){
	if(states[key]){
		states[key] = false;
		count--;
	}
}

/////////////////////////////////////////////////////////////////////
// class Window
/////////////////////////////////////////////////////////////////////

Window::~Window(){
	close();
}

void Window::close(){
	glutDestroyWindow(1);
	init = false;
}

void Window::processEvents(){
	glutMainLoopEvent();
}

void Window::setDrawColor(ColorRGB *color, const float &alpha/*=1*/){
	glColor3f(color->r, color->g, color->b);
}

void Window::setDrawColor(const ColorRGB &color, const float &alpha/*=1*/){
	glColor3f(color.r, color.g, color.b);
}

void Window::setExternalWindow(const int &id){
	winID = id;
	init = true;
}

void Window::set(){
	glutSetWindow(winID);
}

void Window::clear(const ColorRGB &color/*=Colors::BLACK*/){
	glClear(GL_COLOR_BUFFER_BIT);
}

void Window::drawPixel(const int &x, const int &y){
	glBegin(GL_POINTS);
		addVertex(x, y);
	glEnd();
}

void Window::drawPixel(const int *x, const int *y, const size_t &N){
	for(size_t i = 0; i < N; i++) // Draw N pixels
		drawPixel(x[i], y[i]);
}

void Window::drawLine(const int &x1, const int &y1, const int &x2, const int &y2){
	glBegin(GL_LINES);
		addVertex(x1, y1);
		addVertex(x2, y2);
	glEnd();
}

void Window::drawLine(const int *x, const int *y, const size_t &N){
	if(N < 2) // Nothing to draw
		return;
	for(size_t i = 0; i < N-1; i++)
		drawLine(x[i], y[i], x[i+1], y[i+1]);
}

/*void Window::drawPolygon(const std::vector<vector3> &vertices){
	glBegin(GL_POLYGON);
		for(auto vertex=vertices.begin(); vertex != vertices.end(); vertex++)
			addVertex(vertex->x, vertex->y);
	glEnd();
}*/

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

	MAIN_WINDOW = this;

	// Open the SDL window
	glutInit(&dummyArgc, NULL);
	glutInitDisplayMode(GLUT_SINGLE);
	glutInitWindowSize(W*nMult, H*nMult);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("gbc");

	// Set keyboard handler
	glutKeyboardFunc(handleKeys);
	glutSpecialFunc(handleSpecialKeys);

	// Keyboard up handler
	glutKeyboardUpFunc(handleKeysUp);
	glutSpecialUpFunc(handleSpecialKeysUp);

	// Set window size handler
	glutReshapeFunc(reshapeScene);

	// Disable the display function
	//glutDisplayFunc(0);

	// Disable the idle function
	//glutIdleFunc(0);

	// Disable keyboard repeat
	glutIgnoreKeyRepeat(1);

	// Set the current window ID
	winID = glutGetWindow();
	std::cout << " id=" << winID << std::endl;

	init = true;
}

void Window::addVertex(const int &x, const int &y){
	// Convert from screen space to world space
	glVertex2i(x, y);
}
