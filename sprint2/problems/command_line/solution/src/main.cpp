#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "json_loader.h"
#include "request_handler.h"

#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>

#include <boost/json.hpp>

#include "application.h"

#include <boost/program_options.hpp>

using namespace std::literals;
namespace net = boost::asio;
namespace json = boost::json;

namespace {

namespace sys = boost::system;
namespace http = boost::beast::http;


// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

struct Args {
    // std::filesystem::path& json_path;
    std::string json_path;
    std::string path_from_arg;
    bool randomize_spawn;
    std::string tick_period;
    bool tick_set;
};


[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};
    Args args;
    desc.add_options()           //
        ("help,h", "Show help")  //
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")  //
        ("config-file,c", po::value(&args.json_path)->value_name("file"s),
         "set config file path") //
        ("www-root,w", po::value(&args.path_from_arg)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        if (vm.contains("help"s)) {
            std::cout << desc;
            return std::nullopt;
        }
        if(!vm.contains("config-file"))
        {
            throw std::runtime_error("JSON config files have not been specified");
        }
        if(!vm.contains("www-root"))
        {
            throw std::runtime_error("Static server files have not been specified");
        }
        if(vm.contains("tick-period"))
        {
            args.tick_set = true;
        }
        if(vm.contains("randomize-spawn-points"))
        {
            args.randomize_spawn = true;
        }



        
        // return std::nullopt;
        return args;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

int main(int argc, const char* argv[]) {
    // if (argc != 3) {
    //     std::cerr << "Usage: game_server <game-config-json> <static-src_dir>"sv << std::endl;
    //     return EXIT_FAILURE;
    // }
    try{
        if(auto args = ParseCommandLine(argc, argv))
        {
            try {
        // 1. Загружаем карту из файла и построить модель игры
                const auto address = net::ip::make_address("0.0.0.0");
                constexpr unsigned short port = 8080;
                model::Game game = json_loader::LoadGame((*args).json_path);


                model::Players players;

                application::ApplicationFacade AppFacade(game, players);


                // model::PlayerTokens tokens;
                // std::cout << tokens.generate_token() << std::endl;
                // std::cout << game.AddPlayer("01", "02") << std::endl;



                // 2. Инициализируем io_context
                const unsigned num_threads = std::thread::hardware_concurrency();
                net::io_context ioc{(int)num_threads};

                // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
                net::signal_set signals(ioc, SIGINT, SIGTERM);
                signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                    if (!ec) {
                        ioc.stop();
                    }
                });

                // Создаём strand для обработки запросов к API
                auto api_strand = net::make_strand(ioc);
                // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры

                int timer_period = 0;
                // std::shared_ptr<http_handler::Ticker> timer;
                try{
                    timer_period = std::stoi((*args).tick_period);
                    if((*args).tick_set)
                    {
                        // std::cout << "set timer\n";
                        AppFacade.AutoTimerModeEnable();       
                    }
                    
                }
                catch(...)
                {
                    timer_period = 0;
                }

                if((*args).randomize_spawn)
                {
                    AppFacade.SetRandomSpawn();
                }

                auto timer = std::make_shared<http_handler::Ticker>(api_strand, std::chrono::milliseconds(timer_period), (*args).tick_set, [&AppFacade](auto arg){
                            AppFacade.TimerTickAuto(arg);
                });
                timer->Start();


                
                
                // Создаём обработчик запросов в куче, управляемый shared_ptr
                auto handler = std::make_shared<http_handler::RequestHandler>(game, players, AppFacade, api_strand, (*args).path_from_arg);/*прочие параметры, нужные RequestHandler*/
                // http_handler::RequestHandler handler{game/*, api_strand*/, argv[2]};

                // 4*. Создаём декоратор для логгера

                http_handler::LoggingRequestHandler logging_handler{*handler};

                // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
                
                // http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                //     handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
                // });

                http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto addr, auto&& send) {
                    logging_handler(std::forward<decltype(req)>(req), addr, std::forward<decltype(send)>(send));
                });

                // Запускаем обработку запросов
                // http_server::ServeHttp(ioc, {address, port}, logging_handler);
                

                // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
                json::value custom_data{{"port"s, port},{"address"s, address.to_string()}};
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "server started"sv;

                // std::cout << "Server has started..."sv << std::endl;
                

                // 6. Запускаем обработку асинхронных операций
                RunWorkers(std::max(1u, num_threads), [&ioc] {
                    ioc.run();
                });
            } catch (const std::exception& ex) {
                // std::cerr << ex.what() << std::endl;
                json::value custom_data{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}};
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "server exited"sv;
                return EXIT_FAILURE;
            }

            // std::cout << "Server has stopped..."sv << std::endl;
            json::value custom_data{{"code"s, 0}};
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "server exited"sv;
                }
    }
    catch(const std::exception& e){
        std::cout << e.what() << std::endl;
    }
    
    


    

    // original solution
    // if (argc != 3) {
    //     std::cerr << "Usage: game_server <game-config-json> <static-src_dir>"sv << std::endl;
    //     return EXIT_FAILURE;
    // }

    // const auto address = net::ip::make_address("0.0.0.0");
    // constexpr unsigned short port = 8080;

    // try {
    //     // 1. Загружаем карту из файла и построить модель игры
    //     model::Game game = json_loader::LoadGame(argv[1]);


    //     model::Players players;

    //     application::ApplicationFacade AppFacade(game, players);


    //     // model::PlayerTokens tokens;
    //     // std::cout << tokens.generate_token() << std::endl;
    //     // std::cout << game.AddPlayer("01", "02") << std::endl;



    //     // 2. Инициализируем io_context
    //     const unsigned num_threads = std::thread::hardware_concurrency();
    //     net::io_context ioc{(int)num_threads};

    //     // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
    //     net::signal_set signals(ioc, SIGINT, SIGTERM);
    //     signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
    //         if (!ec) {
    //             ioc.stop();
    //         }
    //     });

    //     // Создаём strand для обработки запросов к API
    //     auto api_strand = net::make_strand(ioc);
    //     // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры

    //     // http_handler::Ticker timer(api_strand, std::chrono::milliseconds(1000), [](auto arg){});

    //     auto timer = std::make_shared<http_handler::Ticker>(api_strand, std::chrono::milliseconds(1000), [&AppFacade](auto arg){
    //         AppFacade.TimerTickAuto(arg);
    //     });
    //     // Создаём обработчик запросов в куче, управляемый shared_ptr
    //     auto handler = std::make_shared<http_handler::RequestHandler>(game, players, AppFacade, api_strand, argv[2]);/*прочие параметры, нужные RequestHandler*/
    //     // http_handler::RequestHandler handler{game/*, api_strand*/, argv[2]};

    //     // 4*. Создаём декоратор для логгера

    //     http_handler::LoggingRequestHandler logging_handler{*handler};

    //     // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        
    //     // http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
    //     //     handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
    //     // });

    //     http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto addr, auto&& send) {
    //         logging_handler(std::forward<decltype(req)>(req), addr, std::forward<decltype(send)>(send));
    //     });

    //     // Запускаем обработку запросов
    //     // http_server::ServeHttp(ioc, {address, port}, logging_handler);
        

    //     // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    //     json::value custom_data{{"port"s, port},{"address"s, address.to_string()}};
    //     BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
    //                         << "server started"sv;

    //     // std::cout << "Server has started..."sv << std::endl;
    //     timer->Start();

    //     // 6. Запускаем обработку асинхронных операций
    //     RunWorkers(std::max(1u, num_threads), [&ioc] {
    //         ioc.run();
    //     });
    // } catch (const std::exception& ex) {
    //     // std::cerr << ex.what() << std::endl;
    //     json::value custom_data{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}};
    //     BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
    //                         << "server exited"sv;
    //     return EXIT_FAILURE;
    // }

    // // std::cout << "Server has stopped..."sv << std::endl;
    // json::value custom_data{{"code"s, 0}};
    // BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
    //                         << "server exited"sv;
}
