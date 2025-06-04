// ------- Archivo: SearchMenu.cpp -------

#include "SearchMenu.h"
#include "Client.h"
#include <iostream>

void SearchGameMenu::centerTextOrigin(sf::Text& text) {
    //if (!text.getFont()) return; // Asegurarse que la fuente existe
    sf::FloatRect text_bounds = text.getLocalBounds();
    text.setOrigin({ text_bounds.size.x / 2.f, text_bounds.size.y / 2.f });
}

SearchGameMenu::SearchGameMenu(sf::RenderWindow* w) :
    window(w),
    casualMatchmakingButton(nullptr),
    titleText(nullptr),
    m_statusDisplay(nullptr),
    fontLoadedSuccessfully(false),
    m_requestedMatchmaking(false) {

    if (!window) {
        std::cerr << "Error: Ventana no proporcionada a SearchGameMenu." << std::endl;
        return;
    }

    width = window->getSize().x;
    height = window->getSize().y;

    if (font.openFromFile(fontsPath + fontName)) {
        fontLoadedSuccessfully = true;
        std::cout << "[SearchGameMenu] Fuente '" << fontName << "' cargada correctamente." << std::endl;
    }
    else {
        fontLoadedSuccessfully = false;
        std::cerr << "Error al cargar la fuente: " << fontsPath + fontName << std::endl;
    }

    if (fontLoadedSuccessfully) {
        titleText = new sf::Text(font, titleString, titleTextSize);
        titleText->setFillColor(titleTextColor);
        centerTextOrigin(*titleText);
        titleText->setPosition({ static_cast<float>(width) / 2.f, titleYPos });

        m_statusDisplay = new sf::Text(font, "Pulsa 'Buscar Partida Amistosa'", 24);
        m_statusDisplay->setFillColor(sf::Color::Black); // Color del texto de estado
        centerTextOrigin(*m_statusDisplay);
        m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f }); // Posición del texto de estado

        sf::Vector2f button_pos = {
            static_cast<float>(width) / 2.f - buttonSize.x / 2.f,
            buttonYPos
        };
        casualMatchmakingButton = new Button(buttonSize, button_pos, casualMatchmakingButtonText, font, buttonColor, buttonTextColor);
    }
    else {
        std::cerr << "SearchGameMenu: La fuente no se cargo, no se crearan elementos de texto/boton dependientes." << std::endl;
    }
}

SearchGameMenu::~SearchGameMenu() {
    delete titleText;
    delete casualMatchmakingButton;
    delete m_statusDisplay;
}

GameState SearchGameMenu::Update() {
    if (!window || !Client::getInstance()) {
        std::cerr << "SearchGameMenu::Update - Ventana o instancia de cliente nula." << std::endl;
        return GameState::EXIT;
    }

    Client* client = Client::getInstance();
    // No es necesario llamar a client->runTcp() aquí, se hace en el bucle principal de Main.cpp o Login.cpp

    // Comprobar primero si la partida ha sido encontrada Y la conexión UDP está lista
    if (client->hasMatchBeenFound() && client->isConnectedToGameServer()) {
        std::cout << "[SearchMenu] Partida encontrada y conexión UDP lista. Cambiando a GameState::GAME." << std::endl;
        m_requestedMatchmaking = false; // Resetear para la próxima vez
        return GameState::GAME;
    }

    // Actualizar texto de estado basado en el estado del cliente
    if (fontLoadedSuccessfully && m_statusDisplay) {
        std::string current_status_text;
        if (client->hasMatchBeenFound() && !client->isConnectedToGameServer()) {
            // Partida encontrada, pero aún conectando al servidor de juego UDP
            current_status_text = "Partida encontrada. Conectando al servidor de juego...";
        }
        else if (client->isInMatchmakingQueue_flag_getter()) {
            // En cola activamente
            current_status_text = "En cola, buscando oponente...";
        }
        else if (m_requestedMatchmaking && !client->isInMatchmakingQueue_flag_getter() && !client->hasMatchBeenFound()) {
            // Se solicitó matchmaking, no está en cola (quizás esperando confirmación S_ADDED_TO_QUEUE) y no se ha encontrado partida
            // Esto podría ser "Enviando solicitud..." o si S_ADDED_TO_QUEUE nunca llega, un estado de error.
            current_status_text = "Procesando solicitud de matchmaking...";
        }
        else {
            // Estado inicial o después de un error donde no está en cola ni ha encontrado partida
            current_status_text = "Pulsa 'Buscar Partida Amistosa'";
        }

        // Si hubo un error en S_MATCH_FOUND (ej. deserialización) y m_matchFound es false pero m_isInMatchmakingQueue también es false
        // esto podría mostrar "Procesando solicitud..." o "Pulsa..." dependiendo de m_requestedMatchmaking.
        // Un mensaje de error más explícito podría ser útil si el cliente detecta un fallo en S_MATCH_FOUND.
        // Por ahora, esta lógica es un intento de cubrir los estados principales.

        m_statusDisplay->setString(current_status_text);
        centerTextOrigin(*m_statusDisplay); // Re-centrar si el texto cambió
        m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
    }

    // Procesar eventos de la UI
    std::optional<sf::Event> eventOpt;
    while ((eventOpt = window->pollEvent())) {
        if (eventOpt) {
            GameState state_from_event = EventHandler(*eventOpt);
            if (state_from_event != GameState::SEARCH) { // Si el evento causa un cambio de estado (ej. EXIT o botón presionado)
                // m_requestedMatchmaking se maneja en onCasualMatchmakingPressed o al salir de SEARCH
                return state_from_event;
            }
        }
    }
    Render(window); // Redibujar en cada frame del Update

    return GameState::SEARCH; // Permanecer en este estado si no hay cambio
}

void SearchGameMenu::Render(sf::RenderWindow* windowToRenderOn) {
    if (!windowToRenderOn) return;

    windowToRenderOn->clear(backgroundColor);

    if (fontLoadedSuccessfully) {
        if (titleText) windowToRenderOn->draw(*titleText);
        if (m_statusDisplay) windowToRenderOn->draw(*m_statusDisplay);
        if (casualMatchmakingButton) casualMatchmakingButton->draw(*windowToRenderOn); // Usar windowToRenderOn
    }
    else {
        // Opcional: Dibujar texto de error si la fuente no cargó
    }
    windowToRenderOn->display();
}

void SearchGameMenu::setWindow(sf::RenderWindow* win) {
    this->window = win;
    if (this->window) {
        width = this->window->getSize().x;
        height = this->window->getSize().y;

        if (fontLoadedSuccessfully) {
            if (titleText) {
                centerTextOrigin(*titleText);
                titleText->setPosition({ static_cast<float>(width) / 2.f, titleYPos });
            }
            if (m_statusDisplay) {
                centerTextOrigin(*m_statusDisplay);
                m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
            }
            if (casualMatchmakingButton) {
                sf::Vector2f button_pos = {
                    static_cast<float>(width) / 2.f - buttonSize.x / 2.f,
                    buttonYPos
                };
                casualMatchmakingButton->setPosition(button_pos); // Asumo que Button tiene setPosition
            }
        }
    }
}

GameState SearchGameMenu::onCasualMatchmakingPressed() {
    Client* client = Client::getInstance();
    if (!client) return GameState::SEARCH; // No debería pasar

    std::cout << "[SearchMenu] Botón 'Buscar Partida Amistosa' presionado." << std::endl;
    if (client->requestMatchmakingFriendly()) {
        std::cout << "[SearchMenu] Solicitud de matchmaking enviada al servidor." << std::endl;
        m_requestedMatchmaking = true; // Marcar que se ha hecho una solicitud
        // El estado del m_statusDisplay se actualizará en el próximo Update()
    }
    else {
        std::cout << "[SearchMenu] Error al enviar solicitud de matchmaking (cliente no conectado o error de send)." << std::endl;
        m_requestedMatchmaking = false; // No se pudo enviar
        if (fontLoadedSuccessfully && m_statusDisplay) {
            m_statusDisplay->setString("Error al conectar. Intenta de nuevo.");
            centerTextOrigin(*m_statusDisplay);
            m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
        }
    }
    return GameState::SEARCH; // Permanecer en SEARCH, Update() manejará los cambios de estado del cliente
}

GameState SearchGameMenu::EventHandler(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        if (window) window->close();
        return GameState::EXIT;
    }

    if (fontLoadedSuccessfully && casualMatchmakingButton && casualMatchmakingButton->handleEvent(event, *window)) {
        return onCasualMatchmakingPressed();
    }
    return GameState::SEARCH; // Por defecto, permanecer en este estado
}