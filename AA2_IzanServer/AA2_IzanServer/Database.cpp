#include "Database.h"
#include "Config.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <functional>

#include <mysql/jdbc.h>

Database::Database()
    : host("tcp://127.0.0.1:3306"), // Puerto por defecto XAMPP
    user("root"),
    password("enti"),
    databaseName("3enRaya_db") {
}

// Genero el hash con un "salt" propio para mayor seguridad en BD
std::string Database::hashPassword(const std::string& plainPassword) const {
    const std::string salt = "AA2_3EN_RAYA_SALT_2026";
    const std::string valueToHash = salt + plainPassword;

    std::size_t hashValue = std::hash<std::string>{}(valueToHash);

    std::ostringstream stream;
    stream << hashValue;

    return stream.str();
}

bool Database::registerUser(const std::string& username, const std::string& plainPassword) {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        // PreparedStatement: Uso interrogantes (?) para evitar Inyección SQL
        std::unique_ptr<sql::PreparedStatement> checkStatement(
            connection->prepareStatement("SELECT COUNT(*) AS total FROM users WHERE userName = ?")
        );

        checkStatement->setString(1, username);
        std::unique_ptr<sql::ResultSet> result(checkStatement->executeQuery());

        if (result->next() && result->getInt("total") > 0) {
            return false; // El user ya existe
        }

        std::unique_ptr<sql::PreparedStatement> insertStatement(
            connection->prepareStatement(
                "INSERT INTO users (userName, password, points, wins, losses) VALUES (?, ?, ?, 0, 0)"
            )
        );

        insertStatement->setString(1, username);
        insertStatement->setString(2, hashPassword(plainPassword));
        insertStatement->setInt(3, Config::STARTING_POINTS);
        insertStatement->executeUpdate();

        return true;
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] registerUser: " << error.what() << "\n";
        return false;
    }
}

bool Database::checkLogin(const std::string& username, const std::string& plainPassword) {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "SELECT COUNT(*) AS total FROM users WHERE userName = ? AND password = ?"
            )
        );

        statement->setString(1, username);
        statement->setString(2, hashPassword(plainPassword)); // Compruebo contra el hash

        std::unique_ptr<sql::ResultSet> result(statement->executeQuery());

        return result->next() && result->getInt("total") > 0;
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] checkLogin: " << error.what() << "\n";
        return false;
    }
}

int Database::getPlayerPoints(const std::string& username) {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement("SELECT points FROM users WHERE userName = ?")
        );

        statement->setString(1, username);
        std::unique_ptr<sql::ResultSet> result(statement->executeQuery());

        if (result->next()) {
            return result->getInt("points");
        }
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] getPlayerPoints: " << error.what() << "\n";
    }
    return Config::STARTING_POINTS;
}

int Database::getPlayerPosition(const std::string& username) {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        // OJO PROFE: Uso una query avanzada (ROW_NUMBER() OVER) para ordenar virtualmente la tabla 
        // por puntos (DESC) y luego por victorias y buscar la posición real del jugador.
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "SELECT ranked.position FROM "
                "(SELECT userName, ROW_NUMBER() OVER (ORDER BY points DESC, wins DESC, losses ASC, userName ASC) AS position FROM users) AS ranked "
                "WHERE ranked.userName = ?"
            )
        );

        statement->setString(1, username);
        std::unique_ptr<sql::ResultSet> result(statement->executeQuery());

        if (result->next()) {
            return result->getInt("position");
        }
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] getPlayerPosition: " << error.what() << "\n";
    }
    return 0;
}

std::vector<PlayerData> Database::getRankingWithPlayer(const std::string& username) {
    std::vector<PlayerData> ranking;
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        // Primero saco el TOP 10 general
        std::unique_ptr<sql::PreparedStatement> topStatement(
            connection->prepareStatement(
                "SELECT userName, points, wins, losses FROM users ORDER BY points DESC, wins DESC, losses ASC, userName ASC LIMIT ?"
            )
        );
        topStatement->setInt(1, Config::RANKING_LIMIT);
        std::unique_ptr<sql::ResultSet> topResult(topStatement->executeQuery());

        bool requesterIsInTop = false;

        while (topResult->next()) {
            PlayerData player;
            player.name = topResult->getString("userName");
            player.points = topResult->getInt("points");
            player.wins = topResult->getInt("wins");
            player.losses = topResult->getInt("losses");

            if (player.name == username) {
                requesterIsInTop = true;
            }
            ranking.push_back(player);
        }

        // Truco: Si el jugador NO está en el top 10, hago otra query para adjuntar sus datos al final de la lista
        if (!requesterIsInTop) {
            std::unique_ptr<sql::PreparedStatement> selfStatement(
                connection->prepareStatement(
                    "SELECT userName, points, wins, losses FROM users WHERE userName = ?"
                )
            );
            selfStatement->setString(1, username);
            std::unique_ptr<sql::ResultSet> selfResult(selfStatement->executeQuery());

            if (selfResult->next()) {
                PlayerData player;
                player.name = selfResult->getString("userName");
                player.points = selfResult->getInt("points");
                player.wins = selfResult->getInt("wins");
                player.losses = selfResult->getInt("losses");
                ranking.push_back(player);
            }
        }
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] getRankingWithPlayer: " << error.what() << "\n";
    }
    return ranking;
}

void Database::applyMatchResults(const std::vector<std::string>& standings) {
    if (standings.size() < Config::MAX_PLAYERS) {
        return;
    }

    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> connection(driver->connect(host, user, password));
        connection->setSchema(databaseName);

        // MUY IMPORTANTE: AutoCommit(false) crea una 'Transacción'. Si el servidor peta actualizando al 3er jugador, 
        // no se guarda nada. Evita que la BD quede corrupta con datos a medias.
        connection->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE users SET points = points + ?, wins = wins + ?, losses = losses + ? WHERE userName = ?"
            )
        );

        statement->setInt(1, Config::POINTS_WIN_1ST);
        statement->setInt(2, 1);
        statement->setInt(3, 0);
        statement->setString(4, standings[0]);
        statement->executeUpdate();

        statement->setInt(1, Config::POINTS_WIN_2ND);
        statement->setInt(2, 0);
        statement->setInt(3, 0);
        statement->setString(4, standings[1]);
        statement->executeUpdate();

        statement->setInt(1, -Config::POINTS_LOSE_3RD);
        statement->setInt(2, 0);
        statement->setInt(3, 0);
        statement->setString(4, standings[2]);
        statement->executeUpdate();

        statement->setInt(1, -Config::POINTS_LOSE_4TH);
        statement->setInt(2, 0);
        statement->setInt(3, 1);
        statement->setString(4, standings[3]);
        statement->executeUpdate();

        // Guardo la transacción completa
        connection->commit();
        std::cout << "[RANKING] Match validated and database updated.\n";
    }
    catch (sql::SQLException& error) {
        std::cout << "[DB ERROR] applyMatchResults: " << error.what() << "\n";
    }
}