#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors)
        : authors_{authors} {
    }

    void AddAuthor(const std::string& name) override;
    std::vector<std::pair<std::string, std::string>> ShowAuthors() override;
    void SaveBook(std::string& title, std::string& author_id, int pub_year) override;
    std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) override;
    std::vector<std::pair<std::string, int>> ShowBooks() override;

private:
    domain::AuthorRepository& authors_;
};

}  // namespace app
