#ifndef SERVER_HPP
# define SERVER_HPP

# include <unistd.h>
# include <map>
# include "Socket.hpp"
# include "../Parser/Serverinfo.hpp"
# include "../Server/Client.hpp"
namespace ft
{
    class Server
    {
        public:
            Server() {}
            Server(int port, ft::ServerInfo serv): socket(port), info(serv) {
                // client_fd = socket.accept_connection();
            }
            
            ~Server() {
                // close(client_fd);
            }

            /**
             * @brief Keep waiting for a connection
             * 
             */
            
            // int getFd(void)
            // {
            //     return client_fd;
            // }

            ft::Socket getSocket(void)
            {
                return this->socket;
            }

            int getFd(void)
            {
                return this->socket.getSocketfd();
            }

            ft::ServerInfo  getInfo(void)
            {
                return this->info;
            }
            // void    bind_fd(int fd)
            // {
            //     client_fd = fd;
            // }

        private:
            ft::ServerInfo info;
            ft::Socket socket;



    };
}

#endif