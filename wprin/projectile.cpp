#include "stdafx.h"

unsigned int global_id = 0;
extern map *global_map;
extern database *global_data;
extern sf::Vector2f *global_camera;

projectile::projectile(unsigned int damage, unsigned int attack, unsigned int type, unsigned int side, unsigned int magic, float x, float y, float _xDir, float _yDir, unsigned int serverID, bool venemous) :
	damage(damage), attack(attack), type(type), side(side), x(x), y(y), dead(false), magic(magic), venemous(venemous), canDrop(true)
{
	//generate an id
	id = serverID * MAXID + global_id;
	global_id = (global_id + 1) % MAXID;
	pTimer = PARTICLETIMER;

	unsigned int curlStart = global_data->getValue(DATA_PROJECTILE, type, 6);
	if (curlStart != 0)
	{
		if (rand() % 2 == 0)
			curlDir = 1;
		else
			curlDir = -1;

		float angle = atan2(_yDir, _xDir);
		angle -= (float) M_PI / 180 * curlStart * curlDir;
		while (angle < 0)
			angle += (float) M_PI * 2;
		xDir = cos(angle);
		yDir = sin(angle);
	}
	else
	{
		curlDir = 0;
		xDir = _xDir;
		yDir = _yDir;
	}
}

projectile::projectile(sf::Packet *loadFrom) : dead(false), canDrop(false)
{
	(*loadFrom) >> x >> y >> xDir >> yDir;

	float uID;
	(*loadFrom) >> uID;
	id = (unsigned int) uID;

	float uDamage, uAttack, uType, uSide, uMagic;
	(*loadFrom) >> uDamage >> uAttack >> uType >> uSide >> uMagic >> venemous;
	damage = (unsigned int) uDamage;
	attack = (unsigned int) uAttack;
	type = (unsigned int) uType;
	side = (unsigned int) uSide;
	magic = (unsigned int) uMagic;

	if (global_data->getValue(DATA_PROJECTILE, type, 6) != 0)
	{
		float uCurlDir;
		(*loadFrom) >> uCurlDir;
		curlDir = (int) uCurlDir;
	}

	pTimer = PARTICLETIMER;
}

void projectile::save(sf::Packet *saveTo)
{
	(*saveTo) << x << y << xDir << yDir;
	(*saveTo) << (float) id;
	(*saveTo) << (float) damage << (float) attack << (float) type << (float) side << (float) magic << venemous;
	if (curlDir != 0)
		(*saveTo) << (float) curlDir;
}

void projectile::drop(float atX, float atY)
{
	if (canDrop && side == TYPE_PLAYER && type == 0)
	{
		//drop an ammo item
		global_map->dropItem(new item(0, 0, 1, 0, 0, 0, global_map->getIdentifier()), atX, atY);
		canDrop = false;
	}
}

void projectile::forceDrop()
{
	drop(x - xDir * DROPBACK, y - yDir * DROPBACK);
}

void projectile::update (float elapsed)
{
	float xPD, yPD;
	xPD = x;
	yPD = y;

	pTimer -= elapsed;
	if (pTimer <= 0)
	{
		pTimer = PARTICLETIMER;
		unsigned int pType = global_data->getValue(DATA_PROJECTILE, type, 5);
		if (pType != DATA_NONE)
			global_map->addParticles(pType, 1, x, y, PARTICLERADIUS);
	}

	if (curlDir != 0)
	{
		unsigned int curlRate = global_data->getValue(DATA_PROJECTILE, type, 7);
		float angle = atan2(yDir, xDir);
		angle += (float) M_PI / 180 * curlRate * curlDir * elapsed;
		while (angle < 0)
			angle += (float) M_PI * 2;
		xDir = cos(angle);
		yDir = sin(angle);
	}

	unsigned int spd = global_data->getValue(DATA_PROJECTILE, type, 0);
	unsigned int chunks = (unsigned int) (spd * elapsed / PROJCHUNKSIZE) + 1;
	float chunkSize = spd * elapsed / chunks;
	for (unsigned int i = 0; i <= chunks; i++)
	{
		//do collision checks
		for (unsigned int j = 0; j < global_map->getNumCreatures(); j++)
		{
			creature *cr = global_map->getCreature(j);
			unsigned short hitSide;
			if (side == TYPE_PLAYER)
				hitSide = TYPE_ENEMY;
			else if (side == TYPE_ENEMY)
				hitSide = TYPE_PLAYER;
			else
				hitSide = DATA_NONE;
			if (cr->projectileInteract() && cr->collideProjectile(x, y) && !cr->dead() && !cr->isInvincible() && cr->getType() == hitSide)
			{
				if (rand() % 100 < DROPCHANCE)
					drop(xPD, yPD);

				//hit that creature
				int knockDir = 1;
				if (xDir < 0)
					knockDir = -1;
				cr->takeHit(damage, attack, knockDir, x, y);
				if (magic != DATA_NONE)
					global_map->magic(cr, magic, hitSide);
				if (venemous)
					global_map->magic(cr, global_data->getValue(DATA_EFFECT, 3, 2), hitSide);
				dead = true;
				return;
			}
		}

		if (global_map->tileSolid(x, y, 1, 0, true))
		{
			//you hit a wall
			dead = true;
			drop(xPD - xDir * DROPBACK, yPD - yDir * DROPBACK);
			return;
		}

		if (i < chunks)
		{
			x += chunkSize * xDir;
			y += chunkSize * yDir;
		}
	}

	if (x < global_camera->x || y < global_camera->y || x > global_camera->x + SCREENWIDTH || y > global_camera->y + SCREENHEIGHT)
	{
		drop(xPD, yPD);
		dead = true; //you're offscreen
	}
}

void projectile::render(drawer *draw)
{
	unsigned int sheet = global_data->getValue(DATA_PROJECTILE, type, 1);
	unsigned int frame = global_data->getValue(DATA_PROJECTILE, type, 2);
	sf::Color color = makeColor(global_data->getValue(DATA_PROJECTILE, type, 3));

	float angle = atan2(yDir, abs(xDir));
	if (xDir < 0)
		angle = -angle;
	while (angle < 0)
		angle += (float) M_PI * 2;
	draw->renderSprite(sheet, frame, x - global_camera->x, y - global_camera->y, xDir < 0, color, angle);

	unsigned int lC = global_data->getValue(DATA_PROJECTILE, type, 4);
	if (lC != DATA_NONE)
	{
		//make a projectile light too
		draw->renderSprite(global_data->getValue(DATA_LIST, 0, 2), 1, x - global_camera->x, y - global_camera->y, false, makeColor(lC));
	}
}