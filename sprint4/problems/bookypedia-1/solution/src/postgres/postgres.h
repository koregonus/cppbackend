#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

#include "../domain/author.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;

    std::vector<std::pair<std::string, std::string>> ShowAuthors() override;

    void SaveBook(const domain::Book& book) override;

    std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) override;
    
    std::vector<std::pair<std::string, int>> ShowBooks() override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
};

}  // namespace postgres