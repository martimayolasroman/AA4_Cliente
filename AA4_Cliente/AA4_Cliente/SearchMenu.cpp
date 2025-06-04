#include "SearchMenu.h"
#include "Client.h"
#include <iostream>

// Función auxiliar para centrar el origen del texto
void SearchGameMenu::centerTextOrigin(sf::Text& text) {
    // Solo centrar si la fuente del texto está disponible (implica que el texto se creó)
    //if (!text.getFont()) return;

    sf::FloatRect text_bounds = text.getLocalBounds();
    // Para sf::Text, getLocalBounds() usualmente tiene left y top en 0
    // por lo que el origen se calcula directamente con width/2 y height/2
    text.setOrigin({ text_bounds.size.x / 2.f, text_bounds.size.y / 2.f });
}

SearchGameMenu::SearchGameMenu(sf::RenderWindow* w) :
    window(w),
    casualMatchmakingButton(nullptr),
    titleText(nullptr),
    m_statusDisplay(nullptr),
    fontLoadedSuccessfully(false), // Inicializar a false
    m_requestedMatchmaking(false) {

    if (!window) {
        std::cerr << "Error: Ventana no proporcionada a SearchGameMenu." << std::endl;
        return;
    }

    width = window->getSize().x;
    height = window->getSize().y;

    // Cargar la fuente y verificar el éxito
    if (font.openFromFile(fontsPath + fontName)) { // Usas openFromFile
        fontLoadedSuccessfully = true;
        std::cout << "[SearchGameMenu] Fuente '" << fontName << "' cargada correctamente." << std::endl;
    }
    else {
        fontLoadedSuccessfully = false;
        std::cerr << "Error al cargar la fuente: " << fontsPath + fontName << std::endl;
    }

    // Solo crear elementos dependientes de la fuente si esta se cargó
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
        // Asumo que Button internamente también verifica la fuente o la maneja.
        // Si Button puede fallar por la fuente, necesitarías pasar fontLoadedSuccessfully
        // o el Button tomaría el sf::Font& y manejaría internamente.
        // Por ahora, asumo que Button se crea.
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

       // Client::getInstance()->run(); // Procesar paquetes TCP

        if (client->hasMatchBeenFound()) {
            std::cout << "[SearchMenu] Partida encontrada por el cliente! Cambiando a GameState::GAME." << std::endl;
            m_requestedMatchmaking = false;
            return GameState::GAME;
        }

        if (fontLoadedSuccessfully && m_statusDisplay) { // Solo actualizar si la fuente y el texto existen
            std::string current_status_text;
            if (m_requestedMatchmaking && !client->isInMatchmakingQueue_flag_getter()) {
                current_status_text = "Conectando a la cola...";
            }
            else if (client->isInMatchmakingQueue_flag_getter()) {
                current_status_text = "En cola, buscando oponente...";
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
                GameState state = EventHandler(*eventOpt);
                if (state != GameState::SEARCH) {
                    m_requestedMatchmaking = false;
                    return state;
                }
            }
        }
        Render(window);
    
    //m_requestedMatchmaking = false;
    return GameState::SEARCH;
}

void SearchGameMenu::Render(sf::RenderWindow* windowToRenderOn) {
    if (!windowToRenderOn) return;

    windowToRenderOn->clear(backgroundColor);

    // Solo dibujar si la fuente se cargó y los elementos existen
    if (fontLoadedSuccessfully) {
        if (titleText) windowToRenderOn->draw(*titleText);
        if (m_statusDisplay) windowToRenderOn->draw(*m_statusDisplay);
        if (casualMatchmakingButton) casualMatchmakingButton->draw(*window);
    }
    else {
        // Opcional: dibujar un mensaje de error si la fuente no cargó
        // sf::Text errorFontText; (crear uno con una fuente por defecto si es posible, o sin fuente)
        // errorFontText.setString("Error: No se pudo cargar la fuente.");
        // errorFontText.setPosition({10,10});
        // errorFontText.setFillColor(sf::Color::Red);
        // windowToRenderOn->draw(errorFontText);
    }

    windowToRenderOn->display();
}

void SearchGameMenu::setWindow(sf::RenderWindow* win) {
    this->window = win;
    if (this->window) {
        width = this->window->getSize().x;
        height = this->window->getSize().y;

        if (fontLoadedSuccessfully) { // Solo reposicionar si la fuente cargó
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

GameState SearchGameMenu::onCasualMatchmakingPressed() {
    if (!Client::getInstance()) return GameState::SEARCH;

    if (Client::getInstance()->requestMatchmakingFriendly()) {
        std::cout << "[SearchMenu] Solicitud de matchmaking enviada." << std::endl;
        m_requestedMatchmaking = true;
    }
    else {
        std::cout << "[SearchMenu] Error al enviar solicitud de matchmaking." << std::endl;
        m_requestedMatchmaking = false;
        if (fontLoadedSuccessfully && m_statusDisplay) { // Solo actualizar si la fuente y el texto existen
            m_statusDisplay->setString("Error al buscar partida. Intenta de nuevo.");
            centerTextOrigin(*m_statusDisplay);
            m_statusDisplay->setPosition({ static_cast<float>(width) / 2.f, titleYPos + 80.f });
        }
    }
    return GameState::SEARCH;
}

GameState SearchGameMenu::EventHandler(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        if (window) window->close();
        return GameState::EXIT;
    }

    // Solo manejar evento de botón si la fuente cargó y el botón existe
    if (fontLoadedSuccessfully && casualMatchmakingButton && casualMatchmakingButton->handleEvent(event, *window)) {
        return onCasualMatchmakingPressed();
    }
    return GameState::SEARCH;
}