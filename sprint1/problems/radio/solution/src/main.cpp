#include "audio.h"
#include <iostream>

#include <boost/asio.hpp>
#include <array>
#include <string>
#include <string_view>

namespace net = boost::asio;
// TCP больше не нужен, импортируем имя UDP
using net::ip::udp;

using namespace std::literals;


void StartServer(uint16_t port)
{
    Player player(ma_format_u8, 1);
    static const size_t max_buffer_size = 65535;
    static const int SampleRate = 44100;
    typedef std::chrono::duration<float> fsec;
    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
        std::array<char, max_buffer_size> recv_buf;
        udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
        auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

        std::cout << "Client said smthg"sv /*<< std::string_view(recv_buf.data(), size)*/ << std::endl;

        fsec time{((float)size/(float)SampleRate)};
        // std::chrono::duration<std::chrono::duration::seconds> time = std::chrono::duration_cast<std::chrono::duration::seconds>(size/SampleRate);

        player.PlayBuffer(recv_buf.data(), size, time);
        std::cout << "Playing done" << std::endl;
            // Отправляем ответ на полученный endpoint, игнорируя ошибку.
            // На этот раз не отправляем перевод строки: размер датаграммы будет получен автоматически.
            // boost::system::error_code ignored_error;
            // socket.send_to(boost::asio::buffer("Hello from UDP-server"sv), remote_endpoint, 0, ignored_error);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        }
}

void StartClient(uint16_t port)
{
    Recorder recorder(ma_format_u8, 1);
    while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address("127.0.0.1", ec), port);
        socket.send_to(net::buffer(&rec_result.data[0], rec_result.frames*recorder.GetFrameSize()), endpoint);
    }
}

int main(int argc, char** argv) {

    static const uint16_t port = 3333;

    if(std::string_view(argv[1]) == "client")
    {
        std::cout << "Client mode activated!" << std::endl;
        StartClient(port);
    }
    else if(std::string_view(argv[1]) == "server")
    {
        std::cout << "Server mode activated!" << std::endl;
        StartServer(port);
    }

    std::cout << "need arg: client or server"sv << std::endl;

    return 0;
}
