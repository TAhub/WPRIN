#include "stdafx.h"

extern database *global_data;
extern map *global_map;

enemy::enemy(float x, float y, unsigned int cType, bool boss): creature(x, y, TYPE_ENEMY, cType, boss), firstFrame(true)
{
	unsigned int AI = global_data->getValue(DATA_CTYPE, cType, 13);
	AItype = global_data->getValue(DATA_AI, AI, 0);
	canJump = global_data->getValue(DATA_AI, AI, 1) == 1;
	//knockback immunity is 2
	//interrupt immunity is 3
	maxRange = global_data->getValue(DATA_AI, AI, 4);
	defenseHops = global_data->getValue(DATA_AI, AI, 5) == 1;
	magic = global_data->getValue(DATA_AI, AI, 6);
	retreatMagic = global_data->getValue(DATA_AI, AI, 7) == 1;
	//invincible is 8
	teleportMagic = global_data->getValue(DATA_AI, AI, 9) == 1;
	//fixed gender is 10

	//general AI behavior init
	hopDir = 0;
	retreatTimer = 0;
	retreatPhase = -1;

	switch(AItype)
	{
	case 0:
		basicAIStart();
		break;
	case 1:
		archerAIStart();
		break;
	case 2:
		manhackAIStart();
		break;
	case 3:
		flowerAIStart();
		break;
	case 4:
		animalAIStart();
		break;
	case 5:
		//chest AI
		if (rand() % 2 == 1)
			facingX = -1;
		else
			facingX = 1;
		target = this;
		break;
	case 6:
		bigWolfAIStart();
		break;
	case 7:
		wolfMakerAIStart();
		break;
	case 8:
		wolfMadeAIStart();
		break;
	case 9:
		neoRollerAIStart();
		break;
	case 10:
		copterAIStart();
		break;
	case 11:
		wprinAIStart();
		break;
	case 12:
		kingAIStart();
		break;
	}
}

void enemy::kingAIStart()
{
	bossPhase = 0;
	bossTimer = 0;
	target = this;
	bossProb = 0;
	bossResource = 0;
}

void enemy::wprinAIStart()
{
	bossPhase = 0;
	bossTimer = 0;
	target = this;
	bossProb = 0;
	bossResource = 0;
}

void enemy::bigWolfAIStart()
{
	bossPhase = 0;
	bossTimer = 0;
	target = this;
	bossProb = 100;
}

void enemy::wolfMakerAIStart()
{
	bossTimer = 0;
	target = this;
}
void enemy::wolfMadeAIStart()
{
	bossTimer = 0;
	target = this;
}

void enemy::neoRollerAIStart()
{
	bossTimer = 0;
	bossPhase = 0;
	target = this;
}

void enemy::animalAIStart()
{
	target = this;
	timer = ANIMALDIRECTIONCHANGEMAX;
	wanderDir = 0;
}

void enemy::basicAIStart()
{
	target = NULL;
}

void enemy::archerAIStart()
{
	target = NULL;
	timer = 0;
}

void enemy::manhackAIStart()
{
	target = NULL;
	angle = 0.0f;
}

void enemy::copterAIStart()
{
	target = NULL;
}

void enemy::flowerAIStart()
{
	target = NULL;
	timer = FLOWERFREQUENCY;
}

void enemy::copterAI(float elapsed)
{
	manhackTargetAcquire();
	flying = false;

	if (attacking() && bowDrawn())
		endAttack();

	if (target != NULL)
	{
		flying = true;
		float myX = getX() + getMaskWidth() / 2;
		float theirX = target->getX() + target->getMaskWidth() / 2;
		float myY = getY() + getMaskHeight() / 2;
		float theirY = target->getY() + target->getMaskHeight() / 2;

		float xDif = abs(myX - theirX);
		float yDif = abs(myY - theirY);

		if (myX > theirX)
			facingX = -1;
		else
			facingX = 1;

		float xMove = 0;
		float yMove = 0;
		if (xDif > maxRange)
		{
			if (myX > theirX)
				xMove = -1;
			else
				xMove = 1;
		}
		else if (xDif < maxRange / 2)
		{
			if (myX > theirX)
				xMove = COPTERBACKOFF;
			else
				xMove = -COPTERBACKOFF;
		}

		if (yDif > moveSpeed() * elapsed)
		{
			if (myY > theirY)
				yMove = -1;
			else
				yMove = 1;
		}
		if (yDif < CLOSERANGE)
			startAttack();

		if (xMove != 0 || yMove != 0)
		{
			float dif = sqrt(xMove * xMove + yMove * yMove);
			flyMove(xMove / dif, yMove / dif, elapsed);
		}
	}
}

void enemy::kingAI(float elapsed)
{
	unsigned int totalBossResource = (unsigned int) ((1 - healthFraction()) / (global_data->getValue(DATA_BOSSDATA, magic, 0) * 0.01f));

	switch(bossPhase)
	{
	case 0:
	case 1:
	case 2:
		if (!global_map->dialogueActive())
		{
			global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 1) + bossPhase);
			bossPhase += 1;
			if (bossPhase == 3)
			{
				switchWeapon();
				global_map->forceMusic(BOSSMUSIC);
			}
		}
		break;
	case 3:
		if (!global_map->dialogueActive())
		{
			timer += elapsed;

			if (timer > global_data->getValue(DATA_BOSSDATA, magic, 9) * 0.01f)
			{
				bossTimer = 0;
				bossProb = 0;

				//pick a phase
				if ((unsigned int) bossResource < totalBossResource)
				{
					bossResource += 1;
					bossPhase = 4;
				}
				else
				{
					if (global_map->isMultiplayer())
						bossProb = rand() % 2;
					else
						bossProb = 0;
					if (global_map->getCreature(bossProb)->dead())
						bossProb = 1 - bossProb;
					if (global_map->getCreature(bossProb)->getX() < getX())
						facingX = -1;
					else
						facingX = 1;
					if (rand() % 4 == 1)
						bossPhase = 7;
					else
						bossPhase = 5 + rand() % 2;
				}
			}
		}
		break;
	case 4:
		//summon
		{
			float bTOld = bossTimer;
			bossTimer += elapsed;
			float sPoint = global_data->getValue(DATA_BOSSDATA, magic, 2) * 0.01f;
			if (bossTimer >= sPoint && bTOld < sPoint)
			{
				//summon
				for (unsigned int i = 0; i < 2; i++)
				{
					float sX = global_data->getValue(DATA_BOSSDATA, magic, 4 + i) * 1.0f;
					float sY = global_map->getHeight() - (1 + GENERATOR_BOTTOMBORDER) * TILESIZE;
					unsigned int sTypeA = 0;
					if (bossResource > global_data->getValue(DATA_BOSSDATA, magic, 6))
						sTypeA += 1;
					global_map->summon(sX, sY, global_data->getValue(DATA_BOSSDATA, magic, 7 + sTypeA), true, true); //it's a boss so it doesn't give EXP
				}
			}
			else if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 3) * 0.01f)
				bossPhase = 3; //back to pick phase
		}
		break;
	case 5:
		{
			//swing
			if (bossTimer == 0)
			{
				startAttack();
				creature *cr = global_map->getCreature(bossProb);
				if (abs(getX() - cr->getX()) > getMaskWidth() / 2)
					bossProb = facingX;
				else
					bossProb = 0;
				if (cr->getY() < getY())
					jump();
			}
			else if (!attacking())
				bossPhase = 3;
			
			bossTimer += elapsed;

			if (bossProb != 0)
				move(bossProb, elapsed);
		}
		break;
	case 6:
		{
			if (bossTimer == 0)
				global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 10), TYPE_PLAYER);
			else if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 11) * 0.01f)
				bossPhase = 3;
			bossTimer += elapsed;
		}
		break;
	case 7:
		{
			if (bossTimer == 0)
				global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 12), TYPE_PLAYER);
			else if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 13) * 0.01f)
				bossPhase = 3;
			bossTimer += elapsed;
		}
		break;
	}
}

creature *enemy::sampleP()
{
	creature *sP = NULL;
	unsigned int numCreatures = global_map->getNumCreatures();
	for (unsigned int i = 0; i < numCreatures; i++)
	{
		creature *cr = global_map->getCreature(i);
		if (cr->getType() == TYPE_PLAYER)
		{
			sP = cr;
			if (!sP->dead())
				return sP;
		}
	}
	return sP;
}

void enemy::wprinAI(float elapsed)
{
	unsigned int totalBossResource = (unsigned int) ((1 - healthFraction()) / (global_data->getValue(DATA_BOSSDATA, magic, 8) * 0.01f));

	if (bossPhase < 10 && healthFraction() < 1)
	{
		global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 0) + 10);
		bossPhase = 10;
		global_map->forceMusic(BOSSMUSIC);
	}

	switch(bossPhase)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		if (!global_map->dialogueActive())
		{
			global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 0) + bossPhase);
			bossPhase += 1;
			if (bossPhase == 10)
				global_map->forceMusic(BOSSMUSIC);
		}
		break;
	case 10:
		//combat start
		if (!global_map->dialogueActive())
		{
			{
				//pick a position BESIDES the one you are at already
				unsigned int pPick = rand() % 2;
				switch(bossProb)
				{
				case 0:
					bossProb = 1 + pPick;
					break;
				case 1:
					if (pPick == 0)
						bossProb = 0;
					else
						bossProb = 2;
					break;
				case 2:
					bossProb = pPick;
					break;
				}
			}

			//teleport to that position
			global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 7), TYPE_PLAYER);
			x = 1.0f * global_data->getValue(DATA_BOSSDATA, magic, bossProb * 2 + 1) - getMaskWidth() / 2;
			y = global_map->getHeight() - (global_data->getValue(DATA_BOSSDATA, magic, bossProb * 2 + 2) - getMaskHeight())
											- (1 + GENERATOR_BOTTOMBORDER) * TILESIZE;

			//reset timer
			bossTimer = 0;

			//pick an attack pattern based on the position
			switch(bossProb)
			{
			case 0:
			case 1:
				flying = false;
				if (bossProb == 0)
					facingX = 1;
				else
					facingX = -1;

				if (rand() % 2 == 0)
					bossPhase = 11;
				else
					bossPhase = 14;
				break;
			case 2:
				flying = true;

				if (rand() % 2 == 0)
					bossPhase = 13;
				else
					bossPhase = 15;
				break;
			}

			if ((unsigned int) bossResource < totalBossResource)
			{
				//summon a monster instead of what you picked
				bossPhase = 12;

				bossResource += 1;
			}
		}
		break;
	case 11:
		//attack
		{
			if (bossTimer > 0 && !attacking())
				bossPhase = 10; //teleport again
			else
			{
				startAttack();
				bossTimer += elapsed;
			}
		}
		break;
	case 12:
		//summon
		{
			float bTOld = bossTimer;
			bossTimer += elapsed;
			float sPoint = global_data->getValue(DATA_BOSSDATA, magic, 9) * 0.01f;
			if (bossTimer >= sPoint && bTOld < sPoint)
			{
				//summon
				unsigned int sXA = 0;
				creature *sP = sampleP();
				if (sP->getX() + sP->getMaskWidth() / 2 < global_map->getWidth() / 2)
					sXA += 1;
				float sX = global_data->getValue(DATA_BOSSDATA, magic, 11 + sXA) * 1.0f;
				float sY = global_map->getHeight() - (1 + GENERATOR_BOTTOMBORDER) * TILESIZE;
				unsigned int sTypeA = 0;
				if (bossResource > global_data->getValue(DATA_BOSSDATA, magic, 13))
					sTypeA += 1;
				global_map->summon(sX, sY, global_data->getValue(DATA_BOSSDATA, magic, 14 + sTypeA), true, true); //it's a boss so it doesn't give EXP
			}
			else if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 10) * 0.01f)
				bossPhase = 10; //teleport
		}

		break;
	case 13:
		//poison rain
		{
			if (bossTimer == 0)
				global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 16), TYPE_PLAYER);
			bossTimer += elapsed;
			if (bossTimer >= global_data->getValue(DATA_BOSSDATA, magic, 17) * 0.01f)
				bossPhase = 10; //use teleport again
		}
		break;
	case 14:
		//moon burst
		{
			if (bossTimer == 0)
				global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 18), TYPE_PLAYER);
			bossTimer += elapsed;
			if (bossTimer >= global_data->getValue(DATA_BOSSDATA, magic, 19) * 0.01f)
				bossPhase = 10; //use teleport again
		}
		break;
	case 15:
		//moon blast
		{
			float oldBT = bossTimer;
			bossTimer += elapsed;
			float dropInterval = global_data->getValue(DATA_BOSSDATA, magic, 23) * 0.01f;
			unsigned int dropR = (unsigned int) (bossTimer / dropInterval);
			while (oldBT <= dropR * dropInterval && oldBT <= global_data->getValue(DATA_BOSSDATA, magic, 24) * 0.01f)
			{
				//use a spell
				float angle = (rand() % 100) * 0.01f * (float) M_PI;
				global_map->addProjectile(getX() + getMaskWidth() / 2, getY() + getMaskHeight() / 2, cos(angle), sin(angle),
											global_data->getValue(DATA_BOSSDATA, magic, 21), global_data->getValue(DATA_BOSSDATA, magic, 22),
											DATA_NONE, TYPE_ENEMY, DATA_NONE, false);
				global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 20), TYPE_PLAYER);
				oldBT += dropInterval;
			}
			if (bossTimer >= global_data->getValue(DATA_BOSSDATA, magic, 25) * 0.01f)
				bossPhase = 10; //use teleport again
		}
		break;

	}
}

void enemy::wolfMakerAI(float elapsed)
{
	float oldBT = bossTimer;
	bossTimer += elapsed;

	if (x > global_data->getValue(DATA_BOSSDATA, magic, 0))
		move(-1, elapsed);

	float t1t = global_data->getValue(DATA_BOSSDATA, magic, 2) * 0.01f;
	if (bossTimer > t1t && oldBT <= t1t)
	{
		//start a dialogue
		global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 3));
	}

	float t2t = global_data->getValue(DATA_BOSSDATA, magic, 4) * 0.01f;
	if (bossTimer > t2t && oldBT <= t2t)
	{
		//turn around and summon a wolf
		facingX = 1;
		global_map->summon(1.0f * global_data->getValue(DATA_BOSSDATA, magic, 6), y + getMaskHeight(), global_data->getValue(DATA_BOSSDATA, magic, 5), true, true);
	}

	float t3t = global_data->getValue(DATA_BOSSDATA, magic, 7) * 0.01f;
	if (bossTimer > t3t && oldBT <= t3t)
	{
		//start a dialogue
		facingX = -1;
		global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 8));
		global_map->forceMusic(BOSSMUSIC);
	}

	if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 9) * 0.01f)
	{
		global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 10), TYPE_PLAYER);
		silentDeath();
	}
}
void enemy::wolfMadeAI(float elapsed)
{
	bossTimer += elapsed;

	if (x > global_data->getValue(DATA_BOSSDATA, magic, 0))
		move(-1, elapsed);

	if (bossTimer > global_data->getValue(DATA_BOSSDATA, magic, 1) * 0.01f)
		silentDeath();
}

void enemy::neoRollerAI(float elapsed)
{
	switch(bossPhase)
	{
	case 0:
		//dialogue state 1
		setAttackAnim(global_data->getValue(DATA_BOSSDATA, magic, 0) * BOSSTOOLM);
		global_map->forceDialogue(global_data->getValue(DATA_BOSSDATA, magic, 1));
		bossPhase = 1;
		break;
	case 1:
		//move to center
		bossTimer += elapsed;
		if (bossTimer >= global_data->getValue(DATA_BOSSDATA, magic, 2) * 0.01f)
		{
			if (getX() + getMaskWidth() / 2 > global_map->getWidth() / 2)
				move(-1, elapsed);
			else if (!global_map->dialogueActive())
			{
				bossTimer = 0;
				bossPhase = 2;
				global_map->forceMusic(BOSSMUSIC);
			}
		}
		break;
	case 2:
		//fade in state
		{
			float end = global_data->getValue(DATA_BOSSDATA, magic, 4) * 0.01f;
			bossTimer += elapsed;
			if (bossTimer >= end)
			{
				setAttackAnim((global_data->getValue(DATA_BOSSDATA, magic, 0) + 4) * BOSSTOOLM);
				bossPhase = 3 + rand() % 3;
				bossTimer = 0;
				bossProb = 0;
				angle = (float) M_PI * 0.5f;
			}
			else
				bossToolAnimSlideScale(global_data->getValue(DATA_BOSSDATA, magic, 0) + 1, global_data->getValue(DATA_BOSSDATA, magic, 0) + 4,
										bossTimer / end);
		}
		break;
	case 3:
		//fire attack
		setAttackAnim((global_data->getValue(DATA_BOSSDATA, magic, 0) + 5) * BOSSTOOLM);

		if (bossTimer == 0)
		{
			//make the starting fireballs
			global_map->forceSound(global_data->getValue(DATA_BOSSDATA, magic, 26));
			global_map->addProjectile(projectileStartX(), projectileStartY(), 1, 0, global_data->getValue(DATA_BOSSDATA, magic, 11), 0, 0, TYPE_ALLY, DATA_NONE, false);
			global_map->addProjectile(projectileStartX(), projectileStartY(), -1, 0, global_data->getValue(DATA_BOSSDATA, magic, 11), 0, 0, TYPE_ALLY, DATA_NONE, false);
		}
		bossTimer += elapsed;

		if (bossTimer >= global_data->getValue(DATA_BOSSDATA, magic, 12) * 0.01f)
		{
			bossPhase = 6;
			bossTimer = 0;
			global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 16), TYPE_PLAYER);
			global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 17), TYPE_PLAYER);
			bossProb = global_data->getValue(DATA_BOSSDATA, magic, 14);
		}

		break;
	case 4:
		//lightning attack
		{
			setAttackAnim((global_data->getValue(DATA_BOSSDATA, magic, 0) + 6) * BOSSTOOLM);
			bossTimer += elapsed;
			float attackA = global_data->getValue(DATA_BOSSDATA, magic, 6) * 0.01f;
			if (bossTimer >= attackA)
			{
				bossTimer -= attackA;

				global_map->forceSound(global_data->getValue(DATA_BOSSDATA, magic, 25));

				//make a projectile
				unsigned int type = global_data->getValue(DATA_BOSSDATA, magic, 7);
				unsigned int damage = global_data->getValue(DATA_BOSSDATA, magic, 8);
				global_map->addProjectile(projectileStartX(), projectileStartY(), cos(angle), sin(angle), type,
											damage, DATA_NONE, TYPE_ENEMY, DATA_NONE, false);
				global_map->addProjectile(projectileStartX(), projectileStartY(), cos(angle + (float) M_PI * 2 / 3), sin(angle + (float) M_PI * 2 / 3), type,
											damage, DATA_NONE, TYPE_ENEMY, DATA_NONE, false);
				global_map->addProjectile(projectileStartX(), projectileStartY(), cos(angle + (float) M_PI * 4 / 3), sin(angle + (float) M_PI * 4 / 3), type,
											damage, DATA_NONE, TYPE_ENEMY, DATA_NONE, false);
				angle += global_data->getValue(DATA_BOSSDATA, magic, 9) * (float) M_PI / 180;

				bossProb += 1;
				if (bossProb >= global_data->getValue(DATA_BOSSDATA, magic, 10))
				{
					bossTimer = 0;
					bossPhase = 6;
					bossProb = global_data->getValue(DATA_BOSSDATA, magic, 15);
				}
			}
		}
		break;
	case 5:
		//ice attack
		{
			setAttackAnim((global_data->getValue(DATA_BOSSDATA, magic, 0) + 7) * BOSSTOOLM);
			bossTimer += elapsed;
			float attackA = global_data->getValue(DATA_BOSSDATA, magic, 19) * 0.01f;
			if (bossTimer >= attackA)
			{
				bossTimer -= attackA;

				bossProb += 1;
				if (bossProb <= global_data->getValue(DATA_BOSSDATA, magic, 20))
				{
					global_map->forceSound(global_data->getValue(DATA_BOSSDATA, magic, 27));
					float ang = (rand() % 86 + 7) * 0.01f * (float) M_PI;
					global_map->addProjectile(projectileStartX(), projectileStartY(), cos(ang), sin(ang),
								global_data->getValue(DATA_BOSSDATA, magic, 18), 0, 0, TYPE_ALLY, DATA_NONE, false);
				}
				else if (bossProb > global_data->getValue(DATA_BOSSDATA, magic, 21))
				{
					global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 23), TYPE_PLAYER);
					bossTimer = 0;
					bossPhase = 6;
					bossProb = global_data->getValue(DATA_BOSSDATA, magic, 22);
				}
			}
		}
		break;
	case 6:
		//cooldown
		{
			setAttackAnim(global_data->getValue(DATA_BOSSDATA, magic, 0) * BOSSTOOLM);
			bossTimer += elapsed;
			float interv = global_data->getValue(DATA_BOSSDATA, magic, 5) * 0.01f;
			if (bossTimer >= interv)
			{
				bossTimer -= interv;

				bossProb -= 1;

				if (bossProb >= 0)
					global_map->magic(this, global_data->getValue(DATA_BOSSDATA, magic, 13), TYPE_PLAYER);
				else if (bossProb < -global_data->getValue(DATA_BOSSDATA, magic, 24))
				{
					bossTimer = 0;
					bossPhase = 2;
				}
			}
		}
		break;
	}
}

void enemy::bigWolfAI(float elapsed)
{
	if (global_map->dialogueActive() || isInvincible())
		return;

	unsigned int switchTo = DATA_NONE;
	chargeD = 0;
	switch(bossPhase)
	{
	case 0:
		//no phase
		//switch to phase 1
		if(rand() % 100 <= bossProb)
		{
			switchTo = 1;
			bossProb = global_data->getValue(DATA_BOSSDATA, magic, 12);
		}
		else
		{
			switchTo = 3;
			bossProb = global_data->getValue(DATA_BOSSDATA, magic, 13);
		}
		break;
	case 1:
		//charge buildup phase
		{
			float bTM = global_data->getValue(DATA_BOSSDATA, magic, 1) * 0.01f;
			if (bossTimer <= bTM / 3)
				setAttackAnim(global_data->getValue(DATA_BOSSDATA, magic, 0) * BOSSTOOLM);
			else
				setAttackAnim(-1);
			if (bossTimer <= bTM * 2 / 3)
			{
				move(-facingX, elapsed * global_data->getValue(DATA_BOSSDATA, magic, 11) * 0.01f);

				if (x > global_map->getWidth() / 2)
					facingX = -1;
				else
					facingX = 1;
			}

			bossTimer -= elapsed;
			if (bossTimer <= 0)
			{
				global_map->forceSound(global_data->getValue(DATA_BOSSDATA, magic, 15));
				switchTo = 2; //start charging!
			}
		}
		break;
	case 2:
		//charge phase
		setAttackAnim(-1);
		float lowestD;
		if (facingX == 1)
			lowestD = abs(global_map->getWidth() - x - getMaskWidth() / 2);
		else
			lowestD = x + getMaskWidth() / 2;

		if (lowestD <= TILESIZE * 1.5 + getMaskWidth() / 2)
		{
			if (!walking()) //you've stopped moving
				switchTo = 0; //switch to a random phase
		}
		else
		{
			chargeD = global_data->getValue(DATA_BOSSDATA, magic, 10);
			move(facingX, elapsed);
		}
		break;
	case 3:
		//spray charge
		{
			float bTM = global_data->getValue(DATA_BOSSDATA, magic, 4) * 0.01f;

			bossTimer -= elapsed;

			if (bossTimer <= 0)
				switchTo = 4;
			else if (bossTimer > bTM / 2)
				setAttackAnim(-1);
			else
			{
				if (x > global_map->getWidth() / 2)
					facingX = -1;
				else
					facingX = 1;

				bossToolAnimSlideScale(global_data->getValue(DATA_BOSSDATA, magic, 2), global_data->getValue(DATA_BOSSDATA, magic, 3), 1 - 2 * bossTimer / bTM);
			}
		}
		break;
	case 4:
		//spray
		{
			float bTM = global_data->getValue(DATA_BOSSDATA, magic, 8) * 0.01f;
			float pInterval = bTM / global_data->getValue(DATA_BOSSDATA, magic, 6);
			float bTBefore = bossTimer;
			bossTimer -= elapsed;
			if (bossTimer < 0)
				bossTimer = 0;
			//now how many pIntervals have passed?
			while (true)
			{
				float nearestInterval = 0;
				while (nearestInterval <= bTBefore)
					nearestInterval += pInterval;
				if (nearestInterval > bTBefore)
					nearestInterval -= pInterval;

				if (nearestInterval >= bossTimer)
				{
					bTBefore -= pInterval;
					
					//make a projectile
					float angle;
					if (facingX == 1)
						angle = 0;
					else
						angle = (float) M_PI;
					float angleA = global_data->getValue(DATA_BOSSDATA, magic, 7) * (float) M_PI / 180;
					angle += (rand() % 200 - 100) * 0.01f * angleA;
					global_map->forceSound(global_data->getValue(DATA_BOSSDATA, magic, 14));
					global_map->addProjectile(projectileStartX(), projectileStartY(), cos(angle), sin(angle),
												global_data->getValue(DATA_BOSSDATA, magic, 5), global_data->getValue(DATA_BOSSDATA, magic, 9),
												DATA_NONE, TYPE_ENEMY, DATA_NONE, false);
				}
				else
					break; //there is no interval that is below btbefore and over bosstimer
			}
			if (bossTimer <= 0)
				switchTo = 0;
			else
				bossToolAnimSlideScale(global_data->getValue(DATA_BOSSDATA, magic, 2), global_data->getValue(DATA_BOSSDATA, magic, 3), bossTimer / bTM);
		}
		break;
	}

	if (switchTo != DATA_NONE)
	{
		bossPhase = switchTo;
		switch(bossPhase)
		{
		case 0:
			//no phase
			break;
		case 1:
			//charge buildup timer length
			bossTimer = global_data->getValue(DATA_BOSSDATA, magic, 1) * 0.01f;
			break;
		case 2:
			//charge
			break;
		case 3:
			//spray charge
			bossTimer = global_data->getValue(DATA_BOSSDATA, magic, 4) * 0.01f;
			break;
		case 4:
			//spray
			bossTimer = global_data->getValue(DATA_BOSSDATA, magic, 8) * 0.01f;
			break;
		}
	}
}

void enemy::bossToolAnimSlideScale(unsigned int startFrame, unsigned int endFrame, float progress)
{
	unsigned int frame = (unsigned int) std::floor(progress * (endFrame - startFrame + 1) + startFrame);
	setAttackAnim( frame * BOSSTOOLM );
}

void enemy::flowerAI(float elapsed)
{
	if (target == NULL)
		target = inRange(ACQUIRERANGE);

	if (timer > 0)
		timer -= elapsed;

	if (target != NULL)
	{
		float myX = getX() + getMaskWidth() / 2;
		float theirX = target->getX() + target->getMaskWidth() / 2;

		if (abs(myX - theirX) <= maxRange)
		{
			if (myX > theirX)
				facingX = -1;
			else
				facingX = 1;

			if (timer <= 0)
			{
				global_map->magic(this, magic, TYPE_PLAYER);
				timer = FLOWERFREQUENCY;
			}
		}
	}
}

void enemy::archerAI(float elapsed)
{
	//archer AI

	dropMode = false;

	if (target == NULL)
		target = inRange(ACQUIRERANGE);

	forceTarget();

	if (timer > 0)
		timer -= elapsed;

	if (attacking() && !weaponMelee() && bowDrawn())
		endAttack(); //properly finish your ranged attack, even if they get out of the way

	if (target != NULL)
	{
		//switch target if necessary
		creature *iR = inRange(ACQUIRERANGE);
		if (iR != NULL)
			target = iR;

		float myX = getX() + getMaskWidth() / 2;
		float theirX = target->getX() + target->getMaskWidth() / 2;

		if (abs(theirX - myX) > maxRange)
		{
			//get into range
			if (theirX > myX)
				move(1, elapsed);
			else
				move(-1, elapsed);
		}
		else if (abs(target->getY() + target->getMaskHeight() / 2 - getY() - getMaskHeight() / 2) < CLOSERANGE && !attacking())
		{
			if (theirX > myX)
				facingX = 1;
			else
				facingX = -1;

			if (abs(theirX - myX) <= weaponReach() + getMaskWidth() / 2 - 1)
			{
				//switch to a melee weapon and attack them
				if (!weaponMelee())
					switchWeapon();

				startAttack();
			}
			else if (abs(theirX - myX) < MEDIUMRANGE)
			{
				//charge them
				if (theirX > myX)
					move(1, elapsed);
				else
					move(-1, elapsed);
			}
			else
			{
				if (magic != DATA_NONE)
				{
					if (timer <= 0)
					{
						global_map->magic(this, magic, TYPE_PLAYER); //use your magic
						timer = ARCHERFREQUENCY;
					}
				}
				else
				{
					//switch to a ranged weapon, and shoot it
					if (weaponMelee())
						switchWeapon();

					startAttack();
				}
			}
		}
		else if (canJump && target->grounded())
		{
			if (target->getY() > y + CLOSERANGE)
				dropMode = true;
			else if (target->getY() < y - CLOSERANGE)
				jump(BIGJUMP);
		}
	}
}

void enemy::basicAI(float elapsed)
{
	//basic AI

	if (target == NULL)
	{
		target = inRange(ACQUIRERANGE);
		if (target != NULL && magic != DATA_NONE && !retreatMagic && !teleportMagic)
			global_map->magic(this, magic, TYPE_PLAYER); //use your self-magic
	}

	dropMode = false;

	forceTarget();

	if (target != NULL)
	{
		creature *iR = inRange(weaponReach() + CLOSERANGE);
		if (iR)
		{
			target = iR;
			startAttack();
		}

		float myX = getX() + getMaskWidth() / 2;
		float theirX = target->getX() + target->getMaskWidth() / 2;

		if (theirX > myX)
			facingX = 1;
		else
			facingX = -1;

		if (abs(theirX - myX) > weaponReach())
		{
			if (theirX > myX)
				move(1, elapsed);
			else
				move(-1, elapsed);
		}

		if (canJump && target->grounded())
		{
			if (target->getY() < getY() - CLOSERANGE)
				jump(BIGJUMP);
			else if (target->getY() > getY() + CLOSERANGE)
				dropMode = true;
		}
	}
}

void enemy::manhackAIOrbitDetails()
{
	orbitSpeed = ((rand() % 100) * 0.01f);
	orbitSpeed = ORBITSPEEDMIN * orbitSpeed + (1 - orbitSpeed) * ORBITSPEEDMAX;
	if (rand() % 2 == 1)
		orbitSpeed *= -1;
	orbitRange = (float) (rand() % (ORBITRANGEMAX - ORBITRANGEMIN) + ORBITRANGEMIN);
	timer = 0;
}

void enemy::manhackTargetAcquire()
{
	if (target == NULL && isOnscreen())
	{
		//get a target
		unsigned int numCreatures = global_map->getNumCreatures();
		std::vector<creature *> possibilities;
		for (unsigned int i = 0; i < numCreatures; i++)
		{
			creature *cr = global_map->getCreature(i);
			if (cr->getType() == TYPE_PLAYER && !cr->dead())
				possibilities.push_back(cr);
		}
		if (possibilities.size() > 0)
		{
			unsigned int pick = rand() % possibilities.size();
			target = possibilities[pick];
			manhackAIOrbitDetails();
		}
	}
}

void enemy::manhackAI(float elapsed)
{
	flying = false;

	manhackTargetAcquire();

	if (!weaponMelee() && attacking() && bowDrawn())
		endAttack();

	if (target != NULL)
	{
		startAttack();
		flying = true;

		float myX = getX() + getMaskWidth() / 2;
		float theirX = target->getX() + target->getMaskWidth() / 2;

		if (theirX > myX)
			facingX = 1;
		else
			facingX = -1;

		//find the desired position
		float desiredX = target->getX() + target->getMaskWidth() / 2;
		float desiredY = target->getY() + target->getMaskHeight() / 2;
		desiredX += orbitRange * cos(angle);
		desiredY += orbitRange * sin(angle);

		if (timer >= 0) //orbit the desired position
			angle += elapsed * orbitSpeed;

		if (weaponMelee())
		{
			//handle the timer
			float oldT = timer;
			timer += elapsed * ORBITTIMERSPEED;
			if (oldT < 0 && timer >= 0)
				manhackAIOrbitDetails(); //move back out again
			else if (timer >= 1 && getY() <= target->getY())
			{
				//move in for the strike!
				timer = orbitRange * -ORBITTIMERSPEED / moveSpeed();
				orbitRange = 0;
			}
		}


		//move in that direction
		float xDif = desiredX - x;
		float yDif = desiredY - y;
		float dis = sqrt(xDif * xDif + yDif * yDif);
		if (dis <= moveSpeed() * elapsed)
		{
			x = desiredX;
			y = desiredY;
		}
		else
			flyMove(xDif / dis, yDif / dis, elapsed);
	}
}

void enemy::animalAI(float elapsed)
{
	forceTarget();

	if (target != NULL)
	{
		//run away
		if (target->getX() + target->getMaskWidth() / 2 > getX() + getMaskWidth() / 2)
			move(-1, elapsed);
		else
			move(1, elapsed);
	}
	else
	{
		//wander
		if (wanderDir != 0)
			move(wanderDir, elapsed);

		timer -= elapsed;
		if (timer <= 0)
		{
			timer = ANIMALDIRECTIONCHANGEMIN + ((rand() % 100) * 0.01f) * (ANIMALDIRECTIONCHANGEMAX - ANIMALDIRECTIONCHANGEMIN);
			wanderDir = rand() % 3 - 1;
		}
	}
}

void enemy::forceTarget()
{
	if (injured() && target == NULL)
	{
		//get a target, even if you can't see anyone
		unsigned int numCreatures = global_map->getNumCreatures();
		std::vector<creature *> possibilities;
		for (unsigned int i = 0; i < numCreatures; i++)
		{
			creature *them = global_map->getCreature(i);
			if (!them->dead() && them->getType() == TYPE_PLAYER && !them->invisible())
				possibilities.push_back(them);
		}
		//pick a random target from the possibilities
		if (possibilities.size() > 0)
		{
			unsigned int pick = rand() % possibilities.size();
			target = possibilities[pick];
		}
	}
}

creature *enemy::inRange(unsigned int distance)
{
	unsigned int numCreatures = global_map->getNumCreatures();
	sf::Vector2f myPos(getX() + getMaskWidth() / 2, getY() + getMaskHeight());
	std::vector<creature *> possibilities;
	for (unsigned int i = 0; i < numCreatures; i++)
	{
		creature *them = global_map->getCreature(i);
		sf::Vector2f theirPos(them->getX() + them->getMaskWidth() / 2, them->getY() + them->getMaskHeight());

		if (!them->dead() && them->getType() == TYPE_PLAYER && !them->invisible() &&
			abs(myPos.x - theirPos.x) < distance && myPos.y >= theirPos.y && myPos.y - MELEEDETECTV <= theirPos.y)
			possibilities.push_back(them);
	}
	if (possibilities.size() > 0)
	{
		unsigned int pick = rand() % possibilities.size();
		return possibilities[pick];
	}
	return NULL;
}

void enemy::update (float elapsed)
{
	if (firstFrame)
	{
		//to ensure that larger AI creatures don't start in walls
		move(1, 0.01f);
		move(-1, 0.01f);
		jump(0.01f);

		firstFrame = false;
	}

	//keep track of position, for animation purposes
	if (!isParalyzed())
		lastX = x;

	if (AItype == 5 || isParalyzed() || summonEffectActive())
	{
		//just stop here; the chest AI doesn't do anything, literally
		//and paralyzed AIs dont either
		creature::update(elapsed);
		return;
	}

	//clean up targets
	if (target != NULL && target != this)
	{
		if (target->dead() || target->invisible())
			target = NULL;
		else
		{
			float xDif = target->getX() + target->getMaskWidth() / 2 - getX() - getMaskWidth() / 2;
			float yDif = target->getY() + target->getMaskHeight() / 2 - getY() - getMaskHeight() / 2;
			float dif = sqrt(xDif * xDif + yDif * yDif);
			if (dif > FORGETRANGE)
				target = NULL; //they got too far away; forget them
		}
	}

	if (!dead())
	{
		//general AI behavior
		if (teleportMagic && healthFraction() <= 0.5f && magic != DATA_NONE && target != NULL)
		{
			//use the magic
			global_map->magic(this, magic, TYPE_PLAYER);

			//halts your attack animation, if necessary
			setAttackAnim(-1);

			//teleport to behind him
			facingX = target->getFacingX();
			x = target->getX() - facingX * (weaponReach() + getMaskWidth() / 2);
			y = target->getY();

			//you can only do this once, so forget your magic
			magic = DATA_NONE;
			teleportMagic = false;
		}
		else if (defenseHops && hopDir == 0 && target != NULL && target->hopCue(this))
		{
			if (target->getX() + target->getMaskWidth() / 2 > getX() + getMaskWidth() / 2)
				hopDir = -1;
			else
				hopDir = 1;
			jump(HOP);
		}
		else if (retreatMagic && healthFraction() < RETREATPOINT && retreatPhase == -1)
		{
			retreatPhase = 0;
			retreatTimer = RETREATDUR;
		}

		if (retreatPhase == 0)
		{
			//run away
			retreatTimer -= elapsed;
			if (retreatTimer <= 0 || target == NULL)
			{
				retreatPhase = 1;
				retreatTimer = RETREATMAGICTIMER;
			}
			else
			{
				int retreatDir;
				if (target->getX() + target->getMaskWidth() / 2 > getX() + getMaskWidth() / 2)
					retreatDir = -1;
				else
					retreatDir = 1;
				move(retreatDir, elapsed * RETREATSPEEDBONUS);
				if (!isOnscreen())
				{
					//you don't want to quite get offscreen, so go back
					move(-retreatDir, elapsed * RETREATSPEEDBONUS);
					retreatTimer = 0;
				}
			}
		}
		else if (retreatPhase == 1)
		{
			if (target != NULL)
			{
				if (target->getX() + target->getMaskWidth() / 2 > getX() + getMaskWidth() / 2)
					facingX = 1;
				else
					facingX = -1;
			}

			retreatTimer -= elapsed;
			if (retreatTimer <= 0)
			{
				retreatTimer = RETREATMAGICTIMER;
				global_map->magic(this, magic, TYPE_PLAYER);
			}

			if (target != NULL && abs(target->getX() + target->getMaskWidth() / 2 - getX() - getMaskWidth() / 2) < STOPRETREATDIS)
				retreatPhase = 2; //retreating is now disabled
			if (healthFraction() == 1.0f)
				retreatPhase = -1; //stop retreating
		}
		else if (hopDir != 0)
		{
			move(hopDir, elapsed);
			if (grounded())
				hopDir = 0;
		}
		else
			switch(AItype)
			{
			case 0:
				basicAI(elapsed);
				break;
			case 1:
				archerAI(elapsed);
				break;
			case 2:
				manhackAI(elapsed);
				break;
			case 3:
				flowerAI(elapsed);
				break;
			case 4:
				animalAI(elapsed);
				break;
			case 5:
				//chest ai
				break;
			case 6:
				bigWolfAI(elapsed);
				break;
			case 7:
				wolfMakerAI(elapsed);
				break;
			case 8:
				wolfMadeAI(elapsed);
				break;
			case 9:
				neoRollerAI(elapsed);
				break;
			case 10:
				copterAI(elapsed);
				break;
			case 11:
				wprinAI(elapsed);
				break;
			case 12:
				kingAI(elapsed);
				break;
			}
	}

	creature::update(elapsed);
}

void enemy::drop()
{
	if (inventory.size() == 0)
		return;
	float off = (inventory.size() - 1) * ITEM_SIZE * 1.0f;
	for (unsigned int i = 0; i < inventory.size(); i++)
		global_map->dropItem(inventory[i], x + getMaskWidth() / 2 - off + i * ITEM_SIZE, y);
	inventory.clear();
}

void enemy::render(drawer *draw, float xAt, float yAt)
{
	if (!invisible())
		creature::render(draw, xAt, yAt);
}