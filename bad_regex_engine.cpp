#include <iostream>
#include <deque>
#include <stack>
#include <set>
#include <exception>
#include <limits>
#include <map>
#include <cstring>
#include <cstdlib>
#include <bitset>
#include <regex>

struct State;
struct CharRange;
typedef std::multimap<CharRange, State*> MapCharToState;
#define MIN_CHAR std::numeric_limits<char>::min()
#define MAX_CHAR std::numeric_limits<char>::max()

enum { EPSILON = 130};

struct CharRange {
    std::set<int> chars;

    CharRange() {}

    CharRange(int from, int to)  {
        addRange(from, to);
    }

    CharRange(int from)  {
        chars.insert(from);
    }

    void    addRange(int from, int to) {
        for (int i = from; i <= to; i++) {
            chars.insert(i);
        }
    }

    void    addRange(int from) {
        chars.insert(from);
    }
    
    void    addRange(const CharRange& range) {
        chars.insert(range.chars.begin(), range.chars.end());
    }

    void    inverseRange()
    {
        std::set<int> tmp;
        for (int i = MIN_CHAR; i <= MAX_CHAR; i++)
        {
            if (!chars.count(i))
                tmp.insert(i);
        }
        chars = tmp;
    }

    bool    operator==(int c) const{
        return chars.find(c) != chars.end();
    }

    bool    operator==(struct CharRange const& c) const
    {
        // check that all the chars in c.chars are in this.chars
        for (std::set<int>::const_iterator it = c.chars.begin(); it != c.chars.end(); it++)
        {
            if (!chars.count(*it))
                return false;
        }
        return true;
    }

    bool    operator<(struct CharRange const& c) const{
        return *(chars.begin()) < *(c.chars.begin());
    }
};

struct StateDFA 
{
    int                         id;
    bool                        is_accepted;
    std::bitset<256>            avail;
    std::map<char, StateDFA*>   transitions;

    StateDFA(int id, bool is_accepted = false) : id(id), is_accepted(is_accepted)
    {}
    
    void    addTransition(char c, StateDFA *next)
    {
        avail[c + 128] = true;
        transitions[c] = next;
    }

    StateDFA *consume(char c)
    {
        if (avail[c + 128])
            return transitions[c];
        return NULL;
    }
};

struct State
{
    static int              count;
    int                     id;
    MapCharToState          transitions;
    bool                    is_accepted;
    bool                    is_visited;
    bool                    deleted;
    std::deque<CharRange>   ranges;

    State(int id, bool is_accepted = false, bool deleted = false) :
        id(id), is_accepted(is_accepted), deleted(deleted), is_visited(false) 
    {}
    
    State *clone()
    {
        std::map<State *, State *>                  mp;
        std::stack<std::pair<State *, State *> >    st;
        State*                                      ret;
        
        ret = new State(++State::count, this->is_accepted, this->deleted);
        st.push(std::make_pair(this, ret));
        mp[this] = ret;
        while (!st.empty())
        {
            std::pair<State *, State *> top = st.top();
            st.pop();

            for(MapCharToState::iterator it = top.first->transitions.begin(); it != top.first->transitions.end(); ++it)
            {
                if (mp.count(it->second) > 0)
                    top.second->addTransition(it->first, mp[it->second]);
                else
                {
                    mp[it->second] = new State(++State::count, it->second->is_accepted, it->second->deleted);
                    top.second->addTransition(it->first, mp[it->second]);
                    st.push(std::make_pair(it->second, mp[it->second]));
                }
            }  
        }
        return ret;
    }

    void    addTransition(CharRange const& c, State* s) 
    { 
        transitions.insert(std::make_pair(c, s));
        if (!(c == EPSILON))
            ranges.push_back(c);
    }

    ~State()
    {}
    
};

int State::count = 0;




class Regex
{
private:
    typedef std::deque<State*>  StateLine;
    std::string                 regex;
    StateDFA                    *dfaStart;
    bool                        match_start;
    bool                        match_end;
    

public:
    

    Regex(std::string const &s): regex(s), match_start(false), match_end(false)
    {
        std::deque<StateLine>       NfaStates;
        std::stack <char>           pending_operators;
        int                         last_operator_code = -1;
        bool                        is_skiped = false;

        State::count = 0;
        if (s.length() == 0)
            throw InvalidRegexException(s, 0);

        // this regex implementation only accept the ^ symbol to be at the beginning of the regex
        // you can't do something like [a-z]+\n^test as the '^' before test well be invalid 
        if (s[0] == '^')
            match_start = true;
        clock_t start = clock();
        
        for(size_t i = match_start; i < s.length(); i++)
        {
            // this regex implementation only accept the $ symbol to be at the end of the regex
            // the condition below raise an exception if it find any character after the $ symbol
            if (match_end)
                deleteNfa_and_raise(NfaStates, i - 1);

            if (is_skiped)
            {
                if (!is_special(s[i]))
                    deleteNfa_and_raise(NfaStates, i);
                else
                {
                    check_if_should_concatenate(last_operator_code, pending_operators);
                    last_operator_code = 0;
                    createNode(constructRange(s[i], true), NfaStates);
                }
                is_skiped = false;
            }
            else if (s[i] == '\\')
                is_skiped = true;
            else if (s[i] == '$')
                match_end = true;
            else if (s[i] == '^') // don't support '^' anywere in the regex except the begining
                deleteNfa_and_raise(NfaStates, i - 1);
            else if (s[i] == '[')
            {
                check_if_should_concatenate(last_operator_code, pending_operators);
                last_operator_code = 0;
                handle_range(i, s, NfaStates);
            }
            // the * + and ? have the highest priority so they will get executed immediately (no need to push to pending_operators)
            else if (s[i] == '*')
            {
                last_operator_code = executePriorityOperator(s[i], pending_operators, i, last_operator_code, NfaStates);
                star(i, NfaStates);
            } else if (s[i] == '+')
            {
                last_operator_code = executePriorityOperator(s[i], pending_operators, i, last_operator_code, NfaStates);
                oneOrPlus(i, NfaStates);
            }
            else if (s[i] == '?')
            {
                last_operator_code = executePriorityOperator(s[i], pending_operators, i, last_operator_code, NfaStates);
                noneOrOne(i, NfaStates);
            }
            else if (s[i] == '|')
            {
                last_operator_code = executePriorityOperator(s[i], pending_operators, i, last_operator_code, NfaStates);
                pending_operators.push(s[i]);
            }
            else if (s[i] == '(')
            {
                if (i + 1 < s.length() && s[i + 1] == ')')
                    deleteNfa_and_raise(NfaStates, i);
                check_if_should_concatenate(last_operator_code, pending_operators);
                // make last_operator_code negative so next char don't get concatinated with what came before (
                last_operator_code = -1;
                pending_operators.push(s[i]);
            }
            else if (s[i] == ')')
            {
                // execute all the operator pending until the last '('
                last_operator_code = executePriorityOperator(s[i], pending_operators, i, 0, NfaStates);
                if (pending_operators.empty() || pending_operators.top() != '(')
                    deleteNfa_and_raise(NfaStates, i);
                pending_operators.pop();
            }
            else if (s[i] == '.')
            {
                check_if_should_concatenate(last_operator_code, pending_operators);
                last_operator_code = 0;
                createNode(CharRange(MIN_CHAR, MAX_CHAR), NfaStates);
            }
            else {
                check_if_should_concatenate(last_operator_code, pending_operators);
                last_operator_code = 0;
                createNode(constructRange(s[i], false), NfaStates);
            }

        }
        executePriorityOperator('(', pending_operators, s.length(), 0, NfaStates);
        clock_t end = clock();
        std::cout << "Time taken For NFA: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
        if (!pending_operators.empty() || is_skiped || NfaStates.size() != 1)
            deleteNfa_and_raise(NfaStates, regex.length());
        start = clock();
        nfaToDfa(dfaStart, NfaStates);
        end = clock();
        std::cout << "Time taken For DFA: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
        deleteNfa(NfaStates);
    }
    bool    test(std::string const& s)
    {
        std::pair <std::string::const_iterator, std::string::const_iterator> res;
        res = findMatch(s);
        return res.first != res.second;
    }

    std::pair<std::string::const_iterator, std::string::const_iterator>
    findMatch(std::string const& s)
    {
        std::pair<std::string::const_iterator, std::string::const_iterator>
        res;
        bool    found = false;
        // if (dfaStart->is_accepted)
        // {
        //     res.first = s.begin();
        //     res.second = s.begin();
        //     found = true;
        // }

        for (size_t j = 0; j < s.size(); j++)
        {
            StateDFA* current = dfaStart;
            for (size_t i = j; i < s.size(); i++)
            {
                current = current->consume(s[i]);
                if (current == NULL)
                    break;
                if (current->is_accepted == true && (!match_end || i == s.length() - 1))
                {
                    res = std::make_pair(s.begin() + j, s.begin() + i + 1);
                    found = true;
                }
            }
            if (found)
                return res;
            if (match_start)
                break ;
        }
        return std::make_pair(s.end(), s.end());
    }

     ~Regex() {
        std::deque<State *> ptrToRemove;
        // fillSet(dfaStart, ptrToRemove);
        for (std::deque<State *>::iterator it = ptrToRemove.begin(); it != ptrToRemove.end(); it++)
        {
            delete *it;
        }
    };

    //hello(abc|[\d?]|ph*p)+\?    
    //dsafdjashello790898?898?kdjsakljasdk
    class InvalidRegexException: public std::exception
    {
        ssize_t             position;
        std::string         regex;
    public:
        InvalidRegexException(std::string const& regex, ssize_t position = -1): regex(regex) , position(position) {
            this->regex = ("\nInvalid Regex Expression: " + this->regex + "\n");
            for (size_t i = 0; i < position + 26; i++)
                this->regex += " ";
            this->regex += "^";
        }
        virtual const char *what() const throw()
        {
            return (this->regex.c_str());
        }
        ~InvalidRegexException() throw() {
        }
    };
private:
    int power(char c)
    {
        if (c == '[' || c == '.' || c == ']' || c == '^'
            || c == '$' || c == '\\' || c == '(' || c == ')')
            return 0;
        if (c == '|')
            return 1;
        if (c == 'c')
            return 2;
        if (c == '*' || c == '+' || c == '?')
            return 3;
        return -1;
    }

    bool valid_range(char c)
    {
        if (c == 'd')
            return true;
        if (c == 'D')
            return true;
        if (c == 'w')
            return true;
        if (c == 'W')
            return true;
        if (c == 's')
            return true;
        if (c == 'S')
            return true;
        return false;
    }

    bool    is_special(char c)
    {
        return power(c) != -1 || valid_range(c);
    }

    void    executeOperator(char c, size_t i, std::deque<StateLine> &states)
    {
        switch (c)
        {
        case '|':
            alternation(i, states);
            break;
        case 'c':
            concatenation(i, states);
            break;
        default:
            deleteNfa_and_raise(states, i);
        }
    }
    
    int    executePriorityOperator(char c, std::stack<char> &pending_operators, size_t i, int last_operator_code, std::deque<StateLine> &states)
    {
        if (last_operator_code && power(c) >= last_operator_code)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }
        while (!pending_operators.empty() && pending_operators.top() != '(' && power(pending_operators.top()) >= power(c))
        {
            char op = pending_operators.top();
            pending_operators.pop();
            executeOperator(op, i, states);
        }
        return power(c);
    }
    
    

    void    handle_range(size_t &i, std::string const& s, std::deque<StateLine> &states)
    {
        bool        skip = false;
        bool        inverse = false;
        bool        from_flag = false;
        char        from = 0;
        CharRange   range;
    
        i++;
        if (i < s.length() && s[i] == '^')
        {
            inverse = true;
            i++;
        }
        while (i < s.length() && (skip || s[i] != ']'))
        {
            if (skip)
            {
                if (isalpha(s[i]) && !valid_range(s[i]))
                    deleteNfa_and_raise(states, i);
                range.addRange(constructRange(s[i], skip));
                skip = false;
            }
            else if (s[i] == '\\')
                skip = true;
            else
            {
                if (from && from_flag)
                {
                    if (from > s[i])
                        deleteNfa_and_raise(states, i);
                    range.addRange(CharRange(from, s[i]));
                    from = 0;
                    from_flag = false;
                } 
                else if (!from)
                    from = s[i];
                else if (s[i] != '-')
                {
                    range.addRange(from);
                    from = s[i];
                }
                else
                    from_flag = true;
            }
            i++;
        }
        if (from)
            range.addRange(from);
        if (from_flag)
            range.addRange('-');
        if (inverse)
            range.inverseRange();
        if (skip || i == s.length() || range.chars.size() == 0)
            deleteNfa_and_raise(states, i);
        createNode(range, states);
    }

    void    deleteNfa_and_raise(std::deque<StateLine> &NfaStates, size_t i)
    {
        deleteNfa(NfaStates);
        throw InvalidRegexException(regex, i);
    }
    void    check_if_should_concatenate(int last_operator_code, std::stack <char> &pending_operators)
    {
        // always concatenate if the last operator is not '|' or at the beginning of the regex
        if (last_operator_code >= 0 && last_operator_code != power('|'))
            pending_operators.push('c');
    }

    CharRange constructRange(char c, bool skiped = false)
    {
        if (!skiped)
            return CharRange(c, c);
        CharRange   range;
        switch (c)
        {
        case 'd':
            range.addRange('0', '9');
            break;
        case 'D':
            range.addRange('0', '9');
            range.inverseRange();
            break;
        case 'w':
            range.addRange('0', '9');
            range.addRange('A', 'Z');
            range.addRange('a', 'z');
            range.addRange('_');
            break;
        case 'W':
            range.addRange('0', '9');
            range.addRange('A', 'Z');
            range.addRange('a', 'z');
            range.addRange('_');
            range.inverseRange();
            break;
        case 's':
            range.addRange(9, 13);
            range.addRange(32);
            break;
        case 'S':
            range.addRange(9, 13);
            range.addRange(32);
            range.inverseRange();
            break;
        default:
            return CharRange(c, c);
        }
        return range;
    }

    std::set <State*>    getEpsilonClosure(State* s, std::map<State*, std::set<State*> >& cache)
    {

        if (cache.find(s) != cache.end())
            return cache[s];
        std::set <State*>    res;
        std::deque<State*>   pending;
        pending.push_back(s);

        while (!pending.empty())
        {
            State* current = pending.front();
            pending.pop_front();
            if (res.count(current))
                continue ;
            res.insert(current);
            for (MapCharToState::iterator it = current->transitions.begin(); it != current->transitions.end(); it++)
            {
                if (it->first == EPSILON && it->second->is_visited == false)
                {
                    it->second->is_visited = true;
                    std::set <State*> tmp_st = getEpsilonClosure(it->second, cache);
                    pending.insert(pending.end(), tmp_st.begin(), tmp_st.end());
                }
            }
            for (MapCharToState::iterator it = current->transitions.begin(); it != current->transitions.end(); it++)
                    it->second->is_visited = false;


        }
        cache[s] = res;
        return res;
    }//hello(abc|[\d?]|ph*p)+\?

    std::set <State*>
    consumeChar(std::set<State*> &states, CharRange c, std::map<State*, std::set<State*> >& cache)
    {
        std::set <State*> res;
        for (std::set<State*>::iterator it = states.begin(); it != states.end(); it++)
        {
            for (MapCharToState::iterator it2 = (*it)->transitions.begin(); it2 != (*it)->transitions.end(); ++it2)
            {
                if (it2->first == c)
                {
                    std::set <State*> tmp_st = getEpsilonClosure(it2->second, cache);
                    res.insert(tmp_st.begin(), tmp_st.end());
                }
            }
        }
        return res;
    }
    
    std::deque<CharRange> getRangesFromStates(std::set<State*> &states)
    {
        std::deque<CharRange>   res;
        std::set<int>           visited;
        std::set<int>           duplicated;
    
        for (std::set<State*>::iterator it = states.begin(); it != states.end(); it++)
        {
            for (MapCharToState::iterator it2 = (*it)->transitions.begin(); it2 != (*it)->transitions.end(); ++it2) 
            {
                if (it2->first == EPSILON)
                    continue ;
                for (std::set<int>::iterator it3 = it2->first.chars.begin(); it3 != it2->first.chars.end(); ++it3)
                {
                    if (visited.count(*it3) == 0)
                        visited.insert(*it3);
                    else
                        duplicated.insert(*it3);
                }
                res.push_back(it2->first);
            }
        }
        // remove duplicated chars from the initial ranges
        for (std::deque<CharRange>::iterator it = res.begin(); it != res.end(); it++)
        {
            for (std::set<int>::iterator it2 = duplicated.begin(); it2 != duplicated.end(); ++it2)
            {
                if (it->chars.count(*it2) != 0)
                    it->chars.erase(*it2);
            }
        }
        // insert every duplicate as a new range
        for (std::set<int>::iterator it = duplicated.begin(); it != duplicated.end(); ++it)
            res.push_back(*it);
        
        return res;
    }
    // TODO: optimize this function
    void    nfaToDfa(StateDFA  *&dfaStart, std::deque<StateLine> &states)
    {
        typedef std::set <State*>                           StateSet;
        typedef std::map <StateSet, std::map<CharRange, StateSet> >  StateSetMP;

        State*                          Accepted = states[0][states[0].size() - 1];
        std::map<State*, StateSet>      cache;
        StateSet                        current_states = getEpsilonClosure(states[0][0], cache);
        std::deque<StateSet>            current_DFAstates;
        StateSetMP                      DFAstate_map;
        std::map <StateSet, StateDFA*>  state_map;

        current_DFAstates.push_back(current_states);
        while (!current_DFAstates.empty())
        {
            StateSet   current_DFAstate;
            StateSet   next_states;

            current_DFAstate = current_DFAstates.front();
            current_DFAstates.pop_front();
            
            // TODO: check if this doesn't do anything usefull
            // if (DFAstate_map.count(current_DFAstate))
            //     continue ;

            std::deque<CharRange> ranges = getRangesFromStates(current_DFAstate);
            for (std::deque<CharRange>::iterator it = ranges.begin(); it != ranges.end(); it++)
            {
                if (it->chars.size() == 0)
                    continue ;
                next_states = consumeChar(current_DFAstate, *it, cache);
                if (!next_states.empty() && !DFAstate_map.count(next_states))
                    current_DFAstates.push_back(next_states);
                (DFAstate_map[current_DFAstate])[*it] = next_states;
            }
            if (ranges.size() == 0)
                (DFAstate_map[current_DFAstate])[EPSILON] = StateSet();
        }

        State::count = 0;
        for (StateSetMP::iterator it = DFAstate_map.begin(); it != DFAstate_map.end(); it++)
            state_map[it->first] = new StateDFA(State::count++);
        dfaStart = state_map[current_states];

        // std::cout << "\nDFA" << std::endl;
        for (StateSetMP::iterator it = DFAstate_map.begin(); it != DFAstate_map.end(); it++)
        {
            if (it->first.count(Accepted))
                state_map[it->first]->is_accepted = true;
            // if (state_map[it->first]->is_accepted)
            //     std::cout << "*";
            // else
            //     std::cout << " ";
            // std::cout << state_map[it->first]->id << ": ";
            // std::cout << "{";
            
            for (std::map<CharRange, StateSet>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
            {
                if (it2->first == EPSILON)
                    continue ;
                for (std::set<int>::iterator it3 = it2->first.chars.begin(); it3 != it2->first.chars.end(); it3++)
                {
                    // std::cout << "(" << (char)*it3 << "," << state_map[it2->second]->id << ") ";
                    state_map[it->first]->addTransition(*it3, state_map[it2->second]);
                }
            }
            // for (int i = 0; i <= 255; i++)
            // {
            //     if (it->second[i].empty())
            //         continue ;
            //     State *next_state = state_map[it->second[i]];
            //     state_map[it->first]->addTransition(i - 128, next_state);
            //     // std::cout << '('<<(char)(i + MIN_CHAR) << ','<< next_state->id << ")";
            // }
            // std::cout << "}\n";
        }
        // std::cout << "HEAD= "<<dfaStart->id << std::endl;
    }

    void deleteNfa(std::deque<StateLine> &states)
    {
        for (size_t i = 0; i < states.size(); i++)
        {
            for (size_t j = 0; j < states[i].size(); j++)
            {
                delete states[i][j];
            }
        }
    }

    void fillSet(State *initState, std::deque<State *> &setPtrs)
    {
        if (initState == NULL || initState->deleted)
            return ;
        initState->deleted = true;
        for (MapCharToState::iterator it = initState->transitions.begin(); it != initState->transitions.end(); it++)
        {
            fillSet(it->second, setPtrs);
        }
        setPtrs.push_front(initState);
    }

   

    void    createNode(CharRange const& c, std::deque<StateLine> &states)
    {
        // std::cout << '\''<<*c.chars.begin()<<'-'<< *c.chars.rbegin() <<'\'' << " ";
        State* start = new State(State::count++);
        State* end = new State(State::count++);

        start->addTransition(c, end);

        StateLine line;
        line.push_back(start);
        line.push_back(end);
        states.push_back(line);
    }

    void    concatenation(int i, std::deque<StateLine> &states)
    {
        // std::cout << '.'<<" ";

        if (states.size() < 2)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }

        StateLine A = states.back();
        states.pop_back();
        StateLine B = states.back();
        states.pop_back();

        B.back()->addTransition(EPSILON, A.front());
        B.insert(B.end(), A.begin(), A.end());
        states.push_back(B);
    }

    void    alternation(int i, std::deque<StateLine> &states)
    {
        // std::cout << '|'<<" ";

        if (states.size() < 2)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }

        StateLine B = states.back();
        states.pop_back();
        StateLine A = states.back();
        states.pop_back();

        State* start = new State(State::count++);
        State* end = new State(State::count++);

        start->addTransition(EPSILON, A.front());
        start->addTransition(EPSILON, B.front());
        A.back()->addTransition(EPSILON, end);
        B.back()->addTransition(EPSILON, end);

        A.push_front(start);
        B.push_back(end);
        A.insert(A.end(), B.begin(), B.end());

        states.push_back(A);
    }

    void    star(int i, std::deque<StateLine> &states)
    {
        // std::cout << '*'<<" ";

        if (states.size() < 1)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }

        StateLine A = states.back();
        states.pop_back();

        State* start = new State(State::count++);
        State* end = new State(State::count++);

        start->addTransition(EPSILON, A.front());
        start->addTransition(EPSILON, end);
        A.back()->addTransition(EPSILON, A.front());
        A.back()->addTransition(EPSILON, end);
        A.push_front(start);
        A.push_back(end);
        states.push_back(A);
    }
    void    oneOrPlus(int i, std::deque<StateLine> &states)
    {

        if (states.size() < 1)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }

        StateLine A = states.back();
        states.pop_back();
        State *copyAState = A.front()->clone();
        StateLine copyA;
        fillSet(copyAState, copyA);
        states.push_back(copyA);
        states.push_back(A);
        star(i, states);
        concatenation(i, states);
    }

    void noneOrOne(int i, std::deque<StateLine> &states)
    {
        if (states.size() < 1)
        {
            deleteNfa(states);
            throw InvalidRegexException(regex, i);
        }

        StateLine A = states.back();
        states.pop_back();
        A.front()->addTransition(EPSILON, A.back());
        states.push_back(A);
    }

    
};


#include <time.h>
int main(int argc, char **argv)
{
    std::cout << (sizeof(StateDFA)) << std::endl;

    {    
        try
        {
        //     /* code */
        //     while (1)
        //     {
        //         /* code */
                // throw std::exception();
                std::string s, toTest;
                std::cin>> s;
                // measure time taking to construct a regex
                {

                clock_t start = clock();
                Regex r(s);
                clock_t end = clock();
                std::cout << "Time taken: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
                }
                Regex v(s);
                do
                {
                    std::cin >> toTest;
                    // toTest = "Sedutperspiciatisundeomnisistenatuserrorsitvoluptatemaccusantiumdoloremquelaudantium,totamremahelloperiam,eaqueipsaquaeabilloinventhellooreveritatisetquasiarchitectobeataevitaedictasuntexplicabo.Nemoenimipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?ipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?Atveroeosetaccusamusetiustoodiodignissiipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?Atveroeosetaccusamusetiustoodiodignissiipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacomipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?Atveroeosetaccusamusetiustoodiodignissimodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?Atveroeosetaccusamusetiustoodiodiipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?AtveroeosetaccusamusetiustoodiodignissignissiAtveroeosetaccusamusetiustoodiodignissimosducimusquiblanditiispraesentiumvoluptatumdelenitiatquecorruptiquosdoloresetquasmolestiasexcepturisintoccaecaticupiditatenonprovident,similiquesuntinculpaquiofficiadeseruntmollitiaanimi,idestlaborumetdolorumfuga.Etharumquidemrerumfacilisesicabo.Nemoenihello56?mipsamvoluptatemquiavoluptassitaspernaturautoditautfugit,sedquiaconsequunturmagnidoloreseosquirationevoluptatemsequinesciunt.Nequeporroquisquamest,quidoloremipsumquiadolorsitamet,consectetur,adipiscivelit,sedquianonnumquameiusmoditemporainciduntutlaboreetdoloremagnamaliquamquaeratvoluptatem.Utenimadminimaveniam,quisnostrumexercitationemullamcorporissuscipitlaboriosam,nisiutaliquidexeacommodiconsequatur?Quisautemveleumiurereprehenderitquiineavoluptatevelitessequamnihilmolestiaeconsequatur,velillumquidoloremeumfugiatquovoluptasnullapariatur?Atveroeosetaccusamusetiustoodiodignissimosducimusquiblanditiispraesentiumvoluptatumdelenitiatquecorruptiquosdoloresetquasmolestiasexcepturisintoccaecaticupiditatenonprovident,similiquesuntinculpaquiofficiadeseruntmollitiaanimi,idestlaborumetdolorumfuga.Etharumquidetetexpeditadistinctio.Namliberotempore,cumsolutanobisesteligendioptiocumquenihilimpeditquominusidquodmaximeplaceatfacere";

                    if (toTest == "exit")
                        break;
                    std::pair <std::string::const_iterator, std::string::const_iterator> res;
                    clock_t start = clock();

                    res = v.findMatch(toTest);
                    clock_t end = clock();
                    std::cout << "Time taken For Match: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

                    if (res.first != res.second)
                    {
                        std::cout << std::string(res.first, res.second) << std::endl;
                    }
                    else
                        std::cout << "no match" << std::endl;

                } while (1);

            // }
            
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    system("leaks a.out");


    // (['"])\w+\1
    // "hello'
}

// a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa


#undef MIN_CHAR
#undef MAX_CHAR

