#pragma once
#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/transaction>

#include <memory>
#include <optional>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/unit_of_work.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& work)
        : work_{work} {
    }

    void Save(const domain::Author& author) override;
    void Delete(const domain::AuthorId& id) override;
    std::vector<domain::Author> GetAll() const override;
    std::optional<domain::Author> GetById(const domain::AuthorId& id) const override;
    std::optional<domain::Author> GetByName(const std::string& name) const override;

private:
    pqxx::work& work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& work)
        : work_{work} {
    }

    void Save(const domain::Book& book) override;
    void Update(const domain::Book& book) override;
    void Delete(const domain::BookId& id) override;
    std::vector<domain::Book> GetAll() const override;
    std::optional<domain::Book> GetById(const domain::BookId& id) const override;
    std::vector<domain::Book> GetByTitle(const std::string& title) const override;
    std::vector<domain::Book> GetByAuthor(const domain::AuthorId& author_id) const override;

private:
    pqxx::work& work_;
    void LoadTags(domain::Book& book) const;
};

class PostgresUnitOfWork : public domain::UnitOfWork {
public:
    explicit PostgresUnitOfWork(pqxx::connection& connection)
        : work_{connection}
        , authors_{work_}
        , books_{work_} {
    }

    domain::AuthorRepository& Authors() override { return authors_; }
    domain::BookRepository& Books() override { return books_; }
    void Commit() override { work_.commit(); }

private:
    pqxx::work work_;
    AuthorRepositoryImpl authors_;
    BookRepositoryImpl books_;
};

class Database;

class PostgresUnitOfWorkFactory : public domain::UnitOfWorkFactory {
public:
    explicit PostgresUnitOfWorkFactory(pqxx::connection& connection)
        : connection_{connection} {
    }

    std::unique_ptr<domain::UnitOfWork> Create() override {
        return std::make_unique<PostgresUnitOfWork>(connection_);
    }

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    PostgresUnitOfWorkFactory& GetFactory() & { return factory_; }

private:
    pqxx::connection connection_;
    PostgresUnitOfWorkFactory factory_{connection_};
};

}  // namespace postgres
