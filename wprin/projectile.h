#pragma once

#include "drawer.h"
#include <SFML/Network.hpp>

#define MAXID 1000000
#define PROJCHUNKSIZE 5
#define PARTICLETIMER 0.2f
#define PARTICLERADIUS 5.0f
#define DROPCHANCE 60
#define DROPBACK 40.0f

class projectile
{
	//position code
	float x, y;
	float xDir, yDir;
	int curlDir;
	bool dead, venemous, canDrop;
	
	//identification code
	unsigned int id, type, side, magic;

	//combat code
	unsigned int damage, attack;

	//graphical code
	float pTimer;

	void drop(float atX, float atY);
public:
	projectile(unsigned int damage, unsigned int attack, unsigned int type, unsigned int side, unsigned int magic, float x, float y, float xDir, float yDir, unsigned int serverID, bool venemous);
	projectile(sf::Packet *loadFrom);
	void update (float elapsed);
	void render (drawer *draw);
	void save (sf::Packet *saveTo);
	bool tryKill (unsigned int idToKill) { return id == idToKill; }
	bool isDead() { return dead; }
	unsigned int getID() { return id; }
	void forceDrop();
};