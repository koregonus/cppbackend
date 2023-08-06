#pragma once
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"

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
        , pub_year_(pub_year) {
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

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int pub_year_;
};

class AuthorRepository {
public:
    virtual void Save(const Author& author) = 0;
    virtual std::vector<std::pair<std::string, std::string>> ShowAuthors() = 0;
    virtual void SaveBook(const Book& book) = 0;
    virtual std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) = 0;
    virtual std::vector<std::pair<std::string, int>> ShowBooks() = 0;

protected:
    ~AuthorRepository() = default;
};

}  // namespace domain
