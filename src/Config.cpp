#include "Config.hpp"
#include <unistd.h>
#include <set>

namespace we
{
    Config::Config()
    {
        this->multiplex_type = Config::MulNone;
        this->client_header_timeout = 60 * 1000;
        this->client_header_buffer_size = 4 * 1024;
        this->client_max_header_size = 16 * 1024 * 1024;
    }

    const ServerBlock   *Config::get_server_block(int socket, const std::string &host) const
    {
        // TODO: implement this
        return NULL;
    }

    ServerBlock::ServerBlock()
    {
        this->listen_socket = -1;
        this->server_send_timeout = 60 * 1000;
        this->server_body_buffer_size = 4 * 1024;
        this->keep_alive = false;
        this->keep_alive_timeout = 60 * 1000;
        this->enable_lingering_close = false;
        this->lingering_close_timeout = 60 * 1000;
    }

    const LocationBlock *ServerBlock::get_location(const std::string& uri) const
    {
        // TODO: implement this
        return NULL;
    }

    LocationBlock::LocationBlock()
    {
        this->client_body_timeout = 60 * 1000;
        this->client_body_in_file = false;
        this->client_body_buffer_size = 4 * 1024;
        this->client_max_body_size = 16 * 1024 * 1024;

        this->pattern = "/";
        this->index.push_back("index.html");

        char tmp[PATH_MAX];
        getcwd(tmp, PATH_MAX);
        this->root = tmp;

        this->autoindex = false;
        this->allow_upload = false;
    }

    bool    LocationBlock::is_allowed_method(std::string method) const
    {
        if (method == "GET")
            return this->allowed_methods.get >= 0;
        else if (method == "POST")
            return this->allowed_methods.post >= 0;
        else if (method == "PUT")
            return this->allowed_methods.put >= 0;
        else if (method == "HEAD")
            return this->allowed_methods.head >= 0;
        else
            return false;
    }

    void    init_config()
    {
        // General directives
        we::register_directive("include", 1, 0, false, true, true, true, true);
        we::register_directive("server", 0, 0, true, true, true, false, false);
        we::register_directive("location", 1, 1, true, true, false, true, false);

        // Root level directives
        we::register_directive("use_events", 1, 0, false, false, true, false, false);
        we::register_directive("default_type", 1, 0, false, false, true, false, false);
        we::register_directive("client_header_timeout", 1, 0, false, false, true, false, false);
        we::register_directive("client_max_header_size", 1, 0, false, false, true, false, false);
        we::register_directive("client_header_buffer_size", 1, 0, false, false, true, false, false);

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

    bool    load_config(const std::string &file_name, Config &config)
    {
        we::Parser parser(file_name);

        init_config();

        parser.block();

        assert(parser.blocks.size() > 0);

        directive_block *root_block = &parser.blocks[0];
        directive_block *server_block = NULL;
        directive_block *location_block = NULL;

        init_root_directives(config, root_block);

        ServerBlock *current_server_config = NULL;
        std::vector<directive_block>::iterator it = parser.blocks.begin() + 1;
        for (; it != parser.blocks.end(); ++it)
        {
            if (it->name == "server")
            {
                // TODO: before overriding the server_block, check for duplicate location patterns

                server_block = &(*it);
                location_block = NULL;

                ServerBlock server_config;
                init_server_directives(config, server_config, root_block, server_block);
                config.server_blocks[server_config.listen_socket].push_back(server_config);
                current_server_config = &config.server_blocks[server_config.listen_socket].back();

                if (server_config.server_names.empty())
                    server_config.server_names.push_back("");

                if ((it + 1) == parser.blocks.end() || (it + 1)->name != "location")
                {
                    LocationBlock location_config;
                    init_location_directives(location_config, root_block, server_block, NULL);
                    current_server_config->locations.push_back(location_config);
                }
            }
            else if (it->name == "location")
            {
                location_block = &(*it);

                LocationBlock location_config;
                if (location_block->args.size() == 2)
                {
                    location_config.pattern = location_block->args[1];
                    if (location_block->args[0] == "=")
                        location_config.modifier = LocationBlock::Modifier_exact;
                    else if (location_block->args[0] == "~")
                        location_config.modifier = LocationBlock::Modifier_regex;
                    else
                        throw std::runtime_error("Invalid pattern modifier in the directive 'location' at " + location_block->path + ":" + std::to_string(location_block->line));
                }
                else
                    location_config.pattern = location_block->args[0];

                init_location_directives(location_config, root_block, server_block, location_block);
                current_server_config->locations.push_back(location_config);
            }
        }

        validate_config(config);
        return true;
    }

    void    validate_config(Config &config)
    {
        for (Config::server_block_iterator it = config.server_blocks.begin(); it != config.server_blocks.end(); ++it)
        {
            std::set<std::string> server_names;

            for (std::vector<ServerBlock>::iterator sb_it = it->second.begin(); sb_it != it->second.end(); ++sb_it)
            {
                for (std::vector<std::string>::iterator sn_it = sb_it->server_names.begin(); sn_it != sb_it->server_names.end(); ++sn_it)
                {
                    if (server_names.count(*sn_it))
                        throw std::runtime_error("conflicting server name \"" + sb_it->server_names[0] + "\" on " + sb_it->listen_addr);
                    server_names.insert(*sn_it);
                }
            }
        }
    }
}