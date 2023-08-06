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

void UseCasesImpl::SaveBook(std::string& title, std::string& author_id, int pub_year)
{
    authors_.SaveBook({BookId::New(), title, AuthorId::FromString(author_id), pub_year});
}

std::vector<std::pair<std::string, int>> UseCasesImpl::ShowAuthorBooks(const std::string& author_id)
{
    return authors_.ShowAuthorBooks(author_id);
}

std::vector<std::pair<std::string, int>> UseCasesImpl::ShowBooks()
{
    return authors_.ShowBooks();
}

}  // namespace app
