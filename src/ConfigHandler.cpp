#include "Config.hpp"

namespace we
{
    static std::string error_to_print(const std::string &val, const directive_data &data)
    {
        return val + " at " + data.path + ":" + std::to_string(data.line) + ":" + std::to_string(data.column);
    }

    static long long atoi(const std::string &val, const directive_data &data)
    {
        size_t i = 0;
        unsigned long long ret = 0;
        int sign = 1;

        if (val[i] == '+' || val[i] == '-')
        {
            sign = (val[i] == '-') * -1;
            i++;
        }
        if (i == val.size())
            throw std::runtime_error(error_to_print("invalid argument", data));
        while (val[i])
        {
            if (!isdigit(val[i]))
                throw std::runtime_error(error_to_print("invalid argument", data));
            ret = ret * 10 + val[i] - '0';
            if (ret > 9223372036854775807ULL + (sign == -1))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            i++;
        }
        if (i != val.size())
            throw std::runtime_error(error_to_print("invalid argument", data));
        if (ret == 9223372036854775808ULL && (sign == -1))
            return -9223372036854775807LL - 1LL;
        return ret * sign;
    }

    static long long atou(const std::string &val, const directive_data &data)
    {
        size_t i = 0;
        unsigned long long ret = 0;

        if (val[i] == '+')
            i++;
        if (i == val.size())
            throw std::runtime_error(error_to_print("invalid argument", data));
        while (val[i])
        {
            if (!isdigit(val[i]))
                throw std::runtime_error(error_to_print("invalid argument", data));
            ret = ret * 10 + (val[i] - '0');
            if (ret > 9223372036854775807ULL)
                throw std::runtime_error(error_to_print("limit exceeded", data));
            i++;
        }

        if (i != val.size())
            throw std::runtime_error(error_to_print("invalid argument", data));
        return ret;
    }

    static char check_validity(const std::string &val, directive_data const &data)
    {
        if (val.back() != 's' && val.back() != 'm' && val.back() != 'h' && val.back() != 'd')
            throw std::runtime_error(error_to_print("invalid argument", data));
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
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000;
            break;
        case 'm':
            if (ret > 9223372036854775807 / (1000 * 60))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60;
            break;
        case 'h':
            if (ret > 9223372036854775807 / (1000 * 60 * 60))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60 * 60;
            break;
        case 'd':
            if (ret > 9223372036854775807 / (1000 * 60 * 60 * 24))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1000 * 60 * 60 * 24;
            break;
        default:
            break;
        }
        *reinterpret_cast<long long *>((char *)block + offset) = ret;
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
            throw std::runtime_error(error_to_print("invalid argument", data));
        *reinterpret_cast<bool *>((char *)block + offset) = ret;
    }

    // data.args[0] == 1mb | 2mb | 4mb | 8mb | 16mb | 32mb | 64mb | 128mb | 256mb | 512mb | 1024mb
    static void set_size_directive(void *block, directive_data const &data, size_t offset)
    {
        if (data.args[0].size() < 2)
            throw std::runtime_error(error_to_print("invalid argument", data));
        if (*(data.args[0].end() - 1) == 'b' && isdigit(*(data.args[0].end() - 2)))
        {
            *reinterpret_cast<long long *>((char *)block + offset) = atou(std::string(data.args[0].begin(), data.args[0].end() - 1), data);
            return;
        }
        long long ret = atou(std::string(data.args[0].begin(), data.args[0].end() - 2), data);
        std::string unit = std::string(data.args[0].end() - 2, data.args[0].end());

        if (unit == "kb")
        {
            if (ret > 9223372036854775807 / (1024))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024;
        }
        else if (unit == "mb")
        {
            if (ret > 9223372036854775807 / (1024 * 1024))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024 * 1024;
        }
        else if (unit == "gb")
        {
            if (ret > 9223372036854775807 / (1024 * 1024 * 1024))
                throw std::runtime_error(error_to_print("limit exceeded", data));
            ret *= 1024 * 1024 * 1024;
        }
        else
            throw std::runtime_error(error_to_print("invalid argument", data));
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
        struct addrinfo hints, *res;

        std::string host;
        std::string port_str;
        long long port;

        std::string::const_iterator pos = std::find(data.args[0].begin(), data.args[0].end(), ':');
        if (pos != data.args[0].end())
        {
            host = std::string(data.args[0].begin(), pos);
            port_str = std::string(pos + 1, data.args[0].end());
        }
        else
        {
            host = "0.0.0.0";
            port_str = data.args[0];
        }

        if (!std::isdigit(port_str[0]))
            throw std::runtime_error(error_to_print("invalid port", data));
        port = we::atoi(port_str, data);

        if (port < 1 || port > 65535)
            throw std::runtime_error(error_to_print("invalid port", data));

        server_config.listen_addr = host + ":" + port_str;
        server_config.listen_port = port_str;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res))
            throw std::runtime_error(error_to_print(std::string("getaddrinfo ") + strerror(errno), data));
        if (res == NULL)
            throw std::runtime_error(error_to_print("host not found in \"" + host + ":" + port_str + "\"  of the \"listen\" directive", data));

        Config::server_sock_iterator it = config.server_socks.find(*res->ai_addr);
        if (it != config.server_socks.end())
        {
            server_config.listen_socket = it->second;
            return; // already listening on this address and port
        }

        if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            throw std::runtime_error(std::string("socket ") + strerror(errno));

        if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) < 0)
            throw std::runtime_error(std::string("fcntl ") + strerror(errno));

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
            throw std::runtime_error(std::string("setsockopt ") + strerror(errno));

        if (bind(listen_fd, res->ai_addr, res->ai_addrlen) < 0)
            throw std::runtime_error(std::string("bind ") + strerror(errno));

        if (listen(listen_fd, 1024) < 0)
            throw std::runtime_error(std::string("listen ") + strerror(errno));

        server_config.listen_socket = listen_fd;
        config.server_socks[*res->ai_addr] = listen_fd;
        freeaddrinfo(res);
    }

    static void handle_location_methods(LocationBlock &location_config, directive_data const &data, bool deny)
    {
        unsigned int flag = 0x00000000;

        if (!deny)
            location_config.allowed_method_found = true;
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
            else if (data.args[i] == "DELETE")
            {
                if (flag & 0x000F0000)
                    throw std::runtime_error(error_to_print("duplicate method 'DELETE'", data));
                location_config.allowed_methods.del += deny ? -1 : 1;
                flag |= 0x000F0000;
            }
            else
                throw std::runtime_error(error_to_print("invalid method", data));
        }
    }

    static void handle_use_events(Config &config, directive_data const &data)
    {
        static const char *mapped_multiplex[] = {
            "none",
            "select",
            "kqueue",
            "poll",
            "epoll",
        };

        if (data.args.size() != 1)
            throw std::runtime_error(error_to_print("invalid number of arguments", data));

        for (size_t i = 1; i < sizeof(mapped_multiplex) / sizeof(mapped_multiplex[0]); i++)
        {
            if (data.args[0] == mapped_multiplex[i])
            {
                config.multiplex_type = (Config::MultiplexingType)(i);
                return;
            }
        }

        throw std::runtime_error(error_to_print("invalid multiplex type", data));
    }

    static void handle_error_page(LocationBlock &location_config, std::vector<directive_data> *data)
    {
        std::vector<directive_data>::const_iterator dd_it = data->begin();
        for (; dd_it != data->end(); dd_it++)
        {
            std::string path = location_config.root;

            if (path.size() && path[path.size() - 1] != '/' && dd_it->args.back()[0] != '/')
                path += '/';
            else if (path.size() && path[path.size() - 1] == '/' && dd_it->args.back()[0] == '/')
                path.erase(path.size() - 1);
            path += dd_it->args.back();

            for (size_t i = 0; i < dd_it->args.size() - 1; i++)
            {
                std::string arg = dd_it->args[i];
                if (arg[0] == '+')
                    arg = '-';
                ssize_t status_code = we::atou(arg, *dd_it);
                // Check if the status code is valid
                if (status_code < 400 || (status_code > 417 && status_code < 500) || status_code > 505)
                    throw std::runtime_error(error_to_print("invalid status code", *dd_it));
                location_config.error_pages[status_code] = path;
            }
        }
    }

    static void handle_return(LocationBlock &location_config, directive_data const &data)
    {
        std::string ret_value = data.args.front();
        if (ret_value[0] == '+')
            ret_value = '-';
        ssize_t ret_code = we::atou(ret_value, data);
        if (ret_code < 300 || ret_code > 308)
            throw std::runtime_error(error_to_print("invalid return code", data));
        location_config.return_code = ret_code;
        location_config.redirect_url = data.args.back();
        location_config.is_redirection = true;
    }

    static directive_data *get_directive_data(const std::string &name, directive_block *root, directive_block *server, directive_block *location)
    {
        if (location && location->directives.count(name))
            return &location->directives[name].front();
        else if (server && server->directives.count(name))
            return &server->directives[name].front();
        else if (root && root->directives.count(name))
            return &root->directives[name].front();
        return NULL;
    }

    static std::vector<directive_data> get_all_directive_data(const std::string &name, directive_block *root, directive_block *server, directive_block *location)
    {
        std::vector<directive_data> ret;

        if (root && root->directives.count(name))
            ret.insert(ret.end(), root->directives[name].begin(), root->directives[name].end());
        if (server && server->directives.count(name))
            ret.insert(ret.end(), server->directives[name].begin(), server->directives[name].end());
        if (location && location->directives.count(name))
            ret.insert(ret.end(), location->directives[name].begin(), location->directives[name].end());

        return ret;
    }

    void init_root_directives(Config &config, directive_block *root)
    {
        static const directive_dispatch dispatcher[] = {
            {"use_events", NULL, 0},
            {"default_type", &we::set_string_directive, OFFSET_OF(Config, default_type)},
            {"client_header_timeout", &we::set_time_directive, OFFSET_OF(Config, client_header_timeout)},
            {"client_max_header_size", &we::set_size_directive, OFFSET_OF(Config, client_max_header_size)},
            {"client_header_buffer_size", &we::set_size_directive, OFFSET_OF(Config, client_header_buffer_size)},
        };

        for (size_t i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
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

    void init_server_directives(Config &config, ServerBlock &server_config, directive_block *root, directive_block *server)
    {
        static const directive_dispatch dispatcher[] = {
            {"listen", NULL, 0},
            {"server_name", NULL, 0},
            {"server_send_timeout", &we::set_time_directive, OFFSET_OF(ServerBlock, server_send_timeout)},
            {"server_body_buffer_size", &we::set_size_directive, OFFSET_OF(ServerBlock, server_body_buffer_size)},
        };

        for (size_t i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
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

        if (server_config.listen_socket == -1)
        {
            directive_data data("listen", 0, 0);
            data.args.push_back("0.0.0.0:80");
            handle_server_listen(config, server_config, data);
        }
    }

    void handle_add_header(LocationBlock &location_config, std::vector<directive_data> *data)
    {
        std::vector<directive_data>::const_iterator it = data->begin();

        for (; it != data->end(); ++it)
        {
            location_config.added_headers[it->args[0]] = it->args[1];
        }
    }

    void init_location_directives(LocationBlock &location_config, directive_block *root, directive_block *server, directive_block *location)
    {
        static const directive_dispatch dispatcher[] = {
            {"root", &we::set_string_directive, OFFSET_OF(LocationBlock, root)},
            {"index", NULL, 0},
            {"error_page", NULL, 0},
            {"autoindex", &we::set_boolean_directive, OFFSET_OF(LocationBlock, autoindex)},
            {"allow_upload", &we::set_boolean_directive, OFFSET_OF(LocationBlock, allow_upload)},
            {"upload_dir", &we::set_string_directive, OFFSET_OF(LocationBlock, upload_dir)},
            {"allowed_methods", NULL, 0},
            {"denied_methods", NULL, 0},
            {"client_body_timeout", &we::set_time_directive, OFFSET_OF(LocationBlock, client_body_timeout)},
            {"client_body_buffer_size", &we::set_size_directive, OFFSET_OF(LocationBlock, client_body_buffer_size)},
            {"client_max_body_size", &we::set_size_directive, OFFSET_OF(LocationBlock, client_max_body_size)},
            {"cgi_pass", &we::set_string_directive, OFFSET_OF(LocationBlock, cgi)},
            {"add_header", NULL, 0},
            {"return", NULL, 0},
        };

        for (size_t i = 0; i < sizeof(dispatcher) / sizeof(dispatcher[0]); i++)
        {
            std::vector<directive_data> data = get_all_directive_data(dispatcher[i].name, root, server, location);

            if (data.empty())
                continue;

            if (dispatcher[i].func == NULL)
            {
                if (strncmp(dispatcher[i].name, "index", 5) == 0)
                    location_config.index = data.back().args;
                else if (strncmp(dispatcher[i].name, "error_page", 10) == 0)
                    handle_error_page(location_config, &data);
                else if (strncmp(dispatcher[i].name, "allowed_methods", 15) == 0)
                    handle_location_methods(location_config, data.back(), false);
                else if (strncmp(dispatcher[i].name, "denied_methods", 13) == 0)
                    handle_location_methods(location_config, data.back(), true);
                else if (strncmp(dispatcher[i].name, "add_header", 9) == 0)
                    handle_add_header(location_config, &data);
                else if (strncmp(dispatcher[i].name, "return", 6) == 0)
                    handle_return(location_config, data.back());
            }
            else
                dispatcher[i].func(&location_config, data.back(), dispatcher[i].offset);
        }

        location_config.handlers[Phase_Reserved_1].push_back(&we::pre_access_handler);
        location_config.handlers[Phase_Reserved_2].push_back(&we::redirect_handler);

        location_config.handlers[Phase_Access].push_back(&we::index_handler);
        location_config.handlers[Phase_Access].push_back(&we::autoindex_handler);

        location_config.handlers[Phase_Access].push_back(&we::conditional_handler);

        // CGI related handlers
        location_config.handlers[Phase_Access].push_back(&we::cgi_handler);

        // Upload related handlers
        location_config.handlers[Phase_Access].push_back(&we::upload_handler);
        // File related handlers
        location_config.handlers[Phase_Access].push_back(&we::file_handler);

        // Error handlers
        location_config.handlers[Phase_Post_Access].push_back(&we::post_access_handler);

        // Inject Headers
        location_config.handlers[Phase_Inject_Headers].push_back(&we::inject_header_handler);

        // create ResponseServer
        location_config.handlers[Phase_Reserved_3].push_back(&we::response_server_handler);

        // Logging handlers
        location_config.handlers[Phase_Logging].push_back(&we::logger_handler);
    }
}