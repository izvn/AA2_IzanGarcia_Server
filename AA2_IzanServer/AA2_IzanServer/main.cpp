#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

struct Sala {
    std::string codigo;
    int jugadores;
    int turno;
    int tablero[3][3];
    int ganador;
};

std::vector<Sala> salasActivas;

bool registrarUsuario(const std::string& user, const std::string& pass) {
    std::ifstream archivoLectura("usuarios.txt");
    std::string u, p;
    if (archivoLectura.is_open()) {
        while (archivoLectura >> u >> p) {
            if (u == user) return false;
        }
        archivoLectura.close();
    }
    std::ofstream archivoEscritura("usuarios.txt", std::ios::app);
    archivoEscritura << user << " " << pass << "\n";
    return true;
}

bool comprobarLogin(const std::string& user, const std::string& pass) {
    std::ifstream archivoLectura("usuarios.txt");
    std::string u, p;
    if (archivoLectura.is_open()) {
        while (archivoLectura >> u >> p) {
            if (u == user && p == pass) return true;
        }
    }
    return false;
}

std::string generarCodigo() {
    std::string codigo = "";
    for (int i = 0; i < 4; ++i) codigo += std::to_string(rand() % 10);
    return codigo;
}

int comprobarGanador(int t[3][3]) {
    for (int i = 0; i < 3; i++) {
        if (t[i][0] != 0 && t[i][0] == t[i][1] && t[i][1] == t[i][2]) return t[i][0];
        if (t[0][i] != 0 && t[0][i] == t[1][i] && t[1][i] == t[2][i]) return t[0][i];
    }
    if (t[0][0] != 0 && t[0][0] == t[1][1] && t[1][1] == t[2][2]) return t[0][0];
    if (t[0][2] != 0 && t[0][2] == t[1][1] && t[1][1] == t[2][0]) return t[0][2];
    bool empate = true;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) if (t[i][j] == 0) empate = false;
    if (empate) return 3;
    return 0;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    std::cout << "=== BOOTSTRAP SERVER INICIADO ===\n";

    sf::TcpListener listener;
    if (listener.listen(50000) != sf::Socket::Status::Done) return -1;

    while (true) {
        sf::TcpSocket client;
        if (listener.accept(client) == sf::Socket::Status::Done) {
            sf::Packet packetRecibido;
            if (client.receive(packetRecibido) == sf::Socket::Status::Done) {
                int type;
                if (packetRecibido >> type) {
                    sf::Packet packetRespuesta;

                    if (type == 1 || type == 2) {
                        std::string user, pass;
                        packetRecibido >> user >> pass;
                        bool exito = (type == 1) ? comprobarLogin(user, pass) : registrarUsuario(user, pass);
                        packetRespuesta << (exito ? 1 : 0);
                        client.send(packetRespuesta);
                    }
                    else if (type == 3) {
                        Sala s;
                        s.codigo = generarCodigo();
                        s.jugadores = 1;
                        s.turno = 1;
                        s.ganador = 0;
                        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) s.tablero[i][j] = 0;
                        salasActivas.push_back(s);
                        packetRespuesta << 1 << s.codigo << 1;
                        client.send(packetRespuesta);
                    }
                    else if (type == 4) {
                        std::string codigo;
                        packetRecibido >> codigo;
                        bool encontrado = false;
                        for (auto& s : salasActivas) {
                            if (s.codigo == codigo && s.jugadores == 1) {
                                s.jugadores = 2;
                                packetRespuesta << 1 << 2;
                                client.send(packetRespuesta);
                                encontrado = true;
                                break;
                            }
                        }
                        if (!encontrado) { packetRespuesta << 0; client.send(packetRespuesta); }
                    }
                    else if (type == 5) {
                        std::string codigo; int id, x, y;
                        packetRecibido >> codigo >> id >> x >> y;
                        bool ok = false;
                        for (auto& s : salasActivas) {
                            if (s.codigo == codigo && s.turno == id && s.tablero[x][y] == 0 && s.ganador == 0) {
                                s.tablero[x][y] = id;
                                s.ganador = comprobarGanador(s.tablero);
                                s.turno = (id == 1) ? 2 : 1;
                                ok = true;
                                break;
                            }
                        }
                        packetRespuesta << (ok ? 1 : 0);
                        client.send(packetRespuesta);
                    }
                    else if (type == 6) {
                        std::string codigo;
                        packetRecibido >> codigo;
                        for (auto& s : salasActivas) {
                            if (s.codigo == codigo) {
                                packetRespuesta << s.turno << s.ganador;
                                for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) packetRespuesta << s.tablero[i][j];
                                client.send(packetRespuesta);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}