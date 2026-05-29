#pragma once
#include <string>

namespace Config {
    // Paso 1 Red: Defino la IP local y los puertos. Separé el puerto del lobby (50000) de los de P2P (50001+)
    const std::string SERVER_IP = "127.0.0.1";
    const unsigned short BOOTSTRAP_PORT = 50000;
    const unsigned short P2P_PORT_BASE = 50001;

    const int GRID_SIZE = 6;
    const int WIN_CONDITION = 3;
    const int MAX_PLAYERS = 4;
    const int TURN_TIME_LIMIT_SEC = 20;

    // Códigos de red (Mi protocolo de comunicación). Así el switch del server/cliente sabe qué llega.
    const int NET_LOGIN = 10;
    const int NET_REGISTER = 11;
    const int NET_CREATE_ROOM = 20;
    const int NET_JOIN_ROOM = 21;
    const int NET_START_P2P = 30;
    const int NET_REPORT_RESULT = 40;
    const int NET_GET_RANKING = 50;

    // Estados de respuesta
    const int SERVER_FAIL = 0;
    const int SERVER_SUCCESS = 1;

    // Stats y reparto de puntos
    const int HOST_PLAYER_ID = 1;
    const int MAX_WINNERS = 3;
    const int RANKING_LIMIT = 10;
    const int STARTING_POINTS = 100;
    const int POINTS_WIN_1ST = 50;
    const int POINTS_WIN_2ND = 15;
    const int POINTS_LOSE_3RD = 15;
    const int POINTS_LOSE_4TH = 50;
}