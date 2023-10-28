#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <exception>

class ConfigHandler
{
	private:
		std::string _data;

		/**
		 * @brief map to save config data
		 * 	- key : server port
		 * 	- value : server config
		 */
		std::map<std::string, std::string> _configMap;

		void			_initializedConfigDataMap( std::ifstream & );
		std::ifstream	_getFileStream( std::string );

	public:
		ConfigHandler();
		ConfigHandler( std::string );
		~ConfigHandler();
		ConfigHandler(const ConfigHandler &);
		ConfigHandler &operator=(const ConfigHandler &);

		void	printData();

	// exceptions
	// file open exception
	class FileOpenException : public std::exception
	{
		public:
			virtual const char* what() const throw();
	};

	// incorrect extension exception
	class IncorrectExtensionException : public std::exception
	{
		public:
			virtual const char* what() const throw();
	};
};

#endif
