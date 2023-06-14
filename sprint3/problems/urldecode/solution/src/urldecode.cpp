#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <iostream>

using namespace std::literals;


const std::string not_valid_chars("!#$&'()* ,/:;=?@[]");


char decode_single_char(char to_decode)
    {
        if(not_valid_chars.find(to_decode) != std::string::npos)
        {
            throw std::invalid_argument("invalid arg");
        }
        char ret = 0;
        if(to_decode < 0x3AU)
        {
            ret = to_decode - '0';
        }
        else if(to_decode > 0x41U && to_decode < 0x47U)
        {
            ret = to_decode - 0x40U + 0x9U;
        }
        else if(to_decode > 0x60U && to_decode < 0x67U)
        {
            ret = to_decode - 0x61U + 0xAU;
        }
        else
        {
            throw std::invalid_argument("invalid arg");
        }

        return ret;
    }

std::string UrlDecode(std::string_view str_from_target) {
    // Реализуйте функцию UrlDecode самостоятельно
    std::string ret;
        for(int i = 0; i < str_from_target.length(); i++)
        {
            if(str_from_target[i] == 37)
            {
                if(i > str_from_target.length() - 3)
                {
                    throw std::invalid_argument("invalid arg");
                }
                char buffer = ((decode_single_char(str_from_target[i+1])) * 16) + decode_single_char(str_from_target[i+2]);
                ret.push_back(buffer);
                i += 2;
            }
            else if(not_valid_chars.find(str_from_target[i]) != std::string::npos)
            {
                // std::cout << to_decode << std::endl;
                throw std::invalid_argument("invalid arg");
            }
            else if(str_from_target[i] == '+')
            {
                ret.push_back(' ');
            }
            else
            {
                ret.push_back(str_from_target[i]);
            }
        }
        return ret;
}
