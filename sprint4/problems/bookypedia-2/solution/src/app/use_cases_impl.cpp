#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::ShowAuthors() {
    return authors_.ShowAuthors();
}

void UseCasesImpl::SaveBook(std::string& title, std::string& author_id, int pub_year, std::optional<std::vector<std::string>> tags)
{
    if(tags)
    {
        authors_.SaveBook({BookId::New(), title, AuthorId::FromString(author_id), pub_year, *tags});
    }
    else
    {
        authors_.SaveBook({BookId::New(), title, AuthorId::FromString(author_id), pub_year});
    }
}

std::vector<std::pair<std::string, int>> UseCasesImpl::ShowAuthorBooks(const std::string& author_id)
{
    return authors_.ShowAuthorBooks(author_id);
}

std::vector<show_book_t> UseCasesImpl::ShowBooks()
{
    return authors_.ShowBooks();
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id)
{
    return authors_.DeleteAuthor(author_id);
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name)
{
    return authors_.EditAuthor(author_id, new_name);
}

void UseCasesImpl::DeleteBook(const std::string& book_id)
{
    return authors_.DeleteBook(book_id);
}

show_single_book_t UseCasesImpl::ShowBook(const std::string& book_id)
{
    return authors_.ShowBook(book_id);
}

void UseCasesImpl::EditBook(const show_single_book_t& book_data, const std::string& book_id)
{
    return authors_.EditBook(book_data, book_id);
}

}  // namespace app
