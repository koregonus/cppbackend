#pragma once

#include <string>
#include <vector>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<std::pair<std::string, std::string>> ShowAuthors() = 0;
    virtual void SaveBook(std::string& title, std::string& author_id, int pub_year) = 0;
    virtual std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) = 0;
    virtual std::vector<std::pair<std::string, int>> ShowBooks() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
