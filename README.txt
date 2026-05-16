Servidor: https://github.com/izvn/AA2_IzanGarcia_Server
Cliente: https://github.com/izvn/AA2_IzanGarcia_Client

Notas para la corrección
El juego cumple con todo lo pedido, pero he añadido un par de cosas para que no se rompa:
- Desconexiones (Alt+F4): Si alguien cierra el juego de golpe en plena partida P2P, el juego no se queda colgado. Detecta la desconexión, le pasa el turno, lo manda al último puesto del ranking y los demás pueden terminar la partida.
- Empates: Si se llena el tablero 6x6 y no hay 3 ganadores, el servidor detecta el empate y termina la partida correctamente para que no se quede en bucle infinito.
- Código limpio: He quitado todos los magic numbers y está programado con la sintaxis estricta de SFML 3.

Uso de IA y código externo
Tal y como permite la normativa de la práctica, he usado Gemini y Claude para consultas técnicas concretas en estas partes exactas:
- Para ver cómo adaptar el código a las nuevas normas de SFML 3 (inicializar textos sin que dé error de compilación y cambiar los antiguos sf::Uint32 por los std::uint32_t de C++).
- Para saber cómo aislar los estados de red (sf::Socket::Status::Disconnected) en el bucle P2P sin que se colgara el juego entero.
- Para solucionar un bug de "race condition" en el servidor que hacía que se puntuara doble si llegaban varios reportes de victoria a la vez.

