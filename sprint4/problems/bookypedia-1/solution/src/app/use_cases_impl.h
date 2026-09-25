#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::Book& book) override;

    std::vector<domain::Author> GetAllAuthors() const override;
    std::vector<domain::Book> GetAllBooks() const override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) const override;
    std::optional<domain::Author> GetAuthorById(const domain::AuthorId& id) const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
