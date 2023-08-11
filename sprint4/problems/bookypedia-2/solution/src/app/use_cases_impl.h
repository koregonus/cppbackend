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
    void SaveBook(std::string& title, std::string& author_id, int pub_year, std::optional<std::vector<std::string>> tags) override;
    void SaveBook(std::string& author_name, std::string& title, std::string& author_id, int pub_year, std::optional<std::vector<std::string>> tags) override;
    std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) override;
    std::vector<show_book_t> ShowBooks() override;
    void DeleteAuthor(const std::string& author_id) override;
    void EditAuthor(const std::string& author_id, const std::string& new_name) override;
    show_single_book_t ShowBook(const std::string& name) override;
    void DeleteBook(const std::string& book_id) override;
    void EditBook(const show_single_book_t& book_data, const std::string& book_id) override;
    std::string GetUUID() override;

private:
    domain::AuthorRepository& authors_;
};

}  // namespace app
