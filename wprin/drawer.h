#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#define COLORARRAYS 20
#define FPINAC 1000

class drawer
{
	sf::RenderTexture blackScreen;
	sf::VertexArray *toDraw;
	sf::VertexArray colorDraw;
	unsigned int *toDrawN;
	unsigned int colorDrawN;
	sf::Texture *textures;
	void resetToDraw();
public:
	bool valid;
	drawer();
	~drawer();
	void renderSprite (unsigned int sheet, unsigned int frame, float x, float y, bool flipped, sf::Color color, float angle = 0);
	void renderColor (float x, float y, float width, float height, sf::Color color);
	void render(sf::RenderWindow *window);
};