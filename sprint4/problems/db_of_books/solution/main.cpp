#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void create_table_if_not_exists(pqxx::connection& conn) {
    pqxx::work txn(conn);
    txn.exec(
        "CREATE TABLE IF NOT EXISTS books ("
        "id SERIAL PRIMARY KEY, "
        "title VARCHAR(100) NOT NULL, "
        "author VARCHAR(100) NOT NULL, "
        "year INTEGER NOT NULL, "
        "ISBN CHAR(13) UNIQUE"
        ")"
    );
    txn.commit();
}

bool add_book(pqxx::connection& conn, const json& payload) {
    try {
        pqxx::work txn(conn);
        
        std::string title = payload.at("title").get<std::string>();
        std::string author = payload.at("author").get<std::string>();
        int year = payload.at("year").get<int>();
        
        if (payload.at("ISBN").is_null()) {
            txn.exec_params(
                "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)",
                title, author, year
            );
        } else {
            std::string isbn = payload.at("ISBN").get<std::string>();
            txn.exec_params(
                "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)",
                title, author, year, isbn
            );
        }
        
        txn.commit();
        return true;
    } catch (const pqxx::sql_error& e) {
        return false;
    } catch (const std::exception& e) {
        return false;
    }
}

json get_all_books(pqxx::connection& conn) {
    pqxx::read_transaction txn(conn);
    
    auto result = txn.exec(
        "SELECT id, title, author, year, ISBN FROM books "
        "ORDER BY year DESC, title ASC, author ASC, ISBN ASC NULLS LAST"
    );
    
    json books = json::array();
    
    for (const auto& row : result) {
        json book;
        book["id"] = row["id"].as<int>();
        book["title"] = row["title"].as<std::string>();
        book["author"] = row["author"].as<std::string>();
        book["year"] = row["year"].as<int>();
        
        if (row["ISBN"].is_null()) {
            book["ISBN"] = nullptr;
        } else {
            book["ISBN"] = row["ISBN"].as<std::string>();
        }
        
        books.push_back(book);
    }
    
    txn.commit();
    return books;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <database_connection_string>" << std::endl;
        return 1;
    }
    
    try {
        pqxx::connection conn(argv[1]);
        
        if (!conn.is_open()) {
            std::cerr << "Cannot open database connection" << std::endl;
            return 1;
        }
        
        create_table_if_not_exists(conn);
        
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            try {
                json request = json::parse(line);
                std::string action = request.at("action").get<std::string>();
                json payload = request.at("payload");
                
                if (action == "add_book") {
                    bool success = add_book(conn, payload);
                    json response;
                    response["result"] = success;
                    std::cout << response.dump() << std::endl;
                } else if (action == "all_books") {
                    json books = get_all_books(conn);
                    std::cout << books.dump() << std::endl;
                } else if (action == "exit") {
                    break;
                }
            } catch (const json::exception& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error processing request: " << e.what() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
