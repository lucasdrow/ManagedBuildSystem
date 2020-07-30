//============================================================================
// Name        : ManagedBuildSystem.cpp
// Author      : Lucas Drowatzky
// Version     : v1.0
// Copyright   : Lucas Drowatzky, LWM 2020
// Description : Build C++ project in dedicated style
//============================================================================

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <fstream>

#include <boost/process.hpp>
#include <boost/filesystem.hpp>


int main(int args, char** argv)
{
	// if wrong arguments:
	if (args != 3)
	{
		std::cout << "Call program as ManagedBuildSystem.exe \"Build Name\" \"Path to ConfigFile\"" << std::endl;
		return 0;
	}

	// read all parameters from console and print!
	std::cout << "Read " << args << " Parameters: \n\t" << "Build Config Name: " << argv[1] << "\t\nPath to config file: " << argv[2] << "\n\n\n";

	std::string buildName 	= argv[1];					// build name (filename of the executable/library/...)
	std::string filePath	= argv[2];					// file path to the config file where to find the build configuration


	// Start parsing the config file
	int 	lineSize 				= 10000;			// max characters per line
	char 	lineStorage[lineSize];						// variable to store parsed line from file

	std::ifstream infile;								// handle for file
	infile.open(filePath,std::ios::in);					// open file

	// common options
	std::string delimiter_comment 						= "#";

	// compiler options
	std::string delimiter_include_start 				= "<include>";
	std::string delimiter_include_stop 					= "</include>";

	std::string delimiter_compile_option_start 			= "<comp_opt>";
	std::string delimiter_compile_option_stop 			= "</comp_opt>";

	std::string delimiter_inputfiles_start 				= "<input_filepath>";
	std::string delimiter_inputfiles_stop 				= "</input_filepath>";

	// linker options
	std::string	delimiter_linker_libPath_start 			= "<libPath>";
	std::string	delimiter_linker_libPath_stop 			= "</libPath>";

	std::string	delimiter_linker_libs_start 			= "<lib>";
	std::string	delimiter_linker_libs_stop 				= "</lib>";

	std::string	delimiter_linker_linker_option_start 	= "<link_opt>";
	std::string	delimiter_linker_linker_option_stop 	= "</link_opt>";

	// arrays to store parsed information
	std::string* includeDirectives 	= new std::string[1000];	// contains paths to include folders
	int includeDirectivesCount 		= 0;						// keeps current count of include directives
	std::string* compilerOptions 	= new std::string[1000];	// contains all compiler options
	int compilerOptionsCount 		= 0;						// keeps current count of compiler options
	std::string* inputFilePaths		= new std::string[1000];	// contains all necessary input file paths
	int inputFilePathsCount 		= 0;						// keeps current count of input file paths
	std::string* inputFileNames		= new std::string[1000];	// contains all filenames (necessary for object-file generation and linking)

	std::string* linkerLibPaths		= new std::string[1000];	// contains all library search paths
	int linkerLibPathsCount 		= 0;						// keeps current count of library search paths
	std::string* linkerLibs			= new std::string[1000]; 	// contains all libs necessary to build project
	int linkerLibsCount 			= 0;						// keeps current count of libs
	std::string* linkerOptions		= new std::string[1000];	// contains all linker options
	int linkerOptionsCount 			= 0;						// keeps current linker options count

	//  start config file parsing
	while(infile.getline(lineStorage,lineSize))
	{
		// prepare line content (char array for key-parsing)
		std::stringstream line;
		line << lineStorage;
		std::string parseLine = line.str();

		// find comment line:
		int pos = parseLine.find(delimiter_comment);
		if (pos == 0)
		{
			// comment line found
			std::cout << "Ignoring comment line: " << lineStorage << std::endl;
			continue;		// ignore comment lines
		}

		// find include directives:
		pos = parseLine.find(delimiter_include_start);
		if (pos == 0)
		{
			// include directive found
			parseLine.erase(0,delimiter_include_start.length());
			pos = parseLine.find(delimiter_include_stop);
			includeDirectives[includeDirectivesCount] = parseLine.substr(0,pos);
			includeDirectivesCount++;
		}

		pos = parseLine.find(delimiter_compile_option_start);
		if (pos == 0)
		{
			// compiler option found
			parseLine.erase(0,delimiter_compile_option_start.length());
			pos = parseLine.find(delimiter_compile_option_stop);
			compilerOptions[compilerOptionsCount] = parseLine.substr(0,pos);
			compilerOptionsCount++;
		}

		pos = parseLine.find(delimiter_inputfiles_start);
		if (pos == 0)
		{
			// input file found
			parseLine.erase(0,delimiter_inputfiles_start.length());
			pos = parseLine.find(delimiter_inputfiles_stop);
			inputFilePaths[inputFilePathsCount] = parseLine.substr(0,pos);
			inputFilePathsCount++;
		}

		pos = parseLine.find(delimiter_linker_libPath_start);
		if (pos == 0)
		{
			// library path for linker found
			parseLine.erase(0,delimiter_linker_libPath_start.length());
			pos = parseLine.find(delimiter_linker_libPath_stop);
			linkerLibPaths[linkerLibPathsCount] = parseLine.substr(0,pos);
			linkerLibPathsCount++;
		}

		pos = parseLine.find(delimiter_linker_libs_start);
		if (pos == 0)
		{
			// library directive for linker found
			parseLine.erase(0,delimiter_linker_libs_start.length());
			pos = parseLine.find(delimiter_linker_libs_stop);
			linkerLibs[linkerLibsCount] = parseLine.substr(0,pos);
			linkerLibsCount++;
		}

		pos = parseLine.find(delimiter_linker_linker_option_start);
		if (pos == 0)
		{
			// library directive for linker found
			parseLine.erase(0,delimiter_linker_linker_option_start.length());
			pos = parseLine.find(delimiter_linker_linker_option_stop);
			linkerOptions[linkerOptionsCount] = parseLine.substr(0,pos);
			linkerOptionsCount++;
		}
	}
	infile.close();


	// prepare file names for object file generation from file paths
	for (int i = 0; i<inputFilePathsCount;i++)
	{
		std::string currentInputFilePath = inputFilePaths[i];

		unsigned int pos = currentInputFilePath.find_last_of("/\\");
		std::string fileRaw = currentInputFilePath.substr(pos+1);

		std::string cpp_delimiter 	= ".cpp";
		std::string c_delimiter 	= ".c";

		pos = fileRaw.find(cpp_delimiter);

		if (pos >= fileRaw.length())
		{
			pos = fileRaw.find(c_delimiter);
		}
		inputFileNames[i] = fileRaw.substr(0,pos);
	}

	// Summary
	std::cout << "\tSummary\n";


	std::cout << "\t\tLinker Directives: " << std::endl;
	for (int i = 0; i<includeDirectivesCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << includeDirectives[i] << std::endl;
	}

	std::cout << "\t\tCompiler Options: " << std::endl;
	for (int i = 0; i<compilerOptionsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << compilerOptions[i] << std::endl;
	}

	std::cout << "\t\tInput File Paths: " << std::endl;
	for (int i = 0; i<inputFilePathsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << inputFilePaths[i] << std::endl;
	}

	std::cout << "\t\tLinker Library Search Paths: " << std::endl;
	for (int i = 0; i<linkerLibPathsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << linkerLibPaths[i] << std::endl;
	}

	std::cout << "\t\tLinker Libraries: " << std::endl;
	for (int i = 0; i<linkerLibsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << linkerLibs[i] << std::endl;
	}

	std::cout << "\t\tLinker Options: " << std::endl;
	for (int i = 0; i<linkerOptionsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << linkerOptions[i] << std::endl;
	}

	std::cout << "\t\tInput File Names: " << std::endl;
	for (int i = 0; i<inputFilePathsCount;i++)
	{
		std::cout << "\t\t\t" << i << "\t" << inputFileNames[i] << std::endl;
	}


	// prepare statements for compiling (static contents like include derivatives and compiler options)
	std::stringstream compilerHeader; 	// contains all includes, options and -o flag

	compilerHeader << "g++ ";

	// concatenate all include directives
	for (int i = 0; i<includeDirectivesCount;i++)
	{
		compilerHeader << "\"-I" << includeDirectives[i] << "\"" << " ";
	}

	// concatenate all compiler options
	for (int i = 0; i<compilerOptionsCount;i++)
	{
		compilerHeader << compilerOptions[i] << " ";
	}

	compilerHeader << "-o" << " ";

	std::string compilerHeader_str = compilerHeader.str();

	// compile all code files (input files *.cpp / *.c)
	for (int i = 0; i<inputFilePathsCount; i++)
	{
		std::stringstream compileCommand;

		compileCommand << compilerHeader_str << "\"" << inputFileNames[i] << ".o" << "\"" << " " << "\"" << inputFilePaths[i] << "\"";

		std::cout << std::endl << std::endl << compileCommand.str() << std::endl << std::endl;
		int result = boost::process::system(compileCommand.str().c_str(), boost::process::std_out > stdout, boost::process::std_err > stderr, boost::process::std_in < stdin);
		std::cout << result << std::endl;
	}


	// prepare statements for linking (static contents like include derivatives and compiler options)
	std::stringstream linkerHeader; 	// contains all Library search paths, libraries etc.

	linkerHeader << "g++ ";

	// concatenate all include directives
	for (int i = 0; i<linkerLibPathsCount;i++)
	{
		linkerHeader << "\"-L" << linkerLibPaths[i] << "\"" << " ";
	}

	linkerHeader << "-o " << buildName << ".exe ";

	for (int i = 0; i<inputFilePathsCount; i++)
	{
		linkerHeader << "\"" << inputFileNames[i] << ".o" << "\" ";
	}

	//linkerHeader << "-pthread ";
	for (int i = 0; i < linkerLibsCount; i++)
	{
		linkerHeader << "-l" << linkerLibs[i] << " ";
	}

	for (int i = 0; i < linkerOptionsCount; i++)
	{
		linkerHeader << linkerOptions[i];
	}

	std::cout << std::endl << std::endl << linkerHeader.str() << std::endl << std::endl;

	int result = boost::process::system(linkerHeader.str().c_str(), boost::process::std_out > stdout, boost::process::std_err > stderr, boost::process::std_in < stdin);
	std::cout << result << std::endl;

	return 0;
}
