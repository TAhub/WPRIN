#include "stdafx.h"

extern sf::Vector2f *global_camera;
extern database *global_data;
extern map *global_map;
extern bool *global_hasFocus;
extern soundPlayer *global_sound;

//a mutex for the networking
sf::Mutex mutex;

//a table of pre-computed random numbers for the greenery drawing
#define GREENERYARRAYLENGTH 50
unsigned int gArray[GREENERYARRAYLENGTH] = {4,4,5,5,5,5,5,5,5,5,5,4,4,5,4,5,5,4,5,5,4,4,5,4,5,4,5,4,4,5,4,4,4,4,5,4,4,4,5,4,5,5,4,5,5,5,5,4,4,5};

//a table of pre-computed random numbers for the background
#define BACKGROUNDARRAYLENGTH 50
unsigned int bArray[BACKGROUNDARRAYLENGTH] = {36,44,61,29,72,91,4,72,3,36,59,93,62,72,76,81,75,92,46,59,33,51,96,5,62,95,64,70,86,92,7,55,16,2,54,59,94,52,1,91,46,66,51,99,10,86,53,2,30,52};

map::map(sf::TcpSocket *_socket, bool _master)
{
	disconnected = false;
	global_map = this;
	usedDoor = NULL;
	killedWprin = false;

	socket = _socket;
	single = _socket == NULL;
	master = _master;

	global_camera->x = -1;
	global_camera->y = -1;
	width = 0;
	height = 0;
	cD = new characterDesigner();

	if (single)
	{
		identifier = 0;
		creatures.push_back(NULL);
	}
	else if (master)
	{
		identifier = 0;
		/*
		//trade identifiers
		identifier = 0;
		sf::Packet idPack;
		idPack << ((float) identifier + 1);
		socket.send(idPack);
		*/

		//prepare for characters
		creatures.push_back(NULL);
		creatures.push_back(NULL);
	}
	else
	{
		/*
		//get your identifier
		sf::Packet idPack;
		socket.receive(idPack);
		float fID;
		idPack >> fID;
		identifier = (unsigned int) fID;
		*/
		identifier = 1;
		
		//prepare for character
		creatures.push_back(NULL);
	}
}

map::~map()
{
	if (cD != NULL)
		delete cD;
	else
		mapClear(false);
}

void map::teleportToSafety(creature *cr)
{
	unsigned int crX = (unsigned int) (cr->getX() / TILESIZE);
	unsigned int crY = height - 1;

	//keep going up until you find a place to be dropped off
	for (; crY > 1; crY -= 1)
	{
		std::vector<unsigned int> spots;
		int crXStart = crX - SAFETYTELEPORTWIDTH / 2;
		if (crXStart < 0)
			crXStart = 0;
		for (unsigned int crXP = (unsigned int) crXStart; crXP < (unsigned int) crXStart + SAFETYTELEPORTWIDTH && crXP < width; crXP++)
			if (!tileSolid(crXP * TILESIZE * 1.0f, crY * TILESIZE * 1.0f) && tileSolid(crXP * TILESIZE * 1.0f, (crY + 1) * 1.0f * TILESIZE) &&
				crXP * TILESIZE >= global_camera->x && crY * TILESIZE >= global_camera->y &&
				crXP * TILESIZE + cr->getMaskWidth() < global_camera->x + SCREENWIDTH && (crY + 1) * TILESIZE + cr->getMaskHeight() < global_camera->y + SCREENHEIGHT)
				spots.push_back(crXP);
		if (spots.size() > 0)
		{
			//pick a random spot
			unsigned int randPick = rand() % spots.size();
			cr->teleport(spots[randPick] * 1.0f * TILESIZE, crY * 1.0f * TILESIZE);
			return;
		}
	}
}

bool map::tileSolid(float x, float y, int yDir, float refY, bool dropMode)
{
	if (height == 0)
		return true; //for character creator
	if (y >= height * TILESIZE || x >= width * TILESIZE || x < 0 || y < 0)
		return false; //it's off the map
	unsigned int i = toI((unsigned int) (x / TILESIZE), (unsigned int) (y / TILESIZE));
	if (tiles[i] == DATA_NONE)
		return false;
	else if (refY != -1 && global_data->getValue(DATA_TILE, tiles[i], 3) == 1 &&
		(dropMode || refY > toY(i) * TILESIZE || yDir != 1))
		return false; //it's a one-way tile, so you can pass through
	else
		return global_data->getValue(DATA_TILE, tiles[i], 2) == 1;
}

void map::addCreature(creature *cr)
{
	creatures.push_back(cr);
}

void map::update(float elapsed)
{
	if (cD != NULL)
	{
		if (!cD->done)
		{
			mutex.lock();
			cD->update(elapsed);
			mutex.unlock();
		}
		else
		{
			tiles = new unsigned int[1];
			if (single || master)
			{
				creatures[0] = cD->getPlayer();

				if (!single)
				{
					//wait until the player packet comes
					sf::Packet receiveP;
					socket->receive(receiveP);
					slave *cr = new slave(&receiveP, TYPE_PLAYER);
					creatures[1] = cr;

					//make a map
					mutex.lock();
					mapGenerate();
					mutex.unlock();

					//send the map to the clients
					mapSend();

					makeEventLog();
				}
				else
				{
					//just make a map
					mutex.lock();
					mapGenerate();
					mutex.unlock();
				}
			}
			else
			{
				//send a player packet
				sf::Packet sendP;
				cD->getPlayer()->save(&sendP);
				socket->send(sendP);

				//wait for a map
				sf::Packet mapP;
				socket->receive(mapP);
				mutex.lock();
				mapRetrieve(&mapP);
				mutex.unlock();
				
				makeEventLog();
			}

			//this is mutexed so that you don't delete the character designer in the middle of using it
			mutex.lock();
			delete cD;
			cD = NULL;
			mutex.unlock();
		}

		return;
	}

	if (single && !(*global_hasFocus))
		return; //pause the game

	//this is mutex-locked
	mutex.lock();

	if (single && mapChange.size() > 0)
	{
		//make a new map
		checkWprinKill();
		mapGenerate();
		mapChange.clear();
		return;
	}

	pollCamera(elapsed);

	for (unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->update(elapsed);

	std::vector<projectile *> newProjectiles;
	for (unsigned int i = 0; i < projectiles.size(); i++)
	{
		projectiles[i]->update(elapsed);

		if (projectiles[i]->isDead())
		{
			if (!single)
			{
				//log the projectile's destruction
				eventLog << EVENT_LOG_PROJECTILEDESTROY;
				eventLog << (float) projectiles[i]->getID();
			}

			//and delete it
			delete projectiles[i];
		}
		else
			newProjectiles.push_back(projectiles[i]);
	}
	projectiles = newProjectiles;

	std::vector<damageNumber *> newDN;
	for (unsigned int i = 0; i < dNums.size(); i++)
	{
		dNums[i]->update(elapsed);
		if (dNums[i]->dead())
			delete dNums[i];
		else
			newDN.push_back(dNums[i]);
	}
	dNums = newDN;

	std::vector<particle *> newPL;
	for (unsigned int i = 0; i < particleList.size(); i++)
	{
		particleList[i]->update(elapsed);
		if (particleList[i]->dead())
			delete particleList[i];
		else
			newPL.push_back(particleList[i]);
	}
	particleList = newPL;

	std::vector<item *> newIT;
	for (unsigned int i = 0; i < items.size(); i++)
	{
		if (!items[i]->removed)
		{
			items[i]->update(elapsed);
			newIT.push_back(items[i]);
		}
		else if (items[i]->merged)
			delete items[i];
	}
	items = newIT;

	for (unsigned int i = 0; i < traps.size(); i++)
		traps[i]->update(elapsed);

	std::vector<pulse *> newPulses;
	for (unsigned int i = 0; i < pulses.size(); i++)
	{
		pulses[i]->time -= elapsed;
		if (pulses[i]->center->dead())
			pulses[i]->pulses = 0;
		else if (pulses[i]->time <= 0)
		{
			pulses[i]->time = pulses[i]->maxTime;
			pulses[i]->pulses -= 1;
			magic(pulses[i]->center, pulses[i]->type, pulses[i]->targets, false, true);
		}
		if (pulses[i]->pulses == 0)
			delete pulses[i];
		else
			newPulses.push_back(pulses[i]);
	}
	pulses = newPulses;

	for (unsigned int i = 0; i < decorations.size(); i++)
		decorations[i]->update(elapsed);

	mutex.unlock();
}

unsigned int map::endState()
{
	bool doneCheck = false;
	if (lastMap())
	{
		doneCheck = true;
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i]->getType() == TYPE_ENEMY && !creatures[i]->dead())
			{
				doneCheck = false;
				break;
			}
	}
	if (doneCheck && killedWprin)
		return 6;
	else if (doneCheck)
		return 5;

	if (disconnected)
	{
		if (!master)
			return tileset; //this is totally wrong, but whatever, disconnecting is kind of like dying
							//if it wasn't this way this message would never come up for the client though
		else
			return 4;
	}
	if (cD != NULL)
		return DATA_NONE; //it's okay
	for (unsigned int i = 0; i < creatures.size(); i++)
	{
		creature *cr = creatures[i];
		if (cr->getType() != TYPE_PLAYER)
			break;
		else if (!cr->dead())
			return DATA_NONE; //it's okay
	}

	//everyone's dead dave
	return tileset;
}

void map::checkWprinKill()
{
	if (type == 1 && (mapNum + 1) == GENERATOR_TILESETINTERVAL * 3)
	{
		killedWprin = true;
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i]->getType() == TYPE_ENEMY && !creatures[i]->dead())
			{
				killedWprin = false;
				return;
			}
	}
}

void map::makeEventLog()
{
	eventLog = sf::Packet();
}

bool map::forgeAt(float x, float y)
{
	unsigned int iX = (unsigned int) (x / TILESIZE);
	unsigned int iY = (unsigned int) (y / TILESIZE);
	for (unsigned int i = 0; i < decorations.size(); i++)
		if (decorations[i]->isForge() && decorations[i]->y == iY &&
			(decorations[i]->x == iX || decorations[i]->x + 1 == iX))
			return true;
	return false;
}

bool map::mapEnd(float x, float y)
{
	if (!single && master)
		return false; //the server can't use doors because

	if (type == 1 && tileset != 2)
	{
		//check to see if there is a single living non-player
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i]->getType() != TYPE_PLAYER && !creatures[i]->dead())
				return false;
	}

	unsigned int xU = (unsigned int) x / TILESIZE;
	unsigned int yU = (unsigned int) y / TILESIZE;
	unsigned int onDoor = DATA_NONE;
	for (unsigned int i = 0; i < doors.size(); i++)
	{
		if (xU == doors[i]->x && yU == doors[i]->y)
		{
			onDoor = i;
			if (master || single)
				usedDoor = doors[i];
		}
	}
	if (onDoor == DATA_NONE)
		return false;

	mapChange.push_back(identifier);
	if (!master && !single)
	{
		//do a player update, to make 100% sure that the server knows what you are like
		playerUpdate();

		//inform the master
		eventLog << EVENT_LOG_MAPCHANGE;
		eventLog << (float) identifier;
		eventLog << (float) onDoor;
	}

	return true;
}

void map::readEventLog(sf::Packet *log)
{
	while (true)
	{
		float eventT;
		(*log) >> eventT;

		if (eventT == EVENT_LOG_END)
			return; //done

		bool discard = (eventT != EVENT_LOG_MAPDATA && eventT != EVENT_LOG_MAPCHANGE && mapChange.size() > 0); //discard the results if you are waiting for a map packet

		if (eventT == EVENT_LOG_PROJECTILEMAKE)
		{
			//make a projectile!
			projectile *proj = new projectile(log);
			if (!discard)
				projectiles.push_back(proj);
		}
		else if (eventT == EVENT_LOG_PROJECTILEDESTROY)
		{
			//destroy a projectile!
			float idToKill;
			(*log) >> idToKill;
			if (!discard)
				killProjectile((unsigned int) idToKill);
		}
		else if (eventT == EVENT_LOG_DAMAGE)
		{
			float uC, uA, uX, uY, uS;
			(*log) >> uC >> uA >> uX >> uY >> uS;
			if (!discard)
				addDamageNumber(creatures[(unsigned int) uC], (int) uA, uX, uY, (unsigned int) uS);
		}
		else if (eventT == EVENT_LOG_MAPCHANGE)
		{
			float uC, uD;
			(*log) >> uC >> uD;
			if (!discard)
			{
				mapChange.push_back((unsigned int) uC);
				usedDoor = doors[(unsigned int) uD];
			}
		}
		else if (eventT == EVENT_LOG_MAPDATA)
		{
			//this is a map data packet!
			//this cannot be discarded

			//save the player's inventory
			std::vector<item *> savedInv;
			for (unsigned int i = 0; i < INVENTORYSIZE; i++)
			{
				item *cri = creatures[identifier]->inventory[i];
				if (cri == NULL)
					savedInv.push_back(NULL);
				else
					savedInv.push_back(new item(cri->getType(), cri->getID(), cri->getNumber(), cri->getMaterial(), cri->getX(), cri->getY(), identifier));
			}

			//log if the wild princess died
			checkWprinKill();

			mapRetrieve(log);

			//copy the inventory over
			for (unsigned int i = 0; i < INVENTORYSIZE; i++)
				creatures[identifier]->inventory[i] = savedInv[i];

			mapChange.clear();
		}
		else if (eventT == EVENT_LOG_ITEMDROP)
		{
			float x, y;
			(*log) >> x >> y;
			item *it = new item(log);
			it->drop(x, y);
			if (discard)
				delete it;
			else
				items.push_back(it);
		}
		else if (eventT == EVENT_LOG_PICKUPREQUEST)
		{
			//get the item that you want picked up
			float pickerID, itemId;
			(*log) >> pickerID >> itemId;

			if (!discard)
				for (unsigned int i = 0; i < items.size(); i++)
					if (items[i]->getIdentifier() == (unsigned int) itemId &&
						!items[i]->merged && !items[i]->removed)
					{
						//pick it up
						creatures[(unsigned int) pickerID]->getItem(items[i]);

						//send confirmation for their request
						eventLog << EVENT_LOG_GETITEM;
						eventLog << itemId;

						break;
					}
		}
		else if (eventT == EVENT_LOG_GETITEM)
		{
			//get the item that you want to get
			float itemId;
			(*log) >> itemId;

			if (!discard)
				for (unsigned int i = 0; i < items.size(); i++)
					if (items[i]->getIdentifier() == (unsigned int) itemId &&
						!items[i]->merged && !items[i]->removed)
					{
						//get the item
						creatures[identifier]->getItem(items[i]);

						break;
					}
		}
		else if (eventT == EVENT_LOG_REMOVEITEM)
		{
			//this can't be discarded

			float itemId;
			(*log) >> itemId;

			for (unsigned int i = 0; i < items.size(); i++)
				if (items[i]->getIdentifier() == (unsigned int) itemId &&
					!items[i]->merged && !items[i]->removed)
				{
					//destroy the item, it's p much gone now
					items[i]->merged = true;
					items[i]->removed = true;

					break;
				}
		}
		else if (eventT == EVENT_LOG_PLAYERUPDATE)
		{
			//read the player update
			float x, y, uID;
			(*log) >> uID;
			(*log) >> x >> y;
			slave *reloaded = new slave(log, TYPE_PLAYER);
			delete creatures[(unsigned int) uID];
			creatures[(unsigned int) uID] = reloaded;
		}
		else if (eventT == EVENT_LOG_MAGIC)
		{
			float uC, uType, uTargets;
			(*log) >> uC >> uType >> uTargets;
			if (!discard)
				magic(creatures[(unsigned int) uC], (unsigned int) uType, (unsigned short) uTargets, false);
		}
		else if (eventT == EVENT_LOG_DEACTIVATETRAP)
		{
			float trapNum;
			(*log) >> trapNum;
			if (!discard)
				traps[(unsigned int) trapNum]->active = false;
		}
		else if (eventT == EVENT_LOG_SUMMON)
		{
			float slot;
			bool loud;
			(*log) >> slot >> loud;
			creature *cr = new slave(log, TYPE_ENEMY);
			if (loud)
			{
				cr->summonEffect();
				global_sound->play(2);
			}
			if ((unsigned int) slot == DATA_NONE)
				creatures.push_back(cr);
			else
			{
				//put it in an existing slot
				delete creatures[(unsigned int) slot];
				creatures[(unsigned int) slot] = cr;
			}
		}
		else if (eventT == EVENT_LOG_DIALOGUE)
		{
			float id;
			(*log) >> id;
			((player *) creatures[identifier])->forceDialogue((unsigned int) id);
		}
		else if (eventT == EVENT_LOG_MUSIC)
		{
			float id;
			(*log) >> id;
			playSong((unsigned int) id);
		}
		else if (eventT == EVENT_LOG_SOUND)
		{
			float id;
			(*log) >> id;
			global_sound->play((unsigned int) id);
		}
	}
}

void map::forceSound(unsigned int id)
{
	global_sound->play(id);

	if (!single)
	{
		eventLog << EVENT_LOG_SOUND;
		eventLog << (float) id;
	}
}

void map::forceMusic(unsigned int id)
{
	playSong(id);

	if (!single)
	{
		eventLog << EVENT_LOG_MUSIC;
		eventLog << (float) id;
	}
}

void map::forceDialogue(unsigned int id)
{
	((player *) creatures[identifier])->forceDialogue(id);

	if (!single)
	{
		eventLog << EVENT_LOG_DIALOGUE;
		eventLog << (float) id;
	}
}

void map::summon(float x, float y, unsigned int cType, bool boss, bool loud)
{
	if (!master && !single)
		return; //clients can't summon

	//find a slot
	unsigned int slot = DATA_NONE;
	creature *cr = new enemy(x, y, cType, boss);
	for (unsigned int i = 0; i < creatures.size(); i++)
		if (creatures[i]->dead() && creatures[i]->getType() == TYPE_ENEMY)
		{
			delete creatures[i]; //free this slot
			creatures[i] = cr;
			slot = i;
			break;
		}

	if (slot == DATA_NONE)
		creatures.push_back(cr);

	if (loud)
	{
		cr->summonEffect();
		global_sound->play(2);
	}

	if (!single)
	{
		eventLog << EVENT_LOG_SUMMON;
		eventLog << (float) slot << loud;
		cr->save(&eventLog);
	}
}

void map::deathReward(unsigned int amount)
{
	//this doesn't have to be online synchronized, because it's very easy to do clientside

	//divide the EXP if it's multiplayer
	if (!single)
		amount = (unsigned int) (amount * 0.75f);

	for (unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->getEXP(amount);
}

void map::magic(creature *center, unsigned int type, unsigned short targets, bool original, bool pulsed)
{
	//do the magic
	unsigned int form = global_data->getValue(DATA_MAGIC, type, 0);
	unsigned int damage = global_data->getValue(DATA_MAGIC, type, 1);
	unsigned int effect = global_data->getValue(DATA_MAGIC, type, 2);
	unsigned int particle = global_data->getValue(DATA_MAGIC, type, 3);
	unsigned int particleNum = global_data->getValue(DATA_MAGIC, type, 4);
	unsigned int particleRadius = global_data->getValue(DATA_MAGIC, type, 5);
	unsigned int projectile = global_data->getValue(DATA_MAGIC, type, 6);
	unsigned int projectileNum = global_data->getValue(DATA_MAGIC, type, 7);
	unsigned int projectileMagic = global_data->getValue(DATA_MAGIC, type, 8);
	unsigned int numPulses = global_data->getValue(DATA_MAGIC, type, 9);
	float pulseOffset = global_data->getValue(DATA_MAGIC, type, 10) * 0.01f;
	float initialDelay = global_data->getValue(DATA_MAGIC, type, 11) * 0.01f;
	unsigned int sound = global_data->getValue(DATA_MAGIC, type, 12);

	float cX = center->getX() + center->getMaskWidth() / 2;
	float cY = center->getY() + center->getMaskHeight() / 2;

	if (original && !pulsed && (numPulses > 1 || initialDelay != 0))
	{
		pulse *p = new pulse();
		p->center = center;
		p->maxTime = pulseOffset;
		if (initialDelay != 0)
			p->time = initialDelay;
		else
			p->time = pulseOffset;
		p->targets = targets;
		p->type = type;
		if (initialDelay != 0)
			p->pulses = numPulses;
		else
			p->pulses = numPulses - 1;
		pulses.push_back(p);
	}

	if (initialDelay != 0 && original && !pulsed)
		return; //don't do anything immediately

	if (sound != DATA_NONE)
		global_sound->play(sound);

	sf::FloatRect *customArea = NULL;

	switch(form)
	{
	case 3:
		//the custom area is really just the particle radius
		customArea = new sf::FloatRect(cX - particleRadius, cY - particleRadius, particleRadius * 2.0f, particleRadius * 2.0f);
		break;
	case 4:
		//the custom area is a small radius around the shooting area
		customArea = new sf::FloatRect(center->projectileStartX() - particleRadius, center->projectileStartY() - particleRadius, particleRadius * 2.0f, particleRadius * 2.0f);
		break;
	case 5:
		//left burst
		customArea = new sf::FloatRect(GENERATOR_SIDEBORDER * TILESIZE * 1.0f, TILESIZE * 1.0f,
										particleRadius * 1.0f, (height - 1 - GENERATOR_BOTTOMBORDER) * TILESIZE * 1.0f);
		break;
	case 6:
		//right burst
		customArea = new sf::FloatRect((width - GENERATOR_SIDEBORDER) * TILESIZE - particleRadius * 1.0f, TILESIZE * 1.0f,
										particleRadius * 1.0f, (height - 1 - GENERATOR_BOTTOMBORDER) * TILESIZE * 1.0f);
		break;
	case 7:
		//floor burst
		customArea = new sf::FloatRect(GENERATOR_SIDEBORDER * TILESIZE * 1.0f, (height - GENERATOR_BOTTOMBORDER) * TILESIZE - particleRadius * 1.0f,
										(width - 2 * GENERATOR_SIDEBORDER) * TILESIZE * 1.0f, particleRadius * 1.0f);
		break;
	}

	if (particle != DATA_NONE)
	{
		//make the projectiles
		if (customArea != NULL)
			addParticles(particle, particleNum, customArea);
		else
			addParticles(particle, particleNum, cX, cY, (float) particleRadius);
	}

	switch(form)
	{
	case 0:
		//basic single-target attack
		if (center->projectileInteract())
		{
			if (damage != 0)
				center->takeHit(damage, DATA_NONE, 0, cX, cY, false);
			center->applyEffect(effect);
		}
		break;
	case 1:
		//fire projectile
		if (center->projectileInteract())
			center->forceShoot(damage, DATA_NONE, projectile, projectileMagic);
		break;
	case 2:
		//healing magic
		if (center->projectileInteract())
		{
			if (damage != 0)
				center->heal(damage);
			center->applyEffect(effect);
		}
		break;
	case 3:
	case 5:
	case 6:
	case 7:
		//AoE damage
		for (unsigned int i = 0; i < creatures.size(); i++)
		{
			if (creatures[i]->projectileInteract() && creatures[i]->getType() == targets && !creatures[i]->isInvincible())
			{
				sf::FloatRect mRect(creatures[i]->getX(), creatures[i]->getY(), creatures[i]->getMaskWidth() * 1.0f, creatures[i]->getMaskHeight() * 1.0f);
				if (customArea->intersects(mRect))
				{
					if (damage != 0)
						creatures[i]->takeHit(damage, DATA_NONE, 0, cX, cY);
					creatures[i]->applyEffect(effect);
				}
			}
		}
		break;
	case 4:
		//nothing actually
		break;
	case 8:
		//random spell
		if (center->projectileInteract())
			magic(center, type + 1 + rand() % 3, targets);
		break;
	case 9:
		//rain
		if (center->projectileInteract())
		{
			float pX = rand() % ((width - GENERATOR_SIDEBORDER * 2) * TILESIZE - 4) + GENERATOR_SIDEBORDER * TILESIZE + 2.0f;
			float pY = TILESIZE + 5.0f;
			addProjectile(pX, pY, 0, 1, projectile, damage, DATA_NONE, TYPE_ENEMY, projectileMagic, false); 
		}
		break;
	}

	if (customArea != NULL)
		delete customArea;

	if (original && !single)
	{
		//tell the other end to do it too
		eventLog << EVENT_LOG_MAGIC;
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i] == center)
			{
				eventLog << (float) i;
				break;
			}
		eventLog << (float) type << (float) targets;
	}
}

void map::deactivateTrap(trap *toDeactivate)
{
	if (!single)
	{
		eventLog << EVENT_LOG_DEACTIVATETRAP;
		for (unsigned int i = 0; i < traps.size(); i++)
			if (traps[i] == toDeactivate)
			{
				eventLog << (float) i;
				break;
			}
	}
}

void map::playerUpdate()
{
	if (!single)
	{
		//make a player update packet
		eventLog << EVENT_LOG_PLAYERUPDATE;
		eventLog << (float) identifier;
		eventLog << creatures[identifier]->getX() << creatures[identifier]->getY();
		creatures[identifier]->save(&eventLog);
	}
}

void map::pickupItem(unsigned int i)
{
	creature *toPickup = creatures[identifier];

	if (master || single)
	{
		//just get it
		toPickup->getItem(items[i]);

		if (!single)
		{
			//make a remove item log for that
			eventLog << EVENT_LOG_REMOVEITEM;
			eventLog << (float) items[i]->getIdentifier();

			if (!items[i]->removed)
			{
				//and remake it, since it wasn't really destroyed
				eventLog << EVENT_LOG_ITEMDROP;
				eventLog << items[i]->getX() << items[i]->getY();
				items[i]->save(&eventLog);
			}
		}
	}
	else
	{
		eventLog << EVENT_LOG_PICKUPREQUEST;
		eventLog << (float) identifier << (float) items[i]->getIdentifier();
	}
}

void map::dropItem(item *item, float x, float y)
{
	item->drop(x, y);
	items.push_back(item);

	if (!single)
	{
		eventLog << EVENT_LOG_ITEMDROP;
		eventLog << x << y;
		item->save(&eventLog);
	}
}

void map::killProjectile(unsigned int idToKill)
{
	bool found = false;
	for (unsigned int i = 0; i < projectiles.size(); i++)
	{
		if (!found)
		{
			found = projectiles[i]->tryKill(idToKill);
			if (found)
			{
				projectiles[i]->forceDrop();
				delete projectiles[i];
			}
		}
		if (found)
			projectiles[i] = projectiles[i + 1];
	}
	if (found)
		projectiles.pop_back();
}

void map::addProjectile(float x, float y, float xDir, float yDir, unsigned int type, unsigned int damage, unsigned int attack, unsigned int side, unsigned int magic, bool venemous)
{
	projectile *proj = new projectile(damage, attack, type, side, magic, x, y, xDir, yDir, identifier, venemous);
	if (!single)
	{
		eventLog << EVENT_LOG_PROJECTILEMAKE;
		proj->save(&eventLog);
	}
	projectiles.push_back(proj);
}

void map::addParticles(unsigned int type, unsigned int particles, float x, float y, float radius)
{
	sf::FloatRect fr(x - radius, y - radius, radius * 2, radius * 2);
	addParticles(type, particles, &fr);
}

void map::addParticles(unsigned int type, unsigned int particles, sf::FloatRect *customArea)
{
	for (unsigned int i = 0; i < particles; i++)
	{
		float xP = customArea->left + (rand() % 100 * 0.01f) * customArea->width;
		float yP = customArea->top + (rand() % 100 * 0.01f) * customArea->height;
		particleList.push_back(new particle(xP, yP, type));
	}
}

void map::addDamageNumber(creature *cr, int amount, float hitX, float hitY, unsigned int sound)
{
	if (amount == 0)
		return;
	unsigned int modAmount;
	sf::Color c;
	if (amount >= 0)
	{
		modAmount = amount;
		c = INTERFACE_DNUMCOLOR;
	}
	else
	{
		modAmount = -amount;
		c = INTERFACE_HNUMCOLOR;
	}
	dNums.push_back(new damageNumber(cr->getX() + cr->getMaskWidth() / 2, cr->getY() + cr->getMaskHeight() / 2, c, modAmount));
	if (amount > 0)
		cr->bloodEffect(hitX, hitY, amount);
	if (amount == 0)
		sound = DATA_NONE;
	if (sound != DATA_NONE)
		global_sound->play(sound);
	if (!single && cr->makeDamageNum())
	{
		eventLog << EVENT_LOG_DAMAGE;
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i] == cr)
			{
				eventLog << (float) i;
				break;
			}
		eventLog << (float) amount << hitX << hitY << (float) sound;
	}
}

/*void map::networkUpdateSend()
{
	//prepare a packet to send
	sf::Packet outPacket;
	mutex.lock();
	for (unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->sendPacket(&outPacket);
	mutex.unlock();

	fastSocket.send(outPacket, sf::IpAddress(udpTo), udpPortTo);
}*/

/*void map::networkUpdateReceive()
{
	sf::Packet inPacket;
	sf::Socket::Status result = fastSocket.receive(inPacket, sf::IpAddress(udpTo), udpPortTo);
	if (result == sf::Socket::Done)
	{
		mutex.lock();
		std::cout << "Received packet of size " << inPacket.getDataSize() << std::endl;
		for (unsigned int i = 0; i < creatures.size(); i++)
			creatures[i]->receivePacket(&inPacket);
		mutex.unlock();
	}
	else
		std::cout << result << std::endl;
}*/


void map::receivePacket()
{
	sf::Packet inPacket;
	disconnected = socket->receive(inPacket) != socket->Done;
	if (disconnected)
		return;

	mutex.lock();
	readEventLog(&inPacket);
	/**/

	//get the map number
	float uMapNum;
	inPacket >> uMapNum;
	if (uMapNum == (float) mapNum)
	{
		//it's the right map number
		//so the data is valid for this map

		for (unsigned int i = 0; i < creatures.size(); i++)
			creatures[i]->receivePacket(&inPacket);
		if (!inPacket.endOfPacket())
			std::cout << "Warning: invalid packet size" << std::endl;
	}
	/**/
	mutex.unlock();
}

void map::networkAction()
{
	if (disconnected)
		return;
	if (!master)
		receivePacket();
	if (disconnected)
		return;

	mutex.lock();

	if (master && mapChange.size() > 0)
	{
		//generate the map
		checkWprinKill();
		mapGenerate();

		//put it in the event log
		eventLog << EVENT_LOG_MAPDATA;
		mapSend(&eventLog);

		mapChange.clear();
	}

	//finish, copy, and clear the event log
	eventLog << EVENT_LOG_END << (float) mapNum;
	sf::Packet outPacket(eventLog);
	/**/
	for (unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->sendPacket(&outPacket);
	/**/
	makeEventLog();

	mutex.unlock();

	socket->send(outPacket);

	if (master)
		receivePacket();
	if (disconnected)
		return;
}

void map::pollCamera(float elapsed)
{
	//save the old camera
	float oldCX = global_camera->x;
	float oldCY = global_camera->y;

	bool xSmall = width * TILESIZE <= SCREENWIDTH + TILESIZE;
	bool ySmall = height * TILESIZE <= SCREENHEIGHT + TILESIZE;
	
	if (!xSmall || !ySmall)
	{
		//initialize the camera
		global_camera->x = 0;
		global_camera->y = 0;

		//poll the creatures
		unsigned int checkCounter = 0;
		for (unsigned int i = 0; i < creatures.size(); i++)
			creatures[i]->pollCamera(&checkCounter);

		//adjust it
		global_camera->x /= checkCounter;
		global_camera->y /= checkCounter;
		global_camera->x -= SCREENWIDTH / 2;
		global_camera->y -= SCREENHEIGHT / 2;

		//bound the camera
		if (global_camera->x < 0)
			global_camera->x = 0;
		else if (global_camera->x + SCREENWIDTH > width * TILESIZE)
			global_camera->x = width * TILESIZE * 1.0f - SCREENWIDTH;
		if (global_camera->y < 0)
			global_camera->y = 0;
		else if (global_camera->y + SCREENHEIGHT > height * TILESIZE)
			global_camera->y = height * TILESIZE * 1.0f - SCREENHEIGHT;
	}
	
	//bound for small maps
	if (xSmall)
		global_camera->x = 0;
	if (ySmall)
		global_camera->y = 0;

	if (oldCX != -1)
	{
		//and now move it smoothly
		float xD = global_camera->x - oldCX;
		float yD = global_camera->y - oldCY;
		float dis = sqrt(xD * xD + yD * yD);
		float xDB = elapsed * CAMERASPEED * xD / dis;
		float yDB = elapsed * CAMERASPEED * yD / dis;
		if (abs(xDB) < abs(xD))
			global_camera->x = oldCX + xDB;
		if (abs(yDB) < abs(yD))
			global_camera->y = oldCY + yDB;
	}
}

void map::mapShadow(unsigned int x, unsigned int y, drawer *draw)
{
	//examine the tile
	bool c = tileSolid(x * TILESIZE * 1.0f, y * TILESIZE * 1.0f, 1, 0, true);
	bool u = y == 0 || tileSolid(x * TILESIZE * 1.0f, (y - 1) * TILESIZE * 1.0f, 1, 0, true);
	bool ul = y == 0 || x == 0 || tileSolid((x - 1) * TILESIZE * 1.0f, (y - 1) * TILESIZE * 1.0f, 1, 0, true);
	bool ur = y == 0 || x == width - 1 || tileSolid((x + 1) * TILESIZE * 1.0f, (y - 1) * TILESIZE * 1.0f, 1, 0, true);
	bool d = y == height - 1 || tileSolid(x * TILESIZE * 1.0f, (y + 1) * TILESIZE * 1.0f, 1, 0, true);
	bool dl = y == height - 1 || x == 0 || tileSolid((x - 1) * TILESIZE * 1.0f, (y + 1) * TILESIZE * 1.0f, 1, 0, true);
	bool dr = y == height - 1 || x == width - 1 || tileSolid((x + 1) * TILESIZE * 1.0f, (y + 1) * TILESIZE * 1.0f, 1, 0, true);
	bool l = x == 0 || tileSolid((x - 1) * TILESIZE * 1.0f, y * TILESIZE * 1.0f, 1, 0, true);
	bool r = x == width - 1 || tileSolid((x + 1) * TILESIZE * 1.0f, y * TILESIZE * 1.0f, 1, 0, true);

	unsigned int fr = 0;
	bool flipped = false;

	if (!c)
	{
		if (tiles[toI(x, y)] != DATA_NONE && global_data->getValue(DATA_TILE, tiles[toI(x, y)], 3))
		{
			//one-way tile
			fr = 8;
		}
		else if (tiles[toI(x, y)] != DATA_NONE)
		{
			//it's a background tile
			fr = 9;
		}
		else
		{
			//empty tile
			fr = 7;
		}
	}
	else if (r && u && l && d)
	{
		//it's an inside corner... maybe
		if (!ur && ul && dr && dl)
		{
			//upper-right inside corner
			fr = 1;
		}
		else if (ur && !ul && dr && dl)
		{
			//upper-left inside corner
			fr = 1;
			flipped = true;
		}
		else if (ur && ul && !dr && dl)
		{
			//down-right inside corner
			fr = 5;
		}
		else if (ur && ul && dr && !dl)
		{
			//down-left inside corner
			fr = 5;
			flipped = true;
		}
		else
			return; //it's nothing
	}
	else if (!l && !u && d && r && dr)
	{
		//upper-left outside corner
		fr = 2;
	}
	else if (!r && !u && d && l && dl)
	{
		//upper-right outside corner
		fr = 2;
		flipped = true;
	}
	else if (!l && !d && u && r && ur)
	{
		//down-left outside corner
		fr = 6;
	}
	else if (!r && !d && u && l && ul)
	{
		//down-right outside corner
		fr = 6;
		flipped = true;
	}
	else if (d && !u && l && r)
	{
		//top tile
		fr = 0;
	}
	else if (d && u && l && !r)
	{
		//right tile
		fr = 3;
		flipped = true;
	}
	else if (d && u && !l && r)
	{
		//left tile
		fr = 3;
	}
	else if (!d && u && l && r)
	{
		//bottom tile
		fr = 4;
	}
	else
		return; //it's nothing
	
	if (flipped)
		x += 1;

	sf::Color cl;
	if (faction)
		cl = makeColor(global_data->getValue(DATA_TILESET, tileset, 5));
	else
		cl = makeColor(global_data->getValue(DATA_TILESET, tileset, 4));

	draw->renderSprite(global_data->getValue(DATA_LIST, 0, 1), fr, x * TILESIZE * 1.0f - global_camera->x, y * TILESIZE * 1.0f - global_camera->y,
						flipped, cl);
}

void map::backgroundLayer(unsigned int layer, drawer *draw)
{
	int xParal = global_data->getValue(DATA_BACKGROUNDLAYER, layer, 0);
	int bHeight = global_data->getValue(DATA_BACKGROUNDLAYER, layer, 1);
	int yAdd = global_data->getValue(DATA_BACKGROUNDLAYER, layer, 2);
	unsigned int sheet = global_data->getValue(DATA_BACKGROUNDLAYER, layer, 3);
	unsigned int sWidth = global_data->getValue(DATA_SHEET, sheet, 1);

	float bX = global_camera->x * xParal * 0.01f;
	float bY = (SCREENHEIGHT - bHeight) * (global_camera->y) / (height * TILESIZE - SCREENHEIGHT);

	unsigned int start = (unsigned int) floor(bX / sWidth);
	unsigned int end = (unsigned int) ceil((bX + SCREENWIDTH) / sWidth);
	for (unsigned int i = start; i <= end; i++)
	{
		unsigned int frame = i % BACKGROUNDARRAYLENGTH;
		frame = bArray[frame];
		frame %= global_data->getValue(DATA_BACKGROUNDLAYER, layer, 5);
		frame = global_data->getValue(DATA_BACKGROUNDLAYER, layer, 6 + frame);
		if (frame != DATA_NONE)
			draw->renderSprite(sheet, frame, i * sWidth - bX, bY + yAdd, false, makeColor(global_data->getValue(DATA_BACKGROUNDLAYER, layer, 4)));
	}
}

void map::render(drawer *draw)
{
	if (cD != NULL)
	{
		if (!cD->done)
		{
			mutex.lock();
			cD->render(draw);
			mutex.unlock();
		}
		return;
	}

	if (single && !(*global_hasFocus))
	{
		renderText(draw, "PAUSED", INTERFACE_INACTIVECOLOR, SCREENWIDTH / 2, SCREENHEIGHT / 2, true);
		return;
	}

	//find the things to draw
	unsigned int xStart = (unsigned int) (global_camera->x / TILESIZE);
	unsigned int yStart = (unsigned int) (global_camera->y / TILESIZE);
	unsigned int xEnd = xStart + (SCREENWIDTH / TILESIZE) + 1;
	unsigned int yEnd = yStart + (SCREENHEIGHT / TILESIZE) + 1;

	//draw tiles

	unsigned int greeneryColor;
	sf::Color greeneryColorC;
	if (faction)
		greeneryColor = global_data->getValue(DATA_TILESET, tileset, 3);
	else
		greeneryColor = global_data->getValue(DATA_TILESET, tileset, 2);
	if (greeneryColor != DATA_NONE)
		greeneryColorC = makeColor(greeneryColor);

	for (unsigned int y = yStart; y <= yEnd && y < height; y++)
		for (unsigned int x = xStart; x <= xEnd && x < width; x++)
		{
			unsigned int i = toI(x, y);
			if (tiles[i] != DATA_NONE)
			{
				draw->renderSprite(0, global_data->getValue(DATA_TILE, tiles[i], 0),
					x * TILESIZE - global_camera->x, y * TILESIZE - global_camera->y,
					false, makeColor(global_data->getValue(DATA_TILE, tiles[i], 1)));

				if (greeneryColor != DATA_NONE)
				{
					unsigned int greenery = global_data->getValue(DATA_TILE, tiles[i], gArray[(x + y) % GREENERYARRAYLENGTH]);
					if (greenery != DATA_NONE)
					{
						if (!faction)
							greenery += 2;
						draw->renderSprite(0, greenery, x * TILESIZE - global_camera->x,
										y * TILESIZE - global_camera->y, false, greeneryColorC);
					}
				}
			}
			mapShadow(x, y, draw);
		}

	//draw doors
	for (unsigned int i = 0; i < doors.size(); i++)
		if (doors[i]->x >= xStart && doors[i]->x <= xEnd && doors[i]->y >= yStart && doors[i]->y <= yEnd)
			doors[i]->render(draw);

	//draw items
	for (unsigned int i = 0; i < items.size(); i++)
		items[i]->render(draw);

	//draw creatures
	for (unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->render(draw);

	//draw projectiles
	for (unsigned int i = 0; i < projectiles.size(); i++)
		projectiles[i]->render(draw);

	//draw damage numbers
	for (unsigned int i = 0; i < dNums.size(); i++)
		dNums[i]->render(draw);

	//draw particles
	for (unsigned int i = 0; i < particleList.size(); i++)
		particleList[i]->render(draw);

	//draw traps
	for (unsigned int i = 0; i < traps.size(); i++)
		traps[i]->render(draw);

	//draw decorations
	for (unsigned int i = 0; i < decorations.size(); i++)
		decorations[i]->render(draw);

	//draw background
	unsigned int layers = global_data->getValue(DATA_TILESET, tileset, DATA_TILESET_LAYERNUM);
	for (unsigned int i = 0; i < layers; i++)
		backgroundLayer(global_data->getValue(DATA_TILESET, tileset, DATA_TILESET_LAYERNUM + 1 + i), draw);
}

//map handling
void map::mapClear(bool sparePlayers)
{
	delete[] tiles;
	for (unsigned int i = 0; i < creatures.size(); i++)
		if (!sparePlayers || creatures[i]->getType() != TYPE_PLAYER)
			delete creatures[i];
	creatures.clear();
	for (unsigned int i = 0; i < projectiles.size(); i++)
		delete projectiles[i];
	projectiles.clear();
	for (unsigned int i = 0; i < dNums.size(); i++)
		delete dNums[i];
	dNums.clear();
	for (unsigned int i = 0; i < particleList.size(); i++)
		delete particleList[i];
	particleList.clear();
	for (unsigned int i = 0; i < items.size(); i++)
		delete items[i];
	items.clear();
	for (unsigned int i = 0; i < traps.size(); i++)
		delete traps[i];
	traps.clear();
	for (unsigned int i = 0; i < pulses.size(); i++)
		delete pulses[i];
	pulses.clear();
	for (unsigned int i = 0; i < doors.size(); i++)
		delete doors[i];
	doors.clear();
	for (unsigned int i = 0; i < decorations.size(); i++)
		delete decorations[i];
	decorations.clear();
}

void map::mapGenerate()
{
	//apply the door's stats, if applicable
	unsigned int mapMod;
	if (usedDoor != NULL)
	{
		tileset = usedDoor->tileset;
		faction = usedDoor->faction;
		mapMod = usedDoor->mapMod;
		type = usedDoor->type;
		mapNum = usedDoor->num;
		usedDoor = NULL;
	}
	else
	{
		//the map number starts at 0 now, and the type at 0
		mapNum = 0;
		type = 0;
		mapMod = DATA_NONE;
		tileset = mapNum / GENERATOR_TILESETINTERVAL;
		faction = false;
	}

	std::vector<creature *> savedCreatures;
	if (creatures.size() > 0)
	{
		//this is a map re-generation
		for (unsigned int i = 0; i < creatures.size(); i++)
			if (creatures[i]->getType() == TYPE_PLAYER)
				savedCreatures.push_back(creatures[i]);

		mapClear(true);
	}

	while (!mapGenerateInner(savedCreatures, mapMod)) {}

	//play an appropriate song
	playSong(tileset);
}

void map::generateTown(std::vector<std::vector<int>> *platforms, std::vector<std::vector<int>> *jumpGaps, unsigned int townShape)
{
	std::vector<int> topPlatform;
	std::vector<int> bottomPlatform;
	for (unsigned int i = 0; i < GENERATOR_TOWNWIDTH; i++)
	{
		bool top = global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3) == 1;
		bool bottom = global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3 + GENERATOR_TOWNWIDTH * 3 + 1) == 1;

		if (topPlatform.size() == 0)
		{
			//make a new top platform
			topPlatform.push_back(i * GENERATOR_TOWNCHUNKWIDTH);
			topPlatform.push_back(i * GENERATOR_TOWNCHUNKWIDTH - 1);
			topPlatform.push_back(0);
		}
		if (bottomPlatform.size() == 0)
		{
			//make a new bottom platform
			bottomPlatform.push_back(i * GENERATOR_TOWNCHUNKWIDTH);
			bottomPlatform.push_back(i * GENERATOR_TOWNCHUNKWIDTH - 1);
			bottomPlatform.push_back(GENERATOR_TOWNHEIGHT);
		}

		if (top)
			topPlatform[1] += GENERATOR_TOWNCHUNKWIDTH;
		else
		{
			if (topPlatform[1] > topPlatform[0])
				platforms->push_back(topPlatform);
			topPlatform.clear();
		}

		if (bottom)
			bottomPlatform[1] += GENERATOR_TOWNCHUNKWIDTH;
		else
		{
			if (bottomPlatform[1] > bottomPlatform[0])
				platforms->push_back(bottomPlatform);
			bottomPlatform.clear();
		}
	}

	if (topPlatform.size() > 0 && topPlatform[1] > topPlatform[0])
		platforms->push_back(topPlatform);
	if (bottomPlatform.size() > 0 && bottomPlatform[1] > bottomPlatform[0])
		platforms->push_back(bottomPlatform);
}

bool map::generatePlatforms(std::vector<std::vector<int>> *platforms, std::vector<std::vector<int>> *jumpGaps, unsigned int desiredPlatforms)
{
	//make the initial platform
	std::vector<int> startPlatform;
	startPlatform.push_back(0); //starts at x 0
	startPlatform.push_back(GENERATOR_PLATFORMSTART); //use the start length
	startPlatform.push_back(0); //y is 0; this doesn't really matter much
	(*platforms).push_back(startPlatform);

	for (unsigned int q = 0; q < GENERATOR_MAXTRIES && platforms->size() < desiredPlatforms; q++)
	{
		if (q == GENERATOR_MAXTRIES - 1)
		{
			//it's stuck in a loop
			return false;
		}

		//first, pick a platform to base it off
		//specifically, pick two and choose the one further right
		unsigned int platFrom1 = rand() % (platforms->size());
		unsigned int platFrom2 = rand() % (platforms->size());

		unsigned int platFrom;
		if ((*platforms)[platFrom1][0] > (*platforms)[platFrom2][0])
			platFrom = platFrom1;
		else
			platFrom = platFrom2;

		//next, pick a direection
		int yDir;
		if (rand() % 2 == 1)
			yDir = 1;
		else
			yDir = -1;

		std::vector<int> newPlatform;

		//find the X position to start at
		if (yDir == -1 && platFrom != 0)
		{
			//you are jumping up, so it can start at any point on the initial platform
			newPlatform.push_back(rand() % ((*platforms)[platFrom][1] - (*platforms)[platFrom][0]) + (*platforms)[platFrom][0]);
		}
		else
		{
			//you are going down (or jumping up for the first time), so it has to happen at the end
			newPlatform.push_back((*platforms)[platFrom][1] - 1);
		}

		if (yDir == -1)
		{
			//add a random factor for the jump length
			newPlatform[0] += rand() % (GENERATOR_JUMPMAXH - GENERATOR_JUMPMINH) + GENERATOR_JUMPMINH;
		}

		if (rand() % 100 < GENERATOR_LONGJUMPCHANCE)
		{
			//it's a long jump!
			newPlatform[0] += GENERATOR_LONGJUMPLENGTH;
		}

		//find the x position to end at
		newPlatform.push_back(newPlatform[0] + rand() % (GENERATOR_PLATFORMMAX - GENERATOR_PLATFORMMIN) + GENERATOR_PLATFORMMIN);

		//find the y position to be in
		newPlatform.push_back((*platforms)[platFrom][2] + yDir * (rand() % (GENERATOR_JUMPMAXV - GENERATOR_JUMPMINV) + GENERATOR_JUMPMINV));

		//will this platform touch any others?
		bool valid = true;
		for (unsigned int j = 0; j < platforms->size(); j++)
		{
			if (abs((*platforms)[j][2] - newPlatform[2]) <= 2 && //close in y
				(*platforms)[j][1] >= newPlatform[0] - 2 && newPlatform[1] >= (*platforms)[j][0] - 2) //it's intersecting, or close to
			{
				//there two platforms will crowd each other
				valid = false;
				break;
			}
		}

		if (valid)
		{
			int jumpGapL = newPlatform[0] - (*platforms)[platFrom][1] - 1;
			if (jumpGapL > 0)
			{
				int lowerY;
				if (yDir == 1)
					lowerY = newPlatform[2];
				else
					lowerY = (*platforms)[platFrom][2];

				//push a jump gap to make SURE that there's no wall blocking this jump
				std::vector<int> jumpGap;
				jumpGap.push_back((*platforms)[platFrom][1] + 1);
				jumpGap.push_back(newPlatform[0] - 1);
				jumpGap.push_back(lowerY);
				jumpGaps->push_back(jumpGap);
			}
			

			//push the platform
			(*platforms).push_back(newPlatform);
		}
	}

	return true;
}

bool map::mapGenerateInner(std::vector<creature *> savedCreatures, unsigned int mapMod)
{
	//pick an encounter data
	float thingsPer;
	if (single)
		thingsPer = global_data->getValue(DATA_TILESET, tileset, 9) * 0.01f; //singleplayer enemy density
	else
		thingsPer = global_data->getValue(DATA_TILESET, tileset, 10) * 0.01f; //multiplayer enemy density
	unsigned int encounterDatas = global_data->getValue(DATA_TILESET, tileset, 6);
	unsigned int encounterData = 0;
	for (unsigned int i = 0; i < global_data->getEntrySize(DATA_LIST, encounterDatas); i++)
	{
		unsigned int eD = global_data->getValue(DATA_LIST, encounterDatas, i);
		if ((unsigned int) global_data->getValue(DATA_ENCOUNTERDATA, eD, 0) <= mapNum)
			encounterData = eD;
		else
			break;
	}

	//get the factors of that encounter data
	unsigned int ramp = mapNum - global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 0);
	unsigned int desiredPlatforms = ramp * global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 2) +
									global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 1);
	unsigned int desiredEncounters = ramp * global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 4) +
									global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 3);
	unsigned int bossList = global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 5);
	unsigned int enemyList = global_data->getValue(DATA_ENCOUNTERDATA, encounterData, 6);
	if (faction)
	{
		bossList += 1;
		enemyList += 1;
	}

	//trap stuff
	unsigned int trapsList = global_data->getValue(DATA_TILESET, tileset, 7);
	unsigned int desiredTraps = (unsigned int) ((desiredPlatforms - 2) * global_data->getValue(DATA_TILESET, tileset, 8) * 0.01f);
	if (desiredTraps == 0)
		desiredTraps = 1;

	//food stuff
	unsigned int foodAnimal = global_data->getValue(DATA_TILESET, tileset, 11);
	unsigned int foodAnimalNumber = global_data->getValue(DATA_TILESET, tileset, 12);
	if (!single)
		foodAnimalNumber = (unsigned int) (foodAnimalNumber * MULTIP_FOODMOD);

	//decoration stuff
	unsigned int decorationsList = global_data->getValue(DATA_TILESET, tileset, 13);
	unsigned int decorationsNumber = (unsigned int) (GENERATOR_DECORATIONSPERPLATFORM * (desiredPlatforms - 2));

	//map type variables
	unsigned int startTile = global_data->getValue(DATA_TILESET, tileset, 0);
	unsigned int backTileTypes = global_data->getValue(DATA_TILESET, tileset, 1);

	//platform list
	std::vector<std::vector<int>> platforms;
	std::vector<std::vector<int>> jumpGaps;

	unsigned int townShape = rand() % global_data->getCategorySize(DATA_TOWNSHAPE);

	if (type == 2)
		generateTown(&platforms, &jumpGaps, townShape);
	else if (type == 1)
	{
		//make the boss arena
		std::vector<int> platform;
		platform.push_back(0);
		platform.push_back(SCREENWIDTH / TILESIZE - 2);
		platform.push_back(0);
		platforms.push_back(platform);
	}
	else if (generatePlatforms(&platforms, &jumpGaps, desiredPlatforms))
	{
		//do the basic multiplication of h
		for (unsigned int i = 0; i < platforms.size(); i++)
		{
			platforms[i][0] *= GENERATOR_HMULT;
			platforms[i][1] *= GENERATOR_HMULT;
			platforms[i][1] += 1;
		}
		for (unsigned int i = 0; i < jumpGaps.size(); i++)
		{
			jumpGaps[i][0] *= GENERATOR_HMULT;
			jumpGaps[i][1] *= GENERATOR_HMULT;
			jumpGaps[i][1] += 1;
		}
	}
	else
		return false;

	//get bounds info
	int top = 999;
	int bottom = -999;
	int right = -999;
	unsigned int furthestPlatform = 0;
	for (unsigned int i = 0; i < platforms.size(); i++)
	{
		if (platforms[i][2] < top)
			top = platforms[i][2];
		if (platforms[i][2] > bottom)
			bottom = platforms[i][2];
		if (platforms[i][1] > right)
		{
			right = platforms[i][1];
			furthestPlatform = i;
		}
	}

	//set up boss and trap lists
	//this happens early because it is a point where the generator may fail
	std::vector<std::vector<unsigned int>> enemiesP;
	std::vector<std::vector<unsigned int>> trapsP;
	std::vector<unsigned int> decorationsP;
	if (type == 0)
	{
		for (unsigned int i = 0; i < platforms.size(); i++)
		{
			std::vector<unsigned int> eP;
			std::vector<unsigned int> tP;
			enemiesP.push_back(eP);
			trapsP.push_back(tP);
			decorationsP.push_back(DATA_NONE);
		}

		//mark that tile as a no-go for encounters and traps
		enemiesP[furthestPlatform].push_back(0);
		trapsP[furthestPlatform].push_back(0);
		decorationsP[furthestPlatform] = 0;

		//distribute encounters
		for (unsigned int i = 0; i < 1 + desiredEncounters; i++)
		{
			bool success;
			if (i == 0)
				success = generateEnemy(&platforms, &enemiesP, bossList, 1, 1.0f);
			else
				success = generateEnemy(&platforms, &enemiesP, enemyList, 0, thingsPer);
			if (!success)
				return false; //failed to place an enemy
		}

		//distribute traps
		for (unsigned int i = 0; i < desiredTraps; i++)
			if (!generateTrap(&platforms, &trapsP, trapsList, thingsPer))
				return false; //failed to place an enemy

		//distribute decorations
		for (unsigned int i = 0; i < decorationsNumber; i++)
		{
			unsigned int fTimer = mapNum % GENERATOR_TILESETINTERVAL;
			if (!generateDecoration(&platforms, &decorationsP, decorationsList, i == 0 && (fTimer == 3 || fTimer == 7)))
				return false; //failed to place a decoration
		}
	}

	//move all the platforms
	for (unsigned int i = 0; i < platforms.size(); i++)
	{
		platforms[i][0] += GENERATOR_SIDEBORDER;
		platforms[i][1] += GENERATOR_SIDEBORDER;
		platforms[i][2] += GENERATOR_TOPBORDER - top;
	}
	for (unsigned int i = 0; i < jumpGaps.size(); i++)
	{
		jumpGaps[i][0] += GENERATOR_SIDEBORDER;
		jumpGaps[i][1] += GENERATOR_SIDEBORDER;
		jumpGaps[i][2] += GENERATOR_TOPBORDER - top;
	}

	//get the dimensions
	width = 2 * GENERATOR_SIDEBORDER + right + 1;
	height = (bottom - top) + GENERATOR_TOPBORDER + GENERATOR_BOTTOMBORDER + 1;

	//note: now that something has been generated, you cannot quit the generation or else there will be a memory leak

	//make the tiles array, and a quick binary array for easy platform checks
	unsigned int platVarStart = rand() % 11;
	tiles = new unsigned int[width * height];
	bool *pTiles = new bool[width * height];
	bool *gTiles = new bool[width * height];
	bool *cTiles = new bool[width * height];
	unsigned int *nTiles = new unsigned int[width * height];
	for (unsigned int i = 0; i < width * height; i++)
	{
		tiles[i] = DATA_NONE;
		pTiles[i] = false;
		gTiles[i] = false;
		cTiles[i] = true;
		nTiles[i] = DATA_NONE;
	}

	//mark the platform parts on the binary array
	for (unsigned int i = 0; i < platforms.size(); i++)
	{
		for (unsigned int j = (unsigned int) platforms[i][0]; j <= (unsigned int) platforms[i][1]; j++)
		{
			unsigned int rI = toI(j, (unsigned int) platforms[i][2]);
			pTiles[rI] = true;
			unsigned int startN = nTiles[rI];
			for (unsigned int y = (unsigned int) platforms[i][2]; y < height; y++)
				if (nTiles[toI(j, y)] == startN)
					nTiles[toI(j, y)] = i;
		}

		//apply ceiling changes
		int cStart = platforms[i][0] - GENERATOR_CEILXADD;
		if (cStart < 0)
			cStart = 0;
		int cEnd = platforms[i][1] + GENERATOR_CEILXADD;
		if (cEnd >= (int) width)
			cEnd = width - 1;
		for (unsigned int j = (unsigned int) cStart; j <= (unsigned int) cEnd; j++)
			cTiles[toI(j, (unsigned int) platforms[i][2] - GENERATOR_CEILYADD)] = false;
	}

	//apply jumpgaps, if necessary
	for (unsigned int i = 0; i < jumpGaps.size(); i++)
		for (unsigned int j = (unsigned int) jumpGaps[i][0]; j <= (unsigned int) jumpGaps[i][1]; j++)
			gTiles[toI(j, (unsigned int) jumpGaps[i][2])] = true;

	//now place tiles on a collumn-by-collumn basis
	for (unsigned int x = 0; x < width; x++)
	{
		//first, look up to see if there is anything over here at all, and if you should start out solid
		bool pit = true;
		bool solidMode = true;
		for (unsigned int y = 0; y < height; y++)
		{
			if (gTiles[toI(x, height - y - 1)])
				solidMode = false;
			else if (pTiles[toI(x, height - y - 1)])
			{
				pit = false;
				break;
			}
		}

		if (!pit)
		{
			//we know it's not a pit, so make some ground tiles
			for (unsigned int y = 0; y < height; y++)
			{
				unsigned int rI = toI(x, height - y - 1);
				unsigned int nMult = (nTiles[rI] + platVarStart) % backTileTypes;
				if (pTiles[rI])
				{
					if (solidMode)
						tiles[rI] = startTile + 1;
					else
						tiles[rI] = startTile + 3 + 2 * nMult;
					solidMode = false;
				}
				else if (solidMode)
					tiles[rI] = startTile;
				else
					tiles[rI] = startTile + 2 + 2 * nMult;
			}
			//remove any excess back tiles
			for (unsigned int y = 0; y < height; y++)
			{
				if (!tileSolid(x * TILESIZE * 1.0f, y * TILESIZE * 1.0f))
					tiles[toI(x, y)] = DATA_NONE;
				else
					break;
			}
		}
	}

	//clear up the binary arrays
	delete[] pTiles;
	delete[] gTiles;
	delete[] nTiles;

	//fill the borders
	for (unsigned int x = 0; x < width; x++)
		for (unsigned int y = 0; y < height; y++)
		{
			if (cTiles[toI(x, y)] && (y == 0 || type != 1))
				tiles[toI(x, y)] = startTile;
			else
				break;
		}
	for (unsigned int y = 0; y < height; y++)
		for (unsigned int x = 0; x < GENERATOR_SIDEBORDER; x++)
		{
			tiles[toI(x, y)] = startTile;
			tiles[toI(width - x - 1, y)] = startTile;
		}

	//clean up the last array
	delete[] cTiles;

	//place the doors IF you arent on the last
	if (!lastMap())
	{
		unsigned int nD = GENERATOR_NUMDOORS;
		if (type == 2)
			nD = GENERATOR_NUMTOWNDOORS;
		for (unsigned int i = 0; i < nD; i++)
		{
			unsigned int doorX;
			unsigned int doorY;
			if (type == 2)
			{
				doorX = townPosOf(2, townShape).x + i;
				doorY = townPosOf(2, townShape).y - 1;
			}
			else
			{
				doorX = platforms[furthestPlatform][1] - i;
				doorY = platforms[furthestPlatform][2] - 1;
			}
			door *newDoor = new door(mapNum, type, doorX, doorY);
			doors.push_back(newDoor);
			if (type == 1 || newDoor->type != 0)
				break; //doors to boss rooms and such should not have multiples
		}
	}

	//place the player(s)
	float startX;
	float startY;
	if (type == 2)
	{
		//get the town start position
		startX = townPosOf(1, townShape).x * TILESIZE * 1.0f;
		startY = townPosOf(1, townShape).y * TILESIZE * 1.0f;
	}
	else
	{
		startX = (platforms[0][0] + 2) * TILESIZE * 1.0f;
		startY = (platforms[0][2]) * TILESIZE * 1.0f;
	}
	//place saved players
	for (unsigned int i = 0; i < savedCreatures.size(); i++)
	{
		savedCreatures[i]->teleport(startX + i * 20, startY);
		addCreature(savedCreatures[i]);
	}
	
	if (type == 0)
	{
		//use the platform tags to place enemies, traps, and decorations
		for (unsigned int i = 0; i < platforms.size(); i++)
		{
			if (enemiesP[i].size() > 0)
				for (unsigned int j = 0; j < enemiesP[i][0]; j++)
				{
					int cX = rand() % (platforms[i][1] - platforms[i][0]) + platforms[i][0];
					int cY = platforms[i][2] - 2;
					addCreature(new enemy(cX * TILESIZE * 1.0f, cY * TILESIZE * 1.0f, enemiesP[i][j * 2 + 1], enemiesP[i][j * 2 + 2] == 1));
				}
			if (trapsP[i].size() > 0)
				for (unsigned int j = 0; j < trapsP[i][0]; j++)
				{
					int cX = rand() % (platforms[i][1] - platforms[i][0] - 2) + platforms[i][0] + 1;
					int cXA = rand() % TILESIZE;
					int cY = platforms[i][2];
					traps.push_back(new trap(cX * TILESIZE * 1.0f + cXA, cY * TILESIZE * 1.0f, trapsP[i][j + 1]));
				}
			if (decorationsP[i] != DATA_NONE && i != furthestPlatform)
			{
				int cX = rand() % (platforms[i][1] - platforms[i][0] - 4) + platforms[i][0] + 2;
				int cY = platforms[i][2] - 1;
				decorations.push_back(new decoration(cX, cY, decorationsP[i]));
			}
		}

		//place food animals
		for (unsigned int i = 0; i < foodAnimalNumber; i++)
			freePlace(DATA_NONE, foodAnimal, &platforms, furthestPlatform);
	}
	else if (type == 1)
	{
		//place the boss
		float bossX = (width - 3) * TILESIZE * 1.0f;
		float bossY = (height - 3) * TILESIZE * 1.0f;
		if (tileset == 1)
			bossX -= TILESIZE * 2;
		unsigned int bossN = global_data->getValue(DATA_LIST, 3, 3 * tileset + 1);
		addCreature(new enemy(bossX, bossY, bossN, true));
		bossN = global_data->getValue(DATA_LIST, 3, 3 * tileset + 2);
		if (bossN != DATA_NONE) //add the second boss
			addCreature(new enemy(bossX, bossY, bossN, true));
	}
	else if (type == 2)
	{
		townNPCs(townShape, false);
		townNPCs(townShape, true);
	}

	//place decorations
	if (type == 2)
	{
		townDecorations(townShape, false);
		townDecorations(townShape, true);
	}

	if (mapMod != DATA_NONE)
	{
		//add unique stuff
		for (unsigned int i = 0; i < (unsigned int) global_data->getValue(DATA_MAPMOD, mapMod, 3); i++)
			freePlace(DATA_NONE, global_data->getValue(DATA_LIST, global_data->getValue(DATA_MAPMOD, mapMod, 2), tileset), &platforms, furthestPlatform);
		for (unsigned int i = 0; i < (unsigned int) global_data->getValue(DATA_MAPMOD, mapMod, 5); i++)
			freePlace(global_data->getValue(DATA_LIST, global_data->getValue(DATA_MAPMOD, mapMod, 4), tileset), DATA_NONE, &platforms, furthestPlatform);
	}

	//initialize camera
	global_camera->x = -1;
	global_camera->y = -1;

	//it was a success
	return true;
}

sf::Vector2u map::townPos(unsigned int i, bool top)
{
	unsigned int x = GENERATOR_SIDEBORDER + i * GENERATOR_TOWNCHUNKWIDTH;
	unsigned int y = height - 1 - GENERATOR_BOTTOMBORDER;
	if (top)
		y -= GENERATOR_TOWNHEIGHT;
	return sf::Vector2u(x, y);
}
sf::Vector2u map::townPosOf(unsigned int id, unsigned int townShape)
{
	for (unsigned int i = 0; i < GENERATOR_TOWNWIDTH; i++)
	{
		if (global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3 + 1) == id ||
			global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3 + 2) == id)
			return townPos(i, true);
		if (global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3 + GENERATOR_TOWNWIDTH * 3 + 2) == id ||
			global_data->getValue(DATA_TOWNSHAPE, townShape, i * 3 + GENERATOR_TOWNWIDTH * 3 + 3) == id)
			return townPos(i, false);
	}
	return townPos(0, true);
}

void map::townDecorations(unsigned int townShape, bool top)
{
	for (unsigned int i = 0; i < GENERATOR_TOWNWIDTH; i++)
	{
		unsigned int id = i * 3 + 2;
		if (!top)
			id += GENERATOR_TOWNWIDTH * 3 + 1;

		unsigned int dID = global_data->getValue(DATA_TOWNSHAPE, townShape, id);
		if (dID > 2)
			decorations.push_back(new decoration(townPos(i, top).x, townPos(i, top).y - 1, global_data->getValue(DATA_TOWNDATA, tileset, dID)));
	}
}

void map::townNPCs(unsigned int townShape, bool top)
{
	for (unsigned int i = 0; i < GENERATOR_TOWNWIDTH; i++)
	{
		unsigned int id = i * 3 + 1;
		if (!top)
			id += GENERATOR_TOWNWIDTH * 3 + 1;

		float x = 1.0f * (TILESIZE * townPos(i, top).x + TILESIZE / 2 + rand() % TILESIZE);
		float y = 1.0f * TILESIZE * townPos(i, top).y;

		unsigned int cID = global_data->getValue(DATA_TOWNSHAPE, townShape, id);
		if (cID != 0)
		{
			unsigned int vID = global_data->getValue(DATA_TOWNDATA, tileset, cID);
			if (vID != DATA_NONE)
			{
				creature *villager = new npc(x, y, vID);
				if (villager->isMerchant())
				{
					//move them a bit back
					villager->teleport(1.0f * TILESIZE * townPos(i, top).x, villager->getY());

					//make a merchant inventory
					unsigned int inventory = global_data->getValue(DATA_TOWNDATA, tileset, 0);
					for (unsigned int j = 1; j <= GENERATOR_SHOPITEMS; j++)
					{
						//randomly assemble the shop item from the shop sublists
						unsigned int form = rand() % (global_data->getEntrySize(DATA_LIST, inventory) / 2);
						unsigned int formL = global_data->getValue(DATA_LIST, inventory, form * 2);
						unsigned int matL = global_data->getValue(DATA_LIST, inventory, form * 2 + 1);
						unsigned int formP = rand() % (global_data->getEntrySize(DATA_LIST, formL) / 2);
						unsigned int matP = rand() % global_data->getEntrySize(DATA_LIST, matL);


						item *it = new item(global_data->getValue(DATA_LIST, formL, formP * 2),
											global_data->getValue(DATA_LIST, formL, formP * 2 + 1), 1,
											global_data->getValue(DATA_LIST, matL, matP),
											villager->getX() + villager->getMaskWidth() + GENERATOR_SHOPXOFF * j, villager->getY(), identifier);
						it->calculateCost(); //it's a shop item, so it has to have a cost
						items.push_back(it);
					}
				}
				addCreature(villager);
			}
		}
	}
}

void map::freePlace(unsigned int trapType, unsigned int creatureType, std::vector<std::vector<int>> *platforms, unsigned int finalPlatform)
{
	//pick a platform for it to be on
	unsigned int plat = finalPlatform;
	while (plat == finalPlatform)
		plat = rand() % (platforms->size() - 1) + 1;
		
	//place it randomly on the platform
	int cX = rand() % ((*platforms)[plat][1] - (*platforms)[plat][0] - 2) + (*platforms)[plat][0] + 1;
	int cY = (*platforms)[plat][2] - 1;
	if (creatureType != DATA_NONE)
		addCreature(new enemy(cX * TILESIZE * 1.0f, cY * TILESIZE * 1.0f, creatureType, false));
	else
		traps.push_back(new trap(cX * TILESIZE * 1.0f, (cY + 1) * TILESIZE * 1.0f, trapType));
}

unsigned int map::numThings (float thingsPer)
{
	unsigned int num = (unsigned int) thingsPer;
	float rem = thingsPer - num;
	if (rand() % 100 < (int) (100 * rem))
		num += 1;
	return num;
}

bool map::generateTrap(std::vector<std::vector<int>> *platforms, std::vector<std::vector<unsigned int>> *trapsP, unsigned int list, float thingsPer)
{
	for (unsigned int q = 0; q < GENERATOR_MAXTRIES; q++)
	{
		unsigned int plat = rand() % (platforms->size() - 1) + 1;
		if ((*trapsP)[plat].size() == 0)
		{
			unsigned int numTraps = numThings(thingsPer);
			(*trapsP)[plat].push_back(numTraps);
			for (unsigned int i = 0; i < numTraps; i++)
			{
				unsigned int pick = rand() % global_data->getEntrySize(DATA_LIST, list);

				(*trapsP)[plat].push_back(global_data->getValue(DATA_LIST, list, pick));
			}
			return true;
		}
	}
	return false; //failed to generate
}

bool map::generateDecoration(std::vector<std::vector<int>> *platforms, std::vector<unsigned int> *decorationsP, unsigned int list, bool forge)
{
	for (unsigned int q = 0; q < GENERATOR_MAXTRIES; q++)
	{
		unsigned int plat = rand() % (platforms->size() - 1) + 1;
		if ((*decorationsP)[plat] == DATA_NONE)
		{
			unsigned int pick;
			if (forge)
				pick = 0;
			else
				pick = 1 + rand() % (global_data->getEntrySize(DATA_LIST, list) - 1);
			(*decorationsP)[plat] = global_data->getValue(DATA_LIST, list, pick);
			return true;
		}
	}
	return false;
}

bool map::generateEnemy(std::vector<std::vector<int>> *platforms, std::vector<std::vector<unsigned int>> *enemiesP, unsigned int list, unsigned int bossN, float thingsPer)
{
	for (unsigned int q = 0; q < GENERATOR_MAXTRIES; q++)
	{
		unsigned int plat = rand() % (platforms->size() - 1) + 1;
		if ((*enemiesP)[plat].size() == 0)
		{
			//how many encounters?
			unsigned int numEncounters = numThings(thingsPer);

			//place the encounters
			(*enemiesP)[plat].push_back(numEncounters);
			for (unsigned int i = 0; i < numEncounters; i++)
			{
				//pick a monster to place
				unsigned int pick = rand() % global_data->getEntrySize(DATA_LIST, list);
			
				//place it
				(*enemiesP)[plat].push_back(global_data->getValue(DATA_LIST, list, pick));
				(*enemiesP)[plat].push_back(bossN);
			}
			return true;
		}
	}
	return false; //failure
}

void map::mapRetrieve(sf::Packet *loadFrom)
{
	sf::Packet mapPack;
	if (loadFrom == NULL)
	{
		//wait for a map packet
		socket->receive(mapPack);
		loadFrom = &mapPack;
	}

	if (creatures.size() > 0)
	{
		//this is loading a new map!
		mapClear(false); //so clear the old one from memory
	}
	
	//retrieve dimensions
	float uWidth, uHeight, uTileset, uMapNum, uType;
	(*loadFrom) >> uWidth >> uHeight >> uTileset >> faction >> uMapNum >> uType;
	width = (unsigned int) uWidth;
	height = (unsigned int) uHeight;
	tileset = (unsigned int) uTileset;
	mapNum = (unsigned int) uMapNum;
	type = (unsigned int) uType;

	//retrieve tiles
	tiles = new unsigned int[width * height];
	for (unsigned int i = 0; i < width * height; i++)
	{
		float uTile;
		(*loadFrom) >> uTile;
		tiles[i] = (unsigned int) uTile;
	}

	//retrieve doors
	float numDoors;
	(*loadFrom) >> numDoors;
	for (unsigned int i = 0; i < (unsigned int) numDoors; i++)
		doors.push_back(new door(loadFrom));

	//retrieve decorations
	float numDecorations;
	(*loadFrom) >> numDecorations;
	for (unsigned int i = 0; i < (unsigned int) numDecorations; i++)
		decorations.push_back(new decoration(loadFrom));

	//retrieve traps
	float uNumTraps;
	(*loadFrom) >> uNumTraps;
	for (unsigned int i = 0; i < (unsigned int) uNumTraps; i++)
		traps.push_back(new trap(loadFrom));

	//retrieve items
	float uNumItems;
	(*loadFrom) >> uNumItems;
	for (unsigned int i = 0; i < (unsigned int) uNumItems; i++)
	{
		float x, y;
		(*loadFrom) >> x >> y;
		item *it = new item(loadFrom);
		it->drop(x, y);
		items.push_back(it);
	}

	//retrieve creatures
	float uNumCreatures;
	(*loadFrom) >> uNumCreatures;
	for (unsigned int i = 0; i < (unsigned int) uNumCreatures; i++)
	{
		//retrieve typing data
		float uType;
		(*loadFrom) >> uType;
		unsigned int type = (unsigned int) uType;

		if (type == TYPE_PLAYER && i != 0)
			addCreature(new player(loadFrom, type));
		else
			addCreature(new slave(loadFrom, type));
	}

	//initialize camera
	global_camera->x = -1;
	global_camera->y = -1;

	//play an appropriate song
	playSong(tileset);
}

void map::mapSend(sf::Packet *sendTo)
{
	bool sendHere;
	sf::Packet mapPack;

	if (sendTo == NULL)
	{
		//make a map packet
		sendTo = &mapPack;
		sendHere = true;
	}
	else
		sendHere = false;

	//store dimensions
	(*sendTo) << (float) width << (float) height << (float) tileset << faction << (float) mapNum << (float) type;

	//store tiles
	for (unsigned int i = 0; i < width * height; i++)
		(*sendTo) << (float) tiles[i];

	//store doors
	(*sendTo) << (float) doors.size();
	for (unsigned int i = 0; i < doors.size(); i++)
		doors[i]->save(sendTo);

	//store decorations
	(*sendTo) << (float) decorations.size();
	for (unsigned int i = 0; i < decorations.size(); i++)
		decorations[i]->save(sendTo);

	//store traps
	(*sendTo) << (float) traps.size();
	for (unsigned int i = 0; i < traps.size(); i++)
		traps[i]->save(sendTo);

	//store items
	(*sendTo) << (float) items.size();
	for (unsigned int i = 0; i < items.size(); i++)
	{
		(*sendTo) << items[i]->getX() << items[i]->getY();
		items[i]->save(sendTo);
	}

	//store creatures
	(*sendTo) << (float) creatures.size();
	for (unsigned int i = 0; i < creatures.size(); i++)
	{
		//package typing data
		(*sendTo) << (float) creatures[i]->getType();

		//do a full save
		creatures[i]->save(sendTo);
	}

	if (sendHere)
	{
		//send the packet
		socket->send(mapPack);
	}
}

void door::save(sf::Packet *saveTo)
{
	(*saveTo) << (float) tileset << faction << (float) x << (float) y << (float) mapMod << (float) type << (float) num;
}

door::door(sf::Packet *loadFrom)
{
	float uTileset, uX, uY, uMapMod, uType, uNum;
	(*loadFrom) >> uTileset >> faction >> uX >> uY >> uMapMod >> uType >> uNum;
	tileset = (unsigned int) uTileset;
	x = (unsigned int) uX;
	y = (unsigned int) uY;
	mapMod = (unsigned int) uMapMod;
	type = (unsigned int) uType;
	num = (unsigned int) uNum;
}

door::door(unsigned int oldNum, unsigned int oldType, unsigned int x, unsigned int y) : x(x), y(y)
{
	if (oldType == 0 && oldNum % GENERATOR_TILESETINTERVAL == GENERATOR_TILESETINTERVAL - 1)
	{
		//it'll be a boss level
		type = 1;
		num = oldNum;
		//have to set faction later, since for boss levels it's based on the tileset
	}
	else if (oldType == 0 && (oldNum % GENERATOR_TILESETINTERVAL == 1 || oldNum % GENERATOR_TILESETINTERVAL == 5))
	{
		//it'll be an NPC level
		type = 2;
		num = oldNum;
		faction = false; //towns are ALWAYS first king faction
	}
	else
	{
		//normal level
		type = 0;
		num = oldNum + 1;
		faction = rand() % 2 == 1; //faction is random
	}

	tileset = num / GENERATOR_TILESETINTERVAL;
	if (type == 1) //set the faction based on the nature of the boss encounter
		faction = global_data->getValue(DATA_LIST, 3, 3 * tileset) == 1;
	else if (tileset == 3) //the lava level is ALWAYS first king
		faction = false;
		

	if (type == 0 && rand() % 100 < GENERATOR_MAPMODCHANCE)
		mapMod = rand() % global_data->getCategorySize(DATA_MAPMOD);
	else
		mapMod = DATA_NONE;
}

void door::render (drawer *draw)
{
	float xS = x * TILESIZE - global_camera->x;
	float yS = y * TILESIZE - global_camera->y;
	draw->renderSprite(0, global_data->getValue(DATA_DOOR, tileset, 0), xS, yS, false, 
						makeColor(global_data->getValue(DATA_DOOR, tileset, 1)));
	unsigned int fA = 0;
	if (faction)
		fA = 1;
	unsigned int greenery = global_data->getValue(DATA_DOOR, tileset, 2 + fA);
	if (greenery != DATA_NONE)
		draw->renderSprite(0, greenery, xS, yS, false, makeColor(global_data->getValue(DATA_TILESET, tileset, 2 + fA)));
	if (mapMod != DATA_NONE)
		draw->renderSprite(0, global_data->getValue(DATA_MAPMOD, mapMod, 0), xS, yS, false,
							makeColor(global_data->getValue(DATA_MAPMOD, mapMod, 1)));
}

void decoration::save (sf::Packet *saveTo)
{
	(*saveTo) << (float) x << (float) y << (float) id;
}
decoration::decoration (sf::Packet *loadFrom) : timer(0)
{
	float uX, uY, uID;
	(*loadFrom) >> uX >> uY >> uID;
	x = (unsigned int) uX;
	y = (unsigned int) uY;
	id = (unsigned int) uID;
}
void decoration::update (float elapsed)
{
	unsigned int particle = global_data->getValue(DATA_DECORATION, id, 6);
	if (particle != DATA_NONE && onScreen())
	{
		float speed = global_data->getValue(DATA_DECORATION, id, 9) * 0.01f;
		timer += speed * elapsed;
		if (timer >= 1)
		{
			timer -= 1;

			//make the particle
			global_map->addParticles(particle, 1, 1.0f * x * TILESIZE + global_data->getValue(DATA_DECORATION, id, 7),
													1.0f * y * TILESIZE + global_data->getValue(DATA_DECORATION, id, 8), 2);
		}
	}
}
decoration::decoration(unsigned int _x, unsigned int y, unsigned int id) : y(y), id(id), timer(0)
{
	unsigned int sheet = global_data->getValue(DATA_DECORATION, id, 0);
	unsigned int width = global_data->getValue(DATA_SHEET, sheet, 1);
	x = _x;
	if (width < TILESIZE * GENERATOR_TOWNCHUNKWIDTH && rand() % 2 == 1)
		x += 1; //to make it a bit random
}
bool decoration::isForge()
{
	return global_data->getValue(DATA_DECORATION, id, 10) == 1;
}
bool decoration::onScreen()
{
	unsigned int sheet = global_data->getValue(DATA_DECORATION, id, 0);
	unsigned int width = global_data->getValue(DATA_SHEET, sheet, 1);
	unsigned int height = global_data->getValue(DATA_SHEET, sheet, 2);
	return x * TILESIZE + width >= global_camera->x && y * TILESIZE + height >= global_camera->y &&
		x * TILESIZE <= global_camera->x + SCREENWIDTH && y * TILESIZE <= global_camera->y + SCREENHEIGHT;
}
void decoration::render (drawer *draw)
{
	if (onScreen())
	{
		draw->renderSprite(global_data->getValue(DATA_DECORATION, id, 0), global_data->getValue(DATA_DECORATION, id, 1), x * TILESIZE - global_camera->x, y * TILESIZE - global_camera->y, false,
				makeColor(global_data->getValue(DATA_DECORATION, id, 2)));

		unsigned int glow = global_data->getValue(DATA_DECORATION, id, 3);
		if (glow != DATA_NONE)
			draw->renderSprite(global_data->getValue(DATA_LIST, 0, 2), 0,
								x * TILESIZE + global_data->getValue(DATA_DECORATION, id, 4) - global_camera->x,
								y * TILESIZE + global_data->getValue(DATA_DECORATION, id, 5) - global_camera->y, false, makeColor(glow));
	}
}