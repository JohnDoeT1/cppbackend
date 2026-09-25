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
    virtual void AddBook(int publication_year, const std::string& title,
                         const std::string& author_name, const std::vector<std::string>& tags) = 0;
    virtual void AddBook(int publication_year, const std::string& title, const domain::AuthorId& author_id,
                         const std::vector<std::string>& tags) = 0;

    virtual std::vector<domain::Author> GetAllAuthors() const = 0;
    virtual std::optional<domain::Author> GetAuthorByName(const std::string& name) const = 0;
    virtual std::optional<domain::Author> GetAuthorById(const domain::AuthorId& id) const = 0;

    virtual std::vector<domain::Book> GetAllBooks() const = 0;
    virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) const = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) const = 0;
    virtual std::optional<domain::Book> GetBookById(const domain::BookId& id) const = 0;

    virtual bool DeleteAuthorByName(const std::string& name) = 0;
    virtual bool DeleteAuthorById(const domain::AuthorId& id) = 0;
    virtual bool EditAuthorById(const domain::AuthorId& id, const std::string& new_name) = 0;

    virtual bool DeleteBookById(const domain::BookId& id) = 0;
    virtual bool EditBookById(const domain::BookId& id, const std::optional<std::string>& new_title,
                              const std::optional<int>& new_year,
                              const std::optional<std::vector<std::string>>& new_tags) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
