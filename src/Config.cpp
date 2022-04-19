#include "Config.hpp"

namespace we
{
    struct directive_dispatch
    {
        const char *name;
        void (*func)(void *, directive_data const &, size_t);
        size_t offset;
    };

    static void    handle_server_listen(Config &config, ServerBlock &server_config, directive_data const &data)
    {
        int listen_fd, optval = 1;
        struct sockaddr_in srv_addr;

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

    static void    handle_location_methods(LocationBlock &location_config, directive_data const &data, bool deny)
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
                    throw std::runtime_error("Duplicate method 'GET' at " + data.path + " line " + std::to_string(data.line) + ":" + std::to_string(data.column));
                location_config.allowed_methods.get += deny ? -1 : 1;
                flag |= 0x0000000F;
            }
            else if (data.args[i] == "POST")
            {
                if (flag & 0x000000F0)
                    throw std::runtime_error("Duplicate method 'POST' at " + data.path + " line " + std::to_string(data.line) + ":" + std::to_string(data.column));
                location_config.allowed_methods.post += deny ? -1 : 1;
                flag |= 0x000000F0;
            }
            else if (data.args[i] == "PUT")
            {
                if (flag & 0x00000F00)
                    throw std::runtime_error("Duplicate method 'PUT' at " + data.path + " line " + std::to_string(data.line) + ":" + std::to_string(data.column));
                location_config.allowed_methods.put += deny ? -1 : 1;
                flag |= 0x00000F00;
            }
            else if (data.args[i] == "HEAD")
            {
                if (flag & 0x0000F000)
                    throw std::runtime_error("Duplicate method 'HEAD' at " + data.path + " line " + std::to_string(data.line) + ":" + std::to_string(data.column));
                location_config.allowed_methods.head += deny ? -1 : 1;
                flag |= 0x0000F000;
            }
        }
    }

    void    init_config()
    {
        // Root level directives
        we::register_directive("use_events", 1, 0, false, false, true, false, false);
        we::register_directive("default_type", 1, 0, false, false, true, false, false);
        we::register_directive("client_header_timeout", 1, 0, false, false, true, false, false);
        we::register_directive("client_max_header_size", 1, 0, false, false, true, false, false);
        we::register_directive("client_header_buffer_size", 1, 0, false, false, true, false, false);

        // General directives
        we::register_directive("include", 1, 0, false, true, true, true, true);
        we::register_directive("server", 0, 0, true, true, true, false, false);
        we::register_directive("location", 1, 0, true, true, false, true, false);

        // Server level directives
        we::register_directive("listen", 1, 0, false, true, false, true, false);
        we::register_directive("server_name", 1, -1, false, false, false, true, false);
        we::register_directive("server_send_timeout", 1, 0, false, false, true, true, false);
        we::register_directive("server_body_buffer_size", 1, 0, false, false, true, true, false);
        we::register_directive("keep_alive", 1, 0, false, false, true, true, false);
        we::register_directive("keep_alive_timeout", 1, 0, false, false, true, true, false);
        we::register_directive("enable_lingering_close", 1, 0, false, false, true, true, false);
        we::register_directive("lingering_close_timeout", 1, 0, false, false, true, true, false);

        // Location level directives
        we::register_directive("root", 1, 0, false, false, true, true, true);
        we::register_directive("index", 1, -1, false, false, true, true, true);
        we::register_directive("autoindex", 1, 0, false, false, false, true, true);
        we::register_directive("allow_upload", 1, 0, false, false, false, false, true);
        we::register_directive("upload_dir", 1, 0, false, false, false, false, true);
        we::register_directive("allowed_methods", 1, -1, false, false, true, true, true);
        we::register_directive("denied_methods", 1, -1, false, false, true, true, true);
        we::register_directive("client_body_timeout", 1, 0, false, false, true, true, true);
        we::register_directive("client_body_in_file", 1, 0, false, false, true, true, true);
        we::register_directive("client_body_buffer_size", 1, 0, false, false, true, true, true);
        we::register_directive("client_max_body_size", 1, 0, false, false, true, true, true);
    }

    directive_data &get_directive_data(const std::string &name, directive_block *root, directive_block *server, directive_block *location)
    {
        if (location && location->directives.count(name))
            return location->directives.find(name)->second;
        else if (server && server->directives.count(name))
            return server->directives.find(name)->second;
        else if (root && root->directives.count(name))
            return root->directives.find(name)->second;
        else // TODO: Get directive from default config
            throw std::runtime_error("Directive not found: " + name);
    }

    void    init_server_directives(Config &config, ServerBlock &server_config, directive_block *root, directive_block *server)
    {
        static const directive_dispatch dispatcher[] = {
            { "listen", NULL, 0 },
            { "server_name", &we::set_string_directive, OFFSET_OF(ServerBlock, server_name) },
            { "server_send_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, server_send_timeout) },
            { "server_body_buffer_size", &we::set_size_directive, OFFSET_OF(ServerBlock, server_body_buffer_size) },
            { "keep_alive", &we::set_boolean_directive, OFFSET_OF(ServerBlock, keep_alive) },
            { "keep_alive_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, keep_alive_timeout) },
            { "enable_lingering_close", &we::set_boolean_directive, OFFSET_OF(ServerBlock, enable_lingering_close) },
            { "lingering_close_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, lingering_close_timeout) },
        };

        for (int i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
        {
            directive_data &data = get_directive_data(dispatcher[i].name, root, server, NULL);

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "listen", 6) == 0)
                    handle_server_listen(config, server_config, data);
            }
            else
                dispatcher[i].func(&server_config, data, dispatcher[i].offset);
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
            directive_data &data = get_directive_data(dispatcher[i].name, root, server, location);

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "index", 5) == 0)
                    location_config.index = data.args;
                else if (strncmp(dispatcher[i].name, "allowed_methods", 15) == 0)
                    handle_location_methods(location_config, data, false);
                else if (strncmp(dispatcher[i].name, "denied_methods", 13) == 0)
                    handle_location_methods(location_config, data, true);
            }
            else
                dispatcher[i].func(&location_config, data, dispatcher[i].offset);
        }
    }

    bool    load_config(const std::string& file_name, Config& config)
    {
        we::Parser parser(file_name);

        init_config();

        parser.block();

        directive_block *root_block = &parser.blocks[0];
        directive_block *server_block = NULL;
        directive_block *location_block = NULL;

        ServerBlock *current_server = NULL;
        std::vector<directive_block>::iterator it = parser.blocks.begin() + 1;
        for (; it != parser.blocks.end(); ++it)
        {
            if (it->name == "server")
            {
                server_block = &(*it);
                location_block = NULL;

                ServerBlock server_config;
                init_server_directives(config, server_config, root_block, server_block);
                config.server_blocks[server_config.listen_socket].push_back(server_config);
                current_server = &config.server_blocks[server_config.listen_socket].back();
            }
            else if (it->name == "location")
            {
                location_block = &(*it);

                LocationBlock location_config;
                init_location_directives(location_config, root_block, server_block, location_block);
                current_server->locations.push_back(location_config);
            }
        }
        return true;
    }

    std::string error_to_print(const std::string & val, const directive_data &data)
    {
        return val + " at " + data.path + " line " + std::to_string(data.line);
    }

    static long long atoi(const std::string & val, const directive_data &data)
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
            ret += (val[i] - '0') * 10;
            if (ret > 9223372036854775807ULL + (sign == -1))
                throw   std::runtime_error(error_to_print("limit exceeded", data));
        }
        if (i != val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        if (ret == 9223372036854775808ULL && (sign == -1))
            return -9223372036854775807LL - 1LL;
        return ret * sign;
    }

    static long long atou(const std::string & val, const directive_data &data)
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
            ret += (val[i] - '0') * 10;
            if (ret > 9223372036854775807ULL)
                throw   std::runtime_error(error_to_print("limit exceeded", data));
        }
        if (i != val.size())
            throw   std::runtime_error(error_to_print("invalid argument", data));
        return ret;
    }

    static char check_validity(const std::string & val, directive_data const &data)
    {
        if (val.back() != 's' || val.back() != 'm' || val.back() != 'h' || val.back() != 'd')
            throw   std::runtime_error(error_to_print("limit exceeded", data));
        return val.back();
    }

    // data.args[0] == 10s | 20m | 30h
    void    set_time_directive(void *block, directive_data const &data, size_t offset)
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
    void    set_number_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<long long *>((char *)block + offset) = atoi(data.args[0], data);
    }

    // // data.args[0] == 1024 | 2048 | 4096 | 8192 | 16384 | 32768 | 65536 (long long) positive
    void    set_unumber_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<long long *>((char *)block + offset) = atou(data.args[0], data);
    }
    // // data.args[0] == on | off
    void    set_boolean_directive(void *block, directive_data const &data, size_t offset)
    {
        bool ret;
        if (strcasecmp(data.args[0].c_str(), "on") == 0 || strcasecmp(data.args[0].c_str(), "true"))
            ret = 1;
        else if (strcasecmp(data.args[0].c_str(), "off") == 0 || strcasecmp(data.args[0].c_str(), "false"))
            ret = 0;
        else
            throw   std::runtime_error(error_to_print("invalid argument", data));
        *reinterpret_cast<bool *>((char *)block + offset) = ret;
    }

    // data.args[0] == 1mb | 2mb | 4mb | 8mb | 16mb | 32mb | 64mb | 128mb | 256mb | 512mb | 1024mb
    void    set_size_directive(void *block, directive_data const &data, size_t offset)
    {
        if (data.args[0].size() < 2)
            throw   std::runtime_error(error_to_print("invalid argument", data));
        long long ret = atou(std::string(data.args[0].begin(), data.args[0].end() - 2), data);
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
        *reinterpret_cast<long long *>((char *)block + offset) = ret;
    }

    // // data.args[0] == "some string"
    void    set_string_directive(void *block, directive_data const &data, size_t offset)
    {
        *reinterpret_cast<std::string *>((char *)block + offset) = data.args[0];
    }

}


