#pragma once
#include <string>

// --- ARCHIVO DE CONFIGURACIÓN GLOBAL ---
namespace Config {
    // Red
    const unsigned short BOOTSTRAP_PORT = 50000;
    const unsigned short P2P_PORT_BASE = 50001;
    const std::string SERVER_IP = "127.0.0.1";

    // Juego
    const int MAX_PLAYERS = 4;
    const int GRID_SIZE = 6;
    const int WIN_CONDITION = 3;
    const int TURN_TIME_LIMIT_SEC = 20;

    // Códigos de Protocolo de Red
    const int NET_LOGIN = 1;
    const int NET_REGISTER = 2;
    const int NET_CREATE_ROOM = 3;
    const int NET_JOIN_ROOM = 4;
    const int NET_START_P2P = 5;
    const int NET_REPORT_RESULT = 6;
    const int NET_GET_RANKING = 7; // NUEVO: Para pedir el Top 10
}