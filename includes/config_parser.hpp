#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <fstream>

class ConfigParser
{
	private:
		std::string	m_fileName;
		std::string	m_data;

	public:
		ConfigParser();
		ConfigParser(std::string );
		ConfigParser(ConfigParser const &);
		~ConfigParser();
		ConfigParser	&operator=(ConfigParser const &);

		void	setFileName(std::string const &);
		void 	printData();

		


};



#endif
