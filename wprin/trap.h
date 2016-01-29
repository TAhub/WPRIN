#pragma once

#include "drawer.h"
#include <SFML/Network.hpp>

#define DETECTW 15

class trap
{
	float x, y, timer;
	unsigned int type;
	bool original;

	float cycle();
	unsigned int frameOn();
	bool detects();
	bool constant();
	bool onScreen();
public:
	trap(float x, float y, unsigned int _type);
	trap(sf::Packet *loadFrom);
	void save(sf::Packet *saveTo);
	void update(float elapsed);
	void render(drawer *draw);
	bool active;
};