#include "./Server/Server.hpp"
#include "./Parser/Parser.hpp"
#include "./includes/colour.h"
#include <iostream>
#include <vector>
#include <poll.h>

using std::cout;
using std::endl;
using std::vector;

int main(void)
{
	std::ifstream file;
	file.open("./config/example.conf");
	const char *temp[] = {"listen", "server_name", "root"};
	std::vector<std::string> test(temp, temp + 3);
	ft::Parser Parse(file, test);

	ft::HTTPServer	Serverlist(Parse.getHTTPServer());
	vector<struct pollfd> fds;
	char buf[65535];


	for (ft::HTTPServer::iterator it = Serverlist.begin(); it != Serverlist.end(); it++)
	{
		struct pollfd tmp;
		tmp.fd = it->second.getFd();
		tmp.events = POLLIN;
		fds.push_back(tmp);
	}

	// for (vector<struct pollfd>::iterator it = fds.begin(); fds)

	int server_size = Serverlist.size();
			bool connection = false;
	// cout << " fds size is " << fds.size()  << endl;
	while (1)
	{

		if (poll(&fds[0], fds.size(), 100))
		{
			// cout << " fds revents is " <<  fds.data()->revents<< endl;
			for (int i = 0; i < fds.size(); i++)
			{
				if (fds[i].revents == 0)
					continue;
				if (i < server_size)
				{
					if (fds[i].revents & POLLIN)
					{
						struct pollfd tmp;
						tmp.fd = Serverlist.findServer(fds[i].fd).getSocket().accept_connection();
						Serverlist.insertClient(fds[i].fd, tmp.fd);
						tmp.events = POLLIN;
						cout << CYAN "[INFO] Incoming connection connected to FD: " << fds[i].fd << " with client FD: " << tmp.fd <<  RESET << endl;
						fds.push_back(tmp);
					}
				}
				else if (fds[i].revents != 0)
				{
					if (fds[i].revents & POLLIN)
					{
				
						int ret = read(fds[i].fd, buf, 3000);
						if (ret)
						{
							Serverlist.findClient(fds[i].fd).insertRequest(buf, ret);
							Serverlist.request(fds[i].fd);
							cout << MAGENTA "[INFO] Client FD : " << fds[i].fd << " is in read mode." RESET << endl;
				
							// cout << Serverlist.findClient(fds[i].fd).getRequest();
							fds[i].events = POLLOUT;
							Serverlist.findClient(fds[i].fd).getRequest().clear();
						}
						else
							connection = true;
					}
					else if (fds[i].revents & POLLOUT)
					{
						cout << MAGENTA "[INFO] Client FD : " << fds[i].fd << " is in send mode." RESET << endl;
						std::string HAHA = Serverlist.findClient(fds[i].fd).getRespond().c_str();
						const char *str = HAHA.c_str();
						write(fds[i].fd, str, strlen(str));
						std::cout << Serverlist.findClient(fds[i].fd).getRespond() << std::endl;
						fds[i].events = POLLIN;
						// poll_length--;
					}
					if (fds[i].revents & POLLHUP || connection == true)
					{
						cout << RED "Deleteing client FD: " << fds[i].fd << " connnected to server FD: "
						<< Serverlist.findServerfd(fds[i].fd) <<  RESET <<  endl;
						Serverlist.eraseClient(fds[i].fd);
						fds.erase(fds.begin() + i);
						connection = false;
					}
				}
			}
		}
	}
}

