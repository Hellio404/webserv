#pragma once

#include "Config.hpp"

#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include <stack>
#include <map>
// TODO: remove c++11 to_string
namespace we
{
    struct directive_info
    {
        unsigned int    num_args: 4;
        unsigned int    num_opt_args: 8;
        unsigned int    allow_block: 1;
        unsigned int    allow_duplicate: 1;
        
        unsigned int    allow_in_root:1;
        unsigned int    allow_in_server:1;
        unsigned int    allow_in_location:1;
    };

    struct directive_data
    {
        std::vector<std::string> args;
        std::string              path;
        unsigned int             line;
        unsigned int             column;
        directive_data(std::string const&, unsigned int, unsigned int);
    };

    struct directive_block 
    {
        size_t                                                  parent_idx;
        std::string                                             name;
        std::multimap <std::string,  directive_data>            directives;
        std::vector<std::string>                                args;
        std::string                                             path;
        unsigned int                                            line;
        unsigned int                                            column;
    };

    class Parser
    {
    private:
        /* data */
        struct file_info {
            std::fstream    *f;
            std::string     path;
            unsigned int    line_number;
            unsigned int    column_number;
            file_info(std::string const&);
            ~file_info();
        };
        unsigned int                    file_level;
        std::fstream*                   file;
        std::stack<file_info*>          files;
    public:
        std::vector <directive_block>   blocks;
        Parser(std::string const &path);
        ~Parser();
        unsigned int    level; // debug
        unsigned int    current_block;
        unsigned int    current_directive;
        char            next();
        char            peek();
        char            consume(char c);
        void            next_token();
        void            skip_comments();
        std::string     get_token();
        std::string     get_arg();
        void            directive();
        void            block();
        void            read_block(const std::string&, const std::string&, unsigned int, unsigned int, std::vector<std::string>);
        void            expected(char c);
        void            unexpected();
        std::string     file_pos();
        std::string     file_pos(unsigned int line, unsigned int column);

        void            remove_file();
        void            add_file(std::string const &path);


    };
    void register_directive(std::string name, unsigned int num_args, unsigned int num_opt_args, bool allow_block, bool allow_duplicate, bool allow_in_root, bool allow_in_server, bool allow_in_location);


}
