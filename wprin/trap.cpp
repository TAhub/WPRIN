#include "stdafx.h"

extern database *global_data;
extern sf::Vector2f *global_camera;
extern map *global_map;

trap::trap(float x, float y, unsigned int _type) : x(x), y(y)
{
	type = _type;

	unsigned int cycleLength = global_data->getValue(DATA_TRAP, type, 6);
	timer = (rand() % cycleLength) * 0.01f;
	original = true;
	active = true;
}
trap::trap(sf::Packet *loadFrom)
{
	(*loadFrom) >> x >> y;

	float uType;
	(*loadFrom) >> uType >> timer;
	type = (unsigned int) uType;
	original = false;
	active = true;
}
void trap::save(sf::Packet *saveTo)
{
	(*saveTo) << x << y;

	(*saveTo) << (float) type << timer;
}
bool trap::detects()
{
	return global_data->getValue(DATA_TRAP, type, 3) == DATA_NONE && global_data->getValue(DATA_TRAP, type, 6) != DATA_NONE;
}
bool trap::constant()
{
	return global_data->getValue(DATA_TRAP, type, 3) == DATA_NONE && global_data->getValue(DATA_TRAP, type, 6) == DATA_NONE;
}
void trap::update(float elapsed)
{
	bool pow = false;
	float cycleLength = global_data->getValue(DATA_TRAP, type, 6) * 0.01f;

	if (constant())
	{
		//constant mode trap
		//these are also always un-animated
		if (active)
			pow = true;
	}
	else if (detects())
	{
		//detect mode trap
		//note that these are always un-animated, because of desynch issues

		if (original && active && onScreen())
		{
			unsigned int numCreatures = global_map->getNumCreatures();
			for (unsigned int i = 0; i < numCreatures; i++)
			{
				creature *cr = global_map->getCreature(i);
				if (abs(cr->getX() + cr->getMaskWidth() / 2 - x) <= DETECTW && cr->getY() <= y && cr->getType() == TYPE_PLAYER && !cr->invisible())
				{
					timer = cycleLength;
					pow = true;
					break;
				}
			}
		}
	}
	else
	{
		//cycle mode trap
		unsigned int oldFrame = frameOn();
		if (active)
			timer += elapsed;

		while (timer >= cycleLength)
			timer -= cycleLength;
		pow = cycle() != -1 && frameOn() == global_data->getValue(DATA_TRAP, type, 4) && oldFrame != frameOn();
	}

	if (pow)
	{
		//it's POW time!
		unsigned int damage = global_data->getValue(DATA_TRAP, type, 7);
		unsigned int projectile = global_data->getValue(DATA_TRAP, type, 12);
		unsigned int magic = global_data->getValue(DATA_TRAP, type, 13);
		if (projectile != DATA_NONE)
		{
			if (original && onScreen())
			{
				global_map->addProjectile(x + global_data->getValue(DATA_TRAP, type, 8), y + global_data->getValue(DATA_TRAP, type, 9),
											0, -1, projectile, damage, DATA_NONE, TYPE_ENEMY, magic, false);
				if (detects())
					active = false; //no need to notify the client, since they can't make projectiles anyway
			}
		}
		else
		{
			sf::FloatRect hitRect(x + global_data->getValue(DATA_TRAP, type, 8),
									y + global_data->getValue(DATA_TRAP, type, 9),
									(float) global_data->getValue(DATA_TRAP, type, 10),
									(float) global_data->getValue(DATA_TRAP, type, 11));
			unsigned int numCreatures = global_map->getNumCreatures();
			for (unsigned int i = 0; i < numCreatures; i++)
			{
				creature *cr = global_map->getCreature(i);
				if (cr->getType() == TYPE_PLAYER && !cr->dead() && cr->projectileInteract() && !cr->invisible())
				{
					sf::FloatRect crRect(cr->getX(), cr->getY(), (float) cr->getMaskWidth(), (float) cr->getMaskHeight());
					if (hitRect.intersects(crRect))
					{
						//they get hit
						cr->takeHit(damage, DATA_NONE, 0, cr->getX() + cr->getMaskWidth() / 2, cr->getY() + cr->getMaskHeight() / 2);
						if (magic != DATA_NONE)
							global_map->magic(cr, magic, TYPE_PLAYER);

						//deactivate yourself
						active = false;
					}
				}
			}

			if (!active)
			{
				//you were deactivated, so warn the map
				global_map->deactivateTrap(this);
			}
		}
	}
}
void trap::render(drawer *draw)
{
	if (cycle() != -1)
	{
		unsigned int sheet = global_data->getValue(DATA_TRAP, type, 0);
		if (sheet != DATA_NONE)
		{
			if (onScreen())
			{
				sf::Color color = makeColor(global_data->getValue(DATA_TRAP, type, 1));
				draw->renderSprite(sheet, frameOn(), x - global_camera->x, y - global_camera->y, false, color);
			}
		}
	}
}
bool trap::onScreen()
{
	unsigned int sheet = global_data->getValue(DATA_TRAP, type, 0);
	unsigned int width = global_data->getValue(DATA_SHEET, sheet, 1);
	unsigned int height = global_data->getValue(DATA_SHEET, sheet, 2);
	return (x > global_camera->x - width && y > global_camera->y - height &&
			x < global_camera->x + SCREENWIDTH + width && y < global_camera->y + SCREENHEIGHT + height);
}
unsigned int trap::frameOn()
{
	if (cycle() == -1 || (constant() && !active))
		return DATA_NONE;
	unsigned int startFrame = global_data->getValue(DATA_TRAP, type, 2);
	if (detects() || constant())
		return startFrame;
	unsigned int endFrame = global_data->getValue(DATA_TRAP, type, 3);
	return (unsigned int) (cycle() * (endFrame - startFrame + 1) + startFrame);
}
float trap::cycle()
{
	if (detects() || constant())
		return 0;
	float animLength = global_data->getValue(DATA_TRAP, type, 5) * 0.01f;

	if (timer > animLength)
		return -1;
	else
		return timer / animLength;
}