#include "Parser.hpp"

namespace we
{

    static std::map<std::string, directive_info> dir_infos;

    void register_directive(std::string name, unsigned int num_args, unsigned int num_opt_args, bool allow_block, bool allow_duplicate, bool allow_in_root, bool allow_in_server, bool allow_in_location)
    {
        if (dir_infos.count(name) > 0)
            throw std::runtime_error("Directive already registered: " + name);
        directive_info &dir = dir_infos[name];
        dir.num_args = num_args;
        dir.num_opt_args = num_opt_args;
        dir.allow_block = allow_block;
        dir.allow_duplicate = allow_duplicate;
        dir.allow_in_root = allow_in_root;
        dir.allow_in_server = allow_in_server;
        dir.allow_in_location = allow_in_location;
    }

    Parser::Parser(std::string const &path) : file(path, std::ios::in), blocks(1)
    {
        if (!file.is_open())
        {
            throw std::runtime_error("Can't open file " + path);
        }
        blocks[0].name = "root";
        current_block = 0;
        line_number = 1;
        column_number = 1;
        // level = 0;
        current_directive = 0;
    }

    char Parser::next()
    {
        if (file.peek() == '\n')
        {
            line_number++;
            column_number = 1;
        }
        else
            column_number++;
        return file.get();
    }

    char Parser::peek()
    {
        return file.peek();
    }

    char Parser::consume(char c)
    {
        if (c == 0 && !file.eof()) // this is to expecte end of file
            unexpected();
        else if (c == 0)
            return 0;

        if (file.eof() || file.peek() != c)
            expected(c);
        return this->next();
    }

    std::string Parser::file_pos()
    {
        return "line " + std::to_string(line_number) + " column " + std::to_string(column_number);
    }

    void Parser::expected(char c)
    {
        std::string ex = c == 0 ? "end of file" : "'" + std::string(1, c) + "'";
        std::string ux = file.eof() ? "end of file" : "'" + std::string(1, file.peek()) + "'";
        throw std::runtime_error(
            "Expected " + ex + " but got " + ux +
            " at " + file_pos());
    }

    void Parser::unexpected()
    {
        std::string ux = file.eof() ? "end of file" : "'" + std::string(1, file.peek()) + "'";
        throw std::runtime_error(
            "Unexpected " + ux +
            " at " + file_pos());
    }

    void Parser::skip_comments()
    {
        if (file.eof() || peek() != '#')
            return;
        while (!file.eof() && peek() != '\n')
            this->next();
    }

    void Parser::next_token()
    {
        while (!file.eof() && (peek() == ' ' || peek() == '\t' || peek() == '#' || peek() == '\n'))
        {
            if (peek() == '#')
                this->skip_comments();
            this->next();
        }
    }

    std::string Parser::get_token()
    {
        std::string token;
        this->next_token();
        while (!file.eof() && (isalpha(peek()) || peek() == '_'))
            token += this->next();
        return token;
    }

    std::string Parser::get_arg()
    {
        std::string arg;
        this->next_token();
        bool is_skipped = false;
        while (!file.eof() && (is_skipped || (peek() != ' ' && peek() != '\t' && peek() != '#' && peek() != '\n' && peek() != ';' && peek() != '{' && peek() != '}')))

        {
            if (peek() == '\\' && !is_skipped)
            {
                is_skipped = true;
                this->next();
            }
            else
            {
                arg += this->next();
                is_skipped = false;
            }
        }
        return arg;
    }

    void Parser::block()
    {
        this->next_token();
        while (!file.eof() && peek() != '}')
        {
            this->directive();
            this->next_token();
        }
        if (current_block == 0)
            this->consume(0);
    }

    void Parser::read_block(const std::string &name)
    {

        this->consume('{');
        // std::cout <<"\n" << std::string(level, '\t') << "{" << std::endl;
        // this->level++;
        size_t old_block = current_block;
        current_block = blocks.size();
        blocks.resize(blocks.size() + 1);
        blocks[current_block].parent_idx = old_block;
        blocks[current_block].name = name;
        if (name == "server")
            current_directive = 1;
        else if (name == "location")
            current_directive = 2;
        this->block();
        this->consume('}');
        // this->level--;
        // std::cout << std::string(level, '\t') << "}" << std::endl;
        current_block = old_block;
    }

    void Parser::directive()
    {
        std::string name = this->get_token();
        if (name.empty())
        {
            if (peek() != ';')
                this->unexpected();
            return (void)this->consume(';');
        }
        // std::cout << std::string(level, '\t') <<  name << " ";

        if (file.eof() || (peek() != '\n' && peek() != ';' && peek() != ' ' && peek() != '\t' && peek() != '#' && peek() != '{'))
            this->unexpected();

        if (dir_infos.count(name) == 0)
            throw std::runtime_error("Unknown directive " + name + " at " + file_pos());

        switch (current_block)
        {
        case 0:
            if (dir_infos[name].allow_in_root == false)
                throw std::runtime_error("Directive " + name + " is not allowed in root " + file_pos());
            break;
        case 1:
            if (dir_infos[name].allow_in_server == false)
                throw std::runtime_error("Directive " + name + " is not allowed in server " + file_pos());
            break;
        case 2:
            if (dir_infos[name].allow_in_location == false)
                throw std::runtime_error("Directive " + name + " is not allowed in location " + file_pos());
            break;
        default:
            break;
        }

        if (this->blocks[current_block].directives.count(name) > 0 && !dir_infos[name].allow_duplicate)
            throw std::runtime_error("Duplicate directive " + name + " at " + file_pos());

        std::vector<std::string> &args = (*this->blocks[current_block].directives.insert(std::make_pair(name, std::vector<std::string>()))).second;

        while (!file.eof() && peek() != ';' && peek() != '{' && peek() != '}')
        {
            std::string arg = this->get_arg();
            if (!arg.empty())
            {
                args.push_back(arg);
                // std::cout << arg << " ";
            }
        }

        if (args.size() < dir_infos[name].num_args)
            throw std::runtime_error("Too few arguments for directive " + name + " at " + file_pos());
        if (args.size() > dir_infos[name].num_args + dir_infos[name].num_opt_args)
            throw std::runtime_error("Too many arguments for directive " + name + " at " + file_pos());

        if (dir_infos[name].allow_block)
        {
            this->read_block(name);
        }
        else
        {
            this->consume(';');
            // std::cout << ";" << std::endl;
        }
    }

    Parser::~Parser()
    {
        file.close();
    }
}
