#include "postgres.h"

#include <pqxx/zview.hxx>

#include <string>

#include <iostream>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<std::pair<std::string, std::string>> AuthorRepositoryImpl::ShowAuthors() {
    pqxx::read_transaction read_trans(connection_);
    auto query_text = "SELECT name, id FROM authors ORDER BY name ASC"_zv;
    std::vector<std::pair<std::string, std::string>> vec;
    for (auto [name, id] : read_trans.query<std::string, std::string>(query_text)) {
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
    work.exec("UPDATE authors SET name=" + work.quote(new_name) + " WHERE id=" + work.quote(author_id));
    work.commit();
}

void AuthorRepositoryImpl::EditBook(const show_single_book_t& book_data, const std::string& book_id)
{
    pqxx::work work{connection_};
    
    work.exec("UPDATE books SET title=" + work.quote(book_data.title) + " WHERE id=" + work.quote(book_id));
    
    work.exec("UPDATE books SET publication_year=" + work.quote(std::to_string(book_data.publication_year)) + " WHERE id=" + work.quote(book_id) + ";");
    if(book_data.tags.size() > 0)
    {
        work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(book_id));
        for(auto& item : book_data.tags)
        {
            work.exec_params(
                R"(
            INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)
            ON CONFLICT (tag) DO UPDATE SET tag=$2;
            )"_zv,
                book_id, item);
        }
    }
    work.commit(); 
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& author_id)
{
    pqxx::read_transaction read_trans(connection_);
    std::vector<std::string> vec_books = {};
    for (auto [id] : read_trans.query<std::string>("SELECT id FROM books WHERE author_id=" + read_trans.quote(author_id))) {
        vec_books.push_back(id);
    }
    read_trans.commit();
    pqxx::work work{connection_};
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
    work.exec("DELETE FROM book_tags WHERE book_id=" + work.quote(book_id));
    work.exec("DELETE FROM books WHERE id=" + work.quote(book_id));
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
    
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET title=$3;
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPubYear());
    if(book.GetTagsSize() > 0)
    {
        auto tags_local = book.GetTags();

        for(auto& item : tags_local)
        {
            work.exec_params(
                R"(
            INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)
            )"_zv,
                book.GetId().ToString(), item);
        }
    }
    work.commit();
}

show_single_book_t AuthorRepositoryImpl::ShowBook(const std::string& book_id)
{
    pqxx::read_transaction read_trans(connection_);
    show_single_book_t ret = {};
    auto first_row = read_trans.query1<std::string, std::string, int>("SELECT books.title, authors.name, books.publication_year FROM authors, books WHERE authors.id=books.author_id AND books.id=" + read_trans.quote(book_id));
    ret.title = std::get<0>(first_row);
    ret.author_name = std::get<1>(first_row);
    ret.publication_year = std::get<2>(first_row);
    for (auto [tags] : read_trans.query<std::string>("SELECT tag FROM book_tags WHERE book_id=" + read_trans.quote(book_id))) {
        ret.tags.push_back(tags);
    }
    return ret;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    // ... создать другие таблицы
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY, author_id UUID,
    title varchar(100) NOT NULL, publication_year integer NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID,
    tag varchar(30) NOT NULL PRIMARY KEY
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres