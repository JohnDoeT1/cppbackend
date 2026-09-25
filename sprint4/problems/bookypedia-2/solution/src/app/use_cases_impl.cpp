#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/unit_of_work.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    auto uow = factory_.Create();
    uow->Authors().Save({AuthorId::New(), name});
    uow->Commit();
}

void UseCasesImpl::AddBook(int publication_year, const std::string& title,
                           const std::string& author_name, const std::vector<std::string>& tags) {
    auto uow = factory_.Create();
    auto author = uow->Authors().GetByName(author_name);
    if (!author) {
        AuthorId new_id = AuthorId::New();
        uow->Authors().Save({new_id, author_name});
        author = Author{new_id, author_name};
    }
    uow->Books().Save({BookId::New(), author->GetId(), title, publication_year, tags});
    uow->Commit();
}

void UseCasesImpl::AddBook(int publication_year, const std::string& title, const AuthorId& author_id,
                           const std::vector<std::string>& tags) {
    auto uow = factory_.Create();
    uow->Books().Save({BookId::New(), author_id, title, publication_year, tags});
    uow->Commit();
}

std::vector<Author> UseCasesImpl::GetAllAuthors() const {
    auto uow = factory_.Create();
    auto result = uow->Authors().GetAll();
    // Не коммитим — только читали.
    return result;
}

std::optional<Author> UseCasesImpl::GetAuthorByName(const std::string& name) const {
    auto uow = factory_.Create();
    return uow->Authors().GetByName(name);
}

std::optional<Author> UseCasesImpl::GetAuthorById(const AuthorId& id) const {
    auto uow = factory_.Create();
    return uow->Authors().GetById(id);
}

std::vector<Book> UseCasesImpl::GetAllBooks() const {
    auto uow = factory_.Create();
    return uow->Books().GetAll();
}

std::vector<Book> UseCasesImpl::GetBooksByTitle(const std::string& title) const {
    auto uow = factory_.Create();
    return uow->Books().GetByTitle(title);
}

std::vector<Book> UseCasesImpl::GetAuthorBooks(const AuthorId& author_id) const {
    auto uow = factory_.Create();
    return uow->Books().GetByAuthor(author_id);
}

std::optional<Book> UseCasesImpl::GetBookById(const BookId& id) const {
    auto uow = factory_.Create();
    return uow->Books().GetById(id);
}

bool UseCasesImpl::DeleteAuthorByName(const std::string& name) {
    auto uow = factory_.Create();
    auto author = uow->Authors().GetByName(name);
    if (!author) return false;
    uow->Authors().Delete(author->GetId());
    uow->Commit();
    return true;
}

bool UseCasesImpl::DeleteAuthorById(const AuthorId& id) {
    auto uow = factory_.Create();
    auto author = uow->Authors().GetById(id);
    if (!author) return false;
    uow->Authors().Delete(id);
    uow->Commit();
    return true;
}

bool UseCasesImpl::EditAuthorById(const AuthorId& id, const std::string& new_name) {
    auto uow = factory_.Create();
    auto author = uow->Authors().GetById(id);
    if (!author) return false;
    author->SetName(new_name);
    uow->Authors().Save(*author);
    uow->Commit();
    return true;
}

bool UseCasesImpl::DeleteBookById(const BookId& id) {
    auto uow = factory_.Create();
    auto book = uow->Books().GetById(id);
    if (!book) return false;
    uow->Books().Delete(id);
    uow->Commit();
    return true;
}

bool UseCasesImpl::EditBookById(const BookId& id, const std::optional<std::string>& new_title,
                                const std::optional<int>& new_year,
                                const std::optional<std::vector<std::string>>& new_tags) {
    auto uow = factory_.Create();
    auto book = uow->Books().GetById(id);
    if (!book) return false;
    if (new_title) book->SetTitle(*new_title);
    if (new_year) book->SetPublicationYear(*new_year);
    if (new_tags) book->SetTags(*new_tags);
    uow->Books().Update(*book);
    uow->Commit();
    return true;
}

}  // namespace app
