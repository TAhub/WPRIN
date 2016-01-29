#pragma once

#include "drawer.h"
#include <SFML/Graphics.hpp>

#define FLOATSPEED 300.0f
#define LIFETIME 1.0f

class damageNumber
{
	float x, y, life;
	sf::Color color;
	unsigned int amount;

public:
	damageNumber(float x, float y, sf::Color color, unsigned int amount) : x(x), y(y), color(color), amount(amount), life(LIFETIME) {}
	void update (float elapsed);
	void render (drawer *draw);
	bool dead() { return life <= 0; }
};