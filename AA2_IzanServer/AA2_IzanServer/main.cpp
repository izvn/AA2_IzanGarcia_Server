#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstdint>
#include "Config.h"

struct Room {
    std::string code;
    std::vector<sf::TcpSocket*> players;
    std::vector<std::string> playerNames;
    std::vector<int> playerPoints;
    std::string hostIP;
};

struct PendingResult {
    std::string roomCode;
    std::vector<std::string> standings;
};

struct PlayerData {
    std::string name; std::string pass; int points; int wins; int losses;
};

std::vector<Room> activeRooms;
std::vector<PendingResult> pendingValidations;
std::vector<std::string> completedRooms; // NUEVO: Evita el bug de los puntos dobles

const int STARTING_POINTS = 1500;
const int POINTS_WIN_1ST = 50;
const int POINTS_WIN_2ND = 15;
const int POINTS_LOSE_3RD = 15;
const int POINTS_LOSE_4TH = 50;

std::vector<PlayerData> loadDatabase() {
    std::vector<PlayerData> db; std::ifstream inFile("db_ranking.txt");
    if (inFile.is_open()) {
        PlayerData p; while (inFile >> p.name >> p.pass >> p.points >> p.wins >> p.losses) db.push_back(p);
        inFile.close();
    }
    return db;
}

void saveDatabase(const std::vector<PlayerData>& db) {
    std::ofstream outFile("db_ranking.txt");
    for (const auto& p : db) outFile << p.name << " " << p.pass << " " << p.points << " " << p.wins << " " << p.losses << "\n";
}

int getPlayerPoints(const std::string& name) {
    auto db = loadDatabase();
    for (const auto& p : db) if (p.name == name) return p.points;
    return STARTING_POINTS;
}

bool registerUser(const std::string& user, const std::string& pass) {
    auto db = loadDatabase();
    for (const auto& p : db) if (p.name == user) return false;
    std::ofstream outFile("db_ranking.txt", std::ios::app);
    outFile << user << " " << pass << " " << STARTING_POINTS << " 0 0\n";
    return true;
}

bool checkLogin(const std::string& user, const std::string& pass) {
    auto db = loadDatabase();
    for (const auto& p : db) if (p.name == user && p.pass == pass) return true;
    return false;
}

void applyMatchResults(const std::vector<std::string>& standings) {
    auto db = loadDatabase();
    for (auto& p : db) {
        if (p.name == standings[0]) { p.points += POINTS_WIN_1ST; p.wins += 1; }
        else if (p.name == standings[1]) { p.points += POINTS_WIN_2ND; }
        else if (p.name == standings[2]) { p.points -= POINTS_LOSE_3RD; }
        else if (p.name == standings[3]) { p.points -= POINTS_LOSE_4TH; p.losses += 1; }
    }
    saveDatabase(db);
    std::cout << "[RANKING] Partida validada y puntos actualizados.\n";
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    std::cout << "=== BOOTSTRAP SERVER INICIADO ===\n";
    sf::TcpListener listener;
    if (listener.listen(Config::BOOTSTRAP_PORT) != sf::Socket::Status::Done) return -1;

    while (true) {
        sf::TcpSocket* newClient = new sf::TcpSocket();
        if (listener.accept(*newClient) == sf::Socket::Status::Done) {
            sf::Packet receivedPacket;
            if (newClient->receive(receivedPacket) == sf::Socket::Status::Done) {
                int type;
                if (receivedPacket >> type) {
                    sf::Packet responsePacket;
                    if (type == Config::NET_LOGIN || type == Config::NET_REGISTER) {
                        std::string user, pass; receivedPacket >> user >> pass;
                        bool success = (type == Config::NET_LOGIN) ? checkLogin(user, pass) : registerUser(user, pass);
                        responsePacket << (success ? 1 : 0); newClient->send(responsePacket);
                    }
                    else if (type == Config::NET_CREATE_ROOM) {
                        std::string reqRoomCode, userName;
                        receivedPacket >> reqRoomCode >> userName;

                        bool roomExists = false;
                        for (auto& room : activeRooms) {
                            if (room.code == reqRoomCode) { roomExists = true; break; }
                        }

                        if (roomExists) {
                            responsePacket << 0;
                            newClient->send(responsePacket);
                        }
                        else {
                            Room newRoom;
                            newRoom.code = reqRoomCode;
                            newRoom.players.push_back(newClient);
                            newRoom.playerNames.push_back(userName);
                            newRoom.playerPoints.push_back(getPlayerPoints(userName));
                            newRoom.hostIP = newClient->getRemoteAddress().value().toString();
                            activeRooms.push_back(newRoom);

                            std::cout << "Sala creada: " << newRoom.code << " por " << userName << "\n";
                            responsePacket << 1 << newRoom.code << 1;
                            newClient->send(responsePacket);
                            continue;
                        }
                    }
                    else if (type == Config::NET_JOIN_ROOM) {
                        std::string roomCode, userName; receivedPacket >> roomCode >> userName;
                        bool found = false;
                        for (auto& room : activeRooms) {
                            if (room.code == roomCode && room.players.size() < Config::MAX_PLAYERS) {
                                room.players.push_back(newClient); room.playerNames.push_back(userName);
                                room.playerPoints.push_back(getPlayerPoints(userName));
                                int playerID = room.players.size();
                                responsePacket << 1 << playerID; newClient->send(responsePacket);
                                found = true;

                                if (room.players.size() == Config::MAX_PLAYERS) {
                                    std::cout << "Sala " << roomCode << " llena. Iniciando P2P...\n";
                                    for (size_t i = 0; i < room.players.size(); ++i) {
                                        sf::Packet p2pPacket;
                                        p2pPacket << Config::NET_START_P2P << room.hostIP << static_cast<std::uint16_t>(Config::P2P_PORT_BASE);
                                        for (int j = 0; j < 4; j++) p2pPacket << room.playerNames[j] << static_cast<std::int32_t>(room.playerPoints[j]);

                                        room.players[i]->send(p2pPacket);
                                        room.players[i]->disconnect(); delete room.players[i];
                                    }
                                    room.players.clear();
                                }
                                break;
                            }
                        }
                        if (!found) { responsePacket << 0; newClient->send(responsePacket); }
                        continue;
                    }
                    else if (type == Config::NET_REPORT_RESULT) {
                        std::string rCode; std::vector<std::string> st(4);
                        receivedPacket >> rCode >> st[0] >> st[1] >> st[2] >> st[3];

                        // IGNORAMOS A LOS REZAGADOS PARA EVITAR PUNTOS DOBLES
                        if (std::find(completedRooms.begin(), completedRooms.end(), rCode) != completedRooms.end()) {
                            continue;
                        }

                        bool matched = false;
                        for (auto it = pendingValidations.begin(); it != pendingValidations.end(); ++it) {
                            if (it->roomCode == rCode && it->standings == st) {
                                applyMatchResults(st);
                                completedRooms.push_back(rCode); // Marcamos como cerrada
                                pendingValidations.erase(it);
                                matched = true; break;
                            }
                        }
                        if (!matched) pendingValidations.push_back({ rCode, st });
                    }
                    else if (type == Config::NET_GET_RANKING) {
                        std::string reqName; receivedPacket >> reqName;
                        auto db = loadDatabase();
                        std::sort(db.begin(), db.end(), [](const PlayerData& a, const PlayerData& b) { return a.points > b.points; });

                        std::vector<PlayerData> topList;
                        for (size_t i = 0; i < db.size(); ++i) {
                            if (i < 10) topList.push_back(db[i]);
                            if (db[i].name == reqName && i >= 10) topList.push_back(db[i]);
                        }

                        responsePacket << static_cast<std::uint32_t>(topList.size());
                        for (size_t i = 0; i < topList.size(); ++i) {
                            std::uint32_t realPos = 0;
                            for (size_t j = 0; j < db.size(); ++j) { if (db[j].name == topList[i].name) { realPos = j + 1; break; } }

                            responsePacket << realPos << topList[i].name
                                << static_cast<std::int32_t>(topList[i].points)
                                << static_cast<std::int32_t>(topList[i].wins)
                                << static_cast<std::int32_t>(topList[i].losses);
                        }
                        newClient->send(responsePacket);
                    }
                }
            }
        }
        delete newClient;
    }
    return 0;
}