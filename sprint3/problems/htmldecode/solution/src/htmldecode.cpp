#include "htmldecode.h"

#include <map>

#include <iostream>

std::map<std::string, std::string, std::less<>> m;

const std::map<std::string, std::string> mnemonics = {{"amp", "&"},
                                                        {"AMP","&"},
                                                        {"lt","<"},
                                                        {"LT","<"},
                                                        {"gt",">"},
                                                        {"GT",">"},
                                                        {"pos","'"},
                                                        {"POS","'"},
                                                        {"quot","%%\""},
                                                        {"QUOT","\""},
                                                                };

std::string HtmlDecode(std::string_view str) {
    // Ќапишите недостающий код самосто€тельно
    std::string ret;
    std::string buf(str);

    for(int i = 0; i < str.length(); i++)
    {
        if(str[i] == '&')
        {

            if(mnemonics.contains((buf.substr(i+1,2))) && i < buf.length()-2)
            {
                ret.append(mnemonics.at(buf.substr(i+1,2)));
                i += buf.substr(i+1,2).length();
                if(buf[i+1] == ';')
                    i++;
            }
            else if(mnemonics.contains((buf.substr(i+1,3))) && i < buf.length()-3)
            {
                ret.append(mnemonics.at(buf.substr(i+1,3)));
                i += buf.substr(i+1,3).length();
                if(buf[i+1] == ';')
                    i++;
            }
            else if(mnemonics.contains((buf.substr(i+1,4))) && i < buf.length()-4)
            {
                ret.append(mnemonics.at(buf.substr(i+1,4)));
                i += buf.substr(i+1,4).length();
                if(buf[i+1] == ';')
                    i++;
            }
            else
                ret.append(&buf[i], 1);
        }
        else
            ret.append(&buf[i], 1);
    }
    
    return ret;
}
