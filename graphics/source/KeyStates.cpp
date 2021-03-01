#include <GLFW/glfw3.h>

#include "KeyStates.hpp"
#include "GraphicsOpenGL.hpp"

void handleKeys(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_UNKNOWN)
		return;
	Window* currentWindow = ActiveWindows::get().find(window);
	if(!currentWindow)
		return;
	switch (action) {
	case GLFW_PRESS:
		if (key == GLFW_KEY_ESCAPE) {
			currentWindow->close();
			return;
		}
		currentWindow->getKeypress()->keyDown(key, mods);
		break;
	case GLFW_RELEASE:
		currentWindow->getKeypress()->keyUp(key, mods);
		break;
	case GLFW_REPEAT:
		break; // Ignore key repeats
	default:
		break;
	};
}

KeyStates::KeyStates() : 
	nCount(0), 
	bStreamMode(false),
	bLeftShift(false),
	bLeftCtrl(false),
	bLeftAlt(false),
	bRightShift(false),
	bRightCtrl(false),
	bRightAlt(false)
{
	reset();
}

void KeyStates::enableStreamMode(){
	bStreamMode = true;
	reset();
}

void KeyStates::disableStreamMode(){
	bStreamMode = false;
	reset();
}

bool KeyStates::poll(const unsigned char &key){ 
	if(states[key]){
		states[key] = false;
		return true;
	}
	return false;
}

void KeyStates::keyDown(const int &key, const int& mods){
	// Check for key modifiers (e.g. shift, ctrl, etc)
	checkKeyMods(mods);

	// Even if 'key' is an ascii character, alpha-numeric characters may not necessarily be in order
	// on every platform. So we need to explicitly convert the key ID to a corresponding character.
	unsigned char ckey = convertKey(key, true);
	if (!ckey) // Key has no corresponding ascii character
		return;

	if (!bStreamMode) {
		if (!states[ckey]) {
			states[ckey] = true;
			nCount++;
		}
	}
	else {
		buffer.push((char)(ckey & 0x7f));
	}
}

void KeyStates::keyUp(const int &key, const int& mods){
	if (bStreamMode)
		return;

	// Check for key modifiers (e.g. shift, ctrl, etc)
	checkKeyMods(mods);

	// Even if 'key' is an ascii character, alpha-numeric characters may not necessarily be in order
	// on every platform. So we need to explicitly convert the key ID to a corresponding character.
	unsigned char ckey = convertKey(key, false);
	if (!ckey) // Key has no corresponding ascii character
		return;

	if (states[ckey]) {
		states[ckey] = false;
		nCount--;
	}
}

void KeyStates::checkKeyMods(const int& mods) {
	// Check key modifier
#ifdef GLFW_MOD_CAPS_LOCK
	bShiftKey = ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT) || ((mods & GLFW_MOD_CAPS_LOCK) == GLFW_MOD_CAPS_LOCK);
#else
	bShiftKey = ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT) || ((mods & GLFW_KEY_CAPS_LOCK) == GLFW_KEY_CAPS_LOCK);
#endif
	bCtrlKey = (mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;
	bAltKey = (mods & GLFW_MOD_ALT) == GLFW_MOD_ALT;
}

unsigned char KeyStates::convertKey(const int &key, bool bKeyDown){
	unsigned char ckey = 0x0;
	switch (key) {
	case GLFW_KEY_0:
		ckey = (!bShiftKey ? 0x30 : 0x29);
		break;
	case GLFW_KEY_1:
		ckey = (!bShiftKey ? 0x31 : 0x21);
		break;
	case GLFW_KEY_2:
		ckey = (!bShiftKey ? 0x32 : 0x40);
		break;
	case GLFW_KEY_3:
		ckey = (!bShiftKey ? 0x33 : 0x23);
		break;
	case GLFW_KEY_4:
		ckey = (!bShiftKey ? 0x34 : 0x24);
		break;
	case GLFW_KEY_5:
		ckey = (!bShiftKey ? 0x35 : 0x25);
		break;
	case GLFW_KEY_6:
		ckey = (!bShiftKey ? 0x36 : 0x5e);
		break;
	case GLFW_KEY_7:
		ckey = (!bShiftKey ? 0x37 : 0x26);
		break;
	case GLFW_KEY_8:
		ckey = (!bShiftKey ? 0x38 : 0x2a);
		break;
	case GLFW_KEY_9:
		ckey = (!bShiftKey ? 0x39 : 0x28);
		break;
	case GLFW_KEY_A:
		ckey = (!bShiftKey ? 0x61 : 0x41);
		break;
	case GLFW_KEY_B:
		ckey = (!bShiftKey ? 0x62 : 0x42);
		break;
	case GLFW_KEY_C:
		ckey = (!bShiftKey ? 0x63 : 0x43);
		break;
	case GLFW_KEY_D:
		ckey = (!bShiftKey ? 0x64 : 0x44);
		break;
	case GLFW_KEY_E:
		ckey = (!bShiftKey ? 0x65 : 0x45);
		break;
	case GLFW_KEY_F:
		ckey = (!bShiftKey ? 0x66 : 0x46);
		break;
	case GLFW_KEY_G:
		ckey = (!bShiftKey ? 0x67 : 0x47);
		break;
	case GLFW_KEY_H:
		ckey = (!bShiftKey ? 0x68 : 0x48);
		break;
	case GLFW_KEY_I:
		ckey = (!bShiftKey ? 0x69 : 0x49);
		break;
	case GLFW_KEY_J:
		ckey = (!bShiftKey ? 0x6a : 0x4a);
		break;
	case GLFW_KEY_K:
		ckey = (!bShiftKey ? 0x6b : 0x4b);
		break;
	case GLFW_KEY_L:
		ckey = (!bShiftKey ? 0x6c : 0x4c);
		break;
	case GLFW_KEY_M:
		ckey = (!bShiftKey ? 0x6d : 0x4d);
		break;
	case GLFW_KEY_N:
		ckey = (!bShiftKey ? 0x6e : 0x4e);
		break;
	case GLFW_KEY_O:
		ckey = (!bShiftKey ? 0x6f : 0x4f);
		break;
	case GLFW_KEY_P:
		ckey = (!bShiftKey ? 0x70 : 0x50);
		break;
	case GLFW_KEY_Q:
		ckey = (!bShiftKey ? 0x71 : 0x51);
		break;
	case GLFW_KEY_R:
		ckey = (!bShiftKey ? 0x72 : 0x52);
		break;
	case GLFW_KEY_S:
		ckey = (!bShiftKey ? 0x73 : 0x53);
		break;
	case GLFW_KEY_T:
		ckey = (!bShiftKey ? 0x74 : 0x54);
		break;
	case GLFW_KEY_U:
		ckey = (!bShiftKey ? 0x75 : 0x55);
		break;
	case GLFW_KEY_V:
		ckey = (!bShiftKey ? 0x76 : 0x56);
		break;
	case GLFW_KEY_W:
		ckey = (!bShiftKey ? 0x77 : 0x57);
		break;
	case GLFW_KEY_X:
		ckey = (!bShiftKey ? 0x78 : 0x58);
		break;
	case GLFW_KEY_Y:
		ckey = (!bShiftKey ? 0x79 : 0x59);
		break;
	case GLFW_KEY_Z:
		ckey = (!bShiftKey ? 0x7a : 0x5a);
		break;
	case GLFW_KEY_APOSTROPHE:
		ckey = (!bShiftKey ? 0x27 : 0x22);
		break;
	case GLFW_KEY_COMMA:
		ckey = (!bShiftKey ? 0x2c : 0x3c);
		break;
	case GLFW_KEY_MINUS:
		ckey = (!bShiftKey ? 0x2d : 0x5f);
		break;
	case GLFW_KEY_PERIOD:
		ckey = (!bShiftKey ? 0x2e : 0x3e);
		break;
	case GLFW_KEY_SLASH:
		ckey = (!bShiftKey ? 0x2f : 0x3f);
		break;
	case GLFW_KEY_SEMICOLON:
		ckey = (!bShiftKey ? 0x3b : 0x3a);
		break;
	case GLFW_KEY_EQUAL:
		ckey = (!bShiftKey ? 0x3d : 0x2b);
		break;
	case GLFW_KEY_LEFT_BRACKET:
		ckey = (!bShiftKey ? 0x5b : 0x7b);
		break;
	case GLFW_KEY_BACKSLASH:
		ckey = (!bShiftKey ? 0x5c : 0x7c);
		break;
	case GLFW_KEY_RIGHT_BRACKET:
		ckey = (!bShiftKey ? 0x5d : 0x7d);
		break;
	case GLFW_KEY_GRAVE_ACCENT:
		ckey = (!bShiftKey ? 0x60 : 0x7e);
		break;
	case GLFW_KEY_SPACE:
		ckey = 0x20;
		break;
	case GLFW_KEY_ENTER:
		ckey = 0xd;
		break;
	case GLFW_KEY_TAB:
		ckey = 0x9;
		break;
	case GLFW_KEY_BACKSPACE:
		ckey = 0x8;
		break;
	case GLFW_KEY_INSERT:
		break;
	case GLFW_KEY_DELETE:
		//ckey = 0x7f;
		break;
	case GLFW_KEY_PAGE_UP:
		break;
	case GLFW_KEY_PAGE_DOWN:
		break;
	case GLFW_KEY_HOME:
		break;
	case GLFW_KEY_END:
		break;
	case GLFW_KEY_PRINT_SCREEN:
		break;
	case GLFW_KEY_PAUSE:
		break;
	case GLFW_KEY_F1:
		ckey = 0xf1;
		break;
	case GLFW_KEY_F2:
		ckey = 0xf2;
		break;
	case GLFW_KEY_F3:
		ckey = 0xf3;
		break;
	case GLFW_KEY_F4:
		ckey = 0xf4;
		break;
	case GLFW_KEY_F5:
		ckey = 0xf5;
		break;
	case GLFW_KEY_F6:
		ckey = 0xf6;
		break;
	case GLFW_KEY_F7:
		ckey = 0xf7;
		break;
	case GLFW_KEY_F8:
		ckey = 0xf8;
		break;
	case GLFW_KEY_F9:
		ckey = 0xf9;
		break;
	case GLFW_KEY_F10:
		ckey = 0xfa;
		break;
	case GLFW_KEY_F11:
		ckey = 0xfb;
		break;
	case GLFW_KEY_F12:
		ckey = 0xfc;
		break;
	case GLFW_KEY_LEFT:
		ckey = 0x50;
		break;
	case GLFW_KEY_UP:
		ckey = 0x52;
		break;
	case GLFW_KEY_RIGHT:
		ckey = 0x4f;
		break;
	case GLFW_KEY_DOWN:
		ckey = 0x51;
		break;
	// KEY MODIFIERS //
	case GLFW_KEY_LEFT_SHIFT:
		bLeftShift = bKeyDown;
		break;
	case GLFW_KEY_RIGHT_SHIFT:
		bRightShift = bKeyDown;
		break;
	case GLFW_KEY_LEFT_CONTROL:
		bLeftCtrl = bKeyDown;
		break;
	case GLFW_KEY_RIGHT_CONTROL:
		bRightCtrl = bKeyDown;
		break;
	case GLFW_KEY_LEFT_ALT:
		bLeftAlt = bKeyDown;
		break;
	case GLFW_KEY_RIGHT_ALT:
		bRightAlt = bKeyDown;
		break;
	default:
		break;
	}
	return ckey;
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
	bLeftShift = false;
	bLeftCtrl = false;
	bLeftAlt = false;
	bRightShift = false;
	bRightCtrl = false;
	bRightAlt = false;
	nCount = 0;
}
