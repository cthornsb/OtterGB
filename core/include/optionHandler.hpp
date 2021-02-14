#ifndef OPTIONHANDLER_HPP
#define OPTIONHANDLER_HPP

#include <string>
#include <vector>
#include <getopt.h>

/*! \class optionExt
 *  \brief Provides an extension to the option struct expected by <a href="http://man7.org/linux/man-pages/man3/getopt.3.html">getopt</a>
 *  \author Cory R. Thornsberry (cthornsb@vols.utk.edu)
 *  \date May 31, 2019
*/

class optionExt{
  public:
	const char* name; ///< String containing the long name of the option
	int has_arg; ///< Flag indicating whether an argument is required or not
	int *flag; ///< Flag indicating how getopt will return results for long options (typically NULL)
	int val; ///< Short option character
  
  	std::string argstr; ///< The argument syntax for the command line option
	std::string helpstr; ///< The help & syntax string to print when -h is passed
	std::string argument; ///< The argument received from getopt_long (if available)
	bool active; ///< Set to true if this option was selected by the user

	/** Default constructor
	  */
	optionExt() : has_arg(0), flag(0), val(0), active(false) { }

	/** Option constructor (see <a href="http://man7.org/linux/man-pages/man3/getopt.3.html">getopt</a> documentation for more info)
	  * @param name_ Name of the long option (e.g. --name_)
	  * @param has_arg_ Flag indicating whether an argument is required or not
	  * @param flag_ Flag indicating how getopt will return results for long options (typically NULL)
	  * @param val_ Short option character (e.g. -h) or NULL if one is not required
	  * @param arg_ String containing argument syntax or empty if not required
	  * @param help_ String containing help message
	  */	
	optionExt(const char *name_, const int &has_arg_, int *flag_, const int &val_, const std::string &arg_, const std::string &help_);
	
	/** Print a formatted help string to stdout
	  */
	void print(const size_t &len_=0, const std::string &prefix_="");

	/** Return an option struct which is correctly formatted for <a href="http://man7.org/linux/man-pages/man3/getopt.3.html">getopt</a>
	  */	
	option getOption();
};

/*! \class optionHandler
 *  \brief Handles command line options added by the user
 *  \author Cory R. Thornsberry (cthornsb@vols.utk.edu)
 *  \date May 31, 2019
*/

class optionHandler{
  public:
	/** Default constructor
	  */
	optionHandler();
	
	/** Destructor
	  */
	~optionHandler(){ }

	/** SyntaxStr is used to print a linux style usage message to the screen. Prints a standard usage message by default
	  * @param  name_ The name of the program
	  */
	void syntaxStr(char *name_);

	/** Print a command line argument help dialogue
	  * @param  name_ The name of the program
	  */  
	void help(char *name_);
	
	/** Add a command line option to the option list
	  * @param  opt_ The option to add to the list
	  */
	void add(optionExt opt_);

	/** Setup user options and initialize all required objects
	  * @param  argc Number of arguments passed from the command line
	  * @param  argv Array of strings passed as arguments from the command line
	  * @return True if only user defined options were passed from the command line and return false otherwise
	  */
	bool setup(int argc, char *argv[]);

	/** Get a pointer to an option in the vector of user defined options
	  * @param index_ The index of the requested user defined option
	  * @return A pointer to the option at the requested index if it exists and return NULL otherwise
	  */
	optionExt *getOption(const size_t &index_);

	/** Return true if user input was read successfully and return false otherwise.
	  */
	bool good() const { return bGood; }

  private:
  	bool bGood; ///< Set to true if user input read successfully
  	
	std::vector<option> longOpts; ///< Vector of all command line options
	std::vector<optionExt> baseOpts; ///< Default command line options
	std::vector<optionExt> userOpts; ///< User defined command line options
	std::string optstr; ///< String of valid command line option flags
};

/** Add a command line option to the option list
  * @param opt_ The option to add to the list
  */
void addOption(optionExt opt_, std::vector<optionExt> &vec, std::string &optstr);

#endif

