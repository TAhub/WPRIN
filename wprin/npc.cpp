#include "stdafx.h"

extern database *global_data;

npc::npc(float x, float y, unsigned int cType) : creature(x, y, TYPE_ALLY, cType, false)
{
	unsigned int AI = global_data->getValue(DATA_CTYPE, cType, 13);
	wander = global_data->getValue(DATA_AI, AI, 4) == 1;
	startX = x;

	if (rand() % 2 == 1)
		facingX = 1;
	else
		facingX = -1;

	wanderD = rand() % 3 - 1;
	wanderT = WANDERTMIN;
}

void npc::update(float elapsed)
{
	lastX = x;

	if (wander)
	{
		if (wanderD != 0)
		{
			move(wanderD, elapsed);
			if (x < startX - TILESIZE / 2)
			{
				//you went too far
				move(1, elapsed * 2);
				wanderD = rand() % 2;
			}
			else if (x > startX + TILESIZE / 2)
			{
				//you went too far
				move(-1, elapsed * 2);
				wanderD = rand() % 2 - 1;
			}
		}

		wanderT -= elapsed;
		if (wanderT <= 0)
		{
			wanderD = rand() % 3 - 1;
			wanderT = ((rand() % 100) * 0.01f * (WANDERTMAX - WANDERTMIN)) + WANDERTMIN;
		}
	}

	creature::update(elapsed);
}