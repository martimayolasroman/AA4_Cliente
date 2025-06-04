
#include "SearchMenu.h"
#include "Client.h"
#include <iostream>

// centerTextOrigin: Centra el origen de un objeto sf::Text para facilitar el posicionamiento.
void SearchGameMenu::centerTextOrigin(sf::Text& text) {
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
        m_statusDisplay->setFillColor(sf::Color::Black);
        centerTextOrigin(*m_statusDisplay);
        m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });

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

// ~SearchGameMenu: Destructor de la clase, libera la memoria de los objetos de UI.
SearchGameMenu::~SearchGameMenu() {
    delete titleText;
    delete casualMatchmakingButton;
    delete m_statusDisplay;
}

// Update: Actualiza la lógica del menú de búsqueda, gestionando el estado del cliente y la UI.
GameState SearchGameMenu::Update() {
    if (!window || !Client::getInstance()) {
        std::cerr << "SearchGameMenu::Update - Ventana o instancia de cliente nula." << std::endl;
        return GameState::EXIT;
    }

    Client* client = Client::getInstance();

    if (client->hasMatchBeenFound() && client->isConnectedToGameServer()) {
        std::cout << "[SearchMenu] Partida encontrada y conexión UDP lista. Cambiando a GameState::GAME." << std::endl;
        m_requestedMatchmaking = false;
        return GameState::GAME;
    }

    if (fontLoadedSuccessfully && m_statusDisplay) {
        std::string current_status_text;
        if (client->hasMatchBeenFound() && !client->isConnectedToGameServer()) {
            current_status_text = "Partida encontrada. Conectando al servidor de juego...";
        }
        else if (client->isInMatchmakingQueue_flag_getter()) {
            current_status_text = "En cola, buscando oponente...";
        }
        else if (m_requestedMatchmaking && !client->isInMatchmakingQueue_flag_getter() && !client->hasMatchBeenFound()) {
            current_status_text = "Procesando solicitud de matchmaking...";
        }
        else {
            current_status_text = "Pulsa 'Buscar Partida Amistosa'";
        }

        m_statusDisplay->setString(current_status_text);
        centerTextOrigin(*m_statusDisplay);
        m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
    }

    std::optional<sf::Event> eventOpt;
    while ((eventOpt = window->pollEvent())) {
        if (eventOpt) {
            GameState state_from_event = EventHandler(*eventOpt);
            if (state_from_event != GameState::SEARCH) {
                return state_from_event;
            }
        }
    }
    Render(window);

    return GameState::SEARCH;
}

// Render: Dibuja todos los elementos de la interfaz de usuario en la ventana.
void SearchGameMenu::Render(sf::RenderWindow* windowToRenderOn) {
    if (!windowToRenderOn) return;

    windowToRenderOn->clear(backgroundColor);

    if (fontLoadedSuccessfully) {
        if (titleText) windowToRenderOn->draw(*titleText);
        if (m_statusDisplay) windowToRenderOn->draw(*m_statusDisplay);
        if (casualMatchmakingButton) casualMatchmakingButton->draw(*windowToRenderOn);
    }
    else {
    }
    windowToRenderOn->display();
}

// setWindow: Establece la ventana de renderizado y ajusta la posición de los elementos de UI.
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
                casualMatchmakingButton->setPosition(button_pos);
            }
        }
    }
}

// onCasualMatchmakingPressed: Maneja el evento de presionar el botón de búsqueda de partida amistosa.
GameState SearchGameMenu::onCasualMatchmakingPressed() {
    Client* client = Client::getInstance();
    if (!client) return GameState::SEARCH;

    std::cout << "[SearchMenu] Botón 'Buscar Partida Amistosa' presionado." << std::endl;
    if (client->requestMatchmakingFriendly()) {
        std::cout << "[SearchMenu] Solicitud de matchmaking enviada al servidor." << std::endl;
        m_requestedMatchmaking = true;
    }
    else {
        std::cout << "[SearchMenu] Error al enviar solicitud de matchmaking (cliente no conectado o error de send)." << std::endl;
        m_requestedMatchmaking = false;
        if (fontLoadedSuccessfully && m_statusDisplay) {
            m_statusDisplay->setString("Error al conectar. Intenta de nuevo.");
            centerTextOrigin(*m_statusDisplay);
            m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
        }
    }
    return GameState::SEARCH;
}

// EventHandler: Procesa los eventos de la ventana, como cerrar la ventana o la interacción con el botón.
GameState SearchGameMenu::EventHandler(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        if (window) window->close();
        return GameState::EXIT;
    }

    if (fontLoadedSuccessfully && casualMatchmakingButton && casualMatchmakingButton->handleEvent(event, *window)) {
        return onCasualMatchmakingPressed();
    }
    return GameState::SEARCH;
}