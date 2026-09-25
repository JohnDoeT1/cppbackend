#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::Book& book) {
    books_.Save(book);
}

std::vector<domain::Author> UseCasesImpl::GetAllAuthors() const {
    return authors_.GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetAllBooks() const {
    return books_.GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const domain::AuthorId& author_id) const {
    return books_.GetByAuthor(author_id);
}

std::optional<domain::Author> UseCasesImpl::GetAuthorById(const domain::AuthorId& id) const {
    return authors_.GetById(id);
}

}  // namespace app
