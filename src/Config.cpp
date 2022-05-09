#include "Config.hpp"
#include <unistd.h>
#include <set>
# define MAX_BUFFER_SIZE (1024 * 1024 * 128)
namespace we
{
    Config::Config()
    {
        this->multiplex_type = Config::MulNone;
        this->client_header_timeout = 60 * 1000;
        this->client_header_buffer_size = 2 * 1024 * 1024;
        this->client_max_header_size = 16 * 1024 * 1024;
        this->max_internal_redirect = 10;

        this->default_type = "application/octet-stream";
    }

    const ServerBlock   *Config::get_server_block(int socket, const std::string &host) const
    {
        Config::server_block_const_iterator pos = this->server_blocks.find(socket);

        if (pos == this->server_blocks.end())
            return NULL;

        for (std::vector<ServerBlock>::const_iterator sb_it = pos->second.begin(); sb_it != pos->second.end(); ++sb_it)
        {
            for (std::vector<std::string>::const_iterator sn_it = sb_it->server_names.begin(); sn_it != sb_it->server_names.end(); ++sn_it)
            {
                if (*sn_it == host)
                    return &(*sb_it);
            }
        }
        return &pos->second.front();
    }

    std::string         Config::get_mime_type(const std::string &filepath) const
    {
        std::string::size_type pos = filepath.rfind('.');

        if (pos == std::string::npos)
            return this->default_type;

        std::string ext = filepath.substr(pos + 1);
        std::map<std::string, std::string>::const_iterator it = this->mime_types.find(ext);

        if (it != this->mime_types.end())
            return it->second;

        return this->default_type;
    }

    ServerBlock::ServerBlock()
    {
        this->listen_socket = -1;
        this->server_send_timeout = 60 * 1000;
        this->server_body_buffer_size = 2 * 1024 * 1024;
    }

    const LocationBlock *ServerBlock::get_location(const std::string& uri) const
    {
        std::vector<LocationBlock>::const_iterator it = this->locations.begin();
        const LocationBlock *regex = NULL;
        const LocationBlock *normal = NULL;

        for (; it != this->locations.end(); ++it)
        {
            switch (it->modifier)
            {
                case LocationBlock::Modifier_none:
                    if (!regex && std::strncmp(it->pattern.c_str(), uri.c_str(), it->pattern.size()) == 0)
                    {
                        if (!normal || it->pattern.size() > normal->pattern.size())
                            normal = &(*it);
                    }
                    break;
                case LocationBlock::Modifier_exact:
                    if (it->pattern == uri)
                        return &(*it);
                    break;
                case LocationBlock::Modifier_regex:
                case LocationBlock::Modifier_regex_icase:
                    if (!regex && it->regex->test(uri))
                        regex = &(*it);
                    break;
                default:
                    break;
            }
        }
        if (regex)
            return regex;
        return normal;
    }

    LocationBlock::LocationBlock()
    {
        this->client_body_timeout = 60 * 1000;
        this->client_body_buffer_size = 2 * 1024 * 1024;
        this->client_max_body_size = 16 * 1024 * 1024;
        this->regex = NULL;
        this->pattern = "/";
        this->modifier = LocationBlock::Modifier_none;

        this->index.push_back("index.html");

        char tmp[PATH_MAX];
        getcwd(tmp, PATH_MAX);
        this->root = tmp;
        this->upload_dir = tmp;

        this->autoindex = false;
        this->allow_upload = false;

        this->handlers.resize(PHASE_NUMBER);
        this->is_redirection = false;
        this->return_code = 404;

        memset(&this->allowed_methods, 0, sizeof(this->allowed_methods));
        this->allowed_method_found = false;
    }

    LocationBlock::~LocationBlock()
    {
        if (this->regex)
            delete this->regex;
    }

    LocationBlock::LocationBlock(LocationBlock const &other)
    {
        this->client_body_timeout = other.client_body_timeout;
        this->client_body_buffer_size = other.client_body_buffer_size;
        this->client_max_body_size = other.client_max_body_size;

        this->pattern = other.pattern;
        this->modifier = other.modifier;
        if (other.regex)
            this->regex = new ft::Regex(this->pattern, this->modifier == LocationBlock::Modifier_regex_icase ? ft::Regex::iCase : 0);
        else
            this->regex = NULL;

        this->index = other.index;
        this->root = other.root;
        this->autoindex = other.autoindex;
        this->allow_upload = other.allow_upload;
        this->upload_dir = other.upload_dir;
        this->allowed_methods = other.allowed_methods;
        this->handlers = other.handlers;
        this->error_pages = other.error_pages;
        this->is_redirection = other.is_redirection;
        this->redirect_url = other.redirect_url;
        this->return_code = other.return_code;
        this->allowed_method_found = other.allowed_method_found;
        this->cgi = other.cgi;
        this->added_headers = other.added_headers;
    }

    std::string LocationBlock::get_error_page(int status) const
    {
        if (this->error_pages.count(status))
            return this->error_pages.find(status)->second;
        return "";
    }

    bool    LocationBlock::is_allowed_method(std::string method) const
    {
        if (method == "GET")
            return this->allowed_methods.get >= allowed_method_found;
        else if (method == "POST")
            return this->allowed_methods.post >= allowed_method_found;
        else if (method == "PUT")
            return this->allowed_methods.put >= allowed_method_found;
        else if (method == "HEAD")
            return this->allowed_methods.head >= allowed_method_found || this->allowed_methods.get >= allowed_method_found;
        else if (method == "DELETE")
            return this->allowed_methods.del >= allowed_method_found;
        else
            return false;
    }

    void    init_config()
    {
        // General directives
        we::register_directive("include", 1, 0, false, true, true, true, true);
        we::register_directive("server", 0, 0, true, true, true, false, false);
        we::register_directive("location", 1, 1, true, true, false, true, false);
        we::register_directive("types", 0, 0, true, false, true, false, false);

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
        we::register_directive("add_header", 2, 0, false, true, true, true, true);
        
        // Location level directives
        we::register_directive("root", 1, 0, false, false, true, true, true);
        we::register_directive("index", 1, -1, false, false, true, true, true);
        we::register_directive("autoindex", 1, 0, false, false, false, true, true);
        we::register_directive("allow_upload", 1, 0, false, false, false, false, true);
        we::register_directive("upload_dir", 1, 0, false, false, false, false, true);
        we::register_directive("error_page", 2, -1, false, true, true, true, true);
        we::register_directive("allowed_methods", 1, -1, false, false, true, true, true);
        we::register_directive("denied_methods", 1, -1, false, false, true, true, true);
        we::register_directive("client_body_timeout", 1, 0, false, false, true, true, true);
        we::register_directive("client_body_buffer_size", 1, 0, false, false, true, true, true);
        we::register_directive("client_max_body_size", 1, 0, false, false, true, true, true);
        we::register_directive("cgi_pass", 1, 0, false, false, false, false, true);
        we::register_directive("return", 2, 0, false, false, false, false, true);
    }

    static void validate_config(Config &config)
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
        if (config.client_header_timeout <= 0)
            throw std::runtime_error("client_header_timeout must be greater than 0");
        if (config.client_max_header_size <= 0)
            throw std::runtime_error("client_max_header_size must be greater than 0");
        if (config.client_header_buffer_size <= 0 || config.client_header_buffer_size > config.client_max_header_size || config.client_header_buffer_size > MAX_BUFFER_SIZE)
            throw std::runtime_error("client_header_buffer_size invalid size for the buffer");
    }

    static void validate_server_config(ServerBlock *server_config, directive_block *root_block, directive_block *server_block)
    {
        std::set<std::pair<std::string, LocationBlock::Modifier> > locations;

        for (std::vector<LocationBlock>::iterator lb_it = server_config->locations.begin(); lb_it != server_config->locations.end(); ++lb_it)
        {
            if (lb_it->client_body_timeout <= 0)
                throw std::runtime_error("client_body_timeout must be greater than 0");
            if (lb_it->client_max_body_size <= 0)
                throw std::runtime_error("client_max_body_size must be greater than 0");
            if (lb_it->client_body_buffer_size <= 0 || lb_it->client_body_buffer_size > lb_it->client_max_body_size || lb_it->client_body_buffer_size > MAX_BUFFER_SIZE)
                throw std::runtime_error("client_body_buffer_size invalid size for the buffer");
            

            std::pair<std::string, LocationBlock::Modifier> location = std::make_pair(lb_it->pattern, lb_it->modifier);
            if (locations.count(location))
                throw std::runtime_error("conflicting location pattern \"" + lb_it->pattern + "\" on " + server_config->listen_addr);
            locations.insert(location);
        }

        if (locations.count(std::make_pair("/", LocationBlock::Modifier_none)) == 0)
        {
            LocationBlock location_config;
            init_location_directives(location_config, root_block, server_block, NULL);
            server_config->locations.push_back(location_config);
        }

        if (server_config->server_send_timeout <= 0)
            throw std::runtime_error("server_send_timeout must be greater than 0");
        if (server_config->server_body_buffer_size <= 0 || server_config->server_body_buffer_size > MAX_BUFFER_SIZE)
            throw std::runtime_error("server_body_buffer_size invalid size for the buffer");
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
                if (current_server_config != NULL)
                    validate_server_config(current_server_config, root_block, server_block);

                server_block = &(*it);
                location_block = NULL;

                ServerBlock server_config;
                init_server_directives(config, server_config, root_block, server_block);
                config.server_blocks[server_config.listen_socket].push_back(server_config);
                current_server_config = &config.server_blocks[server_config.listen_socket].back();

                if (current_server_config->server_names.empty())
                    current_server_config->server_names.push_back("");

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
                    else if (location_block->args[0] == "~" || location_block->args[0] == "~*")
                    {
                        location_config.modifier = location_block->args[0] == "~*" ? LocationBlock::Modifier_regex_icase: LocationBlock::Modifier_regex;
                        int flags = location_block->args[0] == "~*" ? ft::Regex::iCase : 0;
                        try
                        {
                            location_config.regex = new ft::Regex(location_config.pattern, flags);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("Invalid regex pattern in the directive 'location' at " + location_block->path + ":" + std::to_string(location_block->line));
                        }
                        
                    }
                    else
                        throw std::runtime_error("Invalid pattern modifier in the directive 'location' at " + location_block->path + ":" + std::to_string(location_block->line));
                }
                else
                {
                    location_config.modifier = LocationBlock::Modifier_none;
                    location_config.pattern = location_block->args[0];
                }

                init_location_directives(location_config, root_block, server_block, location_block);
                current_server_config->locations.push_back(location_config);
            }
            else if (it->name == "types")
            {
                std::map <std::string,  std::vector<directive_data> >::iterator it2 = it->directives.begin();
                for (; it2 != it->directives.end(); ++it2)
                {
                    std::vector<directive_data>::iterator it3 = it2->second.begin();
                    for (; it3 != it2->second.end(); ++it3)
                    {
                        std::vector<std::string>::iterator it4 = it3->args.begin();
                        for (; it4 != it3->args.end(); ++it4)
                        {
                            if (config.mime_types.count(*it4))
                                throw std::runtime_error("Redefinition of the type of the extention " + *it4 +" at "+ it3->path + ":" + std::to_string(it3->line));

                           config.mime_types[*it4] = it2->first;
                        }
                    }
                }
            }
        }

        if (current_server_config != NULL)
            validate_server_config(current_server_config, root_block, server_block);
        validate_config(config);

        return true;
    }
}
