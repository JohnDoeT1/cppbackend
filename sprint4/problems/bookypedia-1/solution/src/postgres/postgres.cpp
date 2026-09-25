#include "postgres.h"

#include <pqxx/result>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    pqxx::read_transaction r{connection_};
    auto result = r.exec("SELECT id, name FROM authors ORDER BY name ASC"_zv);
    std::vector<domain::Author> authors;
    authors.reserve(result.size());
    for (const auto& row : result) {
        authors.emplace_back(
            domain::AuthorId::FromString(row["id"].as<std::string>()),
            row["name"].as<std::string>());
    }
    return authors;
}

std::optional<domain::Author> AuthorRepositoryImpl::GetById(const domain::AuthorId& id) const {
    pqxx::read_transaction r{connection_};
    auto result = r.exec_params("SELECT id, name FROM authors WHERE id = $1"_zv,
                                id.ToString());
    if (result.empty()) {
        return std::nullopt;
    }
    const auto& row = result[0];
    return domain::Author{
        domain::AuthorId::FromString(row["id"].as<std::string>()),
        row["name"].as<std::string>()};
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPublicationYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    pqxx::read_transaction r{connection_};
    auto result = r.exec(
        "SELECT id, author_id, title, publication_year FROM books ORDER BY title ASC"_zv);
    std::vector<domain::Book> books;
    books.reserve(result.size());
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthor(const domain::AuthorId& author_id) const {
    pqxx::read_transaction r{connection_};
    auto result = r.exec_params(
        "SELECT id, author_id, title, publication_year FROM books "
        "WHERE author_id = $1 "
        "ORDER BY publication_year ASC, title ASC"_zv,
        author_id.ToString());
    std::vector<domain::Book> books;
    books.reserve(result.size());
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>());
    }
    return books;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title varchar(100) NOT NULL,
    publication_year integer NOT NULL
);
)"_zv);
    work.commit();
}

}  // namespace postgres
