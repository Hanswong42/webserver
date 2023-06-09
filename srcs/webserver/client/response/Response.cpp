#include "Response.hpp"
#include "helper.hpp"

ft::Response::Response(ft::ServerInfo* info, const map<string, string>& env) : info(info), _response(), size(0), root("root"), auto_index(false), redirection(false), env(env)
{
}
ft::Response::~Response()
{
}

int ft::Response::insertResponse(string infile)
{
	this->_response = infile;
	this->size += infile.size();
	return this->size;
}

/**
 * @brief fetch and initalize info such as "root, allowed_method, index" from locationblock
 * if failed, initalized with the server block config instead
 *
 * @param prefix
 * @param value
 * @return vector<string>
 */
vector<string> ft::Response::initalizeLocationConfig(string prefix, string value)
{
	vector<string> ret;

	try
	{
		ret = info->getLocationInfo(prefix, value);
	}
	catch (const std::exception &e)
	{
		std::cerr << BLUE "[DBUG] " << value << " is not specified in [" << prefix << "] , initialized with server config" << '\n';
		try
		{
			ret = info->getConfigInfo(value);
		}
		catch (const std::exception &e)
		{
			std::cerr << RED "[DBUG] "<< value << " is not specified in server config, initialized with default config" << '\n';
		}
	}
	return ret;
}
int ft::Response::allowedMethod(ft::Request *request)
{
	vector<string> allowed_method;
	if (!prefererentialPrefixMatch(this->target).empty())
	{
		allowed_method = initalizeLocationConfig(this->prefix, "allow_methods");
		for (vector<string>::iterator it = allowed_method.begin(); it != allowed_method.end(); it++)
			if (!request->getMethod().compare(*it))
				return OK;
	}
	else
	{
		return NOT_FOUND;
	}
	return METHOD_NOT_ALLOWED;
}

bool	ft::Response::checkCookie(ft::Request *request)
{
	string cookieHash = request->getCookie().second;
	if (!cookieHash.empty())
		return (info->findCookie("Cookie", cookieHash));
	return false;
}

void ft::Response::parseResponse(ft::Request *request)
{
	this->cookie = checkCookie(request);
	this->prefix = prefererentialPrefixMatch(request->getTarget());
	this->target = pageRedirection(request->getTarget());
	this->status_code = allowedMethod(request);

	try
	{
		root = info->getConfigInfo("root").front();
	}
	catch (const std::exception &e)
	{
	}
	// std::cout << "CURRENTLY THE TARGET IS " << prefix<<  "Request method is " << request->getMethod()<< std::endl;
	if (this->status_code != OK && this->status_code != MOVED_PERMANENTLY)
		insertResponse(responseHeader(this->status_code).append(errorPage()));
	else
	{
		if (request->getMethod() == "GET")
			methodGet(request);
		else if (request->getMethod() == "POST")
			methodPost(request);
		else if (request->getMethod() == "DELETE")
			methodDelete(request);
		else
			insertResponse(responseHeader(this->status_code).append(errorPage()));
	}
}

string getStatus(int status_code)
{
	switch (status_code)
	{
	case OK:
		return " 200 Ok";
	case NOT_FOUND:
		return " 404 Not Found";
	case METHOD_NOT_ALLOWED:
		return " 405 Method Not Allowed";
	case PAYLOAD_TOO_LARGE:
		return " 413 Payload Too Large";
	case MOVED_PERMANENTLY:
		return " 301 Moved Permanently";
	case TEMPORARY_REDIRECT:
		return " 307 Temporary Redirect";
	case PERMANENTLY_REDIRECT:
		return " 308 Permanent Redirect";
	default:
		return " 500 Internal Server Error";
	}
}



char*	dynamicDup(string s)
{
	char *str = new char[s.length() + 1];
	strcpy(str, s.c_str());
	return str;
}

char**	mapToChar(map<string,string> envs)
{
	char **env = new char *[envs.size() + 1];
	int i = 0;
	for (map<string,string>::iterator it = envs.begin(); it != envs.end(); it++, i++)
		env[i] = dynamicDup(it->first + "=" + it->second);
	env[i] = NULL;
	return env;
}

// string	paramToString(ft::Request *request)
// {
// 	string query;

// 	map<string,string> params = request->getParams();

// 	std::cout << RED << params.size() << RESET << std::endl;
// 	for (map<string,string>::const_iterator it = params.begin(); it != params.end(); it++)
// 	{
// 		// std::cout << "HERE?" << it->first << it->second << std::endl;
// 		query.append(it->first + "=" + it->second);
// 		if (it++ != params.end())
// 			query.append("&");
// 	}
// 	return query;
// }

int	ft::Response::executeCGI(string prefix, ft::Request *request)
{
	string	cgipath;
	int		status;
	// std::cout << RED "HERE1" RESET << std::endl;

	std::string s;
	std::stringstream out;
	out << request->getcontentLength();
	try
	{
		cgipath = info->getLocationInfo(prefix, "fastcgi_pass").front();
	}
	catch(const std::exception& e)
	{
		this->status_code = 404;
		return(insertResponse(responseHeader(this->status_code).append(errorPage())));
	}
	this->env.insert(std::make_pair("QUERY_STRING", request->getQuery()));
	this->env.insert(std::make_pair("SIZE", out.str()));
	// this->env.insert(std::make_pair("BODY_STRING", request->getBody()));
	int readfd[2];
	int	writefd[2];
	char buf[BUFFER_SIZE];
	pipe(readfd);
	pipe(writefd);
	int	pid = fork();
	if (pid == 0)
	{
		char **envs = mapToChar(this->env);
		char **args = new char*[3];
		args[0] = dynamicDup("/usr/bin/python3");
		args[1] = dynamicDup(cgipath);
		args[2] = NULL;
	
		dup2(readfd[1], STDOUT_FILENO);
		dup2(writefd[0], STDIN_FILENO);
		close(readfd[0]);
		close(readfd[1]);
		close(writefd[0]);
		close(writefd[1]);
		chdir(pathAppend(root, prefix).c_str());
		if (execve("/usr/bin/python3", args, envs) == -1)
			exit(127);
    }
	else
	{
		// std::cout << pathAppend(root, prefix) << std::endl;

		while (!request->getBody().empty())
		{
			std::cout << request->getBody().length() << std::endl;
			if (request->getBody().length() > BUFFER_SIZE)
			{
				write(writefd[1], request->getBody().substr(0, BUFFER_SIZE).c_str(), BUFFER_SIZE);
				request->eraseBody(0, BUFFER_SIZE);
			}
			else
			{
				write(writefd[1], request->getBody().c_str(), request->getBody().size());
				request->eraseBody(0, request->getBody().size());
			}
		}
		close(writefd[1]);
		close(writefd[0]);
	
		waitpid(pid, &status, 0);
		if (WEXITSTATUS(status) != 0)
			return (insertResponse(responseHeader(NOT_FOUND).append(errorPage())));
		close(readfd[1]);
		string cgireturn;
		while(read(readfd[0], buf, BUFFER_SIZE) > 0)
			cgireturn.append(string(buf));
		close(readfd[0]);
		return (insertResponse(responseHeader(this->status_code).append(cgireturn)));
		
	}
	return (0);
	// dynamicFree(envs);
	// dynamicFree(args);

}
#include <random>

tm* _generateExpirationTime(int expireTimeSeconds)
{
    time_t curr_time;
    curr_time = time(NULL);

    time_t expiry_time_t = curr_time + expireTimeSeconds;
    tm *gmt_time = gmtime(&expiry_time_t);
	return (gmt_time);
}

std::string _generateExpirationStr(tm *expiry_time)
{
	char	expiry_buf[32];

	std::memset(expiry_buf, 0, sizeof(expiry_buf));
    strftime(expiry_buf, sizeof(expiry_buf), "%a, %d %b %Y %H:%M:%S GMT", expiry_time);
    return (std::string(expiry_buf));
}

std::string	_generateHash()
{
    std::string hash;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 35);
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < 32; i++)
	{
        hash += characters[distrib(gen)];
	}
	return (hash);
}

#include <sstream>

std::string	ft::Response::generateCookie()
{
	std::string hash;

	hash = "Cookie=" + _generateHash();
	info->insertCookie("Cookie", hash);
	return hash + "; expires=" + _generateExpirationStr(_generateExpirationTime(30)) + ";"; ;
}

string ft::Response::responseHeader(int status_code)
{
	this->status_code = status_code;
	string ret = "HTTP/1.1" + getStatus(status_code) + "\r\nContent-Type: */*\r\n";
	if (status_code != OK || cookie == true)
		return (ret + "\r\n");
	else
		return (ret + "Set-Cookie: " + generateCookie() + "\r\n\r\n");
}

/**
 * @brief open direct url first 
 * Exact match: Nginx first looks for an exact match between the request URL and a configured location block. If it finds a match, it sends the request to the corresponding backend server.

Preferential Prefix match: If no exact match is found, Nginx looks for a location block whose prefix matches the beginning of the request URL. If multiple location blocks have a matching prefix, Nginx chooses the one with the longest prefix.

Regular expression match: If no prefix match is found, Nginx looks for a location block that matches the request URL using a regular expression.

Default match: If no match is found, Nginx sends the request to the default server or displays a 404 error.
 *
 * @param request
 */

string ft::Response::prefererentialPrefixMatch(string url)
{
	if (info->getLocationCount(url))
		return url;
	else
	{
		// std::cout << " I enter here w/ url ==== " << url << std::endl;
		if (url.find_last_of('/') == string::npos)
			return ("/");
		else
			return prefererentialPrefixMatch(url.substr(0, url.find_last_of('/')));
	}
}
string ft::Response::errorPage(void)
{
	vector<string> page;
	stringstream ss;
	ss << this->status_code;

	string status = ss.str();
	std::fstream file;
	if (info->getConfigCount(status))
	{
		page = info->getConfigInfo(status);
		for (vector<string>::iterator it = page.begin(); it != page.end(); it++)
		{
			file.open(ft::pathAppend(root, *it));
			if (file.is_open())
				return (string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
		}
	}
	return defaultErrorPage();
}

string	ft::Response::pageRedirection(string target)
{
	try
	{
		if (!info->getLocationInfo(prefix, "return").empty())
		{
			// std::cout << BLUE "redirection prefix " << prefix << "redirection substr =" << target.substr(target.find(prefix) + prefix.length()) << RESET << std::endl;
			string redir = pathAppend(info->getLocationInfo(prefix, "return").front(), target.substr(target.find(prefix) + prefix.length()));
			redirection = true;
			this->status_code = MOVED_PERMANENTLY;
			std::cout << CYAN "[INFO] Redirection exist, redirecting to " << redir << RESET << std::endl;
			return redir;
		}
	}
	catch(const std::exception& e)
	{	
	}
	this->status_code = OK;
	return target;
}

string ft::Response::autoIndexGenerator(string prefix)
{
	string path = ft::pathAppend(root, prefix);
	string response;
	vector<std::pair<string, int> > dir_contents;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(path.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			
			string file_path = pathAppend(path, string(ent->d_name));
			
			struct stat sb;
			stat(file_path.c_str(), &sb);
			// std::cout << RED "this size" << ret << "this path" << file_path <<  RESET << std::endl;
			dir_contents.push_back(std::make_pair(ent->d_name, sb.st_size));
		}
		closedir(dir);
	}

	// Sort the directory contents alphabetically by name
	// sort(dir_contents.begin(), dir_contents.end());

	// Write the HTTP response

	response = "<!DOCTYPE html>\n";
	response += "<html>\n";
	response += "<head>\n";
	response += "<title>Index of " + path + "</title>\n";
	response += "</head>\n";
	response += "<body>\n";
	response += "<h1>Index of " + path + "</h1>\n";
	response += "<hr>\n";
	response += "<table>\n";
	response += "<thead>\n";
	response += "<tr>\n";
	response += "<th>Name</th>\n";
	response += "<th>Size</th>\n";
	response += "</tr>\n";
	response += "</thead>\n";
	response += "<tbody>\n";

	for (vector<std::pair<string, int> >::const_iterator it = dir_contents.begin(); it != dir_contents.end(); ++it)
	{
		response += "<tr>\n";
		response += "<td><a href=\"" + pathAppend(prefix, it->first) + "\">" + it->first + "</a></td>\n";
		response += "<td>" + std::to_string(it->second) + "</td>\n";
		response += "</tr>\n";
	}

	response += "</tbody>\n";
	response += "</table>\n";
	response += "<hr>\n";
	response += "</body>\n";
	response += "</html>\n";
	return response;
}

string ft::Response::defaultErrorPage(void)
{
	string response;
	response = "<!DOCTYPE html>\n";
	response += "<html>\n";
	response += "<head>\n";
	response += "<title>This is default error page of" + getStatus(this->status_code) + "</title>";
	response += "</head>\n";
	response += "<body>\n";
	response += "<h1>" + getStatus(this->status_code) + " </h1>\n";
	response += "</body>\n";
	response += "</html>\n";

	return response;
}

void ft::Response::methodGet(ft::Request *request)
{
	std::cout << RED "File opened at " << pathAppend(root, target) << RESET << std::endl;
	std::fstream file;
	/*exact match*/
		
	file.open(ft::pathAppend(root, target));
	// if (file.is_open())

	string prefix = prefererentialPrefixMatch(target);
	/*if failed, do prefix matching*/

	if (!file.is_open())
	{
		if (!prefix.empty())
		{
			if (!initalizeLocationConfig(prefix, "root").empty())
				this->root = initalizeLocationConfig(prefix, "root").front();
			if (!initalizeLocationConfig(prefix, "index").empty())
				this->index = initalizeLocationConfig(prefix, "index");
			if (!initalizeLocationConfig(prefix, "autoindex").empty())
			{
				if (!initalizeLocationConfig(prefix, "autoindex").front().compare("on"))
					this->auto_index = true;
			}
			if (!request->getQuery().empty())
			{
				executeCGI(prefix, request);
				return;
			}
			if (cookie == true)
				file.open(ft::pathAppend(root, prefix, index.front()));
			else
				file.open(ft::pathAppend(root, prefix, index.back()));
		}
	}
	if (!file.is_open())
	{
		if (this->auto_index == true)
			insertResponse(responseHeader(this->status_code).append(autoIndexGenerator(prefix)));
		else
			insertResponse(responseHeader(NOT_FOUND).append(errorPage()));
	}
	else
		insertResponse(responseHeader(this->status_code).append(string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>())));
}

size_t	ft::Response::maxBodySize()
{
	string size;
	try
	{
		size = info->getConfigInfo("client_max_body_size").front();
	}
	catch(const std::exception& e)
	{
		size = "0";
		std::cerr << e.what() << '\n';
	}	
	switch (size.back())
	{
	case 'K':
		return(std::atoi(size.substr(0, size.size() - 1).c_str()) * 1000);
	case 'M':
		return(std::atoi(size.substr(0, size.size() - 1).c_str()) * 1000000);
	default:
		return(std::atoi(size.c_str()));
	}
}

void ft::Response::methodPost(ft::Request *request)
{
	string target = pageRedirection(request->getTarget());
	string prefix = prefererentialPrefixMatch(target);
	
	if (!request->getBody().empty())
	{
		if (request->getBody().length() > maxBodySize() && maxBodySize() > 0)
		{
			this->status_code = PAYLOAD_TOO_LARGE;
			insertResponse(responseHeader(this->status_code).append(errorPage()));
		}
		else
			executeCGI(prefix, request);
	}
}

bool isSubdirectory(const std::string &request_path)
{
	const std::string &root = "root";
	char *root_dir = realpath(root.c_str(), NULL);

	// Get the absolute path of the request
	char *abs_path = realpath(request_path.c_str(), NULL);
	if (abs_path == NULL)
	{
		std::cerr << "Error resolving path " << request_path << std::endl;
		return false;
	}

	// Check if the absolute path starts with the root directory path and is not the root directory itself
	bool is_subdir = (strcmp(root_dir, abs_path) != 0) && (strncmp(root_dir, abs_path, strlen(root_dir)) == 0);
	free(abs_path);
	free(root_dir);
	return is_subdir;
}

// Recursively delete all files and subdirectories under a given directory
int delete_directory(const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (dir == NULL)
	{
		// Failed to open directory
		return 1;
	}

	dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		// Ignore '.' and '..' directories
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
		{
			continue;
		}

		std::string full_path = path + "/" + std::string(entry->d_name);

		struct stat stat_buf;
		if (lstat(full_path.c_str(), &stat_buf) == -1)
		{
			// Failed to stat file or directory
			continue;
		}

		if (S_ISDIR(stat_buf.st_mode))
		{
			// Recursively delete subdirectory
			if (delete_directory(full_path) != 0)
			{
				closedir(dir);
				return 1;
			}
			if (rmdir(full_path.c_str()) != 0)
			{
				closedir(dir);
				return 1;
			}
		}
		else
		{
			// Delete file
			if (remove(full_path.c_str()) != 0)
			{
				closedir(dir);
				return 1;
			}
		}
	}

	closedir(dir);

	// Delete directory itself
	if (rmdir(path.c_str()) != 0)
	{
		return 1;
	}

	return 0;
}

void ft::Response::methodDelete(ft::Request *request)
{
	(void)request;
	std::string request_path = root + target;
	if (!isSubdirectory(request_path.c_str()) || access(request_path.c_str(), F_OK))
	{
		insertResponse(responseHeader(NOT_FOUND).append("Not Found!"));
		return;
	}
	int result = remove(request_path.c_str());

	if (result == 0)
	{
		insertResponse(responseHeader(this->status_code).append("Deleted!"));
	}
	else
	{
		std::cerr << "Unable to delete file" << std::endl;
		result = delete_directory(request_path);

		if (result == 0)
		{
			this->status_code = OK;
			insertResponse(responseHeader(this->status_code).append("Deleted!"));
		}
		else
		{
			std::cerr << "Unable to delete dir" << std::endl;
			this->status_code = INTERNAL_SERVER_ERROR;
			insertResponse(responseHeader(this->status_code).append("Internal server error"));
		}
	}
}

int ft::Response::returnResponse(int fd)
{
	int ret;
	if (this->size > BUFFER_SIZE)
	{
		string tmp = _response.substr(0, BUFFER_SIZE);
		size -= BUFFER_SIZE;
		_response.erase(0, BUFFER_SIZE);
		ret = send(fd, tmp.c_str(), BUFFER_SIZE, 0);
	}
	else
	{
		size = 0;
		ret = send(fd, _response.c_str(), _response.size(), 0);
		_response.clear();
	}
	return ret;
}

bool ft::Response::empty()
{
	if (size == 0)
		return true;
	return false;

}