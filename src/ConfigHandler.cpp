#include "Config.hpp"

namespace we
{
    static std::string error_to_print(const std::string &val, const directive_data &data)
    {
        return val + " at " + data.path + " line " + std::to_string(data.line) + ":" + std::to_string(data.column);
    }

    static long long atoi(const std::string &val, const directive_data &data)
    {
        size_t      i = 0;
        unsigned long long   ret = 0;
        int         sign = 1;

        if (val[i] == '+' || val[i] == '-')
        {
            sign = (val[i] == '-') * -1;
            i++;
        }
        if (i == val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        while (val[i])
        {
            if (!isalnum(val[i]))
                throw   std::runtime_error(error_to_print("invalid argument", data));
            ret = ret * 10 + val[i] - '0';
            if (ret > 9223372036854775807ULL + (sign == -1))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            i++;
        }
        if (i != val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        if (ret == 9223372036854775808ULL && (sign == -1))
            return -9223372036854775807LL - 1LL;
        return ret * sign;
    }

    static long long atou(const std::string &val, const directive_data &data)
    {
        size_t      i = 0;
        unsigned long long   ret = 0;

        if (val[i] == '+')
            i++;
        if (i == val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        while (val[i])
        {
            if (!isalnum(val[i]))
                throw   std::runtime_error(error_to_print("invalid argument", data));
            ret = ret * 10 + (val[i] - '0');
            if (ret > 9223372036854775807ULL)
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            i++;
        }

        if (i != val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        return ret;
    }

    static char check_validity(const std::string &val, directive_data const &data)
    {
        if (val.back() != 's' && val.back() != 'm' && val.back() != 'h' && val.back() != 'd')
            throw   std::runtime_error(error_to_print("invalid argument", data));
        return val.back();
    }

    // data.args[0] == 10s | 20m | 30h
    static void set_time_directive(void *block, directive_data const &data, size_t offset)
    {
        char ch = check_validity(data.args[0], data);

        long long ret = atou(std::string(data.args[0].begin(), data.args[0].end() - 1), data);

        switch (ch)
        {
        case 's':
            if (ret > 9223372036854775807 / 1000)
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000;
            break;
        case 'm':
            if (ret > 9223372036854775807 / (1000 * 60))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60;
            break;
        case 'h':
            if (ret > 9223372036854775807 / (1000 * 60 * 60))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60 * 60;
            break;
        case 'd':
            if (ret > 9223372036854775807 / (1000 * 60 * 60 * 24))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60 * 60 * 24;
            break;
        default:
            break;
        }
        *reinterpret_cast<long long *>((char *)block + offset) = ret;
    }

    // data.args[0] == 1024 | -2048 | 4096 | -8192 | -16384 | 32768 | 65536 (long long)
    static void set_number_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<long long *>((char *)block + offset) = atoi(data.args[0], data);
    }

    // data.args[0] == 1024 | 2048 | 4096 | 8192 | 16384 | 32768 | 65536 (long long) positive
    static void set_unumber_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<long long *>((char *)block + offset) = atou(data.args[0], data);
    }

    // data.args[0] == on | off
    static void set_boolean_directive(void *block, directive_data const &data, size_t offset)
    {
        bool ret;
        if (strcasecmp(data.args[0].c_str(), "on") == 0 || strcasecmp(data.args[0].c_str(), "true") == 0)
            ret = 1;
        else if (strcasecmp(data.args[0].c_str(), "off") == 0 || strcasecmp(data.args[0].c_str(), "false") == 0)
            ret = 0;
        else
            throw   std::runtime_error(error_to_print("invalid argument", data));
        *reinterpret_cast<bool *>((char *)block + offset) = ret;
    }

    // data.args[0] == 1mb | 2mb | 4mb | 8mb | 16mb | 32mb | 64mb | 128mb | 256mb | 512mb | 1024mb
    static void set_size_directive(void *block, directive_data const &data, size_t offset)
    {
        if (data.args[0].size() < 2)
            throw   std::runtime_error(error_to_print("invalid argument", data));
        long long ret = atou(std::string(data.args[0].begin(), data.args[0].end() - 2), data);
        std::cerr << "ret = " << ret << std::endl;
        std::string unit = std::string(data.args[0].end() - 2, data.args[0].end());
        if (unit == "kb")
        {
            if (ret > 9223372036854775807 / (1024))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024;
        }
        else if (unit == "mb")
        {
            if (ret > 9223372036854775807 / (1024 * 1024))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024 * 1024;
        }
        else if (unit == "gb")
        {
            if (ret > 9223372036854775807 / (1024 * 1024 * 1024))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024 * 1024 * 1024;
        }
        else
            throw   std::runtime_error(error_to_print("invalid argument", data));
        *reinterpret_cast<long long *>((char *)block + offset) = ret;
    }

    // data.args[0] == "some string"
    static void set_string_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<std::string *>((char *)block + offset) = data.args[0];
    }

    static void handle_server_listen(Config &config, ServerBlock &server_config, directive_data const &data)
    {
        int listen_fd, optval = 1;
        struct sockaddr_in srv_addr;
        std::cerr << "srv_addr.sin_port = " << srv_addr.sin_port << std::endl;

        size_t pos = data.args[0].find(':');

        memset(&srv_addr, 0, sizeof(srv_addr));
        srv_addr.sin_family = AF_INET; // IPv4
        if (pos == std::string::npos)
        {
            srv_addr.sin_port = htons(std::atoi(data.args[0].c_str()));
            srv_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
        }
        else
        {
            // TODO: handle getaddrinfo
            srv_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
            srv_addr.sin_port = htons(std::atoi(data.args[0].substr(pos + 1).c_str()));
        }

        Config::server_sock_iterator it = config.server_socks.find(*reinterpret_cast<struct sockaddr *>(&srv_addr));
        if (it != config.server_socks.end())
        {
            server_config.listen_socket = it->second;
            server_config.server_addrs.push_back(srv_addr);
            server_config.server_socks.push_back(it->second);
            return ; // already listening on this address and port
        }

        if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            throw std::runtime_error(strerror(errno));

        if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) < 0)
            throw std::runtime_error(strerror(errno));

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
            throw std::runtime_error(strerror(errno));

        if (bind(listen_fd, reinterpret_cast<struct sockaddr *>(&srv_addr), sizeof(srv_addr)) < 0)
            throw std::runtime_error(strerror(errno));

        if (listen(listen_fd, SOMAXCONN) < 0)
            throw std::runtime_error(strerror(errno));

        server_config.listen_socket = listen_fd;
        server_config.server_addrs.push_back(srv_addr);
        server_config.server_socks.push_back(listen_fd);
        config.server_socks[*reinterpret_cast<struct sockaddr*>(&srv_addr)] = listen_fd;
    }

    static void handle_location_methods(LocationBlock &location_config, directive_data const &data, bool deny)
    {
        unsigned int flag = 0x00000000;

        if (!deny)
            memset(&location_config.allowed_methods, -1, sizeof(location_config.allowed_methods));
        // TODO: Missing directive name for error handling
        for (size_t i = 0; i < data.args.size(); i++)
        {
            if (data.args[i] == "GET")
            {
                if (flag & 0x0000000F)
                    throw std::runtime_error(error_to_print("duplicate method 'GET'", data));
                location_config.allowed_methods.get += deny ? -1 : 1;
                flag |= 0x0000000F;
            }
            else if (data.args[i] == "POST")
            {
                if (flag & 0x000000F0)
                    throw std::runtime_error(error_to_print("duplicate method 'POST'", data));
                location_config.allowed_methods.post += deny ? -1 : 1;
                flag |= 0x000000F0;
            }
            else if (data.args[i] == "PUT")
            {
                if (flag & 0x00000F00)
                    throw std::runtime_error(error_to_print("duplicate method 'PUT'", data));
                location_config.allowed_methods.put += deny ? -1 : 1;
                flag |= 0x00000F00;
            }
            else if (data.args[i] == "HEAD")
            {
                if (flag & 0x0000F000)
                    throw std::runtime_error(error_to_print("duplicate method 'HEAD'", data));
                location_config.allowed_methods.head += deny ? -1 : 1;
                flag |= 0x0000F000;
            }
        }
    }

    static void handle_use_events(Config &config, directive_data const &data)
    {
        static const char * mapped_multiplex[] = {
            "none", "select", "kqueue", "poll", "epoll",
        };

        if (data.args.size() != 1)
            throw std::runtime_error(error_to_print("invalid number of arguments", data));

        for (int i = 1; i < sizeof(mapped_multiplex) / sizeof(mapped_multiplex[0]); i++)
        {
            if (data.args[0] == mapped_multiplex[i])
            {
                config.multiplex_type = (Config::MultiplexingType)(i);
                return ;
            }
        }

        throw std::runtime_error(error_to_print("invalid multiplex type", data));
    }

    static directive_data *get_directive_data(const std::string &name, directive_block *root, directive_block *server, directive_block *location)
    {
        if (location && location->directives.count(name))
            return &(location->directives.find(name)->second);
        else if (server && server->directives.count(name))
            return &(server->directives.find(name)->second);
        else if (root && root->directives.count(name))
            return &(root->directives.find(name)->second);
        return NULL;
    }

    void    init_root_directives(Config &config, directive_block *root)
    {
        static const directive_dispatch dispatcher[] = {
            { "use_events", NULL, 0 },
            { "default_type", NULL, 0 },
            { "client_header_timeout", &we::set_time_directive, OFFSET_OF(Config, client_header_timeout) },
            { "client_max_header_size", &we::set_size_directive, OFFSET_OF(Config, client_max_header_size) },
            { "client_header_buffer_size", &we::set_size_directive, OFFSET_OF(Config, client_header_buffer_size) },
        };

        for (int i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
        {
            directive_data *data = get_directive_data(dispatcher[i].name, root, NULL, NULL);

            if (data == NULL)
                continue;

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "use_events", 10) == 0)
                    handle_use_events(config, *data);
            }
            else
                dispatcher[i].func(&config, *data, dispatcher[i].offset);
        }
    }

    void    init_server_directives(Config &config, ServerBlock &server_config, directive_block *root, directive_block *server)
    {
        static const directive_dispatch dispatcher[] = {
            { "listen", NULL, 0 },
            { "server_name", NULL, 0 },
            { "server_send_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, server_send_timeout) },
            { "server_body_buffer_size", &we::set_size_directive, OFFSET_OF(ServerBlock, server_body_buffer_size) },
            { "keep_alive", &we::set_boolean_directive, OFFSET_OF(ServerBlock, keep_alive) },
            { "keep_alive_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, keep_alive_timeout) },
            { "enable_lingering_close", &we::set_boolean_directive, OFFSET_OF(ServerBlock, enable_lingering_close) },
            { "lingering_close_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, lingering_close_timeout) },
        };

        for (int i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
        {
            directive_data *data = get_directive_data(dispatcher[i].name, root, server, NULL);

            if (data == NULL)
                continue;

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "listen", 6) == 0)
                    handle_server_listen(config, server_config, *data);
                else if (strncmp(dispatcher[i].name, "server_name", 11) == 0)
                    server_config.server_names = data->args;
            }
            else
                dispatcher[i].func(&server_config, *data, dispatcher[i].offset);
        }
    }

    void    init_location_directives(LocationBlock &location_config, directive_block *root, directive_block *server, directive_block *location)
    {
        static const directive_dispatch dispatcher[] = {
            { "root", &we::set_string_directive, OFFSET_OF(LocationBlock, root) },
            { "index", NULL, 0 },
            { "autoindex", &we::set_boolean_directive, OFFSET_OF(LocationBlock, autoindex) },
            { "allow_upload", &we::set_boolean_directive, OFFSET_OF(LocationBlock, allow_upload) },
            { "upload_dir", &we::set_string_directive, OFFSET_OF(LocationBlock, upload_dir) },
            { "allowed_methods", NULL, 0 },
            { "denied_methods", NULL, 0 },
            { "client_body_timeout", &we::set_time_directive, OFFSET_OF(LocationBlock, client_body_timeout) },
            { "client_body_in_file", &we::set_boolean_directive, OFFSET_OF(LocationBlock, client_body_in_file) },
            { "client_body_buffer_size", &we::set_size_directive, OFFSET_OF(LocationBlock, client_body_buffer_size) },
            { "client_max_body_size", &we::set_size_directive, OFFSET_OF(LocationBlock, client_max_body_size) },
        };

        for (int i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
        {
            directive_data *data = get_directive_data(dispatcher[i].name, root, server, location);

            if (data == NULL)
                continue;

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "index", 5) == 0)
                    location_config.index = data->args;
                else if (strncmp(dispatcher[i].name, "allowed_methods", 15) == 0)
                    handle_location_methods(location_config, *data, false);
                else if (strncmp(dispatcher[i].name, "denied_methods", 13) == 0)
                    handle_location_methods(location_config, *data, true);
            }
            else
                dispatcher[i].func(&location_config, *data, dispatcher[i].offset);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Debugging functions
    ///////////////////////////////////////////////////////////////////////////

    static void print_location_block_info(const LocationBlock & val)
    {
        std::cout << "\t\tLocation : " << std::endl;
        std::cout << "\t\t\tPattern : " << val.pattern << std::endl;
        std::cout << "\t\t\tClient body timeout : " << val.client_body_timeout << std::endl;
        std::cout << "\t\t\tClient body in file : " << val.client_body_in_file << std::endl;
        std::cout << "\t\t\tClient body buffer size : " << val.client_body_buffer_size << std::endl;
        std::cout << "\t\t\tClient max body size : " << val.client_max_body_size << std::endl;
        std::cout << "\t\t\tIndex :";
        for (size_t i = 0; i < val.index.size(); i++)
            std::cout << " " << val.index[i];
        std::cout << std::endl;
        std::cout << "\t\t\tRoot : " << val.root << std::endl;
        std::cout << "\t\t\tAutoindex : " << (val.autoindex ? "true" : "false") << std::endl;
        std::cout << "\t\t\tAllow upload : " << (val.allow_upload ? "true" : "false") << std::endl;
        std::cout << "\t\t\tAllow dir : " << val.upload_dir << std::endl;
        // std::cout << "\t\t\tAllow methods : " << val.allowed_methods << std::endl;
    }

    static void print_server_block_info(const ServerBlock & val)
    {
        std::cout << "\tserver : " << std::endl;
        std::cout << "\t\tlisten_socket : " << val.listen_socket << std::endl;
        std::cout << "\t\tserver_name :";
        for (size_t i = 0; i < val.server_names.size(); i++)
            std::cout << " " << val.server_names[i];
        std::cout << std::endl;
        std::cout << "\t\tserver_send_timeout : " << val.server_send_timeout << std::endl;
        std::cout << "\t\tserver_body_buffer_size : " << val.server_body_buffer_size << std::endl;
        for (size_t i = 0; i < val.locations.size(); i++)
            print_location_block_info(val.locations[i]);
    }

    void    print_config_block_info(const Config & val)
    {
        static const char * mapped_multiplex[] = {
            "none", "select", "kqueue", "poll", "epoll",
        };
        std::cout << "config : " << std::endl;
        std::cout << "\tmultiplex_type : " << mapped_multiplex[val.multiplex_type] << std::endl;
        std::cout << "\tclient_header_timeout : " << val.client_header_timeout << std::endl;
        std::cout << "\tclient_header_buffer_size : " << val.client_header_buffer_size << std::endl;
        std::cout << "\tclient_max_header_size : " << val.client_max_header_size << std::endl;
        Config::server_block_const_iterator it = val.server_blocks.begin();
        while (it != val.server_blocks.end())
        {
            std::cout << "\t-----------------" << std::endl;
            for (size_t i = 0; i < it->second.size(); i++)
                print_server_block_info(it->second[i]);
            ++it;
        }
    }
}