//#include "Players.h"
//
//
//Players::Players(Pieces::Color c, sf::Vector2i initialPosition, std::string assetsPath)
//{
//	color = c;
//	int pieceSeparation = 40;
//	std::vector <sf::Vector2i> initialPositions{
//		initialPosition, 
//		sf::Vector2i(initialPosition.x + pieceSeparation,initialPosition.y), 
//		sf::Vector2i(initialPosition.x,initialPosition.y + pieceSeparation),
//		sf::Vector2i(initialPosition.x + pieceSeparation,initialPosition.y+ pieceSeparation)
//	};
//	for (int i = 0; i < initialPositions.size(); i++) 
//	{
//		Pieces *aux = new Pieces(color, initialPositions[i], assetsPath);
//		pieces.push_back(aux);
//
//	}
//}
//
//Players::Players(Pieces::Color c, std::vector<sf::Vector2i> initialPositionsVector, std::string assetsPath)
//{
//	color = c;
//
//	for (int i = 0; i < initialPositionsVector.size(); i++)
//	{
//		Pieces* aux = new Pieces(color, initialPositionsVector[i], assetsPath);
//		pieces.push_back(aux);
//
//	}
//}
//
//int Players::getSelectedNumCasilla()
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		if (pieces[i]->getSelected()) {
//			return pieces[i]->getNumCasilla();
//		}
//	}
//	return -2;
//}
//
//void Players::movePiece(sf::Vector2i v, int numCasillaAdded)
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		if (pieces[i]->getSelected()) {
//			//std::cout << "muevo ficha: " << i << "a siguiente casilla" << std::endl;
//			pieces[i]->addNumCasilla(numCasillaAdded);
//			pieces[i]->setPosition(v);
//			return;
//		}
//	}
//	std::cout << "ninguna ficha seleccionada" << std::endl;
//}
//
//void Players::movePiece(sf::Vector2i v, int numCasillaAdded, int pieceId)
//{
//	//std::cout << "muevo ficha: " << i << "a siguiente casilla" << std::endl;
//	pieces[pieceId]->addNumCasilla(numCasillaAdded);
//	pieces[pieceId]->setPosition(v);
//}
//
//void Players::movePieceHome(sf::Vector2i v, int pieceId)
//{
//	
//	pieces[pieceId]->setPosition(v);
//	pieces[pieceId]->changeTextureToNonSelected();
//	pieces[pieceId]->setFinalNumCasilla();
//}
//
//void Players::DrawPieces(sf::RenderWindow* window)
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		pieces[i]->Draw(window);
//	}
//}
//
//int Players::getNumCasillaFromPieceId(int pieceId)
//{
//	if (pieces[pieceId] != nullptr)
//	{
//		return pieces[pieceId]->getNumCasilla();
//	}
//	std::cerr << "ERROR:  intentando hacer getNumCasilla de una pieza que no existe" << std::endl;
//}
//
//Pieces* Players::getPieceById(int id)
//{
//	return pieces[id];
//}
//
//int Players::getPieceInInitPositionID()
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		if (pieces[i]->getNumCasilla() == -1)
//		{
//			return i;
//		}
//	}
//	return -1;
//}
//
//int Players::getSelectedPieceID()
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		if (pieces[i]->getSelected()) {
//			return i;
//		}
//	}
//
//	std::cerr << "ERROR: Intentando mover ficha inexistente, muevo ficha 0"<<std::endl;
//}
//
//bool Players::hasAnyPieceSelected()
//{
//	for (int i = 0; i < pieces.size(); i++)
//	{
//		if (pieces[i]->getSelected()) {
//			return true;
//		}
//	}
//	return false;
//}
