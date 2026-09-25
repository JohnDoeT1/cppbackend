#include "postgres.h"

#include <algorithm>

#include <pqxx/result>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

// ==================== AuthorRepositoryImpl ====================

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    work_.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    work_.exec_params(
        "DELETE FROM book_tags WHERE book_id IN (SELECT id FROM books WHERE author_id = $1);"_zv,
        id.ToString());
    work_.exec_params("DELETE FROM books WHERE author_id = $1;"_zv, id.ToString());
    work_.exec_params("DELETE FROM authors WHERE id = $1;"_zv, id.ToString());
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    auto result = work_.exec("SELECT id, name FROM authors ORDER BY name ASC;"_zv);
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
    auto result = work_.exec_params("SELECT id, name FROM authors WHERE id = $1;"_zv,
                                    id.ToString());
    if (result.empty()) return std::nullopt;
    const auto& row = result[0];
    return domain::Author{
        domain::AuthorId::FromString(row["id"].as<std::string>()),
        row["name"].as<std::string>()};
}

std::optional<domain::Author> AuthorRepositoryImpl::GetByName(const std::string& name) const {
    auto result = work_.exec_params("SELECT id, name FROM authors WHERE name = $1;"_zv, name);
    if (result.empty()) return std::nullopt;
    const auto& row = result[0];
    return domain::Author{
        domain::AuthorId::FromString(row["id"].as<std::string>()),
        row["name"].as<std::string>()};
}

// ==================== BookRepositoryImpl ====================

namespace {

std::vector<std::string> GetTagStrings(pqxx::work& work, const std::string& book_id) {
    auto r = work.exec_params(
        "SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag ASC;"_zv, book_id);
    std::vector<std::string> tags;
    tags.reserve(r.size());
    for (const auto& row : r) {
        tags.push_back(row["tag"].as<std::string>());
    }
    return tags;
}

}  // namespace

void BookRepositoryImpl::LoadTags(domain::Book& book) const {
    book.SetTags(GetTagStrings(work_, book.GetId().ToString()));
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    work_.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year)
VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPublicationYear());

    // Удаляем старые теги
    work_.exec_params("DELETE FROM book_tags WHERE book_id = $1;"_zv, book.GetId().ToString());

    // Сортируем теги перед вставкой — тесты читают их через STRING_AGG без ORDER BY,
    // а ожидают отсортированными.
    auto tags = book.GetTags();
    std::sort(tags.begin(), tags.end());

    for (const auto& tag : tags) {
        work_.exec_params(
            "INSERT INTO book_tags (book_id, tag) VALUES ($1, $2) ON CONFLICT DO NOTHING;"_zv,
            book.GetId().ToString(), tag);
    }
}

void BookRepositoryImpl::Update(const domain::Book& book) {
    Save(book);
}

void BookRepositoryImpl::Delete(const domain::BookId& id) {
    work_.exec_params("DELETE FROM book_tags WHERE book_id = $1;"_zv, id.ToString());
    work_.exec_params("DELETE FROM books WHERE id = $1;"_zv, id.ToString());
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    auto result = work_.exec(
        R"(
SELECT books.id AS id, books.author_id AS author_id, title, publication_year
FROM books
ORDER BY title ASC, publication_year ASC;
)"_zv);
    std::vector<domain::Book> books;
    books.reserve(result.size());
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>());
        LoadTags(books.back());
    }
    return books;
}

std::optional<domain::Book> BookRepositoryImpl::GetById(const domain::BookId& id) const {
    auto result = work_.exec_params(
        "SELECT id, author_id, title, publication_year FROM books WHERE id = $1;"_zv,
        id.ToString());
    if (result.empty()) return std::nullopt;
    const auto& row = result[0];
    domain::Book book{
        domain::BookId::FromString(row["id"].as<std::string>()),
        domain::AuthorId::FromString(row["author_id"].as<std::string>()),
        row["title"].as<std::string>(),
        row["publication_year"].as<int>()};
    LoadTags(book);
    return book;
}

std::vector<domain::Book> BookRepositoryImpl::GetByTitle(const std::string& title) const {
    auto result = work_.exec_params(
        R"(
SELECT books.id AS id, books.author_id AS author_id, title, publication_year
FROM books
WHERE title = $1
ORDER BY title ASC, publication_year ASC;
)"_zv, title);
    std::vector<domain::Book> books;
    books.reserve(result.size());
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>());
        LoadTags(books.back());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthor(const domain::AuthorId& author_id) const {
    auto result = work_.exec_params(
        R"(
SELECT id, author_id, title, publication_year
FROM books
WHERE author_id = $1
ORDER BY publication_year ASC, title ASC;
)"_zv, author_id.ToString());
    std::vector<domain::Book> books;
    books.reserve(result.size());
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>());
        LoadTags(books.back());
    }
    return books;
}

// ==================== Database ====================

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
    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL REFERENCES books(id),
    tag varchar(30) NOT NULL,
    PRIMARY KEY (book_id, tag)
);
)"_zv);
    work.commit();
}

}  // namespace postgres
