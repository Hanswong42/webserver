#ifndef RESPONSE_HPP
#define RESPONSE_HPP
#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../request/Request.hpp"
#include "../../server/ServerInfo.hpp"
#include "../../../../includes/colour.h"

using std::string;

enum {
	OK = 200,
	CREATED = 201,
	ACCEPTED = 202,
	NO_CONTENT = 204,
	METHOD_NOT_ALLOWED = 405,
	NOT_FOUND = 404,
	PAYLOAD_TOO_LARGE = 413,
	MOVE_PERMANENTLY = 301,
	TEMPORARY_REDIRECT = 307,
	PERMANENTLY_REDIRECT = 308,
	INTERNAL_SERVER_ERROR = 500

};
namespace ft
{
	class Response{
		public:
			Response(ft::ServerInfo* info);
			~Response();

			void	insertResponse(string infile);
			void	parseResponse(ft::Request *request);
			void	methodGet(ft::Request *request);
			void	methodPost(ft::Request *request);
			void	methodDelete(ft::Request *request);
			void	returnResponse(int fd);
			string	responseHeader(int status_code);
			string 	fileOpen(ft::Request *request);
			bool	empty();

		private:
			ft::ServerInfo	*info;
			string			_response;
			int				size;
			int				status_code;
			string			root;
			vector<string>	index;
			bool			auto_index;
			
			void	PrefererentialPrefixMatch(ft::Request *request);
			vector<string>	errorPage(void);
	};
} // namespace ft

#endif
