#include "HeaderParser.hpp"


// #include <fstream>
// #include <sstream>
// int main()
// {
//     std::fstream f("request");
//     std::stringstream ss;
//     ss << f.rdbuf();
//     std::string s = ss.str();
//     we::HeaderParser::Headers_t headers;
//     we::HeaderParser parser(&headers, 1500);
//     try
//     {
//         while (1)
//         {
//             std::string::iterator it = s.begin();
//             if (parser.append(it, s.end()))
//                 break;
//         }
//         // print all headers
//         we::HeaderParser::Headers_t::iterator it = headers.begin();
//         while (it != headers.end())
//         {
//             std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
//             ++it;
//         }
//     } catch (we::HTTPStatusException &e)
//     {
//         std::cout << e.statusCode << " " << e.what() <<std::endl;

//     }
//     catch (std::exception &e)
//     {
//         std::cout << "Exception: " << e.what() << std::endl;
//     }
// }








// GET               HTTP://ddds/index.html/../%26file///././//../         HTTP/1.1         
// Host: facebook.com
// Sec-Ch-Ua: "(Not(A:Brand";v="8", "Chromium";v="99"
// Sec-Ch-Ua-Mobile: ?0
// Sec-Ch-Ua-Platform: "macOS"
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4844.74 Safari/537.36
// Accept:     text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9       
// Sec-Fetch-Site
// Sec-Fetch-Mode: navigate
// Sec-Fetch-User: ?1
// Sec-Fetch-Site:    loooo    l    
// Sec-Fetch-Dest:                       
// Accept-Encoding
// Accept-Language: en-US,en;q=0.9
// Connection: close

// dklsjakljdsakljdsa
// dkjsahkjhdkjhdkjash
// dashjkhdsajkhjkdhsakjhdkjsahjkdsahkjhdsajkh