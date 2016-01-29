#pragma once
#include "drawer.h"
#include "database.h"
#include <SFML/Network.hpp>

#define ITEM_GRAVITY 700
#define ITEM_UPFORCE -300
#define ITEM_SIZE 15
#define ITEM_MAXSTACK 30

class item
{
	float x, y, ySpeed;
	unsigned short type;
	unsigned int id, number, material, identifier, cost;
	bool collision();
	void makeIdentifier();
	unsigned int getNameLine();
public:
	bool removed, merged;

	item (unsigned short type, unsigned int id, unsigned int number, unsigned int material, float x, float y, unsigned int serverID = DATA_NONE);
	item (sf::Packet *loadFrom);
	void save (sf::Packet *saveTo);
	void update (float elapsed);
	void render (drawer *draw);
	void drop (float _x, float _y) {x = _x; y = _y; ySpeed = ITEM_UPFORCE; removed = false; }
	bool merge (item *mergeWith);
	bool canMerge (item *mergeWith);
	void useSome (unsigned int amount);
	unsigned int heals();
	unsigned int ammo();
	unsigned int feeds();
	unsigned int effect();

	//accessors
	unsigned int getID() { return id; }
	unsigned int getIdentifier() { return identifier; }
	unsigned int getNumber() { return number; }
	unsigned int getMaterial() { return material; }
	unsigned int getCost() { return cost; }
	void payFor() { cost = 0; }
	void calculateCost();
	unsigned int returnCost();
	unsigned int returnSaleValue();
	unsigned short getType() { return type; }
	float getX() { return x; }
	float getY() { return y; }
	std::string getName();
	std::string getDescription();
};