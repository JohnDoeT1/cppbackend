#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../domain/unit_of_work.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::UnitOfWorkFactory& factory)
        : factory_{factory} {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(int publication_year, const std::string& title, const std::string& author_name,
                 const std::vector<std::string>& tags) override;
    void AddBook(int publication_year, const std::string& title, const domain::AuthorId& author_id,
                 const std::vector<std::string>& tags) override;

    std::vector<domain::Author> GetAllAuthors() const override;
    std::optional<domain::Author> GetAuthorByName(const std::string& name) const override;
    std::optional<domain::Author> GetAuthorById(const domain::AuthorId& id) const override;

    std::vector<domain::Book> GetAllBooks() const override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) const override;
    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) const override;
    std::optional<domain::Book> GetBookById(const domain::BookId& id) const override;

    bool DeleteAuthorByName(const std::string& name) override;
    bool DeleteAuthorById(const domain::AuthorId& id) override;
    bool EditAuthorById(const domain::AuthorId& id, const std::string& new_name) override;

    bool DeleteBookById(const domain::BookId& id) override;
    bool EditBookById(const domain::BookId& id, const std::optional<std::string>& new_title,
                      const std::optional<int>& new_year,
                      const std::optional<std::vector<std::string>>& new_tags) override;

private:
    domain::UnitOfWorkFactory& factory_;
};

}  // namespace app
