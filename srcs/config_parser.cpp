#include "config_parser.hpp"

ConfigParser::ConfigParser(std::string n)
{
	std::string	ending = ".conf";
	if (n.length() >= ending.length())
	{
		if (n.compare(n.length() - ending.length(), ending.length(), ending))
		{
			std::cerr << "FAIL to open : no .conf" << std::endl;
			return ;
		}
	}
	else
	{
		std::cerr << "FAIL to open : file name" << std::endl;
		return ;
	}
	std::ifstream	fileIn(n.c_str(), std::ifstream::in);
	if (!fileIn.is_open())
	{
		std::cerr << "FAIL to open " << m_fileName << std::endl;
		return ;
	}
	std::string	line;
	while (std::getline(fileIn, line))
	{
		m_data += line;
		m_data += "\n";
	}
	fileIn.close();
	std::cout << m_data << std::endl;
}

ConfigParser::~ConfigParser()
{
	std::cout << "END!" << std::endl;
}
