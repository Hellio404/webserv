#include <functional>
#include <string>
#include <string.h>

namespace we
{
    class LessCaseInsensitive: public std::binary_function<std::string, std::string, bool>
    {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const;
    };
}