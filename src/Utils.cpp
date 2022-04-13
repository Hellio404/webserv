#include "Utils.hpp"

namespace we
{
    bool LessCaseInsensitive::operator()(const std::string& lhs, const std::string& rhs) const
    {
        return (lhs.size() < rhs.size() || (lhs.size() == rhs.size() &&
                strcasecmp(lhs.c_str(), rhs.c_str()) < 0));
    }
}