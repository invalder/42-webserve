#include "config_parser.hpp"

ConfigParser::ConfigParser()
{
	std::cout << "START! Default Constructor" << std::endl;
}


/**
 * @brief Construct a new Config Parser with string argument
 * 			- check if file extension is .conf
 * 			- if not, add .conf to the end of file name
 * 			- read file and save data to m_data
 * 
 * @param file file name with extension
 */
ConfigParser::ConfigParser(std::string file): m_fileName(file)
{
	std::cout << "START! String Constructor" << std::endl;

	// get extension of file from the last dot
	std::string ext = file.substr(file.find_last_of(".") + 1);

	// check if extension is .conf
	std::cout << "Check Extension" << std::endl;
	if (ext.compare("conf"))
	{
		// if not, add .conf to the end of file name
		std::cerr << "FAIL to open : no .conf" << std::endl;
		return ;
	}

	std::cout << "Open File" << std::endl;
	// open file
	std::ifstream fileIn(file.c_str(), std::ifstream::in);
	if (!fileIn.is_open())
	{
		std::cerr << "FAIL to open " << m_fileName << std::endl;
		return ;
	}

	std::cout << "Read File" << std::endl;
	// read file and save data to m_data
	std::string line;
	while (std::getline(fileIn, line))
	{
		std::cout << "Read line: " << line << std::endl;
		this->m_data += line;
		this->m_data += "\n";
		std::cout << "Current data: " << this->m_data << std::endl;
	}
	fileIn.close();
	std::cout << this->m_data << std::endl;
}

// {
// 	std::string	ending = ".conf";
// 	if (n.length() >= ending.length())
// 	{
// 		if (n.compare(n.length() - ending.length(), ending.length(), ending))
// 		{
// 			std::cerr << "FAIL to open : no .conf" << std::endl;
// 			return ;
// 		}
// 	}
// 	else
// 	{
// 		std::cerr << "FAIL to open : file name" << std::endl;
// 		return ;
// 	}
// 	std::ifstream	fileIn(n.c_str(), std::ifstream::in);
// 	if (!fileIn.is_open())
// 	{
// 		std::cerr << "FAIL to open " << m_fileName << std::endl;
// 		return ;
// 	}
// 	std::string	line;
// 	while (std::getline(fileIn, line))
// 	{
// 		m_data += line;
// 		m_data += "\n";
// 	}
// 	fileIn.close();
// 	std::cout << m_data << std::endl;
// }

ConfigParser::~ConfigParser()
{
	std::cout << "END!" << std::endl;
}

void ConfigParser::printData()
{
	std::cout << m_data << std::endl;
}
