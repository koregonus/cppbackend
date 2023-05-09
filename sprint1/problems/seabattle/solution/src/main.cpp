#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        // TODO: реализуйте самостоятельно

        net::streambuf stream_buf;
        boost::system::error_code ec;
        // Работаем с socket
        while(!IsGameEnded())
        {
            PrintFields();
            if(my_initiative)
            {
                std::string ret_buf = ReadMove();
                std::string_view view{ret_buf};
                if(WriteExact(socket, view))
                {
                    bool temp = ReadResult(view, socket);
                    // if(temp)
                    // {
                    //     std::cout << "temp\n";
                        
                    // }
                    my_initiative = temp;
                    
                }
            }
            else
            {
                std::optional<std::string> ret = ReadExact<2>(socket);
                std::cout << ret.value();
                if(ret == std::nullopt)
                {
                    std::cout << "ooops_1\n";
                    break;
                }
                else
                {
                    std::string_view view{ret.value()};
                    std::cout << "what? " << ret.value() << std::endl;
                // size_t x = static_cast<size_t>(ret.value()[0]);
                // size_t y = static_cast<size_t>(ret.value()[1]);
                    std::optional<std::pair<int, int>> parsed = ParseMove(ret.value());
                    if(parsed == std::nullopt)
                    {
                        std::cout << "ooops_2\n";
                        // break;
                    }
                    else
                    {
                        int result = (int)my_field_.Shoot(parsed.value().second, parsed.value().first);
                        std::cout << result << " res" << std::endl;
                        SendResult(socket, result);
                        if(result == 0)
                            my_initiative = true;
                    }
                    
                }  
            }

            
            
            // net::read_until(socket, stream_buf, '\n', ec);
            

            if (ec) {
                std::cout << "Error reading data"sv << std::endl;
                return;
            }

            // std::cout << "Client said: "sv << client_data << std::endl;

            // socket.write_some(net::buffer("Hello, I'm server!\n"sv), ec);
            // ReadMove();
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        std::cout << "x:" << p1 << std::endl;
        std::cout << "y:" << p2 << std::endl;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    static std::string ReadMove()
    {
        std::cout << "Your turn:";
        std::string str;
        std::getline(std::cin, str);
        std::cout << str << std::endl;
        // std::string_view str_sv{str};
        return str;
    }


    bool ReadResult(std::string_view& view, tcp::socket& socket)
    {
        char retval = true;
        std::cout << "Enemy turn!\n";
        std::optional<std::string> ret = ReadExact<1>(socket);
        char trigger = (char)ret.value()[0];
        std::cout << "read_rs" << trigger << std::endl;
        if(ret == std::nullopt)
        {
            std::cout << "ooops_3\n";
            retval = false;
        }
        else
        {
            std::optional<std::pair<int, int>> parsed = ParseMove(view);
            if(parsed == std::nullopt)
            {
                std::cout << "ooops_4\n";
                return false;
            }
            switch((ret.value()[0]-'0'))
            {
                case 0: std::cout << "MISS\n";
                    other_field_.MarkMiss(parsed.value().second, parsed.value().first);
                    retval = false;
                    break;
                case 1: std::cout << "HIT\n";
                    other_field_.MarkHit(parsed.value().second, parsed.value().first);
                    break;
                case 2: std::cout << "KILLED\n";
                    other_field_.MarkKill(parsed.value().second, parsed.value().first);
                    break;
                default: std::cout << "wtf " << (char)ret.value()[0] << "\n";
                    break;
            }
            
            // int result = (int)other_field_.Shoot(parsed.value().first, parsed.value().second);
        }
        // other_field_.

        return retval;
    }

    void SendMove(tcp::socket& socket)
    {
        ;
    }

    void SendResult(tcp::socket& socket, int result)
    {
        std::string ret_buf = std::to_string(result);
        std::cout << "send: " << ret_buf << std::endl;
        std::string_view view{ret_buf};
        WriteExact(socket, view);
    }

    // TODO: добавьте методы по вашему желанию

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);


    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(net::ip::make_address("127.0.0.1"), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
