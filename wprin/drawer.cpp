#include "stdafx.h"

extern database *global_data;
extern sf::Vector2f *global_camera;
extern map *global_map;

drawer::drawer()
{
	blackScreen.create(SCREENWIDTH, SCREENHEIGHT);

	toDraw = new sf::VertexArray[global_data->getCategorySize(DATA_SHEET)];
	toDrawN = new unsigned int[global_data->getCategorySize(DATA_SHEET)];
	textures = new sf::Texture[global_data->getCategorySize(DATA_SHEET)];

	//load textures
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
	{
		std::string texName = global_data->getLine(global_data->getValue(DATA_SHEET, i, 0));
		if (!textures[i].loadFromFile("sprites/" + texName))
		{
			std::cout << "Unable to load texture " << texName << "." << std::endl;
			valid = false;
			return;
		}
	}

	resetToDraw();
	valid = true;
}

drawer::~drawer()
{
	delete toDrawN;
	delete[] toDraw;
	delete[] textures;
}

void drawer::resetToDraw()
{
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
	{
		toDrawN[i] = 0;
		toDraw[i] = sf::VertexArray(sf::Quads, 4 * global_data->getValue(DATA_SHEET, i, 5));
	}
	colorDraw = sf::VertexArray(sf::Quads, 4 * COLORARRAYS);
	colorDrawN = 0;
}

void drawer::renderColor (float x, float y, float width, float height, sf::Color color)
{
	if (colorDrawN >= COLORARRAYS)
	{
		std::cout << "Attempting to draw too many solid colors!" << std::endl;
		return;
	}

	colorDraw[colorDrawN * 4].position = sf::Vector2f(x, y);
	colorDraw[colorDrawN * 4 + 1].position = sf::Vector2f(x + width, y);
	colorDraw[colorDrawN * 4 + 2].position = sf::Vector2f(x + width, y + height);
	colorDraw[colorDrawN * 4 + 3].position = sf::Vector2f(x, y + height);
	colorDraw[colorDrawN * 4].color = color;
	colorDraw[colorDrawN * 4 + 1].color = color;
	colorDraw[colorDrawN * 4 + 2].color = color;
	colorDraw[colorDrawN * 4 + 3].color = color;

	colorDrawN += 1;
}

void drawer::renderSprite (unsigned int sheet, unsigned int frame, float x, float y, bool flipped, sf::Color color, float angle)
{
	if (toDrawN[sheet] >= (unsigned int) global_data->getValue(DATA_SHEET, sheet, 5))
	{
		std::cout << "Attempting to draw too many sprites from sheet " << sheet << "!" << std::endl;
		return; //you are trying to draw too many of that type of sprite; stop now
	}

	unsigned int width = global_data->getValue(DATA_SHEET, sheet, 1);
	unsigned int height = global_data->getValue(DATA_SHEET, sheet, 2);
	unsigned int originX = global_data->getValue(DATA_SHEET, sheet, 3);
	unsigned int originY = global_data->getValue(DATA_SHEET, sheet, 4);

	//modify the position by the origin
	float cX;
	if (flipped)
		cX = x - width + originX;
	else
		cX = x - originX;
	float cY = y - originY;

	//round it to hopefully avoid floating point fairies
	int iX = (int) (x * FPINAC);
	int iY = (int) (y * FPINAC);
	x = (float) (iX / FPINAC);
	y = (float) (iY / FPINAC);

	//set position
	toDraw[sheet][toDrawN[sheet] * 4].position = sf::Vector2f(floor(cX), floor(cY));
	toDraw[sheet][toDrawN[sheet] * 4 + 1].position = sf::Vector2f(floor(cX + width), floor(cY));
	toDraw[sheet][toDrawN[sheet] * 4 + 2].position = sf::Vector2f(floor(cX + width), floor(cY + height));
	toDraw[sheet][toDrawN[sheet] * 4 + 3].position = sf::Vector2f(floor(cX), floor(cY + height));

	if (angle > 0)
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			//point rotation
			sf::Vector2f *p = &(toDraw[sheet][toDrawN[sheet] * 4 + i].position);
			p->x -= x;
			p->y -= y;
			float oX = p->x;
			float oY = p->y;
			p->x = oX * cos(angle) - oY * sin(angle);
			p->y = oY * cos(angle) + oX * sin(angle);
			p->x += x;
			p->y += y;
		}
	}

	//set color
	for (unsigned int i = 0; i < 4; i++)
		toDraw[sheet][toDrawN[sheet] * 4 + i].color = color;

	//set texture coordinates
	if (flipped)
	{
		toDraw[sheet][toDrawN[sheet] * 4].texCoords = sf::Vector2f((frame + 1) * width * 1.0f, 0);
		toDraw[sheet][toDrawN[sheet] * 4 + 1].texCoords = sf::Vector2f(frame * width * 1.0f, 0);
		toDraw[sheet][toDrawN[sheet] * 4 + 2].texCoords = sf::Vector2f(frame * width * 1.0f, height * 1.0f);
		toDraw[sheet][toDrawN[sheet] * 4 + 3].texCoords = sf::Vector2f((frame + 1) * width * 1.0f, height * 1.0f);
	}
	else
	{
		toDraw[sheet][toDrawN[sheet] * 4].texCoords = sf::Vector2f(frame * width * 1.0f, 0);
		toDraw[sheet][toDrawN[sheet] * 4 + 1].texCoords = sf::Vector2f((frame + 1) * width * 1.0f, 0);
		toDraw[sheet][toDrawN[sheet] * 4 + 2].texCoords = sf::Vector2f((frame + 1) * width * 1.0f, height * 1.0f);
		toDraw[sheet][toDrawN[sheet] * 4 + 3].texCoords = sf::Vector2f(frame * width * 1.0f, height * 1.0f);
	}

	toDrawN[sheet] += 1;
}

void drawer::render(sf::RenderWindow *window)
{
	window->clear();

	//draw backgrounds
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
		if (global_data->getValue(DATA_SHEET, i, 6) == 3)
			window->draw(toDraw[i], &textures[i]);

	//draw pre-light sprites
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
		if (global_data->getValue(DATA_SHEET, i, 6) == 0)
			window->draw(toDraw[i], &textures[i]);

	//draw light sprites
	blackScreen.clear();
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
		if (global_data->getValue(DATA_SHEET, i, 6) == 1)
			blackScreen.draw(toDraw[i], &textures[i]);

	/**/
	//draw the blackscreen to the main screen
	sf::VertexArray darkDraw = sf::VertexArray(sf::Quads, 4);
	darkDraw[0].position = sf::Vector2f(0, 0);
	darkDraw[1].position = sf::Vector2f(SCREENWIDTH, 0);
	darkDraw[2].position = sf::Vector2f(SCREENWIDTH, SCREENHEIGHT);
	darkDraw[3].position = sf::Vector2f(0, SCREENHEIGHT);
	darkDraw[3].texCoords = sf::Vector2f(0, 0);
	darkDraw[2].texCoords = sf::Vector2f(SCREENWIDTH, 0);
	darkDraw[1].texCoords = sf::Vector2f(SCREENWIDTH, SCREENHEIGHT);
	darkDraw[0].texCoords = sf::Vector2f(0, SCREENHEIGHT);
	sf::RenderStates states;
	states.blendMode = sf::BlendMultiply;
	states.texture = &(blackScreen.getTexture());

	window->draw(darkDraw, states);
	/**/

	//draw post-light sprites
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SHEET); i++)
		if (global_data->getValue(DATA_SHEET, i, 6) == 2)
			window->draw(toDraw[i], &textures[i]);

	//draw colors
	window->draw(colorDraw);

	window->display();

	resetToDraw();
}