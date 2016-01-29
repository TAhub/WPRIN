#pragma once
#include "drawer.h"

class particle
{
	float x, y, xSpeed, ySpeed, lifetime, timer;
	unsigned int id;

public:
	particle(float x, float y, unsigned int type);
	void update(float elapsed);
	void render(drawer *draw);
	bool dead() { return timer >= lifetime; }
};