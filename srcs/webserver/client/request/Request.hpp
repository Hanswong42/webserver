#ifndef REQUEST_HPP
#define REQUEST_HPP
#include <iostream>
#include <sstream>
#include <map>
using std::string;

namespace ft{
	class Request{
		public:
			Request();
			~Request();
			Request(string	header);
			
			void	parseHeader();
			void	parseBody();
			void	insertHeader(const string& raw_request);
			void	insertBody(const string& raw_request);
			int	findCarriage(void) const;
			string	getTarget(void) const;
			string	getPrefix(void) const;
			string	getMethod(void);
			string	getQuery(void) const;
			string	getBody(void) const;
			int		getcontentLength(void) const;
			string getContentType() const;
			std::map<string, string>	getParams(void);
			std::pair<const string, string> getCookie(void) const;
			void	eraseBody(size_t pos, size_t size);
			int		getRawbytes();
			bool	body_end;
		private:
			string	_header;
			string	_body;
			string	method;
			string	url;
			string	prefix;
			string	version;
			string	contentType;
			string	query_string;
			string	body_string;
			size_t		content_length;
			int		raw_bytes;
			bool	chunk;
			
		
			std::map<string, string> headers;
			std::map<string, string> body;
			std::map<string, string> params;

			void	requestPrefix(void);
	};
}
#endif