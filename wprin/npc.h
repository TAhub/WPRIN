#pragma once

#include "creature.h"

#define WANDERTMIN 0.5f
#define WANDERTMAX 3.0f

class npc : public creature
{
	bool wander;
	float startX, wanderT;
	int wanderD;

public:
	npc(float x, float y, unsigned int cType);
	void update(float elapsed);
};