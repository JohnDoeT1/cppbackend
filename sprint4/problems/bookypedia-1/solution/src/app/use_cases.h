#pragma once
#include <optional>
#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const domain::Book& book) = 0;

    virtual std::vector<domain::Author> GetAllAuthors() const = 0;
    virtual std::vector<domain::Book> GetAllBooks() const = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) const = 0;
    virtual std::optional<domain::Author> GetAuthorById(const domain::AuthorId& id) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
