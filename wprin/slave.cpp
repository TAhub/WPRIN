#include "stdafx.h"

slave::slave(sf::Packet *loadFrom, unsigned short type): creature(loadFrom, type)
{
}

void slave::update(float elapsed)
{
	if (isWalking())
	{
		//move according to prediction
		move(movingX, elapsed);
	}

	creature::update(elapsed);
}

void slave::render(drawer *draw, float xAt, float yAt)
{
	if (!invisible())
		creature::render(draw, xAt, yAt);
}

void slave::receivePacket(sf::Packet *inPacket)
{
	//keep track of position, for predictive purposes
	if (!isParalyzed())
		lastX = x;

	receivePacketInner(inPacket);
}