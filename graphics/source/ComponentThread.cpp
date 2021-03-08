#include <chrono>

#include "ComponentThread.hpp"


void Conditional::wait(){
	condition->wait(uniqueLock);
}

/////////////////////////////////////////////////////////////////////
// class ThreadObject
/////////////////////////////////////////////////////////////////////

ThreadObject::ThreadObject() :
	bUserQuitting(false),
	parent(0x0),
	variables()
{
}

ThreadObject::ThreadObject(ThreadManager* manager) :
	bUserQuitting(false),
	parent(manager),
	variables()
{
	setThreadManager(manager);
}

void ThreadObject::sleep(const unsigned int& ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void ThreadObject::sleepMicro(const unsigned int& us) {
	std::this_thread::sleep_for(std::chrono::microseconds(us));
}

std::thread::id ThreadObject::getThreadID() {
	return std::this_thread::get_id();
}

void ThreadObject::setThreadManager(ThreadManager* manager){
	parent = manager;
	parent->getConditionVariables(variables);
}

void ThreadObject::sync(const unsigned short& which/*=0*/) {
	if (locked(which)) // Only wait if this thread should wait on the specified variable
		variables[which].wait();
}

/////////////////////////////////////////////////////////////////////
// class WorkerThread
/////////////////////////////////////////////////////////////////////

WorkerThread::WorkerThread() :
	ThreadObject()
{
}

WorkerThread::WorkerThread(ThreadManager* manager) :
	ThreadObject(manager)
{
}

bool WorkerThread::execute() {
	while (!bUserQuitting) {
		this->sync();
		this->mainLoop();
	}
	return true;
}

/////////////////////////////////////////////////////////////////////
// class ThreadManager
/////////////////////////////////////////////////////////////////////

void ThreadManager::getConditionVariables(Conditional* arr){
	for (int i = 0; i < NUMBER_VARIABLES; i++)
		arr[i].setConditionalVariable(&variables[i]);
}

void ThreadManager::addThread(ThreadObject* proc) {
	threadList.push_back(proc);
	proc->setThreadManager(this);
}

void ThreadManager::lock(const unsigned short& which/*=0*/) {
	for (auto& th : threadList)
		th->lock(which);
}

void ThreadManager::unlock(const unsigned short& which/*=0*/) {
	for (auto& th : threadList)
		th->unlock(which);
}

void ThreadManager::notify(const unsigned short& which/*=0*/) {
	variables[which].notify_all();
}

void ThreadManager::quit() {
	for (auto& th : threadList)
		th->quit();
	for (int i = 0; i < NUMBER_VARIABLES; i++)
		variables[i].notify_all();
}
