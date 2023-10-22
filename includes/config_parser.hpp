#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <fstream>

class ConfigParser
{
	private:
		std::string	m_fileName;
		std::string	m_data;


		ConfigParser(ConfigParser const &);

		ConfigParser	&operator=(ConfigParser const &);
	public:
		ConfigParser();
		ConfigParser(std::string );
		~ConfigParser();

		void	setFileName(std::string const &);

		


};



#endif
