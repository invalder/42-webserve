#include "config_handler.hpp"

void ConfigHandler::testPrintAll() const
{
	std::map<std::string, std::string>::const_iterator cur;
	std::map<std::string, std::string>::const_iterator end;

	std::cerr << "============== Global Config ===============" << std::endl;
	cur = _globalConfig.begin();
	end = _globalConfig.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "Global =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}

	std::cerr << "============== HTTP config ==============" << std::endl;
	cur = _httpConfig.directives.begin();
	end = _httpConfig.directives.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "HTTP =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}
	std::cerr << "=============== default err pages ===============" << std::endl;
	cur = _httpConfig.defaultErrorPages.begin();
	end = _httpConfig.defaultErrorPages.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "DEPs =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}

	std::vector<Server *>::const_iterator curServ = _httpConfig.servers.begin();

	while (curServ != _httpConfig.servers.end())
	{
		std::cerr << "============== Server config ==============" << std::endl;
		std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << "listener"
				  << " | " << (*curServ)->listener << std::endl;
		// socket address ????
		std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << "listen port"
				  << " | " << (*curServ)->listen_port << std::endl;
		// server Directive
		cur = (*curServ)->directives.begin();
		end = (*curServ)->directives.end();
		while (cur != end)
		{
			std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
			cur++;
		}
		std::vector<Location>::const_iterator curLoc = (*curServ)->locations.begin();
		std::vector<Location>::const_iterator endLoc = (*curServ)->locations.end();
		while (curLoc != endLoc)
		{
			// print content in loc
			std::cerr << "------------------ Location config ------------------" << std::endl;
			std::cerr << std::setw(10) << "LOC =" << std::setw(25) << "Location path"
					  << " | " << curLoc->path << std::endl;
			cur = curLoc->directives.begin();
			end = curLoc->directives.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC dir =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}
			// cgi
			cur = curLoc->cgi.begin();
			end = curLoc->cgi.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC cgi =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}

			// upload
			cur = curLoc->upload.begin();
			end = curLoc->upload.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC upld =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}
			curLoc++;
		}
		curServ++;
	}
	return;
}
