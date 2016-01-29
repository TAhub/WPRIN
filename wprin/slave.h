#pragma once
#include "creature.h"
#include <SFML/Network.hpp>

class slave: public creature
{
public:
	void update(float elapsed);
	void sendPacket(sf::Packet *outPacket, bool always = false) {}
	void receivePacket(sf::Packet *inPacket);
	slave(sf::Packet *loadFrom, unsigned short type);
	bool projectileInteract() { return false; }
	bool makeDamageNum () { return false; }
	void render (drawer *draw, float xAt = -DATA_NONE, float yAt = -DATA_NONE);
};