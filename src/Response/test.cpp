#include <iostream>
#include <vector>
#include <set>
#include <algorithm>



unsigned long long  range_utoa(std::string::const_iterator begin, std::string::const_iterator end, bool& valid)
{
    unsigned long long  ret = 0;
    for (std::string::const_iterator it = begin; it != end; ++it)
    {
        if (!isdigit(*it))
            throw std::runtime_error("Invalid range");
        unsigned long long prev_val = ret;
        ret *= 10;
        ret += *it - '0';
        if (ret < prev_val)
        {
            valid = false;
            return 0;
        }
    }
    valid = true;
    return ret;
}


typedef unsigned long long ull;
bool    handle_range(std::string const& range, std::vector<std::pair<ull, ull> > & vranges)
{
    
    std::set<std::pair <ull, ull> >    ranges;

    if (range.size() > 1024 || range.find("bytes=") != 0)
        return false;
    std::string::const_iterator it = range.begin() + 6;
    try
    {
        while (it != range.end())
        {
            bool                    valid = true;  
            std::pair <ull, ull>    r;
            std::string::const_iterator it_start = it;

            while (it != range.end() && *it != '-')
                ++it;
            if (it == it_start)
                r.first = 0;
            else
                r.first = range_utoa(it_start, it, valid);
            if (*it == '-')
                ++it;
            else if (it != range.end())
                return false;
            it_start = it;
            while (it != range.end() && *it != ',')
                ++it;
            if (it == it_start)
                r.second = -1;
            else
                r.second = range_utoa(it_start, it, valid);
            if (*it == ',')
                ++it;
            else if (it != range.end())
                return false;
            if (r.first >= r.second || !valid)
                continue;

            ranges.insert(r);
            if (ranges.size() > 255)
                return false;
        }
    }
    catch(...)
    {
        return false;
    }
    if (ranges.empty())
        return false;

    vranges.push_back(*ranges.begin());
    std::set<std::pair <ull, ull> >::const_iterator it_r = ranges.begin();
    std::vector<std::pair <ull, ull> >::const_iterator pit_r;
    ++it_r;
    for (; it_r != ranges.end(); ++it_r)
    {
        pit_r = vranges.end() - 1;
        if ((pit_r->second >= it_r->first || pit_r->second + 80 >= it_r->first) && pit_r->second >= it_r->second)
            continue;
        else if (pit_r->second >= it_r->first || pit_r->second + 80 >= it_r->first)
        {
            std::pair <ull, ull> new_r;
            new_r.first = pit_r->first;
            new_r.second = it_r->second;
            vranges.pop_back();
            vranges.push_back(new_r);
        }
        else
            vranges.push_back(*it_r);
    }
    return true;
}

int main()
{
    std::vector<std::pair<ull, ull> > vranges;
    handle_range("bytes=0-100,50-120,130-180,125-160,200-400,1-56456456456456465456456456465456465456465456456465456", vranges);
    for (std::vector<std::pair<ull, ull> >::const_iterator it = vranges.begin(); it != vranges.end(); ++it)
        std::cout << it->first << "-" << it->second << std::endl;
}