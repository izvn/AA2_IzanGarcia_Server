#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

std::vector<std::string> salasActivas;

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

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    std::cout << "=== BOOTSTRAP SERVER INICIADO ===\n";

    sf::TcpListener listener;
    if (listener.listen(50000) != sf::Socket::Status::Done) {
        return -1;
    }

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
                        std::string nuevoCodigo = generarCodigo();
                        salasActivas.push_back(nuevoCodigo);
                        std::cout << "Nueva sala creada: " << nuevoCodigo << "\n";
                        packetRespuesta << 1 << nuevoCodigo;
                        client.send(packetRespuesta);
                    }
                    else if (type == 4) {
                        std::string codigo;
                        packetRecibido >> codigo;
                        bool existe = (std::find(salasActivas.begin(), salasActivas.end(), codigo) != salasActivas.end());
                        if (existe) std::cout << "Jugador se unio a la sala: " << codigo << "\n";
                        packetRespuesta << (existe ? 1 : 0);
                        client.send(packetRespuesta);
                    }
                }
            }
        }
    }
    return 0;
}