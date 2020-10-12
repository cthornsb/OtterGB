#include <iostream>
#include <sstream>
#include <string.h>

#include "optionHandler.hpp"

/////////////////////////////////////////////////////////////////////
// class optionExt
/////////////////////////////////////////////////////////////////////

optionExt::optionExt(const char *name_, const int &has_arg_, int *flag_, const int &val_, const std::string &argstr_, const std::string &helpstr_) : 
  name(name_), has_arg(has_arg_), flag(flag_), val(val_), argstr(argstr_), helpstr(helpstr_), active(false) {
}

void optionExt::print(const size_t &len_/*=0*/, const std::string &prefix_/*=""*/){
	std::stringstream stream;
	stream << prefix_ << "--" << name << " ";
	if(val) stream << "(-" << (char)val << ") ";
	stream << argstr;
	
	if(stream.str().length() < len_)
		stream << std::string(len_ - stream.str().length(), ' ');
	
	stream << "- " << helpstr;
	std::cout << stream.str() << std::endl;
}

option optionExt::getOption(){
	struct option output;
	output.name = name;
	output.has_arg = has_arg;
	output.flag = flag;
	output.val = val;
	return output;
}

/////////////////////////////////////////////////////////////////////
// class optionHandler
/////////////////////////////////////////////////////////////////////

optionHandler::optionHandler(){
	baseOpts.push_back(optionExt("help", no_argument, NULL, 'h', "", "Display this dialogue"));
	
	optstr = "h";
}

/** SyntaxStr is used to print a linux style usage message to the screen.
  * Prints a standard usage message by default.
  * \param[in]  name_ The name of the program.
  * \return Nothing.
  */
void optionHandler::syntaxStr(char *name_){
	std::cout << " usage: " << name_ << " [options]\n";
}

/** Print a command line argument help dialogue.
  * \param[in]  name_ The name of the program.
  * \return Nothing.
  */  
void optionHandler::help(char *name_){
	syntaxStr(name_);
	std::cout << "  Available options:\n";
	for(std::vector<optionExt>::iterator iter = baseOpts.begin(); iter != baseOpts.end(); iter++){
		if(!iter->name) continue;
		iter->print(40, "   ");
	}
	for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
		if(!iter->name) continue;
		iter->print(40, "   ");
	}
}

/** Add a command line option to the option list.
  * \param[in]  opt_ The option to add to the list.
  * \return Nothing.
  */
void optionHandler::add(optionExt opt_){
	addOption(opt_, userOpts, optstr);
}

/** Setup user options and initialize all required objects.
  * \param[in]  argc Number of arguments passed from the command line.
  * \param[in]  argv Array of strings passed as arguments from the command line.
  * \return True upon success and false otherwise.
  */
bool optionHandler::setup(int argc, char *argv[]){
	// Build the vector of all command line options.
	for(std::vector<optionExt>::iterator iter = baseOpts.begin(); iter != baseOpts.end(); iter++){
		longOpts.push_back(iter->getOption()); 
	}
	for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
		longOpts.push_back(iter->getOption()); 
	}

	// Append all zeros onto the option list. Required for getopt_long.
	struct option zero_opt { 0, 0, 0, 0 };
	longOpts.push_back(zero_opt);

	int idx = 0;
	int retval = 0;

	//getopt_long is not POSIX compliant. It is provided by GNU. This may mean
	//that we are not compatable with some systems. If we have enough
	//complaints we can either change it to getopt, or implement our own class. 
	while ( (retval = getopt_long(argc, argv, optstr.c_str(), longOpts.data(), &idx)) != -1) {
		if(retval == 0x0){ // Long option
			for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
				if(strcmp(iter->name, longOpts[idx].name) == 0){
					iter->active = true;
					if(optarg)
						iter->argument = std::string(optarg);
					break;
				}
			}
		}
		else if(retval == 0x3F){ // Unknown option, '?'
			return false;
		}
		else{ // Single character option.
			switch(retval) {
				case 'h' :
					help(argv[0]);
					return false;
				default:
					for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
						if(retval == iter->val){
							iter->active = true;
							if(optarg)
								iter->argument = std::string(optarg);
							break;
						}
					}
					break;
			}
		}
	}//while
	
	return true;
}

optionExt *optionHandler::getOption(const size_t &index_){
	if(index_ < userOpts.size()){
		return &userOpts.at(index_);
	}
	return NULL;
}

/** Add a command line option to the option list.
  * \param[in]  opt_ The option to add to the list.
  * \return Nothing.
  */
void addOption(optionExt opt_, std::vector<optionExt> &vec, std::string &optstr){
	char tempChar = opt_.val;
	if(tempChar){
		if(optstr.find(tempChar) != std::string::npos)
			opt_.val = 0x0;
		else{
			optstr += tempChar;
			if(opt_.has_arg == required_argument)
				optstr += ":";
			else if(opt_.has_arg == optional_argument)
				optstr += "::";
		}
	}
	vec.push_back(opt_);
}

