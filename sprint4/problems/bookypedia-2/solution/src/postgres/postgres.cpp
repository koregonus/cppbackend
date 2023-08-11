#include "postgres.h"

#include <pqxx/zview.hxx>

#include <string>

#include <iostream>

#include <thread>

#include <chrono>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec("START TRANSACTION;");
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.exec("COMMIT;");
    work.commit();
}

std::vector<std::pair<std::string, std::string>> AuthorRepositoryImpl::ShowAuthors() {
    pqxx::read_transaction read_trans(connection_);
    auto query_text = "SELECT id, name FROM authors ORDER BY name;"_zv;
    std::vector<std::pair<std::string, std::string>> vec;
    for (auto [id, name] : read_trans.query<std::string, std::string>(query_text)) {
        vec.push_back(std::pair{name, (id)});
    }

    return vec;
}

std::vector<std::pair<std::string, int>> AuthorRepositoryImpl::ShowAuthorBooks(const std::string& author_id)
{
    pqxx::read_transaction read_trans(connection_);
    std::vector<std::pair<std::string, int>> vec = {};
    for (auto [title, year] : read_trans.query<std::string, int>("SELECT title, publication_year FROM books WHERE author_id=" + read_trans.quote(author_id)
                                                                       +  " ORDER BY publication_year ASC, title ASC")) {
        vec.push_back(std::pair{title, year});
    }
    return vec;
}

void AuthorRepositoryImpl::EditAuthor(const std::string& author_id, const std::string& new_name)
{
    pqxx::work work{connection_};
    work.exec("START TRANSACTION;");
    work.exec("UPDATE authors SET name=" + work.quote(new_name) + " WHERE id=" + work.quote(author_id) + ";");
    work.exec("COMMIT;");
    work.commit();
}

void AuthorRepositoryImpl::EditBook(const show_single_book_t& book_data, const std::string& book_id)
{
    pqxx::work work{connection_};
    work.exec("START TRANSACTION;");
    work.exec("UPDATE books SET title=" + work.quote(book_data.title) + " WHERE id=" + work.quote(book_id) + " ;");
    
    work.exec("UPDATE books SET publication_year=" + work.quote(std::to_string(book_data.publication_year)) + " WHERE id=" + work.quote(book_id) + " ;");
    if(book_data.tags.size() > 0)
    {
        work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(book_id) + " ;");
        for(auto& item : book_data.tags)
        {
            work.exec_params(
                R"(
            INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
            )"_zv,
                book_id, item);
        }
    }
    else if(book_data.tags.size() == 0)
    {
        work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(book_id) + " ;");
    }
    work.exec("COMMIT;");
    work.commit(); 
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& author_id)
{
    pqxx::read_transaction read_trans(connection_);
    std::vector<std::string> vec_books = {};
    for (auto [id] : read_trans.query<std::string>("SELECT id FROM books WHERE author_id=" + read_trans.quote(author_id) + ";")) {
        vec_books.push_back(id);
    }
    read_trans.commit();
    pqxx::work work{connection_};
    work.exec("START TRANSACTION;");
    for(auto& item : vec_books)
    {
        work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(item));
        work.exec("DELETE FROM books WHERE id=" + work.quote(item));
    }
    work.exec("DELETE FROM authors WHERE id=" + work.quote(author_id));
    work.commit();
}

void AuthorRepositoryImpl::DeleteBook(const std::string& book_id)
{
    pqxx::work work{connection_};
    work.exec("START TRANSACTION;");
    work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(book_id));
    work.exec("DELETE FROM books WHERE id=" + work.quote(book_id));
    work.exec("COMMIT;");
    work.commit(); 
}


std::vector<show_book_t> AuthorRepositoryImpl::ShowBooks()
{
    pqxx::read_transaction read_trans(connection_);
    // auto query_text = "SELECT title, publication_year FROM books ORDER BY title ASC"_zv;
    auto query_text = "SELECT books.title, authors.name, books.publication_year, books.id FROM authors, books WHERE authors.id=books.author_id ORDER BY books.title ASC, authors.name ASC, books.publication_year ASC;"_zv;
    std::vector<show_book_t> vec = {};
    for (auto [title, name, year, book_id] : read_trans.query<std::string, std::string, int, std::string>(query_text)) {
        vec.push_back({title, name, year, book_id});
    }
    return vec;
}

void AuthorRepositoryImpl::SaveBook(const domain::Book& book) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    // std::cout << book.GetAuthorId().ToString() << std::endl;
    
    work.exec("START TRANSACTION;");
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPubYear());
    if(book.GetTagsSize() > 0)
    {
        auto tags_local = book.GetTags();
        std::string query_text("INSERT INTO book_tags (book_id, tag) VALUES ");
        for(int i = 0; i < tags_local.size(); i++)
        {
            query_text.append("(" + work.quote(book.GetId().ToString()) + "," + work.quote(tags_local[i]) + ")");
            if(i == tags_local.size()-1)
            {
                query_text.append(";");
            }
            else
            {
                query_text.append(",");   
            }
            // work.exec_params(
            //     R"(
            // INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
            // )"_zv,
            //     book.GetId().ToString(), tags_local);
        }
        work.exec(query_text);
        // std::cout << query_text << std::endl;
    }
    work.exec("COMMIT;");
    work.commit();
}

void AuthorRepositoryImpl::SaveBook(const domain::Author& author, const domain::Book& book) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    
    work.exec("START TRANSACTION;");
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2);
)"_zv,
        author.GetId().ToString(), author.GetName());


    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPubYear());
    if(book.GetTagsSize() > 0)
    {
        auto tags_local = book.GetTags();
        std::string query_text("INSERT INTO book_tags (book_id, tag) VALUES ");
        for(int i = 0; i < tags_local.size(); i++)
        {
            query_text.append("(" + work.quote(book.GetId().ToString()) + "," + work.quote(tags_local[i]) + ")");
            if(i == tags_local.size()-1)
            {
                query_text.append(";");
            }
            else
            {
                query_text.append(",");   
            }
        }
        work.exec(query_text);
    }
    work.exec("COMMIT;");
    work.commit();
}

show_single_book_t AuthorRepositoryImpl::ShowBook(const std::string& book_id)
{
    // std::chrono::milliseconds timespan(100); // or whatever
    // std::this_thread::sleep_for(timespan);
    pqxx::read_transaction read_trans(connection_);
    show_single_book_t ret = {};
    auto first_row = read_trans.query1<std::string, std::string, int>("SELECT books.title, authors.name, books.publication_year FROM authors, books WHERE authors.id=books.author_id AND books.id=" + read_trans.quote(book_id) + " ;");
    ret.title = std::get<0>(first_row);
    ret.author_name = std::get<1>(first_row);
    ret.publication_year = std::get<2>(first_row);
    for (auto [tags] : read_trans.query<std::string>("SELECT tag FROM book_tags WHERE book_id=" + read_trans.quote(book_id) + " ;")) {
        ret.tags.push_back(tags);
        // std::cout << tags << std::endl;
    }
    return ret;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT firstindex PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL UNIQUE
);
)"_zv);
    // ... создать другие таблицы
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID PRIMARY KEY,
    title varchar(100) NOT NULL, publication_year INT, author_id UUID, CONSTRAINT fk_authors FOREIGN KEY(author_id) REFERENCES authors(id)
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID,
    tag varchar(30) NOT NULL, CONSTRAINT fk_books FOREIGN KEY(book_id) REFERENCES books(id)
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres