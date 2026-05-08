#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <optional>

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

int main() {
    std::cout << "=== BOOTSTRAP SERVER INICIADO ===\n";

    sf::TcpListener listener;
    if (listener.listen(50000) != sf::Socket::Status::Done) {
        std::cerr << "Error abriendo puerto.\n";
        return -1;
    }

    std::cout << "Servidor a la escucha en puerto 50000...\n";

    while (true) {
        sf::TcpSocket client;
        if (listener.accept(client) == sf::Socket::Status::Done) {
            sf::Packet packetRecibido;

            if (client.receive(packetRecibido) == sf::Socket::Status::Done) {
                int type;
                std::string user, pass;

                if (packetRecibido >> type >> user >> pass) {
                    sf::Packet packetRespuesta;
                    bool exito = false;

                    if (type == 1) {
                        std::cout << "[PETICION LOGIN] Usuario: " << user << "\n";
                        exito = comprobarLogin(user, pass);
                    }
                    else if (type == 2) { 
                        std::cout << "[PETICION REGISTRO] Usuario: " << user << "\n";
                        exito = registrarUsuario(user, pass);
                    }

                    packetRespuesta << (exito ? 1 : 0);
                    client.send(packetRespuesta);

                    std::cout << "-> Respuesta enviada: " << (exito ? "EXITO" : "FALLO") << "\n\n";
                }
            }
        }
    }
    return 0;
}