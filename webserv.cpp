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
    // Ignore SIGPIPE signals
    signal(SIGPIPE, SIG_IGN);

    we::Config config;
    we::EventLoop eventLoop;
    we::AMultiplexing *multiplexer = NULL;

    try
    {
        if (ac > 2)
            throw std::runtime_error("Usage: " + std::string(av[0]) + " [config_file]");

        if (ac == 2)
            we::load_config(av[1], config);
        else
            we::load_config("./conf/webserv.conf", config);

        multiplexer = we::get_instance(config.multiplex_type);
        if (multiplexer == NULL)
            throw std::runtime_error("Could not create multiplexer");

        we::Config::server_block_const_iterator it = config.server_blocks.begin();
        while (it != config.server_blocks.end())
        {
            multiplexer->add(it->first, NULL, we::AMultiplexing::Read);
            ++it;
        }

        while (1)
        {
            multiplexer->wait(eventLoop.get_next_activation_time());

            int fd = -1;
            while ((fd = multiplexer->get_next_fd()) != -1)
            {
                we::BaseConnection *connection = multiplexer->get_connection(fd);

                try
                {
                    if (connection == NULL)
                        new we::Connection(fd, eventLoop, config, *multiplexer);
                    else
                        connection->handle_connection();
                }
                catch (...)
                {
                    if (connection)
                        delete connection;
                    connection = NULL;
                }
            }
            eventLoop.run();
        }

        delete multiplexer;
        return 0;
    }
    catch (std::exception const &e)
    {
        if (multiplexer)
            delete multiplexer;

        std::cerr << e.what() << std::endl;
        return 1;
    }
}