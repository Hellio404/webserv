#include "src/Multiplexing.hpp"
#include "src/Connection.hpp"
#include "src/EventLoop.hpp"
#include "src/Config.hpp"
#include "src/Utils.hpp"
#include "src/Date.hpp"
#include <Regex.hpp>
#include <fcntl.h>

int main()
{
    we::Config config;
    we::AMultiplexing *multiplexing = new we::MultiplexingSelect();
    we::EventLoop eventLoop;
    try
    {
        we::load_config("./conf/webserv.conf", config);
    }
    catch(std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    print_config_block_info(config);

    // int listen_fd, optval = 1;
    // struct sockaddr_in srv_addr;
    // srv_addr.sin_family = AF_INET; // IPv4
    // srv_addr.sin_port = htons(80); // port 80
    // srv_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

    // if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    // {
    //     std::cerr << "Failed to create socket" << std::endl;
    //     return (0);
    // }

    // if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) < 0)
    // {
    //     std::cerr << "Failed to make a non-blocking socket" << std::endl;
    //     return (0);
    // }

    // if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    // {
    //     std::cerr << "Failed to set socket options" << std::endl;
    //     return (0);
    // }

    // if (bind(listen_fd, reinterpret_cast<struct sockaddr *>(&srv_addr), sizeof(srv_addr)) < 0)
    // {
    //     std::cerr << "Failed to bind socket" << std::endl;
    //     return (0);
    // }

    // if (listen(listen_fd, SOMAXCONN) < 0)
    // {
    //     std::cerr << "Failed to listen on socket" << std::endl;
    //     return (0);
    // }

    // // Have kqueue watch the listen socket
    // multiplexing->add(listen_fd, NULL, we::AMultiplexing::Read);
    // config.server_socks[*reinterpret_cast<struct sockaddr*>(&srv_addr)] = listen_fd;

    // std::cout << "Server started on port 80" << std::endl;

    // while (true)
    // {
    //     int fd = -1;

    //     multiplexing->wait();
    //     while ((fd = multiplexing->get_next_fd()) != -1)
    //     {
    //         we::Connection *connection = multiplexing->get_connection(fd);
    //         if (connection == NULL)
    //         {
    //             new we::Connection(fd, eventLoop, config, *multiplexing);
    //         }
    //         else
    //         {
    //             connection->handle_connection();
    //         }
    //     }
        
    // }
}