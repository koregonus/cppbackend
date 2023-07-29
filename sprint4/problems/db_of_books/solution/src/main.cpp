#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <boost/json.hpp>

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        // // Подключаемся к БД, указывая её параметры в качестве аргумента
        pqxx::connection conn{argv[1]};

        // // Создаём транзакцию. Это понятие будет разобрано в следующих уроках.
        // // Транзакция нужна, чтобы выполнять запросы.
        pqxx::work w(conn);

        // // Используя транзакцию создадим таблицу в выбранной базе данных:
        w.exec(
            "CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL ,year integer NOT NULL, ISBN char(13) UNIQUE);"_zv);
        w.commit();

        constexpr auto tag_add_book = "add_book_trans"_zv;
        constexpr auto tag_add_book_null_isbn = "add_book_null_isbn_trans"_zv;
        conn.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);
        conn.prepare(tag_add_book_null_isbn, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)"_zv);

        constexpr auto result_ok = R"({"result": true})"sv;
        constexpr auto result_not_ok = R"({"result": false})"sv;

        std::string request;

        while(1)
        {
            std::getline(std::cin, request);
            auto value = boost::json::parse(request);
            std::string req_action = boost::json::value_to<std::string>(value.at("action"));
            if(req_action == "exit")
            {
                break;
            }
            if(req_action == "add_book")
            {
                auto payload = value.at("payload");
                auto title = boost::json::value_to<std::string>(payload.at("title"));
                auto author = boost::json::value_to<std::string>(payload.at("author"));
                auto year = boost::json::value_to<int>(payload.at("year"));
                std::string isbn;
                if(!payload.at("ISBN").is_null())
                {
                    isbn = boost::json::value_to<std::string>(payload.at("ISBN"));
                }

                std::cout << title << std::endl;
                std::cout << author << std::endl;
                std::cout << year << std::endl;
                std::cout << isbn << std::endl;

                bool not_okich = false;

                pqxx::work add_trans(conn);
                if(!payload.at("ISBN").is_null())
                {
                    try
                    {
                        add_trans.exec_prepared(tag_add_book, title, author, year, isbn);
                    }
                    catch(...)
                    {
                        not_okich = true;
                    }
                }
                else
                {
                    try
                    {
                        add_trans.exec_prepared(tag_add_book_null_isbn, title, author, year);
                    }
                    catch(...)
                    {
                        not_okich = true;
                    }
                }
                try
                {
                    if(not_okich)
                    {
                        std::cout << result_not_ok << std::endl;
                    }
                    else
                    {
                        add_trans.commit();
                        std::cout << result_ok << std::endl;
                    }
                    
                }
                catch(...)
                {
                    std::cout << result_not_ok << std::endl;
                }
            }
            if(req_action == "all_books")
            {
                pqxx::read_transaction read_trans(conn);
                {
                    auto query_text = "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv;

                    boost::json::array arr;
                    // Выполняем запрос и итерируемся по строкам ответа
                    for (auto [id, title, author, year, ISBN] : read_trans.query<int, std::string, std::string, int, std::optional<std::string>>(query_text)) {
                        boost::json::object obj;
                        obj["id"] = id;
                        obj["title"] = title;
                        obj["author"] = author;
                        obj["year"] = year;
                        if(ISBN != std::nullopt)
                        {
                            obj["ISBN"] = *ISBN;
                        }
                        else
                        {
                            obj["ISBN"] = nullptr;
                        }

                        arr.push_back(obj);
                    }
                    std::cout << arr << std::endl;
                }
            }
            request.clear();

        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}