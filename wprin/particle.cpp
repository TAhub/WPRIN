#include "stdafx.h"

extern database *global_data;
extern sf::Vector2f *global_camera;

particle::particle(float x, float y, unsigned int type) : x(x), y(y), timer(0)
{
	id = type;
	xSpeed = (rand() % 200 * 0.01f - 1) * global_data->getValue(DATA_PARTICLE, id, 3);
	ySpeed = (rand() % 100 * 0.01f) * global_data->getValue(DATA_PARTICLE, id, 4);
	lifetime = global_data->getValue(DATA_PARTICLE, id, 6) * 0.01f;
}

void particle::update(float elapsed)
{
	x += xSpeed * elapsed;
	y += ySpeed * elapsed;
	ySpeed += global_data->getValue(DATA_PARTICLE, id, 5) * elapsed;
	timer += elapsed;
}

void particle::render(drawer *draw)
{
	sf::Color c = makeColor(global_data->getValue(DATA_PARTICLE, id, 2));
	c.a = (sf::Uint8) (255 * (1 - timer / lifetime));
	draw->renderSprite(global_data->getValue(DATA_PARTICLE, id, 0),
						global_data->getValue(DATA_PARTICLE, id, 1),
						x - global_camera->x, y - global_camera->y, false, c);
}