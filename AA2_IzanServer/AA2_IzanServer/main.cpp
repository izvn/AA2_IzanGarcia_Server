#include <SFML/Network.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstdint>

#include "Config.h"
#include "Database.h"

// Guardo la sala en memoria RAM mientras esperan
struct Room {
    std::string code;
    std::string matchId;
    std::vector<sf::TcpSocket*> players;
    std::vector<std::string> playerNames;
    std::vector<int> playerPoints;
    std::string hostIP;
    unsigned short p2pPort;
};

// Estructura para el sistema antitrampas (esperar 2 reportes iguales)
struct PendingResult {
    std::string matchId;
    std::vector<std::string> standings;
};

std::vector<Room> activeRooms;
std::vector<PendingResult> pendingValidations;
std::vector<std::string> completedMatches;

// Creo un ID único juntando el código de sala + la hora exacta del PC
std::string createMatchId(const std::string& roomCode) {
    static int matchCounter = 0;
    matchCounter++;
    return roomCode + "_" + std::to_string(static_cast<int>(std::time(nullptr))) + "_" + std::to_string(matchCounter);
}

// Doy un puerto P2P nuevo a cada sala para que no choquen si hay varias partidas a la vez
unsigned short createP2PPort() {
    static unsigned short nextPort = Config::P2P_PORT_BASE;
    const unsigned short port = nextPort;
    nextPort++;
    return port;
}

// Limpio el socket para evitar Memory Leaks
void deleteSocket(sf::TcpSocket* socket) {
    if (socket != nullptr) {
        socket->disconnect();
        delete socket;
    }
}

// Busco si la sala ya existe iterando el vector
bool roomExists(const std::string& roomCode) {
    for (const auto& room : activeRooms) {
        if (room.code == roomCode) {
            return true;
        }
    }
    return false;
}

// Borro la sala usando una función lambda (std::remove_if) que busca el código
void removeRoom(const std::string& roomCode) {
    activeRooms.erase(
        std::remove_if(
            activeRooms.begin(),
            activeRooms.end(),
            [&roomCode](const Room& room) {
                return room.code == roomCode;
            }
        ),
        activeRooms.end()
    );
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    std::cout << "=== BOOTSTRAP SERVER STARTED ===\n";

    Database database;
    sf::TcpListener listener;

    if (listener.listen(Config::BOOTSTRAP_PORT) != sf::Socket::Status::Done) {
        std::cout << "[SERVER] No se pudo abrir el puerto " << Config::BOOTSTRAP_PORT << "\n";
        return -1;
    }

    // Bucle principal del servidor, escuchando siempre
    while (true) {
        sf::TcpSocket* newClient = new sf::TcpSocket();
        bool keepSocketAlive = false; // Flag para no borrar el socket si entra al Lobby

        if (listener.accept(*newClient) == sf::Socket::Status::Done) {
            sf::Packet receivedPacket;

            if (newClient->receive(receivedPacket) == sf::Socket::Status::Done) {
                int type = 0;

                // Extraigo el identificador (type) para saber qué me piden
                if (receivedPacket >> type) {
                    sf::Packet responsePacket;

                    if (type == Config::NET_LOGIN || type == Config::NET_REGISTER) {
                        std::string user;
                        std::string pass;

                        receivedPacket >> user >> pass;
                        bool success = false;

                        if (type == Config::NET_LOGIN) {
                            success = database.checkLogin(user, pass);
                            std::cout << "[AUTH] Login de " << user << ": " << (success ? "OK" : "FAILED") << "\n";
                        }
                        else {
                            success = database.registerUser(user, pass);
                            std::cout << "[AUTH] Registro de " << user << ": " << (success ? "OK" : "FAILED") << "\n";
                        }

                        responsePacket << (success ? Config::SERVER_SUCCESS : Config::SERVER_FAIL);
                        newClient->send(responsePacket);
                    }
                    else if (type == Config::NET_CREATE_ROOM) {
                        std::string reqRoomCode;
                        std::string userName;

                        receivedPacket >> reqRoomCode >> userName;

                        if (roomExists(reqRoomCode)) {
                            responsePacket << Config::SERVER_FAIL;
                            newClient->send(responsePacket);
                            std::cout << "[ROOM] No se pudo crear. Ya existe la sala " << reqRoomCode << "\n";
                        }
                        else {
                            Room newRoom;
                            newRoom.code = reqRoomCode;
                            newRoom.matchId = createMatchId(reqRoomCode);
                            newRoom.players.push_back(newClient); // Añado el socket
                            newRoom.playerNames.push_back(userName);
                            newRoom.playerPoints.push_back(database.getPlayerPoints(userName));
                            newRoom.hostIP = Config::SERVER_IP;
                            newRoom.p2pPort = createP2PPort();

                            activeRooms.push_back(newRoom);

                            responsePacket << Config::SERVER_SUCCESS << reqRoomCode << Config::HOST_PLAYER_ID;
                            newClient->send(responsePacket);

                            keepSocketAlive = true; // Lo mantengo abierto porque está en la sala de espera

                            std::cout << "[ROOM] Sala creada: " << reqRoomCode
                                << " | Match: " << newRoom.matchId
                                << " | P2P Port: " << newRoom.p2pPort << "\n";
                        }
                    }
                    else if (type == Config::NET_JOIN_ROOM) {
                        std::string roomCode;
                        std::string userName;

                        receivedPacket >> roomCode >> userName;
                        bool found = false;

                        for (auto& room : activeRooms) {
                            if (room.code == roomCode && room.players.size() < Config::MAX_PLAYERS) {
                                room.players.push_back(newClient);
                                room.playerNames.push_back(userName);
                                room.playerPoints.push_back(database.getPlayerPoints(userName));

                                int playerID = static_cast<int>(room.players.size());

                                responsePacket << Config::SERVER_SUCCESS << playerID;
                                newClient->send(responsePacket);

                                found = true;
                                keepSocketAlive = true; // Mantengo su conexión en el lobby

                                std::cout << "[ROOM] " << userName << " se une a "
                                    << roomCode << " como jugador " << playerID << "\n";

                                // PASO CLAVE: Si se llena, echo a los 4 clientes hacia P2P
                                if (room.players.size() == Config::MAX_PLAYERS) {
                                    std::cout << "[ROOM] Sala llena. Iniciando partida "
                                        << room.matchId << " en puerto " << room.p2pPort << "\n";

                                    for (size_t i = 0; i < room.players.size(); ++i) {
                                        sf::Packet p2pPacket;
                                        p2pPacket << Config::NET_START_P2P
                                            << room.hostIP
                                            << static_cast<std::uint16_t>(room.p2pPort)
                                            << room.matchId;

                                        for (int j = 0; j < Config::MAX_PLAYERS; j++) {
                                            p2pPacket << room.playerNames[j] << static_cast<std::int32_t>(room.playerPoints[j]);
                                        }

                                        room.players[i]->send(p2pPacket);
                                        deleteSocket(room.players[i]); // Les corto la conexión central, ahora son P2P
                                    }

                                    room.players.clear();
                                    removeRoom(roomCode); // Elimino la sala de la RAM
                                }
                                break;
                            }
                        }

                        if (!found) {
                            responsePacket << Config::SERVER_FAIL;
                            newClient->send(responsePacket);
                            std::cout << "[ROOM] No se pudo unir a la sala " << roomCode << "\n";
                        }
                    }
                    else if (type == Config::NET_REPORT_RESULT) {
                        std::string matchId;
                        std::vector<std::string> standings(Config::MAX_PLAYERS);

                        receivedPacket >> matchId;
                        for (int i = 0; i < Config::MAX_PLAYERS; i++) {
                            receivedPacket >> standings[i];
                        }

                        // Evito procesar la partida si ya se cerró
                        if (std::find(completedMatches.begin(), completedMatches.end(), matchId) != completedMatches.end()) {
                            std::cout << "[RANKING] Resultado duplicado ignorado para " << matchId << "\n";
                        }
                        else {
                            bool matched = false;

                            // OJO PROFE: Aquí está mi sistema Antitrampas. 
                            // Si encuentro el reporte guardado de otro jugador y coincide exactamente con este...
                            for (auto it = pendingValidations.begin(); it != pendingValidations.end(); ++it) {
                                if (it->matchId == matchId && it->standings == standings) {
                                    database.applyMatchResults(standings); // LO VALIDO
                                    completedMatches.push_back(matchId);
                                    pendingValidations.erase(it);
                                    matched = true;
                                    std::cout << "[RANKING] Partida validada por pares: " << matchId << "\n";
                                    break;
                                }
                            }

                            // Si soy el primero en reportar, lo guardo y espero confirmación de otro
                            if (!matched) {
                                pendingValidations.push_back({ matchId, standings });
                                std::cout << "[RANKING] Esperando segunda validacion para " << matchId << "\n";
                            }
                        }
                    }
                    else if (type == Config::NET_GET_RANKING) {
                        std::string reqName;
                        receivedPacket >> reqName;

                        auto ranking = database.getRankingWithPlayer(reqName);

                        responsePacket << static_cast<std::uint32_t>(ranking.size());

                        // Envío la info al cliente
                        for (const auto& player : ranking) {
                            std::uint32_t realPosition = static_cast<std::uint32_t>(database.getPlayerPosition(player.name));

                            responsePacket << realPosition
                                << player.name
                                << static_cast<std::int32_t>(player.points)
                                << static_cast<std::int32_t>(player.wins)
                                << static_cast<std::int32_t>(player.losses);
                        }

                        newClient->send(responsePacket);
                        std::cout << "[RANKING] Ranking enviado a " << reqName << "\n";
                    }
                }
            }
        }

        if (!keepSocketAlive) {
            deleteSocket(newClient);
        }
    }

    return 0;
}