
#include "game.h"

// Function to load map from .txt file (definition)
// Constants like WINDOW_WIDTH, TILE_SIZE, COLOR_DEFAULT_FLOOR_GRAY, COLOR_PLATFORM_BROWN are available from Game.h
std::vector<sf::RectangleShape> loadMap(const std::string& filename) {
    std::vector<sf::RectangleShape> platforms;
    std::ifstream inputFile(filename);
    std::string line;
    float y = 0;

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open map file: " << filename << std::endl;
        // Create a default floor if map loading fails
        sf::RectangleShape floor;
        floor.setSize({ WINDOW_WIDTH, TILE_SIZE });
        floor.setFillColor(COLOR_DEFAULT_FLOOR_GRAY);
        floor.setPosition({ 0, WINDOW_HEIGHT - TILE_SIZE });
        platforms.push_back(floor);
        return platforms;
    }

    while (std::getline(inputFile, line)) {
        float x = 0;
        for (char c : line) {
            if (c == 'P') {
                sf::RectangleShape platform;
                platform.setSize({ TILE_SIZE, TILE_SIZE });
                platform.setFillColor(COLOR_PLATFORM_BROWN);
                platform.setPosition({ x, y });
                platforms.push_back(platform);
            }
            x += TILE_SIZE;
        }
        y += TILE_SIZE;
    }
    inputFile.close();
    return platforms;
}

Game::Game(sf::RenderWindow* window)
    : m_window(window),
    m_player(), // Player constructor from Player.h is called
    m_healthText(nullptr),
    m_livesText(nullptr),
    m_gameOverText(nullptr),
    m_gameOverState(false) {

    bool serverConnected = false;
    if (!serverConnected) {
        std::cout << "Server connection failed. Starting local game." << std::endl;
    }

    m_platforms = loadMap("Data/map.txt");

    std::string fontFileName = "your_font.ttf";
    if (!m_font.openFromFile(fontFileName)) {
        std::cerr << "Error: Could not load font: " << fontFileName << std::endl;
        std::cerr << "Please ensure '" << fontFileName << "' is in the same directory as the executable." << std::endl;

        std::ofstream defaultMap("Data/map.txt", std::ios::app);
        if (defaultMap.is_open()) {
            defaultMap.seekp(0, std::ios_base::end);
            if (defaultMap.tellp() == 0) {
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "....PPPP.......................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "......................PPPP...........ppp.......\n";
                defaultMap << ".....ppp.......................................\n";
                defaultMap << "........................................ppp....\n";
                defaultMap << "PP..............PPPPPP.........................\n";
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "...............................................\n";
                defaultMap << "......................PPPP.....................\n";
                defaultMap << "....................................PPPP.......\n";
                defaultMap << "...............................................\n";
                defaultMap << "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\n";
                defaultMap << "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\n";
            }
            defaultMap.close();
        }
        if (m_platforms.size() == 1 &&
            m_platforms[0].getSize() == sf::Vector2f(WINDOW_WIDTH, TILE_SIZE) &&
            m_platforms[0].getPosition() == sf::Vector2f(0.f, WINDOW_HEIGHT - TILE_SIZE) &&
            m_platforms[0].getFillColor() == COLOR_DEFAULT_FLOOR_GRAY) {
            m_platforms = loadMap("Data/map.txt");
        }
    }
    else {
        m_healthText = new sf::Text(m_font, "Health: " + std::to_string(m_player.health), 24);
        m_healthText->setFillColor(COLOR_TEXT_WHITE);
        m_healthText->setPosition({ 10, 10 });

        m_livesText = new sf::Text(m_font, "Lives: " + std::to_string(m_player.lives), 24);
        m_livesText->setFillColor(COLOR_TEXT_WHITE);
        m_livesText->setPosition({ 10, 40 });

        m_gameOverText = new sf::Text(m_font, "GAME OVER", 72);
        m_gameOverText->setFillColor(COLOR_GAMEOVER_RED);
        sf::FloatRect textRect = m_gameOverText->getLocalBounds();
        m_gameOverText->setOrigin({
            textRect.position.x + textRect.size.x / 2.0f,
            textRect.position.y + textRect.size.y / 2.0f
            });
        m_gameOverText->setPosition({ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f });
    }
}

Game::~Game() {
    delete m_healthText;
    delete m_livesText;
    delete m_gameOverText;
}

void Game::run() {
    while (m_window->isOpen()) {
        float deltaTime = m_clock.restart().asSeconds();
        std::optional<sf::Event> opt_event;
        while ((opt_event = m_window->pollEvent())) {
            sf::Event& event = *opt_event;
            if (event.is<sf::Event::Closed>())
                m_window->close();

            if (!m_gameOverState) {
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left) {
                        m_player.velocity.x = -PLAYER_SPEED;
                        m_player.facingRight = false;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right) {
                        m_player.velocity.x = PLAYER_SPEED;
                        m_player.facingRight = true;
                    }
                    else if ((keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Space) && m_player.onGround) {
                        m_player.velocity.y = -JUMP_STRENGTH;
                        m_player.onGround = false;
                    }
                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    if ((keyReleased->code == sf::Keyboard::Key::A || keyReleased->code == sf::Keyboard::Key::Left) && m_player.velocity.x < 0) {
                        m_player.velocity.x = 0;
                    }
                    else if ((keyReleased->code == sf::Keyboard::Key::D || keyReleased->code == sf::Keyboard::Key::Right) && m_player.velocity.x > 0) {
                        m_player.velocity.x = 0;
                    }
                }
                if ((sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))
                    && m_player.shootTimer <= 0) {
                    m_bullets.emplace_back(m_player.shape.getPosition(), m_player.facingRight); // Creates a Bullet object
                    m_player.shootTimer = SHOOT_COOLDOWN;
                }
            }
        }

        if (m_player.shootTimer > 0) {
            m_player.shootTimer -= deltaTime;
        }

        if (!m_gameOverState) {
            // Apply gravity
            m_player.velocity.y += GRAVITY * deltaTime;

            // Horizontal movement and collision
            m_player.shape.move({ m_player.velocity.x * deltaTime, 0.f });
            sf::FloatRect playerBounds = m_player.shape.getGlobalBounds();
            for (const auto& platform : m_platforms) {
                sf::FloatRect platformBounds = platform.getGlobalBounds();
                std::optional<sf::FloatRect> intersection;
                if ((intersection = playerBounds.findIntersection(platformBounds))) {
                    if (m_player.velocity.x > 0) {
                        m_player.shape.setPosition({
                            platformBounds.position.x - playerBounds.size.x,
                            playerBounds.position.y
                            });
                    }
                    else if (m_player.velocity.x < 0) {
                        m_player.shape.setPosition({
                            platformBounds.position.x + platformBounds.size.x,
                            playerBounds.position.y
                            });
                    }
                    m_player.velocity.x = 0;
                    playerBounds = m_player.shape.getGlobalBounds();
                }
            }

            // Vertical movement and collision
            m_player.onGround = false;
            m_player.shape.move({ 0.f, m_player.velocity.y * deltaTime });
            playerBounds = m_player.shape.getGlobalBounds();
            for (const auto& platform : m_platforms) {
                sf::FloatRect platformBounds = platform.getGlobalBounds();
                std::optional<sf::FloatRect> intersection;
                if ((intersection = playerBounds.findIntersection(platformBounds))) {
                    if (m_player.velocity.y > 0) {
                        m_player.shape.setPosition({
                            playerBounds.position.x,
                            platformBounds.position.y - playerBounds.size.y
                            });
                        m_player.onGround = true;
                    }
                    else if (m_player.velocity.y < 0) {
                        m_player.shape.setPosition({
                           playerBounds.position.x,
                           platformBounds.position.y + platformBounds.size.y
                            });
                    }
                    m_player.velocity.y = 0;
                    playerBounds = m_player.shape.getGlobalBounds();
                }
            }

            if (m_player.shape.getPosition().y + m_player.shape.getSize().y > WINDOW_HEIGHT + TILE_SIZE * 2) {
                m_player.takeDamage(); // Calls Player::takeDamage()
                if (m_player.lives > 0) {
                    m_player.shape.setPosition({ RESPAWN_X, RESPAWN_Y });
                    m_player.velocity = { 0,0 };
                }
            }
            if (m_player.shape.getPosition().x < 0) {
                m_player.shape.setPosition({ 0, m_player.shape.getPosition().y });
            }
            if (m_player.shape.getPosition().x + m_player.shape.getSize().x > WINDOW_WIDTH) {
                m_player.shape.setPosition({ WINDOW_WIDTH - m_player.shape.getSize().x, m_player.shape.getPosition().y });
            }


            for (size_t i = 0; i < m_bullets.size(); ++i) {
                m_bullets[i].shape.move({ m_bullets[i].velocity.x * deltaTime, m_bullets[i].velocity.y * deltaTime });
                bool bulletHitPlatform = false;
                sf::FloatRect bulletBounds = m_bullets[i].shape.getGlobalBounds();
                for (const auto& platform : m_platforms) {
                    std::optional<sf::FloatRect> intersection;
                    if ((intersection = bulletBounds.findIntersection(platform.getGlobalBounds()))) {
                        bulletHitPlatform = true;
                        break;
                    }
                }

                if (bulletHitPlatform || m_bullets[i].shape.getPosition().x < -m_bullets[i].shape.getSize().x || m_bullets[i].shape.getPosition().x > WINDOW_WIDTH) {
                    m_bullets.erase(m_bullets.begin() + i);
                    --i;
                }
            }

            if (m_player.lives <= 0) {
                m_gameOverState = true;
            }

            if (m_healthText) m_healthText->setString("Health: " + std::to_string(m_player.health));
            if (m_livesText) m_livesText->setString("Lives: " + std::to_string(m_player.lives));
        }

        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) {
            m_window->draw(platform);
        }
        m_window->draw(m_player.shape);
        for (const auto& bullet : m_bullets) {
            m_window->draw(bullet.shape);
        }

        if (m_healthText) m_window->draw(*m_healthText);
        if (m_livesText) m_window->draw(*m_livesText);

        if (m_gameOverState && m_gameOverText) {
            m_window->draw(*m_gameOverText);
        }
        m_window->display();
    }
}

//#include "Game.h"
//#include <thread>
//#include <mutex>
//std::mutex m;
//
//
//Game::Game(sf::RenderWindow* w)
//{
//	
//
//	InitPositions();
//
//	window = w;
//
//	width = window->getSize().x;
//	height = window->getSize().y;
//
//	//AQUI TENER UN RECIEVE MY COLOR O ALGO ASÍ 
//	playerNickName = Client::getInstance()->getNickname();
//	//playerNickName = "PLAYER"; //test
//
//
//	//init background
//	parchisBackgroundTexture = LoadTexture(spritesPath +parchisBackgroundImageName);
//	parchisBackgroundSprite = new sf::Sprite(parchisBackgroundTexture);
//	parchisBackgroundSprite->setPosition(sf::Vector2f(getCenter(parchisBackgroundTexture.getSize()).x, getCenter(parchisBackgroundTexture.getSize()).y));
//
//	yourTurnActiveTexture = LoadTexture(spritesPath + yourTurnSpriteImageName);
//	yourTurnInactiveTexture = LoadTexture(spritesPath + yourTurnInactiveSpriteImageName);
//	yourTurnSprite = new sf::Sprite(yourTurnInactiveTexture);
//	yourTurnSprite->setPosition(yourTurnImagePosition);
//
//	initPlayers((Pieces::Color)Client::getInstance()->getColor());
//
//	/*
//	if (isPlayer0Turn)
//	{
//		yourTurnSprite = new sf::Sprite(yourTurnActiveTexture);
//
//	}
//	else
//	{
//		yourTurnSprite = new sf::Sprite(yourTurnInactiveTexture);
//
//	}*/
//
//	//init diceTextures
//	sf::Texture auxText;
//	for (int i = 0; i < 6; i++)
//	{
//		auxText = LoadTexture(spritesPath + std::to_string(i+1) + diceImageName);
//		diceTextures.push_back(auxText);
//	}
//
//	diceSprite = new sf::Sprite(diceTextures[0]);
//	diceSprite->setPosition(sf::Vector2f(yourTurnImagePosition.x+50, yourTurnImagePosition.y + 300));
//
//
//	/////////////Texto
//	if (!font.openFromFile(fontsPath + fontName))
//	{
//		std::cerr << "Error al cargar la fuente" << std::endl;
//	}
//
//	playerNickText = new sf::Text(font, playerNickName, nickNameSize);
//	playerNickText->setFillColor(colorToColor(players[0].getColor()));
//	playerNickText->setPosition(sf::Vector2f(1010, 10));
//
//	throwDiceButton = new Button(sf::Vector2f(200, 60), sf::Vector2f(35, 637), throwDiceButtonString, font, buttonColor, buttonTextColor);
//}
//
//void Game::initPlayers(Pieces::Color c)
//{
//	players.clear();
//
//	players.emplace_back(Pieces::RED, redInitialPositions, spritesPath);
//	players.emplace_back(Pieces::GREEN, greenInitialPositions, spritesPath);
//	players.emplace_back(Pieces::YELLOW, yellowInitialPositions, spritesPath);
//	players.emplace_back(Pieces::BLUE, blueInitialPositions, spritesPath);
//
//	for (size_t i = 0; i < players.size(); ++i) {
//		if (players[i].getColor() == c) {
//			std::swap(players[0], players[i]);
//			break;
//		}
//	}
//
//	switch (c) {
//	case Pieces::RED:
//		//--isPlayer0Turn = true;
//		turnsLeft = 1;
//		UpdateTurn();
//		break;
//
//	case Pieces::BLUE:
//		turnsLeft = 1;
//		break;
//
//	case Pieces::GREEN:
//		turnsLeft = 3;
//		break;
//
//	case Pieces::YELLOW:
//		turnsLeft = 2;
//		break;
//	
//	default:
//		turnsLeft = 0;
//		std::cerr << "ERROR con el color y el turn inicial." << std::endl;
//		break;
//	}
//
//
//}
//
///*
//void Game::initPlayers(Pieces::Color c)
//{
//	players.clear();
//
//	players.emplace_back(Pieces::RED, redInitialPositions, spritesPath);
//	players.emplace_back(Pieces::GREEN, greenInitialPositions, spritesPath);
//	players.emplace_back(Pieces::YELLOW, yellowInitialPositions, spritesPath);
//	players.emplace_back(Pieces::BLUE, blueInitialPositions, spritesPath);
//
//	std::vector<Players>::iterator foundPlayer = std::find_if(players.begin(), players.end(),
//		[c](Players& p) {
//			return p.getColor() == c;
//		});
//
//	if (foundPlayer != players.end()) {
//		std::iter_swap(players.begin(), foundPlayer);
//	}
//}*/
//
//void Game::setWindow(sf::RenderWindow* win)
//{
//	this->window = win;
//}
//GameState Game::Update()
//{
//
//
//	while (window->isOpen()) {
//
//		Client::getInstance()->run();
//
//		//Wait?
//		//sf::sleep(sf::seconds(0.5f));
//		if (Client::getInstance()->isMoveReceived() ) {
//
//			MovePeerPiece(Client::getInstance()->getMovement());
//
//		}
//
//		while (const std::optional event = window->pollEvent()) {
//			EventHandler(*event);
//		}
//
//		//Cuando se detecte que un jugador gane
//		if (hasWin) // HASWIN()
//		{
//			Client::getInstance()->sendGameOver(Client::getInstance()->getColor());
//			return GameState::LOGIN;
//		}
//
//		
//
//		Render(window);
//		//LogCasillas(); //PRINTA EL VECTOR DE CASILLAS PARA VER SI SE ESTÁN PONIENDO CADA UNA EN SU SITIO CORRECTAMENTE (limpia la consola)
//		//movementRecievedFinished = false;
//
//	}
//
//	
//}
//
//void Game::EventHandler(const sf::Event& event)
//{
//	if (event.is<sf::Event::Closed>()) {
//		window->close();
//	}
//
//	if (const sf::Event::KeyPressed* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
//		switch (keyPressed->code)
//		{
//		case sf::Keyboard::Key::Escape:
//			window->close();
//			break;
//		case sf::Keyboard::Key::Space: // prueba simulando otro player
//			//hasWin = true;
//			UpdateTurn();
//			MoveSelectedPiece(Pieces::BLUE, 0, 5);
//			break;
//		//PARA DEBUGAR FORZAR DADO
//		case sf::Keyboard::Key::Num1:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				UpdateTurn();
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 1);
//			}
//
//			break;
//		case sf::Keyboard::Key::Num2:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				UpdateTurn();
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 2);
//			}
//
//			break;
//		case sf::Keyboard::Key::Num3:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				UpdateTurn();
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 3);
//			}
//
//			break;
//		case sf::Keyboard::Key::Num4:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				UpdateTurn();
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 4);
//			}
//
//			break;
//		case sf::Keyboard::Key::Num5:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				UpdateTurn();
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 5);
//			}
//
//			break;
//		case sf::Keyboard::Key::Num6:
//			if (players[0].hasAnyPieceSelected() && isPlayer0Turn) {
//				//UpdateTurn(); Si saca 6 vuelve a tirar 
//				MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), 6);
//			}
//
//			break;
//		default:
//			break;
//		}
//	}
//
//	if (const sf::Event::MouseButtonPressed* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
//
//		switch (mousePressed->button)
//		{
//		case sf::Mouse::Button::Left:
//			//std::cout << mousePressed->position.x << " , " << mousePressed->position.y << std::endl;
//			CheckPieceClick(sf::Vector2f(mousePressed->position.x, mousePressed->position.y), 0); //Player 0 -> el cliente
//			//CheckPieceClick(sf::Vector2f(mousePressed->position.x, mousePressed->position.y), 1); //para testeos
//			//CheckPieceClick(sf::Vector2f(mousePressed->position.x, mousePressed->position.y), 2); //para testeos
//			//CheckPieceClick(sf::Vector2f(mousePressed->position.x, mousePressed->position.y), 3); //para testeos
//
//			break;
//
//		case sf::Mouse::Button::Right:
//
//			break;
//		default:
//			break;
//		}
//	}
//
//	if (throwDiceButton && throwDiceButton->handleEvent(event, *window)) {
//		if (isPlayer0Turn) {
//			std::cout << std::endl << "Turns Left: " << turnsLeft;
//
//			ThrowDice();
//		}
//
//	}
//}
//
//
//sf::Vector2f Game::getCenter(sf::Vector2u recSize)
//{
//	return sf::Vector2f((width - recSize.x)/2, (height - recSize.y) / 2);
//}
//
//sf::Texture Game::LoadTexture(const std::string& filePath)
//{
//	sf::Texture text;
//	if (!text.loadFromFile(filePath)) {
//		std::cerr << "Error al cargar sprite" << std::endl;
//		return text;
//	}
//	return text;
//}
//
//void Game::InitPositions()
//{
//	// Creamos todas las casillas del recorrido general como objetos únicos
//	recorridoGeneral.clear();
//	recorridoGeneral.push_back(new Casillas(0, 0)); // Posición 0 no jugable
//	recorridoGeneral.push_back(new Casillas(715, 688)); // Posición 1
//	recorridoGeneral.push_back(new Casillas(715, 655)); // Posición 2
//	recorridoGeneral.push_back(new Casillas(715, 622)); // Posición 3
//	recorridoGeneral.push_back(new Casillas(715, 589)); // Posición 4
//	recorridoGeneral.push_back(new Casillas(715, 556)); // Posición 5
//	recorridoGeneral.push_back(new Casillas(715, 523)); // Posición 6
//	recorridoGeneral.push_back(new Casillas(715, 490)); // Posición 7
//	recorridoGeneral.push_back(new Casillas(715, 457)); // Posición 8
//
//	recorridoGeneral.push_back(new Casillas(737, 433)); // Posición 9
//	recorridoGeneral.push_back(new Casillas(770, 433)); // Posición 10
//	recorridoGeneral.push_back(new Casillas(803, 433)); // Posición 11
//	recorridoGeneral.push_back(new Casillas(836, 433)); // Posición 12
//	recorridoGeneral.push_back(new Casillas(869, 433)); // Posición 13
//	recorridoGeneral.push_back(new Casillas(902, 433)); // Posición 14
//	recorridoGeneral.push_back(new Casillas(935, 433)); // Posición 15
//	recorridoGeneral.push_back(new Casillas(968, 433)); // Posición 16
//
//	recorridoGeneral.push_back(new Casillas(968, 358)); // Posición 17
//	recorridoGeneral.push_back(new Casillas(968, 281)); // Posición 18
//
//	recorridoGeneral.push_back(new Casillas(937, 284)); // Posición 19
//	recorridoGeneral.push_back(new Casillas(903, 284)); // Posición 20
//	recorridoGeneral.push_back(new Casillas(870, 284)); // Posición 21
//	recorridoGeneral.push_back(new Casillas(837, 284)); // Posición 22
//	recorridoGeneral.push_back(new Casillas(804, 284)); // Posición 23
//	recorridoGeneral.push_back(new Casillas(770, 284)); // Posición 24
//	recorridoGeneral.push_back(new Casillas(737, 284)); // Posición 25
//
//	recorridoGeneral.push_back(new Casillas(718, 265)); // Posición 26
//	recorridoGeneral.push_back(new Casillas(718, 232)); // Posición 27
//	recorridoGeneral.push_back(new Casillas(718, 199)); // Posición 28
//	recorridoGeneral.push_back(new Casillas(718, 166)); // Posición 29
//	recorridoGeneral.push_back(new Casillas(718, 133)); // Posición 30
//	recorridoGeneral.push_back(new Casillas(718, 100)); // Posición 31
//	recorridoGeneral.push_back(new Casillas(718, 67));  // Posición 32
//	recorridoGeneral.push_back(new Casillas(718, 31));  // Posición 33
//
//	recorridoGeneral.push_back(new Casillas(640, 31));  // Posición 34
//	recorridoGeneral.push_back(new Casillas(564, 31));  // Posición 35
//
//	recorridoGeneral.push_back(new Casillas(564, 64));  // Posición 36
//	recorridoGeneral.push_back(new Casillas(564, 97));  // Posición 37
//	recorridoGeneral.push_back(new Casillas(564, 130)); // Posición 38
//	recorridoGeneral.push_back(new Casillas(564, 163)); // Posición 39
//	recorridoGeneral.push_back(new Casillas(564, 196)); // Posición 40
//	recorridoGeneral.push_back(new Casillas(564, 229)); // Posición 41
//	recorridoGeneral.push_back(new Casillas(564, 264)); // Posición 42
//
//	recorridoGeneral.push_back(new Casillas(547, 284)); // Posición 43
//	recorridoGeneral.push_back(new Casillas(513, 284)); // Posición 44
//	recorridoGeneral.push_back(new Casillas(480, 284)); // Posición 45
//	recorridoGeneral.push_back(new Casillas(446, 284)); // Posición 46
//	recorridoGeneral.push_back(new Casillas(413, 284)); // Posición 47
//	recorridoGeneral.push_back(new Casillas(380, 284)); // Posición 48
//	recorridoGeneral.push_back(new Casillas(346, 284)); // Posición 49
//
//	recorridoGeneral.push_back(new Casillas(312, 281));  // Posición 50
//	recorridoGeneral.push_back(new Casillas(312, 358));  // Posición 51
//
//	recorridoGeneral.push_back(new Casillas(312, 433)); // Posición 52
//	recorridoGeneral.push_back(new Casillas(346, 433)); // Posición 53
//	recorridoGeneral.push_back(new Casillas(380, 433)); // Posición 54
//	recorridoGeneral.push_back(new Casillas(413, 433)); // Posición 55
//	recorridoGeneral.push_back(new Casillas(446, 433)); // Posición 56
//	recorridoGeneral.push_back(new Casillas(480, 433)); // Posición 57
//	recorridoGeneral.push_back(new Casillas(513, 433)); // Posición 58
//	recorridoGeneral.push_back(new Casillas(547, 433));  // Posición 59
//
//	recorridoGeneral.push_back(new Casillas(564, 457)); // Posición 60
//	recorridoGeneral.push_back(new Casillas(564, 490)); // Posición 61
//	recorridoGeneral.push_back(new Casillas(564, 523)); // Posición 62
//	recorridoGeneral.push_back(new Casillas(564, 556)); // Posición 63
//	recorridoGeneral.push_back(new Casillas(564, 589)); // Posición 64
//	recorridoGeneral.push_back(new Casillas(564, 622)); // Posición 65
//	recorridoGeneral.push_back(new Casillas(564, 655)); // Posición 66
//	recorridoGeneral.push_back(new Casillas(564, 688));  // Posición 67
//
//	recorridoGeneral.push_back(new Casillas(640, 688));  // Posición 68
//
//	// Ahora creamos las casillas finales
//	recorridoFinalRojo.clear();
//	recorridoFinalRojo.push_back(new Casillas(640, 64));
//	recorridoFinalRojo.push_back(new Casillas(640, 97));
//	recorridoFinalRojo.push_back(new Casillas(640, 130));
//	recorridoFinalRojo.push_back(new Casillas(640, 163));
//	recorridoFinalRojo.push_back(new Casillas(640, 196));
//	recorridoFinalRojo.push_back(new Casillas(640, 229));
//	recorridoFinalRojo.push_back(new Casillas(640, 264));
//	recorridoFinalRojo.push_back(new Casillas(640, 310));
//
//	recorridoFinalAmarillo.clear();
//	recorridoFinalAmarillo.push_back(new Casillas(640, 655));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 622));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 589));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 556));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 523));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 490));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 457));
//	recorridoFinalAmarillo.push_back(new Casillas(640, 410));
//
//	recorridoFinalVerde.clear();
//	recorridoFinalVerde.push_back(new Casillas(346, 358));
//	recorridoFinalVerde.push_back(new Casillas(380, 358));
//	recorridoFinalVerde.push_back(new Casillas(413, 358));
//	recorridoFinalVerde.push_back(new Casillas(446, 358));
//	recorridoFinalVerde.push_back(new Casillas(480, 358));
//	recorridoFinalVerde.push_back(new Casillas(513, 358));
//	recorridoFinalVerde.push_back(new Casillas(547, 358));
//	recorridoFinalVerde.push_back(new Casillas(580, 358));
//
//	recorridoFinalAzul.clear();
//	recorridoFinalAzul.push_back(new Casillas(937, 358));
//	recorridoFinalAzul.push_back(new Casillas(903, 358));
//	recorridoFinalAzul.push_back(new Casillas(870, 358));
//	recorridoFinalAzul.push_back(new Casillas(837, 358));
//	recorridoFinalAzul.push_back(new Casillas(804, 358));
//	recorridoFinalAzul.push_back(new Casillas(770, 358));
//	recorridoFinalAzul.push_back(new Casillas(737, 358));
//	recorridoFinalAzul.push_back(new Casillas(700, 358));
//
//	// Construimos los recorridos finales
//	recorridoRojo = construirRecorridoCompleto(recorridoFinalRojo, casillaSalidaRojo);
//	recorridoAmarillo = construirRecorridoCompleto(recorridoFinalAmarillo, casillaSalidaAmarillo);
//	recorridoVerde = construirRecorridoCompleto(recorridoFinalVerde, casillaSalidaVerde);
//	recorridoAzul = construirRecorridoCompleto(recorridoFinalAzul, casillaSalidaAzul);
//	
//	// Inicializamos las posiciones iniciales
//	redInitialPositions = std::vector<sf::Vector2i>{
//		sf::Vector2i(390, 110),
//		sf::Vector2i(430, 110),
//		sf::Vector2i(390, 150),
//		sf::Vector2i(430, 150)
//	};
//
//	greenInitialPositions = std::vector<sf::Vector2i>{
//		sf::Vector2i(390, 570),
//		sf::Vector2i(430, 570),
//		sf::Vector2i(390, 610),
//		sf::Vector2i(430, 610)
//	};
//
//	yellowInitialPositions = std::vector<sf::Vector2i>{
//		sf::Vector2i(850, 570),
//		sf::Vector2i(890, 570),
//		sf::Vector2i(850, 610),
//		sf::Vector2i(890, 610)
//	};
//
//	blueInitialPositions = std::vector<sf::Vector2i>{
//		sf::Vector2i(850, 110),
//		sf::Vector2i(890, 110),
//		sf::Vector2i(850, 150),
//		sf::Vector2i(890, 150)
//	};
//
//	// Inicializamos las posiciones finales
//	redFinalPositions = {
//		{sf::Vector2i(640, 340), false},
//		{sf::Vector2i(610, 310), false},
//		{sf::Vector2i(670, 310), false},
//		{sf::Vector2i(640, 310), false}
//	};
//
//	greenFinalPositions = {
//		{sf::Vector2i(615, 358), false},
//		{sf::Vector2i(580, 388), false},
//		{sf::Vector2i(580, 328), false},
//		{sf::Vector2i(580, 358), false}
//	};
//
//	yellowFinalPositions = {
//		{sf::Vector2i(640, 378), false},
//		{sf::Vector2i(670, 410), false},
//		{sf::Vector2i(610, 410), false},
//		{sf::Vector2i(640, 410), false}
//	};
//
//	blueFinalPositions = {
//		{sf::Vector2i(660, 358), false},
//		{sf::Vector2i(700, 388), false},
//		{sf::Vector2i(700, 328), false},
//		{sf::Vector2i(700, 358), false}
//	};
//
//}
//
//std::vector<Casillas*> Game::construirRecorridoCompleto(const std::vector<Casillas*>& recorridoFinal, const int casillaSalida)
//{
//	std::vector<Casillas*> recorridoCompleto;
//
//	int totalCasillas = recorridoGeneral.size();
//
//	for (int i = casillaSalida; i < totalCasillas; i++) {
//		recorridoCompleto.push_back(recorridoGeneral[i]);
//	}
//
//	for (int i = 1; i < (casillaSalida - 4); i++) {
//		recorridoCompleto.push_back(recorridoGeneral[i]);
//	}
//
//	for (int i = 0; i < recorridoFinal.size(); i++) {
//		recorridoCompleto.push_back(recorridoFinal[i]);
//	}
//
//	return recorridoCompleto;
//}
//
//
//
//void Game::LoadSprite(const std::string& filePath)
//{
//	sf::Texture text;
//
//	if (!text.loadFromFile(filePath)) {
//		std::cerr << "Error al cargar sprite" << std::endl;
//		return;
//	}
//}
//
//
//bool Game::IsInArea(sf::Vector2f pos, sf::RectangleShape& square)
//{
//	int squareX = square.getPosition().x;
//	int squareY = square.getPosition().y;
//	sf::Vector2f size = square.getSize();
//
//	return (pos.x >= squareX && pos.x <= squareX + size.x &&
//		pos.y >= squareY && pos.y <= squareY + size.y);
//
//}
//bool Game::IsInArea(sf::Vector2f pos, Pieces p) //Cambiar por el area del circulo
//{
//	int squareX = p.getPosition().x;
//	int squareY = p.getPosition().y;
//	sf::Vector2u size = p.getSize();
//
//	return (pos.x >= squareX && pos.x <= squareX + size.x &&
//		pos.y >= squareY && pos.y <= squareY + size.y);
//
//}
//
//void Game::CheckPieceClick(sf::Vector2f mousePos, int numPlayer)
//{
//
//	for (int i = 0; i < players[numPlayer].GetPieces().size(); i++) // cambiar a que solo sea del player0
//	{
//		if (IsInArea(mousePos, *players[numPlayer].GetPieces()[i])) {
//			if (players[numPlayer].GetPieces()[i]->getSelected()) {
//				players[numPlayer].GetPieces()[i]->changeTextureToNonSelected();
//
//			}
//			else {
//				for (int j = 0; j < players[numPlayer].GetPieces().size(); j++)
//					players[numPlayer].GetPieces()[j]->changeTextureToNonSelected();  //deseleccionar todas las fichas
//
//				players[numPlayer].GetPieces()[i]->changeTextureToSelected();
//
//			}
//		}
//	}
//}
//
//void Game::MoveSelectedPiece(Pieces::Color c, int pieceId, int moves)
//{
//
//	Players* p = getPlayerbyColor(c);
//
//	// Si el movimiento viene del cliente, mandarlo a los otros.
//	if (p->getColor() == players[0].getColor()) {
//		std::cout << std::endl << std::endl << std::endl << "---MANDO MOVIMIENTO---" << std::endl << std::endl;
//		Client::getInstance()->sendMove(p->getColor(), pieceId, moves);
//	}
//
//	int currentPos = p->getNumCasillaFromPieceId(pieceId);
//	Pieces* selectedPiece = p->getPieceById(pieceId);
//	if (selectedPiece->isHome()){
//		return;
//	}
//	std::vector<Casillas*>* recorrido = nullptr;
//
//	switch (c) {
//	case Pieces::RED:    recorrido = &recorridoRojo;    break;
//	case Pieces::YELLOW: recorrido = &recorridoAmarillo; break;
//	case Pieces::GREEN:  recorrido = &recorridoVerde;   break;
//	case Pieces::BLUE:   recorrido = &recorridoAzul;    break;
//	default: return;
//	}
//
//	// Si está en casa y no saca un 5, no se mueve
//	if (currentPos == -1 && moves != 5)
//		return;
//
//	int newPos;
//	if (currentPos == -1 && moves == 5) {
//		// Salir de casa a la primera casilla
//		newPos = 0;
//	}
//	else {
//		newPos = currentPos + moves;
//	}
//
//	//Pieza llega al final
//	if (newPos >= recorrido->size() - 1) {
//		MovePieceToHome(p,pieceId);
//		(*recorrido)[currentPos]->removePiece(selectedPiece);
//		(*recorrido)[currentPos]->removeBarrier();
//		return;
//		//newPos = recorrido->size() - 1;
//
//	}
//
//	// si hay barrera en el camino ponerse detras
//	int finalPos = newPos;
//	for (int i = currentPos + 1; i <= newPos; i++) {
//		if (i < 0 || i >= recorrido->size()) continue;
//
//		std::vector<Pieces*> piezas = (*recorrido)[i]->getPieces();
//		if (piezas.size() >= 2) {
//			if (i == 0) {
//				std::cout << std::endl << std::endl << std::endl << "Has intentado sacar ficha en una barrera -- PIERDES TURNO --" << std::endl << std::endl;
//				return;
//			}
//			finalPos = i - 1;
//			break;
//		}
//	}
//
//	newPos = finalPos;
//
//	bool bonus20 = false;
//	// Matar fichas enemigas en las casillas intermedias 
//	for (int i = currentPos + 1; i <= newPos; i++) {
//		if (i < 0 || i >= recorrido->size()) continue;
//
//		Casillas* casilla = (*recorrido)[i];
//		std::vector<Pieces*> piezas = casilla->getPieces();
//
//		if (piezas.size() >= 2) continue; // Barrera, no se puede matar
//
//		for (int j = 0; j < piezas.size(); j++) {
//			Pieces* otra = piezas[j];
//			if (otra->getColor() != selectedPiece->getColor()) {
//				Players* enemigo = getPlayerbyColor(otra->getColor());
//				killPiece(enemigo, otra);
//				casilla->removePiece(otra);
//				bonus20 = true; // Por matar una ficha se moverá 20
//			}
//		}
//	}
//
//	if (!(*recorrido)[newPos]->addPiece(selectedPiece)) {
//		return;
//	}
//
//	// Quitar de la casilla actual
//	if (currentPos > -1) {
//		(*recorrido)[currentPos]->removePiece(selectedPiece);
//		(*recorrido)[currentPos]->removeBarrier();
//	}
//
//	// Mover la pieza y actualizar la posición
//	p->movePiece((*recorrido)[newPos]->getPosition(), newPos, pieceId);
//	(*recorrido)[newPos]->makeBarrier();
//
//	if (bonus20)
//	{
//		MoveSelectedPiece(c, pieceId, 20);
//		std::cout << ColorToString(c) <<" avanza 20 casillas por matar una ficha !! " <<  std::endl;
//	}
//}
//
///*
//void Game::MovePieceRecieved(Pieces::Color c, int pieceId, int numMoves) // Este no deberia usarse ya. Usar el otro nuevo de arriba
//{
//
//	int moves = numMoves;
//	Players p = getPlayerbyColor(c);
//
//
//	int n = p.getNumCasillaFromPieceId(pieceId);
//	int newPos = n + moves;
//	std::cout << "COLOR: " << (int)p.getColor();
//	Pieces auxP;
//
//	switch (c)
//	{
//	case Pieces::RED:
//		newPos = n + moves;
//		if (newPos >= recorridoRojo.size() - 1)
//			newPos = recorridoRojo.size() - 1;
//		auxP.setColor(Pieces::RED);
//		recorridoRojo[newPos]->addPiece(auxP);
//		p.movePiece(recorridoRojo[newPos]->getPosition(), newPos, pieceId);
//		break;
//	case Pieces::YELLOW:
//		newPos = n + moves;
//		if (newPos >= recorridoAmarillo.size() - 1)
//			newPos = recorridoAmarillo.size() - 1;
//		auxP.setColor(Pieces::YELLOW);
//		recorridoAmarillo[newPos]->addPiece(auxP);
//		p.movePiece(recorridoAmarillo[newPos]->getPosition(), newPos, pieceId);
//		break;
//	case Pieces::GREEN:
//		newPos = n + moves;
//		if (newPos >= recorridoVerde.size() - 1)
//			newPos = recorridoVerde.size() - 1;
//		auxP.setColor(Pieces::GREEN);
//		recorridoVerde[newPos]->addPiece(auxP);
//		p.movePiece(recorridoVerde[newPos]->getPosition(), newPos, pieceId);
//		break;
//	case Pieces::BLUE:
//		newPos = n + moves;
//		if (newPos >= recorridoAzul.size() - 1)
//			newPos = recorridoAzul.size() - 1;
//		auxP.setColor(Pieces::BLUE);
//		recorridoAzul[newPos]->addPiece(auxP);
//		p.movePiece(recorridoAzul[newPos]->getPosition(), newPos, pieceId);
//		break;
//	default:
//		break;
//	}
//
//}
//*/
//
//
//void Game::MovePeerPiece(std::tuple<int, int, int> movement)
//{
//	//Actualizar movimiento de los otros peers
//	std::cout << "--- MUEVO PIEZA DEL PLAYER: " << ColorToString((Pieces::Color)std::get<0>(movement))  << std::endl;
//	if (std::get<2>(movement) != 6)
//		UpdateTurn();
//
//	MoveSelectedPiece((Pieces::Color)std::get<0>(movement), std::get<1>(movement), std::get<2>(movement));
// 
//}
//
//
//
//void Game::ThrowDice()
//{
//	if (!players[0].hasAnyPieceSelected()) {
//		std::cout << "Ninguna Piece seleccionada" << std::endl;
//		return;
//	}
//	int random;
//
//	random = (rand() % 6) + 1;
//	std::cout << "Throw DICE: Numero de DADO: " << random << std::endl;
//
//	diceSprite->setTexture(diceTextures[random -1]);
//	
//	if (random == 5){ //Si saca un 5 sale una casilla forzado
//		int id = players[0].getPieceInInitPositionID();
//		if (id != -1) {
//			UpdateTurn();
//			MoveSelectedPiece(players[0].getColor(), id, random);
//		} 
//		return;
//
//	}
//
//	if (random != 6)
//		UpdateTurn();
//	else
//		std::cout << "¡¡Te ha tocado un 6, VUELVES A TIRAR!!" << std::endl;
//
//	MoveSelectedPiece(players[0].getColor(), players[0].getSelectedPieceID(), random);
//
//
//}
//
//
//void Game::UpdateTurn()
//{
//
//	std::cout << "1 UpdateTurn llamado con turnos left: " << turnsLeft << std::endl;
//	if (turnsLeft > 0) {
//		std::cout << "2 Le resto un turno left: " << turnsLeft << std::endl;
//
//		turnsLeft--;
//	}
//	// Cambiamos el sprite dependiendo de si es el turno del cliente (jugador 0)
//	if (turnsLeft == 0) {
//		std::cout << "3.1 Activo IsYourTURN y turnsLeft se queda en: " << turnsLeft << std::endl;
//
//		turnsLeft = players.size();
//		isPlayer0Turn = true;
//		yourTurnSprite->setTexture(yourTurnActiveTexture);
//	}
//	else {
//		std::cout << "3.2 NO ES TU TURNO, turns left; " << turnsLeft << std::endl;
//
//		isPlayer0Turn = false;
//		yourTurnSprite->setTexture(yourTurnInactiveTexture);
//	}
//	std::cout << std::endl;
//
//}
//
//sf::Color Game::colorToColor(Pieces::Color c)
//{
//	switch (c)
//	{
//	case Pieces::RED:
//		return sf::Color::Red;
//	case Pieces::YELLOW:
//		return sf::Color::Yellow;
//	case Pieces::GREEN:
//		return sf::Color::Green;
//	case Pieces::BLUE:
//		return sf::Color::Blue;
//	default:
//		return sf::Color::White;
//	}
//}
//
//void Game::Render(sf::RenderWindow* window)
//{
//	window->clear(backgroundColor); 
//	window->draw(*parchisBackgroundSprite);
//	window->draw(*yourTurnSprite);
//	window->draw(*diceSprite);
//	window->draw(*playerNickText);
//
//	
//	
//	for (int i = 0; i < players.size(); i++)
//	{
//		players[i].DrawPieces(window);
//	}
//	
//	if (throwDiceButton)
//		throwDiceButton->draw(*window); // Dibuja el botón
//	window->display();
//}
//
//
//
//void Game::LiberarMemoria()
//{
//	// Liberar la memoria de los recorridos generales
//	for (auto& casilla : recorridoGeneral) {
//		delete casilla; // Liberar cada puntero de Casillas
//	}
//	recorridoGeneral.clear(); // Limpiar el vector
//
//	// Liberar la memoria de los recorridos finales
//	for (auto& casilla : recorridoFinalRojo) {
//		delete casilla; // Liberar cada puntero de Casillas
//	}
//	recorridoFinalRojo.clear();
//
//	for (auto& casilla : recorridoFinalAmarillo) {
//		delete casilla; // Liberar cada puntero de Casillas
//	}
//	recorridoFinalAmarillo.clear();
//
//	for (auto& casilla : recorridoFinalVerde) {
//		delete casilla; // Liberar cada puntero de Casillas
//	}
//	recorridoFinalVerde.clear();
//
//	for (auto& casilla : recorridoFinalAzul) {
//		delete casilla; // Liberar cada puntero de Casillas
//	}
//	recorridoFinalAzul.clear();
//
//	// Asegurarnos de que no quedan punteros colgando
//	recorridoRojo.clear();
//	recorridoAmarillo.clear();
//	recorridoVerde.clear();
//	recorridoAzul.clear();
//}
//
//
//
//Players* Game::getPlayerbyColor(Pieces::Color c)
//{
//	for (int i = 0; i < players.size(); i++)
//	{
//		if (players[i].getColor() == c) {
//			return &players[i];
//		}
//	}
//	std::cerr << "PLAYERBYCOLOR NOT FOUND" << std::endl;
//	return &players[0];
//}
// 
//void Game::killPiece(Players* player, Pieces* piece)
//{
//	std::vector<sf::Vector2i> initialPositions;
//	switch (piece->getColor())
//	{
//	case Pieces::RED:
//		initialPositions = redInitialPositions;
//		break;
//	case Pieces::YELLOW:
//		initialPositions = yellowInitialPositions;
// 		break;
//	case Pieces::GREEN:
//		initialPositions = greenInitialPositions;
// 		break;
//	case Pieces::BLUE:
//		initialPositions = blueInitialPositions;
// 		break;
//	default:
// 		break;
//	}
//	std::vector<Pieces*> playerPieces = player->GetPieces();
//
//	for (int i = 0; i < initialPositions.size(); i++)
//	{
//		bool occupied = false;
//
//		for (int j = 0; j < playerPieces.size(); j++)
//		{
//			if (playerPieces[j]->getPosition() == initialPositions[i])
//			{
//				occupied = true;
//				break;
//			}
//		}
//
//		if (!occupied)
//		{
//			piece->setPosition(initialPositions[i]);
//			piece->resetNumCasilla();
//			return;
//		}
//	}
//
//
//}
//void Game::MovePieceToHome(Players* player, int pieceID)
//{
//	std::vector<std::tuple<sf::Vector2i, bool>>* finalPositions;
//
//	// Determinar el vector correspondiente al color de la pieza
//	switch (player->getColor()) {
//	case Pieces::RED:
//		finalPositions = &redFinalPositions;
//		break;
//	case Pieces::YELLOW:
//		finalPositions = &yellowFinalPositions;
//		break;
//	case Pieces::GREEN:
//		finalPositions = &greenFinalPositions;
//		break;
//	case Pieces::BLUE:
//		finalPositions = &blueFinalPositions;
//		break;
//	default:
//		return;
//	}
//	
//
//
//	// Buscar la primera posición libre (donde el bool es false)
//	for (int i = 0; i < finalPositions->size(); ++i) {
//
// 		sf::Vector2i pos = std::get<0>((*finalPositions)[i]);
//		bool& ocupado = std::get<1>((*finalPositions)[i]);
//		
//
//		if (!ocupado) {
//			// Mover la pieza a esa posición
//			player->movePieceHome(pos, pieceID);
//			// Marcar la posición como ocupada
//			ocupado = true;
//			std::cout << "Pieza " << ColorToString(player->getColor()) << " movida a su casa en "
//				<< pos.x << ", " << pos.y << std::endl;
//
//			// Verificar si ya están todas las posiciones ocupadas
//			bool todasOcupadas = true;
//			for (const auto& pos : *finalPositions) {
//				if (!std::get<1>(pos)) {
//					todasOcupadas = false;
//					break;
//				}
//			}
//			if (todasOcupadas) {
//				hasWin = true;
//			}
//
//			return;
//		}
//	}
//	// Si no hay espacio libre
//	std::cout << "No hay espacio disponible en casa para la pieza de color "
//		<< ColorToString(player->getColor()) << std::endl;
//}
//
//std::string Game::ColorToString(Pieces::Color c)
//{
//	switch (c) {
//	case Pieces::RED:    return "ROJO";
//	case Pieces::GREEN:  return "VERDE";
//	case Pieces::YELLOW: return "AMARILLO";
//	case Pieces::BLUE:   return "AZUL";
//	default:             return "Desconocida";
//	}
//}
//////chatgpt-- grande
////void Game::LogCasillas()
////{
////	// 1. Limpiar la consola
////#ifdef _WIN32
////	system("cls"); // Comando para Windows
////#else
////	// Asumiendo Linux/macOS
////	system("clear"); // Comando para sistemas tipo Unix
////#endif
////
////	std::cout << "--- Estado Actual de Casillas Ocupadas ---" << std::endl;
////	bool foundOccupied = false;
////
////	// Función auxiliar para procesar un recorrido (evita repetir código)
////	auto checkAndPrintRecorrido = [&](const std::vector<Casillas*>& recorrido, const std::string& nombreRecorrido, const std::string& prefix) {
////		for (size_t i = 0; i < recorrido.size(); ++i) {
////			if (recorrido[i]) { // Comprobar que el puntero no sea nulo
////				// *** AJUSTA ESTA LÍNEA SEGÚN TU CLASE Casillas ***
////				auto piecesEnCasilla = recorrido[i]->getPieces(); // <- ¡MODIFICA SI ES NECESARIO!
////
////				if (!piecesEnCasilla.empty()) { // Si la casilla no está vacía
////					foundOccupied = true; // Marcamos que hemos encontrado algo
////					std::cout << nombreRecorrido << " - Casilla " << prefix << i
////						<< " (Pos: " << recorrido[i]->getPosition().x << "," << recorrido[i]->getPosition().y << "): "; // Asume que Casillas tiene getPosition()
////
////					// Itera sobre las piezas en la casilla
////					for (const auto& pieza : piecesEnCasilla) { // Asume que getPieces() devuelve algo iterable que contiene objetos Pieces (o punteros)
////						// *** AJUSTA ESTA LÍNEA SI getPieces() DEVUELVE PUNTEROS ***
////						// Ejemplo si devuelve std::vector<Pieces*>: std::cout << ColorToString(pieza->getColor()) << " ";
////						std::cout << ColorToString(pieza->getColor()) << " "; // Asume que pieza es un objeto Pieces o referencia
////					}
////					std::cout << std::endl;
////				}
////			}
////		}
////		};
////
////	// 2. Revisar Recorrido General
////	checkAndPrintRecorrido(recorridoGeneral, "General", "");
////
////	// 3. Revisar Recorridos Finales
////	checkAndPrintRecorrido(recorridoFinalRojo, "Final Rojo", "FR");
////	checkAndPrintRecorrido(recorridoFinalAmarillo, "Final Amarillo", "FAm");
////	checkAndPrintRecorrido(recorridoFinalVerde, "Final Verde", "FV");
////	checkAndPrintRecorrido(recorridoFinalAzul, "Final Azul", "FAz");
////
////
////	if (!foundOccupied) {
////		std::cout << "(Todas las casillas revisadas están vacías)" << std::endl;
////	}
////
////	std::cout << "------------------------------------------" << std::endl;
////	
////
////	// Opcional: Pausa breve para poder leer el log antes de que se actualice
////	std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pausa de 0.5 segundos
////}
////
////Game::~Game()
////{
////	LiberarMemoria();
////}