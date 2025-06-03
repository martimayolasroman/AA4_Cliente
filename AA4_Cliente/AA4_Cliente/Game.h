#pragma once
#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Angle.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>

// --- Color Constants ---
const sf::Color COLOR_BACKGROUND(236, 236, 236);
const sf::Color COLOR_PLAYER_BLUE(sf::Color::Blue);
const sf::Color COLOR_BULLET_YELLOW(sf::Color::Red);
const sf::Color COLOR_PLATFORM_BROWN(139, 69, 19);
const sf::Color COLOR_DEFAULT_FLOOR_GRAY(100, 100, 100);
const sf::Color COLOR_TEXT_WHITE(sf::Color::White);
const sf::Color COLOR_GAMEOVER_RED(sf::Color::Red);

// --- Other Constants ---
const unsigned int WINDOW_WIDTH = 1024;
const unsigned int WINDOW_HEIGHT = 768;
const float TILE_SIZE = 32.0f;

const float PLAYER_SPEED = 250.0f; // pixels per second
const float JUMP_STRENGTH = 550.0f;
const float GRAVITY = 1200.0f; // pixels per second^2
const float PLAYER_WIDTH = TILE_SIZE * 0.9f;
const float PLAYER_HEIGHT = TILE_SIZE * 1.4f;

const int PLAYER_HEALTH_MAX = 5;
const int PLAYER_LIVES_MAX = 3;
const float RESPAWN_X = 100.0f;
const float RESPAWN_Y = 100.0f;

const float BULLET_SPEED = 500.0f;
const float BULLET_WIDTH = 10.0f;
const float BULLET_HEIGHT = 5.0f;
const float SHOOT_COOLDOWN = 0.5f; // seconds

// Include Player and Bullet struct declarations after constants are defined
#include "Player.h" // Ahora solo contiene la declaración de struct Player
#include "Bullet.h" // Ahora solo contiene la declaración de struct Bullet

// Function to load map from .txt file (declaration)
std::vector<sf::RectangleShape> loadMap(const std::string& filename);

class Game {
public:
    Game(sf::RenderWindow* window);
    ~Game();
    void run();

private:
    sf::RenderWindow* m_window;
    Player m_player; // Usa Player struct de Player.h
    std::vector<sf::RectangleShape> m_platforms;
    std::vector<Bullet> m_bullets; // Usa Bullet struct de Bullet.h

    sf::Font m_font;
    sf::Text* m_healthText;
    sf::Text* m_livesText;
    sf::Text* m_gameOverText;

    bool m_gameOverState;
    sf::Clock m_clock;
};

#endif // GAME_H

//#include "Players.h"
//#include "Button.h"
//#include "Client.h"
//#include "GameState.h"
//
//class Game
//{
//	Button* throwDiceButton;
//	std::string throwDiceButtonString = "Throw DICE";
//
//	bool hasWin = false;
//
//	unsigned int width, height;
//	sf::RenderWindow* window;
//	sf::Color c = sf::Color::White;
//	sf::Color buttonColor = sf::Color(255, 165, 0);
//	sf::Color buttonTextColor = sf::Color::White;
//	sf::Color backgroundColor = sf::Color(240, 240, 240);
//
//	bool isPlayer0Turn = false;
//	int turnsLeft = 0;
//
//	sf::Texture parchisBackgroundTexture;
//	sf::Sprite* parchisBackgroundSprite;
//	const std::string parchisBackgroundImageName = "fondo.png";
//
//	sf::Texture yourTurnActiveTexture;
//	sf::Texture yourTurnInactiveTexture;
//	sf::Sprite* yourTurnSprite;
//	const std::string yourTurnSpriteImageName = "yourTurnActive.png";
//	const std::string yourTurnInactiveSpriteImageName = "yourTurnInactive.png";
//	sf::Vector2f yourTurnImagePosition = sf::Vector2f(35, 35);
//
//	//DiceTextures
//	std::vector<sf::Texture> diceTextures;
//	sf::Sprite* diceSprite;
//	const std::string diceImageName = "_dots.png";
//
//
//	std::vector<Players> players;
//	std::vector <sf::Vector2i> redInitialPositions;
//	std::vector <sf::Vector2i> greenInitialPositions;
//	std::vector <sf::Vector2i> yellowInitialPositions;
//	std::vector <sf::Vector2i> blueInitialPositions;
//
//	std::vector<std::tuple<sf::Vector2i, bool>> redFinalPositions;
//	std::vector<std::tuple<sf::Vector2i, bool>> greenFinalPositions;
//	std::vector<std::tuple<sf::Vector2i, bool>> yellowFinalPositions;
//	std::vector<std::tuple<sf::Vector2i, bool>> blueFinalPositions;
//
//	const std::string spritesPath = "Assets/Sprites/";
//	const std::string fontsPath = "Assets/Fonts/";
//
//	sf::Texture pieceTexture;
//	sf::Sprite* pieceSprite;
//
//	//Texto
//	sf::Font font;
//	const std::string fontName = "Straw Milky.otf";
//	std::string playerNickName = "PLAYER";
//	sf::Text* playerNickText;
//	int nickNameSize = 40;
//
//	sf::Color colorToColor(Pieces::Color c);
//
//
//	//Recorridos de los players. 
//	//Debería estar dentro de cada player? o no porque son cosas del tablero y el player no tiene porque conocer nada del tablero
//	std::vector<Casillas*> recorridoGeneral;
//
//	std::vector<Casillas*> recorridoRojo;
//	std::vector<Casillas*> recorridoAmarillo;
//	std::vector<Casillas*> recorridoVerde;
//	std::vector<Casillas*> recorridoAzul;
//
//
//	std::vector<Casillas*> recorridoFinalRojo;
//	std::vector<Casillas*> recorridoFinalAmarillo;
//	std::vector<Casillas*> recorridoFinalVerde;
//	std::vector<Casillas*> recorridoFinalAzul;
//	
//	int casillaSalidaRojo = 39; 
//	int casillaSalidaAmarillo = 5;
//	int casillaSalidaVerde = 56;
//	int casillaSalidaAzul = 22;
//
//
//	std::vector<sf::Vector2i> casillasToVectors( std::vector<Casillas> casillasVec) { //Borrar si no hace falta
//		std::vector<sf::Vector2i> posiciones;
// 		for (const auto& casilla : casillasVec) {
//			posiciones.push_back(casilla.getPosition());
//		}
//		return posiciones;
//	}
//
//
//	bool movementRecievedFinished = false;
//
//
//public:
//	Game(sf::RenderWindow*);
//	void setWindow(sf::RenderWindow* win);
//	GameState Update();
//private:
//	void initPlayers(Pieces::Color c);
//	sf::Vector2f getCenter(sf::Vector2u recSize);
//	sf::Texture LoadTexture(const std::string& filePath);
//	void InitPositions();
//	void LoadSprite(const std::string& filePath);
//	void Render(sf::RenderWindow* window);
//	void EventHandler(const sf::Event& event);
//	bool IsInArea(sf::Vector2f pos, sf::RectangleShape& square);
//	bool IsInArea(sf::Vector2f pos, Pieces p);
//
//	void CheckPieceClick(sf::Vector2f mousePos, int numPlayer);
//	void MoveSelectedPiece(Players& p);
//	void MoveSelectedPiece(Pieces::Color c, int pieceId, int moves);
//
//	void MovePeerPiece(std::tuple<int, int, int> movement);
//	int myPlayerID = 0;
//
//	void ThrowDice();
//
//	void UpdateTurn();
//	std::vector<Casillas*> construirRecorridoCompleto(const std::vector<Casillas*>& recorridoFinal, const int casillaSalida);
//
//	void LiberarMemoria();
//
//	void MovePieceRecieved(Pieces::Color c, int pieceId, int numMoves);
//	Players* getPlayerbyColor(Pieces::Color c);
//	void killPiece(Players* player, Pieces* p);
//	void MovePieceToHome(Players* player, int pieceID);
//
//	static std::string ColorToString(Pieces::Color c);
//	void LogCasillas();
//	//~Game();
//};
//
