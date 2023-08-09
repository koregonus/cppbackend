#pragma once
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"
#include "../app/use_cases.h"

namespace domain {

namespace detail {
struct AuthorTag {};
struct BookTag {};
}  // namespace detail


using AuthorId = util::TaggedUUID<detail::AuthorTag>;
using BookId = util::TaggedUUID<detail::BookTag>;

class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

class Book {
public:
    Book(BookId id, std::string title, AuthorId author_id, int pub_year)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , pub_year_(pub_year){
    }

    Book(BookId id, std::string title, AuthorId author_id, int pub_year, std::vector<std::string> tags)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , pub_year_(pub_year)
        , tags_(std::move(tags)){
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const int GetPubYear() const noexcept {
        return pub_year_;
    }

    const size_t GetTagsSize() const noexcept{
        return tags_.size();
    }

    const std::vector<std::string> GetTags() const noexcept{
        return tags_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int pub_year_;
    std::vector<std::string> tags_ = {};
};

class AuthorRepository {
public:
    virtual void Save(const Author& author) = 0;
    virtual std::vector<std::pair<std::string, std::string>> ShowAuthors() = 0;
    virtual void SaveBook(const Book& book) = 0;
    virtual void SaveBook(const Author& author, const Book& book) = 0;
    virtual std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) = 0;
    virtual std::vector<show_book_t> ShowBooks() = 0;
    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void EditAuthor(const std::string& author_id, const std::string& new_name) = 0;
    virtual void EditBook(const show_single_book_t& book_data, const std::string& book_id) = 0;
    virtual show_single_book_t ShowBook(const std::string& book_id) = 0;

protected:
    ~AuthorRepository() = default;
};

}  // namespace domain
