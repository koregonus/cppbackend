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

std::vector<std::pair<std::string, int>> AuthorRepositoryImpl::ShowBooks()
{
    pqxx::read_transaction read_trans(connection_);
    auto query_text = "SELECT title, publication_year FROM books ORDER BY title ASC"_zv;
    std::vector<std::pair<std::string, int>> vec = {};
    for (auto [title, year] : read_trans.query<std::string, int>(query_text)) {
        vec.push_back(std::pair{title, year});
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
    work.commit();
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
    title varchar(100) UNIQUE NOT NULL, publication_year integer NOT NULL
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres