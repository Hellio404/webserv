#include "src/Multiplexing.hpp"
#include "src/Connection.hpp"
#include "src/EventLoop.hpp"
#include "src/Config.hpp"
#include "src/Logger.hpp"
#include "src/Utils.hpp"
#include "src/Date.hpp"
#include <Regex.hpp>
#include <fcntl.h>

static void check_necessary_configuration()
{
    DIR *dir = NULL;

    dir = opendir("./tmp");
    if (dir == NULL)
    {
        if (mkdir("./tmp", 0755) < 0)
            throw std::runtime_error("mkdir: failed to create 'tmp' directory");
    }
    else
        closedir(dir);

    dir = opendir("./log");
    if (dir == NULL)
    {
        if (mkdir("./log", 0755) < 0)
            throw std::runtime_error("mkdir: failed to create 'log' directory");
    }
    else
        closedir(dir);

    we::Logger::get_instance().setAccessLog("./log/access.log");
    we::Logger::get_instance().setErrorLog("./log/error.log");
}

int main(int ac, char **av)
{
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
            we::load_config("./conf/default.conf", config);

        check_necessary_configuration();

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
        we::Logger::get_instance().error(NULL, "critical", e.what());
        return 1;
    }
}
