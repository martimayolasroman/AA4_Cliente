#include "SearchMenu.h"

SearchGameMenu::SearchGameMenu(sf::RenderWindow* w)
{
    window = w;
    if (!window) {
        std::cerr << "Error: Ventana no proporcionada a SearchGameMenu." << std::endl;
        // Considerar lanzar una excepción o manejar el error de otra forma
        return;
    }

    width = window->getSize().x;
    height = window->getSize().y;

    if (!font.openFromFile(fontsPath + fontName))
    {
        std::cerr << "Error al cargar la fuente: " << fontsPath + fontName << std::endl;
    }

    title = new sf::Text(font, titleString, titleTextSize);

    //Set positions
    float centerX = width / 2.0f;

    titlePosition = sf::Vector2f(centerX - (title->getGlobalBounds().size.x / 2.0f), static_cast<float>(titleYPos));
    title->setFillColor(buttonColor); // Usando el mismo color que los botones del login original
    title->setPosition(titlePosition);

    // Casual Matchmaking Button (centrado)
    casualMatchmakingButtonPosition = sf::Vector2f(centerX - (buttonSize.x / 2.0f), static_cast<float>(buttonYPos));
    casualMatchmakingButton = new Button(buttonSize, casualMatchmakingButtonPosition, casualMatchmakingButtonText, font, buttonColor, buttonTextColor);

    // No hay campos de input que inicializar
}

SearchGameMenu::~SearchGameMenu()
{
    delete title;
    delete casualMatchmakingButton;
}

GameState SearchGameMenu::Update()
{
    if (!window) return GameState::EXIT; // Salir si la ventana no es válida

    while (window->isOpen()) {
        // Si Client::getInstance()->run() es necesario para procesar paquetes generales, mantenlo.
        // Si solo era para respuestas de login/registro, podría no ser necesario aquí.
        // Por fidelidad al original Login.cpp, lo mantendré si Client existe.
        if (Client::getInstance()) { // Comprobar si Client existe para evitar crash si no está inicializado
            Client::getInstance()->run();
        }


        // No hay respuestas de login/registro que comprobar aquí.

        std::optional<sf::Event> eventOpt;
        while ((eventOpt = window->pollEvent())) {
            if (eventOpt) {
                GameState state = EventHandler(*eventOpt);
                // Asumo que GameState::LOGIN se usaba como "permanecer en este estado".
                // Deberías tener un GameState::SEARCH_GAME o similar.
                // Por ahora, si EventHandler devuelve algo distinto a un estado "actual", se retorna.
                // Necesitarás ajustar esto según tu enum GameState.
                // Por ejemplo, si tu GameState::LOGIN era el estado "actual" para Login,
                // aquí debería ser GameState::SEARCH_GAME (si existe)
                if (state != GameState::SEARCH) { // CAMBIAR GameState::LOGIN al estado correspondiente a SearchGameMenu
                    return state;
                }
            }
        }
        Render(window); // Dibujar la UI en cada frame del bucle interno de Update
    }
    return GameState::EXIT; // Si la ventana se cierra
}

void SearchGameMenu::Render(sf::RenderWindow* windowToRenderOn)
{
    if (!windowToRenderOn) return;

    windowToRenderOn->clear(backgroundColor);

    if (title)
        windowToRenderOn->draw(*title);
    if (casualMatchmakingButton)
        casualMatchmakingButton->draw(*windowToRenderOn);

    windowToRenderOn->display();
}

void SearchGameMenu::setWindow(sf::RenderWindow* win)
{
    this->window = win;
    if (this->window) {
        width = this->window->getSize().x;
        height = this->window->getSize().y;
        // Re-calcular posiciones si es necesario, aunque generalmente se hace en el constructor.
    }
}

GameState SearchGameMenu::onCasualMatchmakingPressed()
{
    if (Client::getInstance()->requestMatchmakingFriendly()) {
        return GameState::GAME;
    }

    std::cout << "Error al Buscar Partida." << std::endl;
    return GameState::GAME; // TODO Quitar esto 


    // Aquí iría la lógica para iniciar la búsqueda de partida,
    // como enviar un mensaje al servidor a través de Client::getInstance()
    // Client::getInstance()->sendSearchCasualMatchmakingRequest(); // Ejemplo
}

GameState SearchGameMenu::EventHandler(const sf::Event& event)
{
    if (event.is<sf::Event::Closed>()) {
        if (window) window->close();
        return GameState::EXIT; // O el GameState que signifique cerrar la aplicación
    }

    if (casualMatchmakingButton && casualMatchmakingButton->handleEvent(event, *window)) {
        // El botón fue presionado
        return onCasualMatchmakingPressed();
        // Decidir qué GameState devolver. Podría ser el mismo para permanecer,
        // o uno nuevo si la acción del botón implica un cambio inmediato de estado
        // que no dependa de una respuesta del servidor.
        // Por ahora, asumimos que permanece en este menú.
    }

    // No hay manejo de input de texto ni foco de campos.

    // Devuelve el estado actual para indicar que se debe permanecer en este menú.
    // Deberías reemplazar GameState::LOGIN con el GameState correspondiente a este menú,
    // por ejemplo, GameState::SEARCH_GAME.
    return GameState::SEARCH; // CAMBIAR a GameState::SEARCH_GAME o el que corresponda
}