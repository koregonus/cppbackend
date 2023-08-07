#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
#include "../app/use_cases.h"

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
    std::optional<std::vector<std::string>> tags;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    int publication_year;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;

    // void FillBookData(show_single_book_t& book_data, int mode) const;
    std::string FillTitleData(std::string& original_title) const;
    int FillPubYearData(int original_pub_year) const;
    std::vector<std::string> FillTagsData(std::vector<std::string>& tags) const;


    std::optional<std::vector<std::string>> EnterTags() const;
    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor() const;
    std::optional<std::string> SelectBook() const;
    std::optional<std::string> SelectAuthorAdvanced(bool NeedCreation) const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<show_book_t> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui