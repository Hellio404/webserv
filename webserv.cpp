#include "src/Multiplexing.hpp"
#include "src/Connection.hpp"
#include "src/EventLoop.hpp"
#include "src/Config.hpp"
#include "src/Utils.hpp"
#include "src/Date.hpp"
#include <Regex.hpp>
#include <fcntl.h>

int main(int ac, char **av)
{
    we::Config config;
    we::AMultiplexing *multiplexer = NULL;
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
    if (av[1])
        std::cerr << config.get_server_block(4, "sdsd")->get_location(av[1]) << std::endl;
    multiplexer = we::get_instance(config.multiplex_type);
    if (multiplexer == NULL)
    {
        std::cerr << "Unable to create multiplexer" << std::endl;
        return 1;
    }

    we::Config::server_block_const_iterator it = config.server_blocks.begin();
    while (it != config.server_blocks.end())
    {
        multiplexer->add(it->first, NULL, we::AMultiplexing::Read);
        ++it;
    }

    while (1)
    {
        multiplexer->wait(eventLoop.get_next_activation_time());

        int fd;
        
        while ((fd = multiplexer->get_next_fd()) != -1)
        {
            we::Connection *connection = multiplexer->get_connection(fd);
            if (connection == NULL)
                new we::Connection(fd, eventLoop, config, *multiplexer);
            else
            {
                connection->handle_connection();
            }

        }
        eventLoop.run();
    }
    

   
    
}