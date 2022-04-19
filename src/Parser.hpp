#pragma once
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include "Config.hpp"

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

    struct directive_block 
    {
        size_t                                                  parent_idx;
        std::string                                             name;
        std::multimap <std::string, std::vector<std::string> >  directives;
    };

    class Parser
    {
    private:
        /* data */
        std::fstream                    file;
        std::vector <directive_block>   blocks;
    public:
        Parser(std::string const &path);
        ~Parser();
        // unsigned int    level; // debug
        unsigned int    line_number;
        unsigned int    column_number;
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
        void            read_block(const std::string& name);
        void            expected(char c);
        void            unexpected();
        std::string     file_pos();

    };
    void register_directive(std::string name, unsigned int num_args, unsigned int num_opt_args, bool allow_block, bool allow_duplicate, bool allow_in_root, bool allow_in_server, bool allow_in_location);


}
