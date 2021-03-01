#ifndef KEY_STATES_HPP
#define KEY_STATES_HPP

#include <queue>

class GLFWwindow;

/** Handle OpenGL keyboard presses
  * @param window Pointer to the GLFW window in which keyboard event occured
  * @param key The keyboard character which was pressed
  * @param scancode Platform dependent key scancode
  * @param action Keyboard event type (GLFW_PRESS GLFW_RELEASE GLFW_REPEAT)
  * @param mods Specifies whether or not lock keys are active (caps lock, num lock)
  */
void handleKeys(GLFWwindow* window, int key, int scancode, int action, int mods);

class KeyStates{
public:
	/** Default constructor
	  */
	KeyStates();
	
	/** Enable text stream mode
	  */
	void enableStreamMode();
	
	/** Disable text stream mode and return to keypress mode
	  */
	void disableStreamMode();
	
	/** Return true if no keys are currently pressed
	  */
	bool empty() const { 
		return (nCount == 0); 
	}
	
	/** Check the current state of a key without modifying it
	  */
	bool check(const unsigned char &key) const { 
		return states[key]; 
	}

	/** Return true if left shift is currently pressed
	  */	
	bool leftShift() const {
		return bLeftShift;
	}

	/** Return true if right shift is currently pressed
	  */	
	bool rightShift() const {
		return bLeftShift;
	}

	/** Return true if left ctrl is currently pressed
	  */	
	bool leftCtrl() const {
		return bLeftShift;
	}

	/** Return true if right ctrl is currently pressed
	  */	
	bool rightCtrl() const {
		return bLeftShift;
	}

	/** Return true if left alt is currently pressed
	  */	
	bool leftAlt() const {
		return bLeftShift;
	}
	
	/** Return true if right alt is currently pressed
	  */
	bool rightAlt() const {
		return bLeftShift;
	}
	
	/** Poll the state of a key
	  * If the state of the key is currently pressed, mark it as released. 
	  * This method is useful for making the key sensitive to being pressed, but not to being held down.
	  */
	bool poll(const unsigned char &key);
	
	/** Press a key
	  * If bStreamMode is set, key is added to the stream buffer.
	  */
	void keyDown(const int& key, const int& mods);
	
	/** Release a key
	  */
	void keyUp(const int& key, const int& mods);

	/** Get a character from the stream buffer
	  * If the stream buffer is empty, return false.
	  */
	bool get(char& key);

	/** Reset the stream buffer and all key states
	  */
	void reset();

private:
	unsigned short nCount; ///< Number of standard keyboard keys which are currently pressed

	bool bStreamMode; ///< Flag to set keyboard to behave as stream buffer

	bool bShiftKey;

	bool bCtrlKey;

	bool bAltKey;

	bool bLeftShift; ///< Set if left shift is pressed
	
	bool bLeftCtrl; ///< Set if left ctrl is pressed
	
	bool bLeftAlt; ///< Set if left alt is pressed
	
	bool bRightShift; ///< Set if right shift is pressed
	
	bool bRightCtrl; ///< Set if right ctrl is pressed
	
	bool bRightAlt; ///< Set if right alt is pressed

	std::queue<char> buffer; ///< Text stream buffer

	bool states[256]; ///< States of keyboard keys (true indicates key is down) 

	void checkKeyMods(const int& mods);

	/** Press or release a special key (e.g. function key, shift, etc)
	  */
	unsigned char convertKey(const int& key, bool bKeyDown);
};

#endif

