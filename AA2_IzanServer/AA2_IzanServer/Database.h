#pragma once
#include <string>
#include <vector>

// Struct rápido para mover los datos del ranking sin clases complejas
struct PlayerData {
    std::string name;
    int points;
    int wins;
    int losses;
};

class Database {
private:
    std::string host;
    std::string user;
    std::string password;
    std::string databaseName;

    // Privado para que nadie llame al hasheador desde fuera
    std::string hashPassword(const std::string& plainPassword) const;

public:
    Database();

    bool registerUser(const std::string& username, const std::string& plainPassword);
    bool checkLogin(const std::string& username, const std::string& plainPassword);

    int getPlayerPoints(const std::string& username);
    int getPlayerPosition(const std::string& username);

    std::vector<PlayerData> getRankingWithPlayer(const std::string& username);
    void applyMatchResults(const std::vector<std::string>& standings);
};