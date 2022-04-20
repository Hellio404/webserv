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

    directive_data::directive_data(std::string const& path, unsigned int line, unsigned int column)
    {
        this->path = path;
        this->line = line;
        this->column = column;
    }


    Parser::Parser(std::string const &path) :  blocks(1)
    {
        add_file(path);

        blocks[0].name = "_root";
        current_block = 0;
        // level = 0; // debug
        file_level = 0;
        current_directive = 0;
    }

    Parser::file_info::file_info(std::string const& path): path(path), line_number(1), column_number(1)
    {
        f = new std::fstream(path, std::fstream::in);
        if (!f->is_open())
        {
            delete f;
            throw std::runtime_error("Can't open file " + path);
        }
    }

    Parser::file_info::~file_info()
    {
        f->close();
        delete f;
    }
    
    void    Parser::add_file(std::string const &path)
    {
        file_info *f = new file_info(path);
        files.push(f);
        file = files.top()->f;
    }

    void    Parser::remove_file()
    {
        file_info *f = files.top();
        files.pop();
        delete f;
        if (files.empty())
            throw std::runtime_error("Can't remove file: no file left");
        file = files.top()->f;
    }


    char Parser::next()
    {
        if (file->peek() == '\n')
        {
            files.top()->line_number++;
            files.top()->column_number = 1;
        }
        else
            files.top()->column_number++;
        return file->get();
    }

    char Parser::peek()
    {
        return file->peek();
    }

    char Parser::consume(char c)
    {
        if (c == 0 && !file->eof()) // this is to expecte end of file
            unexpected();
        else if (c == 0)
            return 0;

        if (file->eof() || file->peek() != c)
            expected(c);
        return this->next();
    }

    std::string Parser::file_pos()
    {
        return files.top()->path + " line " + std::to_string(files.top()->line_number) + ":" + std::to_string(files.top()->column_number);
    }

    std::string Parser::file_pos(unsigned int line, unsigned int column)
    {
        return files.top()->path + " line " + std::to_string(line) + ":" + std::to_string(column);
    }

    void Parser::expected(char c)
    {
        std::string ex = c == 0 ? "end of file" : "'" + std::string(1, c) + "'";
        std::string ux = file->eof() ? "end of file" : "'" + std::string(1, file->peek()) + "'";
        throw std::runtime_error(
            "Expected " + ex + " but got " + ux +
            " at " + file_pos());
    }

    void Parser::unexpected()
    {
        std::string ux = file->eof() ? "end of file" : "'" + std::string(1, file->peek()) + "'";
        throw std::runtime_error(
            "Unexpected " + ux +
            " at " + file_pos());
    }

    void Parser::skip_comments()
    {
        if (file->eof() || peek() != '#')
            return;
        while (!file->eof() && peek() != '\n')
            this->next();
    }

    void Parser::next_token()
    {
        while (!file->eof() && (peek() == ' ' || peek() == '\t' || peek() == '#' || peek() == '\n'))
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
        while (!file->eof() && (isalpha(peek()) || peek() == '_'))
            token += this->next();
        return token;
    }

    static bool is_special_char(char c)
    {
        return   c == '{' || c == '}' || c == ';'  || c == '#' || c == '\n' || c == '\t' || c == ' ' || c == '\\';
    }
    std::string Parser::get_arg()
    {
        std::string arg;
        this->next_token();
        bool is_skipped = false;
        while (!file->eof() && (is_skipped || (peek() != ' ' && peek() != '\t' && peek() != '#' && peek() != '\n' && peek() != ';' && peek() != '{' && peek() != '}')))

        {
            if (peek() == '\\' && !is_skipped)
            {
                is_skipped = true;
                this->next();
            }
            else
            {
                if (is_skipped && !is_special_char(peek()))
                    arg += '\\';
                    
                arg += this->next();
                is_skipped = false;
            }
        }
        return arg;
    }

    void Parser::block()
    {
        this->next_token();
        while (!file->eof() && peek() != '}')
        {
            this->directive();
            this->next_token();
        }
        if (file_level == 0)
            this->consume(0);
    }

    void Parser::read_block(const std::string &name, const std::string& path, unsigned int line, unsigned int col, std::vector<std::string> args)
    {

        this->consume('{');
        // std::cerr <<"\n" << std::string(level, '\t') << "{" << std::endl;
        // this->level++;
        this->file_level++;
        size_t old_block = current_block;
        current_block = blocks.size();
        blocks.resize(blocks.size() + 1);
        blocks[current_block].parent_idx = old_block;
        blocks[current_block].name = name;
        blocks[current_block].path = path;
        blocks[current_block].line = line;
        blocks[current_block].column = col;
        blocks[current_block].args = args;
        size_t old_directive = current_directive;
        if (name == "server")
            current_directive = 1;
        else if (name == "location")
            current_directive = 2;
        this->block();
        this->consume('}');
        // this->level--;
        this->file_level--;
        // std::cerr << std::string(level, '\t') << "}" << std::endl;
        current_directive = old_directive;
        current_block = old_block;
    }

    void Parser::directive()
    {
        std::string name = this->get_token();

        std::string path = files.top()->path;
        unsigned int line = files.top()->line_number;
        unsigned int col = files.top()->column_number - name.size();

        if (name.empty())
        {
            if (peek() != ';')
                this->unexpected();
            return (void)this->consume(';');
        }
        // std::cerr << std::string(level, '\t') <<  name << " ";

        if (file->eof() || (peek() != '\n' && peek() != ';' && peek() != ' ' && peek() != '\t' && peek() != '#' && peek() != '{'))
            this->unexpected();

        if (dir_infos.count(name) == 0)
            throw std::runtime_error("Unknown directive '" + name + "' at " + file_pos(line, col));

        switch (current_directive)
        {
        case 0:
            if (dir_infos[name].allow_in_root == false)
                throw std::runtime_error("Directive '" + name + "' is not allowed in root " + file_pos(line, col));
            break;
        case 1:
            if (dir_infos[name].allow_in_server == false)
                throw std::runtime_error("Directive '" + name + "' is not allowed in server " + file_pos(line, col));
            break;
        case 2:
            if (dir_infos[name].allow_in_location == false)
                throw std::runtime_error("Directive '" + name + "' is not allowed in location " + file_pos(line, col));
            break;
        default:
            break;
        }

        if (this->blocks[current_block].directives.count(name) > 0 && !dir_infos[name].allow_duplicate)
            throw std::runtime_error("Duplicate directive '" + name + "' at " + file_pos(line, col));

        std::vector<std::string> &args = (*this->blocks[current_block].directives.insert(std::make_pair(name, directive_data(path, line, col)))).second.args;

        while (!file->eof() && peek() != ';' && peek() != '{' && peek() != '}')
        {
            std::string arg = this->get_arg();
            if (!arg.empty())
            {
                args.push_back(arg);
                // std::cerr << arg << " ";
            }
        }

        if (args.size() < dir_infos[name].num_args)
            throw std::runtime_error("Too few arguments for directive '" + name + "' at " + file_pos(line, col));
        if (args.size() > dir_infos[name].num_args + dir_infos[name].num_opt_args)
            throw std::runtime_error("Too many arguments for directive '" + name + "' at " + file_pos(line, col));

        if (dir_infos[name].allow_block)
        {
            this->read_block(name, path, line, col, args);
            this->blocks[current_block].directives.erase(name);
        }
        else
        {
            this->consume(';');
            // std::cerr << ";" << std::endl;
            if (name == "include")
            {
                unsigned int old_level = this->file_level;
                this->file_level = 0;
                this->add_file(args[0]);
                try
                {
                    this->block();
                }
                catch(...)
                {
                    this->remove_file();
                    throw;
                }
                
                this->remove_file();
                this->file_level = old_level;
                this->blocks[current_block].directives.erase(name);
            }
        }
    }

    Parser::~Parser()
    {
        file_info *last = files.top();
        delete last;
        files.pop();
        assert(files.empty());
    }
}
