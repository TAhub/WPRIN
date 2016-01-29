#include "stdafx.h"

extern database *global_data;
extern map *global_map;
extern sf::Vector2f *global_camera;
extern bool *global_hasFocus;
extern soundPlayer *global_sound;

player::player(float x, float y) : creature(x, y, TYPE_PLAYER, 0, false)
{
	playerInit();
}

player::player(std::vector<unsigned int> *data) : creature(data)
{
	playerInit();
}

player::player(sf::Packet *loadFrom, unsigned short type) : creature(loadFrom, type)
{
	playerInit();
}

void player::playerInit()
{
	lastJump = false;
	lastAttack = false;
	lastArrow = false;
	lastSwitch = false;
	lastDrop = false;
	lastMagic = false;
	lastI = false;
	lastK = false;
	itemsD = 0;
	levelD = 0;
	craftingD = 0;
	interfaceTimer = 0;
	reviveTimer = -1;
	line = DATA_NONE;
}

item *player::buyableItemOn()
{
	unsigned int nItems = global_map->getNumItems();
	for (unsigned int i = 0; i < nItems; i++)
	{
		item *it = global_map->getItem(i);
		if (it->getCost() != 0 &&
			abs(x + getMaskWidth() / 2 - it->getX()) < ITEM_SIZE + getMaskWidth() / 2 &&
			abs(y + getMaskHeight() / 2 - it->getY()) < getMaskHeight() / 2)
			return it;
	}
	return NULL;
}

bool player::pickUp()
{
	unsigned int nItems = global_map->getNumItems();
	for (unsigned int i = 0; i < nItems; i++)
	{
		item *it = global_map->getItem(i);
		if (it->getCost() <= money && //you have to be able to pay for it
			abs(x + getMaskWidth() / 2 - it->getX()) < ITEM_SIZE + getMaskWidth() / 2 &&
			abs(y + getMaskHeight() / 2 - it->getY()) < getMaskHeight() / 2 &&
			!it->removed && !it->merged)
		{
			if (it->ammo() != 0 && ammo < maxAmmo)
			{
				//you can carry it as ammo
				global_map->pickupItem(i);
				return true;
			}
			else if (it->ammo() == 0)
				for (unsigned int j = 0; j < INVENTORYSIZE; j++)
					if (inventory[j] == NULL || it->canMerge(inventory[j]))
					{
						//there's an item you can pick up
						//attempt to pick it up!
						global_map->pickupItem(i);
						return true;
					}
		}
	}

	return false;
}

void player::update(float elapsed)
{
	/**
	//debug fly mode
	flying = true;
	creature::update(elapsed);
	elapsed *= 2;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		flyMove(0, -1, elapsed);
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		flyMove(0, 1, elapsed);
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		flyMove(-1, 0, elapsed);
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		flyMove(1, 0, elapsed);
	return;
	/**/

	if (global_map->isTown())
	{
		if (healthFraction() < 1)
			heal(100);
		food = 1;
		mana = 1;
	}

	if (line != DATA_NONE)
	{
		lineTimer -= elapsed;
		if (lineTimer <= 0)
			line = DATA_NONE;
	}

	//keep track of position, for animation purposes
	if (!isParalyzed())
		lastX = x;

	if (dead() || global_map->ended() || isParalyzed())
	{
		//you're dead, so you can't do stuff
		creature::update(elapsed);
		return;
	}

	//do revive timer stuff
	creature *toRevive = NULL;
	unsigned int numCreatures = global_map->getNumCreatures();
	for (unsigned int i = 0; i < numCreatures; i++)
	{
		toRevive = global_map->getCreature(i);
		if (toRevive->getType() == TYPE_PLAYER && toRevive->dead() &&
			abs(getX() + getMaskWidth() / 2 - toRevive->getX() - toRevive->getMaskWidth() / 2) < getMaskWidth() / 2 &&
			abs(getY() - toRevive->getY()) < getMaskHeight() / 2)
			break;
		else
		{
			toRevive = NULL;
			if (global_map->getCreature(i)->getType() != TYPE_PLAYER)
				break;
		}
	}
	if (toRevive != NULL)
	{
		if (reviveTimer == -1)
			reviveTimer = REVIVELENGTH;
		reviveTimer -= elapsed;
		if (reviveTimer <= 0)
		{
			reviveTimer = -1; //reset the timer
			global_map->magic(toRevive, global_data->getValue(DATA_LIST, 0, 5), TYPE_PLAYER);
		}
	}
	else
		reviveTimer = -1;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::L) && perkPoints > 0 && levelD == 0 && craftingD == 0 && itemsD == 0)
	{
		levelD = 1;
		getValidPerks();
		interfaceY = 0;
	}

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::I))
	{
		if (!lastI)
		{
			if (craftingD == 0 && levelD == 0)
			{
				if (itemsD == 0)
				{
					itemsD = 1;
					interfaceX = 0;
					interfaceY = 0;
				}
				else if (itemsD == 1)
					itemsD = -1;
			}
			lastI = true;
		}
	}
	else
		lastI = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::K))
	{
		if (!lastK)
		{
			if (itemsD == 0 && levelD == 0)
			{
				if (craftingD == 0)
				{
					generateApplicableCrafts();
					if (interfaceData.size() > 0)
					{
						craftingD = 1;
						interfaceY = 0;
					}
				}
				else if (craftingD == 1)
					craftingD = -1;
			}
			lastK = true;
		}
	}
	else
		lastK = false;

	//interface switches
	if (levelD == 1)
		levelMode (elapsed);
	else if (craftingD == 1)
		craftingMode (elapsed);
	else if (itemsD == 1)
		itemsMode (elapsed);
	if (itemsD == 1 || craftingD == 1 || levelD == 1)
	{
		interfaceTimer += INTERFACE_SWOOPSPEED * elapsed;
		if (interfaceTimer > 1)
			interfaceTimer = 1;
	}
	else if (itemsD == -1 || craftingD == -1 || levelD == -1)
	{
		interfaceTimer -= INTERFACE_SWOOPSPEED * elapsed;
		if (interfaceTimer < 0)
		{
			interfaceTimer = 0;
			itemsD = 0;
			craftingD = 0;
			levelD = 0;
		}
	}
	else
		moveMode (elapsed);

	creature::update(elapsed);
}

void player::levelMode (float elapsed)
{
	int yA = 0;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		yA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		yA += 1;
	if (yA != 0)
	{
		if (!lastArrow)
		{
			lastArrow = true;
			if (yA == -1 && interfaceY == 0)
				interfaceY = interfaceData.size() - 1;
			else if (yA == 1 && interfaceY == interfaceData.size() - 1)
				interfaceY = 0;
			else
				interfaceY += yA;

			global_sound->play(4);
		}
	}
	else
		lastArrow = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
	{
		if (!lastAttack)
		{
			global_sound->play(5);

			lastAttack = true;

			//apply that perk!
			unsigned int perk = interfaceData[interfaceY];
			perkApply(perk);
			unsigned int perkGive = global_data->getValue(DATA_PERK, perk, 5);
			if (perkGive != DATA_NONE)
			{
				//do you already have a magic item?
				if (magicItem == DATA_NONE)
					magicItem = perkGive;
				else
				{
					//drop an item of that type
					item *it = new item(3, perkGive, 1, 0, 0, 0, global_map->getIdentifier());
					global_map->dropItem(it, getX() + getMaskWidth() / 2, getY() + getMaskHeight() / 2);
				}
			}


			perkPoints -= 1;

			if (perkPoints == 0)
			{
				//return to the normal menu
				levelD = -1;
				
				//tell the map that you have updated
				global_map->playerUpdate();
			}
			else
			{
				//get another perk
				getValidPerks();
			}
		}
	}
	else
		lastAttack = false;
}

void player::getValidPerks ()
{
	interfaceData.clear();
	
	while (interfaceData.size() < INTERFACE_NUMPERKS)
	{
		//pick one at random
		unsigned int pick = rand() % global_data->getCategorySize(DATA_PERK);

		//do you qualify?
		unsigned int perkGive = global_data->getValue(DATA_PERK, pick, 5);
		if (global_map->getTileset() >= (unsigned int) global_data->getValue(DATA_PERK, pick, 7) //have you progressed far enough in the maps?
			&& perkValid(pick) && (perkGive == DATA_NONE || magicItem != perkGive))
		{
			// do you already have that perk in the interface data?
			bool has = false;
			for (unsigned int i = 0; !has && i < interfaceData.size(); i++)
				if (interfaceData[i] == pick)
					has = true;

			if (!has)
				interfaceData.push_back(pick); //it's valid!
		}
	}
}

void player::craftingMode (float elapsed)
{
	int yA = 0;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		yA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		yA += 1;

	if (yA != 0)
	{
		if (!lastArrow)
		{
			lastArrow = true;

			if (interfaceY == 0 && yA == -1)
				interfaceY = interfaceData.size() / 2 - 1;
			else if (interfaceY == interfaceData.size() / 2 - 1 && yA == 1)
				interfaceY = 0;
			else
				interfaceY += yA;
			global_sound->play(4);
		}
	}
	else
		lastArrow = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
	{
		if (!lastAttack)
		{
			//consume the ingredients
			for (unsigned int x = 0; x < 3; x++)
			{
				item it = getCraftingExample(x, interfaceY);
				if (it.getID() != DATA_NONE)
					for (unsigned int i = 0; i < INVENTORYSIZE; i++)
						if (inventory[i] != NULL && inventory[i]->getID() == it.getID() &&
							inventory[i]->getMaterial() == it.getMaterial() &&
							inventory[i]->getType() == it.getType())
						{
							unsigned int n = inventory[i]->getNumber();
							inventory[i]->useSome(it.getNumber());
							it.useSome(n);
							if (inventory[i]->getNumber() == 0)
							{
								//empty that slot
								delete inventory[i];
								inventory[i] = NULL;
							}
							if (it.getNumber() == 0)
								break; //done!
						}
			}

			global_sound->play(5);

			//tell the server that your inventory just majorly changed
			global_map->playerUpdate();

			//make the result
			item resultSample = getCraftingExample(3, interfaceY);
			item *result = new item(resultSample.getType(), resultSample.getID(), resultSample.getNumber(),
									resultSample.getMaterial(), 0, 0, global_map->getIdentifier());
			global_map->dropItem(result, getX() + getMaskWidth() / 2, getY() + getMaskHeight() / 2);

			//reset the recipies list
			lastAttack = true;
			interfaceY = 0;
			generateApplicableCrafts();
			if (interfaceData.size() == 0)
			{
				//no more crafting is possible
				craftingD = -1;
				return;
			}
		}
	}
	else
		lastAttack = false;
}

void player::itemsMode (float elapsed)
{
	int xA = 0;
	int yA = 0;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		yA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		yA += 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		xA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		xA += 1;

	if (xA != 0 || yA != 0)
	{
		if (!lastArrow)
		{
			if (xA == -1 && interfaceX == 0)
				interfaceX = 3;
			else if (xA == 1 && interfaceX == 3)
				interfaceX = 0;
			else
				interfaceX += xA;
			if (yA == -1 && interfaceY == 0)
				interfaceY = 3;
			else if (yA == 1 && interfaceY == 3)
				interfaceY = 0;
			else
				interfaceY += yA;
			lastArrow = true;

			global_sound->play(4);
		}
	}
	else
		lastArrow = false;

	//calculate the interfaceI now that you might have moved the arrow
	unsigned int interfaceI = interfaceX + interfaceY * 4;

	bool shift = *global_hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift));

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shift)
	{
		if (!lastDrop)
		{
			lastDrop = true;

			if (inventory[interfaceI] != NULL)
			{
				//drop the item, empty that slot, and notify an update
				if (onMerchant())
				{
					global_map->forceSound(6);
					money += (unsigned int) (inventory[interfaceI]->returnSaleValue());
					delete inventory[interfaceI]; //it turns into nothing
				}
				else
					global_map->dropItem(inventory[interfaceI], getX() + getMaskWidth() / 2, getY() + getMaskHeight() / 2);
				inventory[interfaceI] = NULL;
				global_map->playerUpdate();
			}
		}
	}
	else
		lastDrop = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !shift)
	{
		if (!lastAttack)
		{
			lastAttack = true;

			item *it = inventory[interfaceI];
			if (it != NULL)
			{
				switch(it->getType())
				{
				case 0:
					{
						bool used = false;
						if ((it->heals() != 0 && healthFraction() != 1.0f))
						{
							heal(it->heals());
							used = true;
						}
						if (it->feeds() != 0 && food < 1)
						{
							global_map->forceSound(3);
							food += it->feeds() * 0.01f;
							if (food > 1)
								food = 1;
							used = true;
						}
						if (it->effect() != DATA_NONE)
						{
							applyEffect(it->effect());
							used = true;
						}

						if (used)
						{
							it->useSome(1);
							if (it->getNumber() == 0)
							{
								inventory[interfaceI] = NULL;
								delete it;
							}
							global_map->playerUpdate();
						}
					}
					break;
				case 1:
					{
						//switch armor
						//make a replacement item
						item *oldArmor = new item(1, armor, 1, armorMaterial, 0, 0, global_map->getIdentifier());
						armor = it->getID();
						armorMaterial = it->getMaterial();
						delete it;
						inventory[interfaceI] = oldArmor;

						global_map->playerUpdate();
					}
					break;
				case 2:
					//switch weapon
					{
						item *oldWeapon = NULL;
						if (weapon2 != DATA_NONE)
						{
							oldWeapon = new item(2, weapon, 1, weaponMaterial, 0, 0, global_map->getIdentifier());
							weapon = it->getID();
							weaponMaterial = it->getMaterial();

							//also interrupt attack
							switchWeapon();
							switchWeapon();
						}
						else
						{
							weapon2 = it->getID();
							weaponMaterial2 = it->getMaterial();
						}
						delete it;
						inventory[interfaceI] = oldWeapon;

						global_map->playerUpdate();
					}
					break;
				case 3:
					//switch magic item
					{
						item *oldMagicItem = NULL;
						if (magicItem != DATA_NONE)
							oldMagicItem = new item(3, magicItem, 1, 0, 0, 0, global_map->getIdentifier());
						magicItem = it->getID();
						delete it;
						inventory[interfaceI] = oldMagicItem;

						global_map->playerUpdate();
					}
					break;
				}
			}
		}
	}
	else
		lastAttack = false;
}

void player::forceDialogue(unsigned int id)
{
	line = id;
	lineTimer = TALKLENGTHBASE + global_data->getLine(line).length() * TALKLENGTHRAMP;
}

bool player::onMerchant()
{
	unsigned int numCreatures = global_map->getNumCreatures();
	for (unsigned int i = 0; i < numCreatures; i++)
	{
		creature *cr = global_map->getCreature(i);
		if (cr->getType() == TYPE_ALLY && cr->isMerchant())
		{
			float xD = cr->getX() + cr->getMaskWidth() / 2 - getX() - getMaskWidth() / 2;
			float yD = cr->getY() + cr->getMaskHeight() / 2 - getY() - getMaskHeight() / 2;
			if (sqrt(xD * xD + yD * yD) < TALKDISTANCE)
				return true;
		}
	}
	return false;
}

void player::talk()
{
	unsigned int numCreatures = global_map->getNumCreatures();
	creature *closest = NULL;
	float closestD = DATA_NONE;
	for (unsigned int i = 0; i < numCreatures; i++)
	{
		creature *cr = global_map->getCreature(i);
		if (cr->getType() == TYPE_ALLY)
		{
			float xD = cr->getX() + cr->getMaskWidth() / 2 - getX() - getMaskWidth() / 2;
			float yD = cr->getY() + cr->getMaskHeight() / 2 - getY() - getMaskHeight() / 2;
			float d = sqrt(xD * xD + yD * yD);
			if (closest == NULL || closestD > d)
			{
				closest = cr;
				closestD = d;
			}
		}
	}

	if (closest != NULL && closestD < TALKDISTANCE)
		forceDialogue(closest->getLine());
}

void player::moveMode (float elapsed)
{
	bool canAttack = !global_map->isTown() && line == DATA_NONE;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::W))
	{
		if (!lastJump)
		{
			lastJump = true;
			jump();
		}
	}
	else
		lastJump = false;

	dropMode = *global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::S);


	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::E))
	{
		if (!lastSwitch)
		{
			lastSwitch = true;
			switchWeapon();
		}
	}
	else
		lastSwitch = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
	{
		if (!lastMagic && canAttack)
		{
			lastMagic = true;
			useMagic();
		}
	}
	else
		lastMagic = false;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
	{
		if (!lastAttack || canHoldAttack())
		{
			lastAttack = true;
			if (!global_map->mapEnd(x, y) && !pickUp())
			{
				if (canAttack)
				{
					if (hasAmmo())
						startAttack();
				}
				else
					talk();
			}
		}
	}
	else
		lastAttack = false;

	if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		endAttack(); //this is just for bows, basically

	int xA = 0;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		xA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		xA += 1;
	if (xA != 0)
		move(xA, elapsed);
}

void player::renderInfoTip (drawer *draw, std::string info)
{
	renderText(draw, info, INTERFACE_ACTIVECOLOR, x - global_camera->x, y - INTERFACE_TIPUP - global_camera->y, true, INTERFACE_TIPW);
}

void player::render(drawer *draw, float xAt, float yAt)
{
	creature::render(draw, xAt, yAt);

	//draw interface bars
	renderBar(draw, INTERFACE_HEALTHX, SCREENHEIGHT - INTERFACE_HEALTHMINUSY, INTERFACE_HEALTHWIDTH, INTERFACE_HEALTHHEIGHT,
					healthFraction(), INTERFACE_HEALTHCOLOR, INTERFACE_HEALTHBACKCOLOR);
	renderBar(draw, INTERFACE_FOODX, SCREENHEIGHT - INTERFACE_FOODMINUSY, INTERFACE_FOODWIDTH, INTERFACE_FOODHEIGHT,
					food, INTERFACE_FOODCOLOR, INTERFACE_FOODBACKCOLOR);
	if (magicItem != DATA_NONE)
		renderBar(draw, INTERFACE_MANAX, SCREENHEIGHT - INTERFACE_MANAMINUSY, INTERFACE_MANAWIDTH, INTERFACE_MANAHEIGHT,
					mana, INTERFACE_MANACOLOR, INTERFACE_MANABACKCOLOR);
	sf::Color expC;
	float expF;
	if (perkPoints > 0)
	{
		expC = INTERFACE_EXPACTIVECOLOR;
		expF = 1.0f;
	}
	else
	{
		expC = INTERFACE_EXPCOLOR;
		expF = experience * 1.0f / toNextLevel();
	}
	renderBar(draw, INTERFACE_EXPX, SCREENHEIGHT - INTERFACE_EXPMINUSY, INTERFACE_EXPWIDTH, INTERFACE_EXPHEIGHT, expF, expC, INTERFACE_EXPBACKCOLOR);

	float ammX = INTERFACE_AMMOX;
	//ammo gauge
	if (hasRanged())
	{
		unsigned int ammoS = global_data->getValue(DATA_LIST, 0, 6);
		for (unsigned int i = 0; i < maxAmmo; i++)
		{
			unsigned int f;
			sf::Color c;
			if (i < soulArrow)
			{
				f = global_data->getValue(DATA_EFFECT, 2, 3);
				c = makeColor(global_data->getValue(DATA_EFFECT, 2, 4));
			}
			else
			{
				c = sf::Color::White;
				if (i < ammo)
					f = 0;
				else
					f = 1;
			}
			draw->renderSprite(ammoS, f, ammX, SCREENHEIGHT - INTERFACE_AMMOMINUSY, false, c);
			ammX += global_data->getValue(DATA_SHEET, ammoS, 1);
		}
	}

	if (magicItem != DATA_NONE)
		renderText(draw, global_data->getLine(global_data->getValue(DATA_MAGICITEM, magicItem, 4)), INTERFACE_ACTIVECOLOR, ammX + INTERFACE_MAGICITEMXADD, SCREENHEIGHT - INTERFACE_MAGICITEMMINUSY);
	std::string adName = "";
	if (attack() >= DATA_NONE)
		adName += "-";
	else
		adName += std::to_string((long long) attack());
	adName += "+ ";
	if (defense() >= DATA_NONE)
		adName += "-";
	else
		adName += std::to_string((long long) defense());
	adName += "^";
	renderText(draw, adName, INTERFACE_ACTIVECOLOR, INTERFACE_AMMOX + INTERFACE_MAGICITEMXADD, SCREENHEIGHT - INTERFACE_MAGICITEMMINUSY * 2);


	//draw special interfaces
	if (craftingD != 0)
	{
		for (unsigned int y = 0; y < interfaceData.size() / 2; y++)
			for (unsigned int x = 0; x < 4; x++)
				renderCraftingBit(x, y, draw);
	}
	else if (itemsD != 0)
	{
		float lY = DATA_NONE;
		float iY = 0;
		float iX = DATA_NONE;
		for (unsigned int y = 0; y < 4; y++)
			for (unsigned int x = 0; x < 4; x++)
			{
				sf::Vector2f pos = renderInventoryBit(x, y, draw);
				if (pos.y < lY)
					lY = pos.y;
				if (pos.y > iY)
					iY = pos.y;
				if (pos.x < iX)
					iX = pos.x;
			}

		//draw interface text
		item *it = inventory[interfaceX + interfaceY * 4];
		renderText(draw, std::to_string(static_cast<long long>(money)) + '$', INTERFACE_ACTIVECOLOR, SCREENWIDTH / 2, lY - INTERFACE_INVH, true);
		if (it != NULL && interfaceTimer == 1)
		{
			std::string nameN = it->getName();
			if (it->getNumber() > 1)
				nameN += "x" + std::to_string(static_cast<long long>(it->getNumber()));
			if (global_map->isTown() && onMerchant())
			{
				nameN += " (SELLS FOR " + std::to_string(static_cast<long long>((it->returnSaleValue()))) + "$";
				if (it->getNumber() > 1)
					nameN += " IN TOTAL)";
				else
					nameN += ")";
			}
			renderText(draw, nameN, INTERFACE_ACTIVECOLOR, iX, iY + INTERFACE_INVH, false, INTERFACE_INVW);
			renderText(draw, it->getDescription(), INTERFACE_ACTIVECOLOR, iX, iY + INTERFACE_INVH * 2, false, INTERFACE_INVW);
		}
	}
	else if (levelD != 0)
	{
		for (unsigned int y = 0; y < interfaceData.size(); y++)
			renderLevelBit(y, draw);
	}

	//draw lines
	if (line != DATA_NONE)
	{
		renderText(draw, global_data->getLine(line), INTERFACE_ACTIVECOLOR, INTERFACE_LINEBORDERX, SCREENHEIGHT - INTERFACE_LINEMINUSY, false, SCREENWIDTH - INTERFACE_LINEBORDERX * 2);
	}

	//draw tip
	bool iTip = false;
	if (interfaceTimer == 0)
	{
		if (global_map->isTown())
		{
			std::string infoTip;
		
			item *bIO = buyableItemOn();
			if (bIO != NULL)
			{
				infoTip = bIO->getName() + " COSTS " + std::to_string(static_cast<long long>(bIO->getCost())) + '$';
			}
			else if (onMerchant())
				infoTip = "DROP ITEMS TO SELL";

			if (infoTip.size() > 0)
			{
				infoTip += "^YOU HAVE " + std::to_string(static_cast<long long>(money)) + '$';
				renderInfoTip(draw, infoTip);
				iTip = true;
			}
		}
		if (!iTip && global_map->forgeAt(x, y))
		{
			renderInfoTip(draw, "WHILE AT A FORGE YOU CAN WORK WITH METAL");
			iTip = true;
		}
		if (!iTip && perkPoints > 0)
		{
			renderInfoTip(draw, "PRESS L TO PICK A PERK");
			iTip = true;
		}
	}
}

void player::renderLevelBit (unsigned int y, drawer *draw)
{
	unsigned int itemSheet = global_data->getValue(DATA_LIST, 0, 3);
	float iYT = (float) (SCREENHEIGHT / 2 - ((INTERFACE_NUMPERKS / 2 - 0.5f) * global_data->getValue(DATA_SHEET, itemSheet, 2)));
	float iY = iYT + (y * global_data->getValue(DATA_SHEET, itemSheet, 2)) + (1 - interfaceTimer) * (SCREENHEIGHT - iYT);

	//get the text color
	sf::Color tC;
	if (y == interfaceY)
		tC = INTERFACE_ACTIVECOLOR;
	else
		tC = INTERFACE_INACTIVECOLOR;

	//get the sprite stats for the perk
	unsigned int frame = global_data->getValue(DATA_PERK, interfaceData[y], 0);
	sf::Color c = makeColor(global_data->getValue(DATA_PERK, interfaceData[y], 1));

	draw->renderSprite(itemSheet, frame, INTERFACE_PERKX, iY, false, c);
	renderText(draw, global_data->getLine(global_data->getValue(DATA_PERK, interfaceData[y], 6)),
				tC, INTERFACE_PERKX + global_data->getValue(DATA_SHEET, itemSheet, 2), iY);

	if (y == interfaceY)
	{
		iY = iYT + (5 * global_data->getValue(DATA_SHEET, itemSheet, 2)) + (1 - interfaceTimer) * (SCREENHEIGHT - iYT);
		renderText(draw, global_data->getLine(global_data->getValue(DATA_PERK, interfaceData[y], 6) + 1), tC,
			SCREENWIDTH * 0.5f, iY, true, (unsigned int) INTERFACE_PERKDESCRIPTIONWIDTH);
	}
}

sf::Vector2f player::renderInventoryBit (unsigned int x, unsigned int y, drawer *draw)
{
	unsigned int itemSheet = global_data->getValue(DATA_LIST, 0, 3);
	float width = (float) global_data->getValue(DATA_SHEET, itemSheet, 1);
	float iX = (float) (SCREENWIDTH / 2 - ((2.5 - x) * width));
	float iYT = (float) (SCREENHEIGHT / 2 - (1.5 * global_data->getValue(DATA_SHEET, itemSheet, 2))) - INTERFACE_INVH;
	float iY = iYT + (y * global_data->getValue(DATA_SHEET, itemSheet, 2));

	//make the floating up animation
	unsigned int cX = x;
	if (itemsD == -1)
		cX = 4 - x;
	float cT = (interfaceTimer - cX * 0.125f) / 0.5f;
	if (cT > 1)
		cT = 1;
	iY += (1 - cT) * (SCREENHEIGHT - iYT);

	//get the color
	sf::Color c;
	if (x == interfaceX && y == interfaceY)
		c = INTERFACE_ACTIVECOLOR;
	else
		c = INTERFACE_INACTIVECOLOR;

	draw->renderSprite(itemSheet, 0, iX, iY, false, c);

	//and now draw the item
	item *it = inventory[x + y * 4];
	if (it != NULL)
	{
		it->drop(iX + global_camera->x, iY + global_camera->y);
		it->render(draw);

		//draw the quantity
		if (it->getNumber() > 1)
			renderNumber(draw, it->getNumber(), c, iX, iY);
	}

	return sf::Vector2f(iX - width / 2, iY);
}
void player::renderCraftingBit (unsigned int x, unsigned int y, drawer *draw)
{
	unsigned int itemSheet = global_data->getValue(DATA_LIST, 0, 3);
	unsigned int xA = x;
	if (x == 3)
		xA += 1;
	float iX = (float) (SCREENWIDTH / 2 - ((2.5 - xA) * global_data->getValue(DATA_SHEET, itemSheet, 1)));
	unsigned int efH = SCREENHEIGHT / global_data->getValue(DATA_SHEET, itemSheet, 2);
	int efY = y;
	if (efH <= interfaceData.size() / 2)
	{
		int yStart = interfaceY - efH / 2;
		if (yStart < 0)
			yStart = 0;
		else if (yStart + efH >= interfaceData.size() / 2)
			yStart = interfaceData.size() / 2 - efH + 1;
		efY -= yStart;
	}

	float iYT = (float) (SCREENHEIGHT / 2 - ((efH / 2 - 0.5) * global_data->getValue(DATA_SHEET, itemSheet, 2)));
	float iY = iYT + (efY * global_data->getValue(DATA_SHEET, itemSheet, 2));

	//make the floating up animation
	iY += (1 - interfaceTimer) * (SCREENHEIGHT - iYT);

	if (iY < 0 || iY > SCREENHEIGHT)
		return; //don't bother rendering them if they are offscreen

	item it = getCraftingExample(x, y);

	if (it.getID() == DATA_NONE)
		return; //nothing

	//get the color
	sf::Color c;
	if (y == interfaceY)
		c = INTERFACE_ACTIVECOLOR;
	else
		c = INTERFACE_INACTIVECOLOR;

	draw->renderSprite(itemSheet, 0, iX, iY, false, c);

	//draw the item
	it.drop(iX + global_camera->x, iY + global_camera->y);
	it.render(draw);

	//draw the quantity
	if (it.getNumber() > 1)
		renderNumber(draw, it.getNumber(), c, iX, iY);

	if (y == interfaceY && x == 3)
	{
		renderText(draw, it.getName(), INTERFACE_ACTIVECOLOR, iX + global_data->getValue(DATA_SHEET, itemSheet, 1), iY);
	}
}

bool player::hasItemsForRecipie (unsigned int y)
{
	//get the total requirements
	std::vector<item> recReq;
	for (unsigned int x = 0; x < 3; x++)
	{
		item it = getCraftingExample (x, y);
		if (it.getID() != DATA_NONE)
		{
			for (unsigned int i = 0; i < recReq.size(); i++)
			{
				it.merge(&(recReq[i]));
				if (it.merged)
					break;
			}
			if (!it.merged)
				recReq.push_back(it);
		}
	}
	
	//see if those requirements are met
	for (unsigned int j = 0; j < recReq.size(); j++)
	{
		unsigned int numRequired = recReq[j].getNumber();
		for (unsigned int i = 0; i < INVENTORYSIZE; i++)
		{
			if (inventory[i] != NULL &&
				inventory[i]->getID() == recReq[j].getID() &&
				inventory[i]->getType() == recReq[j].getType() &&
				inventory[i]->getMaterial() == recReq[j].getMaterial())
			{
				if (inventory[i]->getNumber() < numRequired)
					numRequired -= inventory[i]->getNumber();
				else
					numRequired = 0;
				if (numRequired == 0)
					break;
			}
		}

		if (numRequired > 0)
			return false;
	}
	return true;
}

void player::generateApplicableCrafts()
{
	//generate an initial list of everything
	interfaceData.clear();
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_RECIPIE); i++)
	{
		unsigned int iterationsSize = 1;
		if (global_data->getValue(DATA_RECIPIE, i, 7) != DATA_NONE)
			iterationsSize = global_data->getEntrySize(DATA_LIST, global_data->getValue(DATA_RECIPIE, i, 7));
		
		if (global_data->getValue(DATA_RECIPIE, i, 9 + getRace()) == 0)
			iterationsSize = 0; //your race cannot craft this

		if (global_data->getValue(DATA_RECIPIE, i, 12) == 1 && !global_map->forgeAt(getX() + getMaskWidth() / 2, getY() + getMaskHeight() / 2))
			iterationsSize = 0; //you need a forge to craft that

		for (unsigned int j = 0; j < iterationsSize; j++)
		{
			interfaceData.push_back(i);
			interfaceData.push_back(j);
		}
	}

	//filter it
	std::vector<unsigned int> newID;
	for (unsigned int i = 0; i < interfaceData.size() / 2; i++)
		if (hasItemsForRecipie(i))
		{
			newID.push_back(interfaceData[i * 2]);
			newID.push_back(interfaceData[i * 2 + 1]);
		}

	interfaceData = newID;
}

item player::getCraftingExample (unsigned int x, unsigned int y)
{
	unsigned int recipie = interfaceData[y * 2];
	unsigned int iteration = interfaceData[y * 2 + 1];
	unsigned int itemID;
	unsigned int itemMaterialList;
	unsigned int itemMaterial;
	unsigned short itemType = 0;

	switch(x)
	{
	case 0:
		itemID = global_data->getValue(DATA_RECIPIE, recipie, 0);
		itemMaterialList = global_data->getValue(DATA_RECIPIE, recipie, 1);
		break;
	case 1:
		itemID = global_data->getValue(DATA_RECIPIE, recipie, 2);
		itemMaterialList = global_data->getValue(DATA_RECIPIE, recipie, 3);
		break;
	case 2:
		itemID = global_data->getValue(DATA_RECIPIE, recipie, 4);
		itemMaterialList = global_data->getValue(DATA_RECIPIE, recipie, 5);
		break;
	case 3:
		itemID = global_data->getValue(DATA_RECIPIE, recipie, 6);
		itemMaterialList = global_data->getValue(DATA_RECIPIE, recipie, 7);
		itemType = global_data->getValue(DATA_RECIPIE, recipie, 8);
		break;
	}

	if (itemMaterialList == DATA_NONE)
		itemMaterial = 0;
	else
		itemMaterial = global_data->getValue(DATA_LIST, itemMaterialList, iteration);

	//get the resultant item
	return item(itemType, itemID, 1, itemMaterial, 0, 0);
}

void player::renderBar (drawer *draw, float x, float y, float width, float height, float percent, sf::Color colorFront, sf::Color colorBack)
{
	draw->renderColor(x, y, width, height, sf::Color::Black);
	draw->renderColor(x + INTERFACE_BT, y + INTERFACE_BT, width - INTERFACE_BT * 2, height - INTERFACE_BT * 2, colorBack);
	draw->renderColor(x + INTERFACE_BT, y + INTERFACE_BT, (width - INTERFACE_BT * 2) * percent, height - INTERFACE_BT * 2, colorFront);
}