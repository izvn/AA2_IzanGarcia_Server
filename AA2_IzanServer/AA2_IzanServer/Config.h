#pragma once
#include <string>

// Global configuration namespace shared across Client and Server
namespace Config {

    // Server IP Address (Use "127.0.0.1" for local testing, change to public IP for remote play)
    const std::string SERVER_IP = "127.0.0.1";

    // Port used by the Bootstrap Server to listen for incoming connections
    const unsigned short BOOTSTRAP_PORT = 50000;

    // Base port used for Peer-to-Peer direct connections
    const unsigned short P2P_PORT_BASE = 50001;

    // Board dimensions (6x6 as required by the design document)
    const int GRID_SIZE = 6;

    // Consecutive pieces needed to win
    const int WIN_CONDITION = 3;

    // Total players per room
    const int MAX_PLAYERS = 4;

    // Maximum time allowed per turn in seconds before auto-skipping
    const int TURN_TIME_LIMIT_SEC = 20;

    // Authentication
    const int NET_LOGIN = 10;
    const int NET_REGISTER = 11;

    // Matchmaking
    const int NET_CREATE_ROOM = 20;
    const int NET_JOIN_ROOM = 21;
    const int NET_START_P2P = 30;

    // Data Sync
    const int NET_REPORT_RESULT = 40;
    const int NET_GET_RANKING = 50;
}