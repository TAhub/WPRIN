#include "stdafx.h"

extern sf::Vector2f *global_camera;

void damageNumber::update (float elapsed)
{
	life -= elapsed;
	y -= FLOATSPEED * elapsed;
}

void damageNumber::render (drawer *draw)
{
	renderNumber(draw, amount, color, x - global_camera->x, y - global_camera->y);
}