
#include <iostream>

#include "Graphics.hpp"
#include "SystemGBC.hpp"
#include "GPU.hpp"

Window *MAIN_WINDOW = 0x0;

#ifndef USE_OPENGL
	#include <SDL2/SDL.h>

	void KeypressEvent::decode(const SDL_KeyboardEvent* evt, const bool &isDown){
		key = evt->keysym.sym;
		unsigned short modifier = evt->keysym.mod;
		none   = modifier & KMOD_NONE;
		lshift = modifier & KMOD_LSHIFT;
		rshift = modifier & KMOD_RSHIFT;
		lctrl  = modifier & KMOD_LCTRL;
		rctrl  = modifier & KMOD_RCTRL;
		lalt   = modifier & KMOD_LALT;
		ralt   = modifier & KMOD_RALT;	
		lgui   = modifier & KMOD_LGUI;
		rgui   = modifier & KMOD_RGUI;
		num    = modifier & KMOD_NUM;
		caps   = modifier & KMOD_CAPS;
		mode   = modifier & KMOD_MODE;
		down = isDown;
	}

	void MouseEvent::decode(const SDL_MouseButtonEvent* evt, const bool &isDown){
		lclick = evt->button & SDL_BUTTON_LMASK;
		mclick = evt->button & SDL_BUTTON_MMASK;
		rclick = evt->button & SDL_BUTTON_RMASK;
		x1     = evt->button & SDL_BUTTON_X1MASK;
		x2     = evt->button & SDL_BUTTON_X2MASK;
		clicks = evt->clicks;
		down = isDown;
	}

	void MouseEvent::decode(const SDL_MouseMotionEvent* evt){
		lclick = evt->state & SDL_BUTTON_LMASK;
		mclick = evt->state & SDL_BUTTON_MMASK;
		rclick = evt->state & SDL_BUTTON_RMASK;
		x1     = evt->state & SDL_BUTTON_X1MASK;
		x2     = evt->state & SDL_BUTTON_X2MASK;
		x = evt->x;
		y = evt->y;
		xrel = evt->xrel;
		yrel = evt->yrel;
	}

	Window::~Window(){
		//delete rectangle;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void Window::setDrawColor(const ColorRGB &color, const float &alpha/*=1*/){
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, ColorRGB::toUChar(alpha));
	}

	void Window::clear(const ColorRGB &color/*=Colors::BLACK*/){
		setDrawColor(color);
		SDL_RenderClear(renderer);
	}

	void Window::drawPixel(const int &x, const int &y){
		if(nMult == 1)
			SDL_RenderDrawPoint(renderer, x, y);
		else{
			rectangle->h = nMult;
			rectangle->w = nMult;
			rectangle->x = x*nMult;
			rectangle->y = y*nMult;
			SDL_RenderDrawRect(renderer, rectangle);
		}
	}

	void Window::drawPixel(const int *x, const int *y, const size_t &N){
		for(size_t i = 0; i < N; i++) // Draw N pixels
			drawPixel(x[i], y[i]);
	}

	void Window::drawLine(const int &x1, const int &y1, const int &x2, const int &y2){
		SDL_RenderDrawLine(renderer, x1*nMult, y1*nMult, x2*nMult, y2*nMult);
	}

	void Window::drawLine(const int *x, const int *y, const size_t &N){
		if(N == 0) // Nothing to draw
			return;
		for(size_t i = 0; i < N-1; i++)
			drawLine(x[i], y[i], x[i+1], y[i+1]);
	}

	void Window::render(){
		SDL_RenderPresent(renderer);
	}

	bool Window::status(){
		static SDL_Event event;
		if(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_KEYDOWN:
					lastKey.decode(&event.key, true);
					if(lastKey.key == 0x1B) // Escape
						return false;
					break;
				case SDL_KEYUP:
					lastKey.decode(&event.key, false);
					break;
				case SDL_MOUSEBUTTONDOWN:
					lastMouse.decode(&event.button, true);
					break;
				case SDL_MOUSEBUTTONUP:
					lastMouse.decode(&event.button, false);
					break;
				case SDL_MOUSEMOTION:
					lastMouse.decode(&event.motion);
					break;
				case SDL_QUIT:
					return false;
				default:
					break;
			}
		}
		return true;
	}

	void Window::initialize(){
		if(init) return;

		rectangle = new SDL_Rect;

		// Open the SDL window
		SDL_Init(SDL_INIT_VIDEO);
		SDL_CreateWindowAndRenderer(W*nMult, H*nMult, 0, &window, &renderer);
		clear();
		
		init = true;
	}
#else
	#include <GL/freeglut.h>

	void display(void){
	}

	void handleKeys(unsigned char key, int x, int y){
		if(key == 0x1B){ // Escape
			MAIN_WINDOW->close();
			return;
		}
		MAIN_WINDOW->getKeypress()->key = key;
		MAIN_WINDOW->getKeypress()->down = true;
	}

	void handleKeysUp(unsigned char key, int x, int y){
		MAIN_WINDOW->getKeypress()->down = false;
	}

	// Ugh
	void handleSpecialKeys(int key, int x, int y){
		unsigned char ckey;
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
		MAIN_WINDOW->getKeypress()->key = ckey;
		MAIN_WINDOW->getKeypress()->down = true;
	}

	void handleSpecialKeysUp(int key, int x, int y){
		MAIN_WINDOW->getKeypress()->down = false;
	}

	void reshapeScene(GLint width, GLint height){
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		
		const float ratio = 160.0/144;
		int wprime = width;
		int hprime = height;

		if(width > height){
			wprime = height * ratio; // Adjust width for desired aspect ratio
			glPointSize(hprime/144.0); // Set chunky pixel size
		}
		else{
			hprime = width / ratio;
			glPointSize(wprime/160.0); // Set chunky pixel size
		}

		MAIN_WINDOW->setWidth(wprime);
		MAIN_WINDOW->setHeight(hprime);

		glViewport(0, 0, wprime, hprime); // Update the viewport
		gluOrtho2D(0, 160, 144, 0);
		glMatrixMode(GL_MODELVIEW);

		glutPostRedisplay();
	}

	void idleScene(void){
	}

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

	void Window::setDrawColor(const ColorRGB &color, const float &alpha/*=1*/){
		glColor3f(color.r, color.g, color.b);
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

	void Window::drawPolygon(const std::vector<vector3> &vertices){
		glBegin(GL_POLYGON);
			for(auto vertex=vertices.begin(); vertex != vertices.end(); vertex++)
				addVertex(vertex->x, vertex->y);
		glEnd();
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

		// Set the display function (does nothing)
	    glutDisplayFunc(display);

		// Set the idle function (does nothing)
		glutIdleFunc(idleScene);
		
		init = true;
	}
	
	void Window::addVertex(const int &x, const int &y){
		// Convert from screen space to world space
		glVertex2i(x, y);
	}
#endif
