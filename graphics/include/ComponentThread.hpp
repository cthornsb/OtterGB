#ifndef COMPONENT_THREAD_HPP
#define COMPONENT_THREAD_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>

constexpr int NUMBER_VARIABLES = 10;

class ThreadManager;

class Conditional{
public:
	/** Default constructor
	  */
	Conditional() :
		bLocked(false),
		condition(0x0),
		mutexLock(),
		uniqueLock(mutexLock)
	{
	}

	/** Copy of mutex disallowed
	  */
	Conditional(const Conditional&) = delete;

	/** Copy of mutex disallowed
	  */
	Conditional& operator = (const Conditional&) = delete;

	Conditional(std::condition_variable* variable) :
		bLocked(false),
		condition(variable),
		mutexLock(),
		uniqueLock(mutexLock)
	{
	}
	
	void setConditionalVariable(std::condition_variable* variable){
		condition = variable;
	}
	
	bool locked() const {
		return bLocked;
	}
	
	void lock(){
		bLocked = true;
	}
	
	void unlock(){
		bLocked = false;
	}
	
	void wait();

private:
	bool bLocked; ///< Set if conditional variable is in use

	std::condition_variable* condition; ///< Pointer to conditional variable

	std::mutex mutexLock; ///< Mutex lock

	std::unique_lock<std::mutex> uniqueLock; ///< Unique lock
};

class ThreadObject {
public:
	/** Default constructor
	  */
	ThreadObject();

	/** Thread manager constructor
	  */
	ThreadObject(ThreadManager* manager);

	/** Quit main loop
	  */
	void quit() {
		bUserQuitting = true;
	}

	/** Return true if thread will wait for conditional variable to unlock when sync() is called
	  */
	bool locked(const unsigned short& which=0) const {
		return variables[which].locked();
	}

	/** Set thread to wait for conditional variable to unlock when sync() is called
	  */
	void lock(const unsigned short& which=0) {
		variables[which].lock();
	}

	/** Set thread to not wait for conditional variable when sync() is called
	  */
	void unlock(const unsigned short& which=0) {
		variables[which].unlock();
	}

	/** Sleep this thread for at least ms milliseconds
	  */
	void sleep(const unsigned int& ms);

	/** Sleep this thread for at least us microseconds
	  */
	void sleepMicro(const unsigned int& us);

	/** Get current thread ID
	  */
	std::thread::id getThreadID();

	/** Set pointer to thread manager
	  */
	void setThreadManager(ThreadManager* manager);

	/** Execute main loop
	  */
	virtual bool execute() { 
		return true;
	}

	/** Wait for thread sync notification (if lock is enabled)
	  */
	void sync(const unsigned short& which=0);

protected:
	bool bUserQuitting; ///< Set when main loop should exit

	ThreadManager* parent; ///< Pointer to thread manager object

	Conditional variables[NUMBER_VARIABLES]; ///< Array of condition_variables (hard-coded size for simplicity)
};

class WorkerThread : public ThreadObject {
public:
	/** Default constructor
	  */
	WorkerThread();

	/** Thread manager constructor
	  */
	WorkerThread(ThreadManager* manager);

	/** Execute main loop
	  */
	bool execute() override;

protected:
	/** Main loop function called repeatedly from execute()
	  */
	virtual void mainLoop() { }
};

class ThreadManager {
public:
	/** Default constructor
	  */
	ThreadManager() :
		variables(),
		threadList()
	{
	}

	/** N variables constructor
	  */
	ThreadManager(unsigned short& N) :
		variables(),
		threadList()
	{
		//for(unsigned short i = 0; i < N; i++)
		//	variables.push_back(new std::condition_variable());
	}

	/** Get pointers to all condition_variables
	  */
	void getConditionVariables(Conditional* arr);

	/** Add a worker thread
	  */
	void addThread(ThreadObject* proc);

	/** Lock one of the condition_variables
	  */
	void lock(const unsigned short& which=0);

	/** Unlock one of the condition_variables
	  */
	void unlock(const unsigned short& which=0);

	/** Notify one of the condition_variables
	  */
	void notify(const unsigned short& which=0);

	/** Set all threads to quit and then notify all waiting threads
	  */
	void quit();

private:
	std::condition_variable variables[NUMBER_VARIABLES];

	std::vector<ThreadObject*> threadList;
};

#endif
