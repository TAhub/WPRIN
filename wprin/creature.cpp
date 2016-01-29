#include "stdafx.h"
extern map *global_map;
extern database *global_data;
extern sf::Vector2f *global_camera;

creature::~creature()
{
	for (unsigned int i = 0; i < inventory.size(); i++)
		if (inventory[i] != NULL)
			delete inventory[i];
}

creature::creature(std::vector<unsigned int> *data)
{
	type = TYPE_PLAYER;
	initInner(0, 0, (*data)[0], false);

	gender = (*data)[1] == 1;
	hat = global_data->getValue(DATA_LIST, 2, (*data)[2] * 2);

	unsigned int numParts = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM);
	unsigned int colorL = global_data->getValue(DATA_CTYPE, (*data)[0], 1);

	colors[0] = makeColor(global_data->getValue(DATA_COLORLIST, global_data->getValue(DATA_COLORLIST, colorL, 0), (*data)[3]));
	colors[1] = makeColor(global_data->getValue(DATA_COLORLIST, global_data->getValue(DATA_COLORLIST, colorL, 1), (*data)[3]));
	colors[2] = makeColor(global_data->getValue(DATA_COLORLIST, global_data->getValue(DATA_COLORLIST, colorL, 2), (*data)[5]));
	parts[numParts - 1] = global_data->getValue(DATA_LIST, global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM + 2 * numParts), (*data)[4]);
}

creature::creature(float _x, float _y, unsigned short _type, unsigned int cType, bool boss)
{
	type = _type;
	initInner(_x, _y, cType, boss);
}

void creature::silentDeath()
{
	for (unsigned int i = 0; i < inventory.size(); i++)
		delete inventory[i];
	inventory.clear();
	health = 0;
}

void creature::initInner(float _x, float _y, unsigned int cType, bool boss)
{
	race = global_data->getValue(DATA_CTYPE, cType, 0);
	miscInit();

	//appearance
	unsigned int numParts = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM);
	for (unsigned int i = 0; i < numParts; i++)
		parts.push_back(getPartFromList(global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM + 1 + numParts + i)));
	unsigned int colorList = global_data->getValue(DATA_CTYPE, cType, 1);
	for (unsigned int i = 0; i < global_data->getEntrySize(DATA_COLORLIST, colorList); i++)
		colors.push_back(makeColor(getPartFromList(global_data->getValue(DATA_COLORLIST, colorList, i), DATA_COLORLIST)));
	glow = global_data->getValue(DATA_CTYPE, cType, 16);
	gender = rand() % 2 == 1;

	//AI-inherited qualities
	if (type == TYPE_ENEMY)
	{
		unsigned int AI = global_data->getValue(DATA_CTYPE, cType, 13);
		knockbackImmune = global_data->getValue(DATA_AI, AI, 2) == 1;
		interruptImmune = global_data->getValue(DATA_AI, AI, 3) == 1;
		invincible = global_data->getValue(DATA_AI, AI, 8) == 1;
		unsigned int fixedG = global_data->getValue(DATA_AI, AI, 10);
		if (fixedG != DATA_NONE)
			gender = fixedG == 1;
		hurtSelf = global_data->getValue(DATA_AI, AI, 11) == 1;
	}
	else
	{
		knockbackImmune = false;
		interruptImmune = false;
		invincible = false;
		hurtSelf = false;
	}

	//stats
	level = global_data->getValue(DATA_CTYPE, cType, 15);
	baseMoveSpeed = global_data->getValue(DATA_CTYPE, cType, 5);
	maxHealth = global_data->getValue(DATA_CTYPE, cType, 2);
	unsigned int levelF = 1 + (level * 9 / 16);
	if (type == TYPE_PLAYER)
		levelF = 0;
	int baseAttackA = global_data->getValue(DATA_CTYPE, cType, 3) + levelF;
	int baseDefenseA = global_data->getValue(DATA_CTYPE, cType, 4) + levelF;
	if (baseAttackA <= 0)
		baseAttack = 0;
	else
		baseAttack = baseAttackA;
	if (baseDefenseA <= 0)
		baseDefense = 0;
	else
		baseDefense = baseDefenseA;

	if (boss && global_map->isMultiplayer())
	{
		//double your health, to account for it being multiplayer
		maxHealth *= 2;
	}

	if (type != TYPE_PLAYER) //slight speed randomization
		baseMoveSpeed = (unsigned int) (baseMoveSpeed * (1.0f - SPEEDRANDOMF + (rand() % 100) * 0.01f * SPEEDRANDOMF));

	//equipment
	weapon = global_data->getValue(DATA_CTYPE, cType, 6);
	weaponMaterial = global_data->getValue(DATA_CTYPE, cType, 7);
	weapon2 = global_data->getValue(DATA_CTYPE, cType, 8);
	weaponMaterial2 = global_data->getValue(DATA_CTYPE, cType, 9);
	armor = global_data->getValue(DATA_CTYPE, cType, 10);
	armorMaterial = global_data->getValue(DATA_CTYPE, cType, 11);
	hat = global_data->getValue(DATA_CTYPE, cType, 12);
	magicItem = DATA_NONE;

	if (type == TYPE_ALLY)
	{
		//get villager stuff
		unsigned int AI = global_data->getValue(DATA_CTYPE, cType, 13);
		unsigned int linesStart = global_data->getValue(DATA_AI, AI, 0);
		unsigned int linesEnd = global_data->getValue(DATA_AI, AI, 1);
		line = rand() % (linesEnd - linesStart + 1) + linesStart;
		unsigned int hatList = global_data->getValue(DATA_AI, AI, 2);
		if (hatList != DATA_NONE) //if it is none keep your current hat
			hat = getPartFromList(hatList);
		unsigned int armorMaterialList = global_data->getValue(DATA_AI, AI, 3);
		if (armorMaterialList != DATA_NONE) //if it is none keep your current material
			armorMaterial = getPartFromList(armorMaterialList);
		merchant = global_data->getValue(DATA_AI, AI, 5) == 1;
	}

	//resources
	health = maxHealth;

	//position
	teleport(_x, _y);

	//inventory
	if (type == TYPE_PLAYER)
	{
		//give them an empty inventory
		for (unsigned int i = 0; i < INVENTORYSIZE; i++)
			inventory.push_back(NULL);
	}
	else //give them a drop
	{
		if (level == 0 || boss || rand() % 100 < DROPS_CHANCE)
		{
			unsigned int dropList = global_data->getValue(DATA_CTYPE, cType, 14);
			if (dropList != DATA_NONE)
			{
				if ((boss || rand() % 100 < DROPS_RARECHANCE) && level > 0)
					dropList += 1; //it's a rare drop instead; level 0 creatures (wild animals, etc)
				unsigned int pick = rand() % (global_data->getEntrySize(DATA_LIST, dropList) / 3);
				inventory.push_back(new item(global_data->getValue(DATA_LIST, dropList, pick * 3),
										global_data->getValue(DATA_LIST, dropList, pick * 3 + 1),
										1, global_data->getValue(DATA_LIST, dropList, pick * 3 + 2), 0, 0, global_map->getIdentifier()));
			}
		}
	}

	//experience
	perkPoints = 0;
	if (type == TYPE_PLAYER || boss || level == 0) //players start with no EXP; bosses drop no EXP; level 0 enemies are not real challenges
		experience = 0;
	else
		experience = DREWARD_EXPERIENCEBASE + DREWARD_EXPERIENCERAMP * level;
}

void creature::summonEffect()
{
	summonEffectTimer = SUMMONEFFECTLENGTH;
}

void creature::miscInit()
{
	//position stuff
	facingX = 1;
	lastX = x;
	knockback = 0;

	//misc perk stats
	efficiency = 0;
	maxAmmo = BASEMAXAMMO;
	ammo = maxAmmo;

	//misc resources
	food = 1;
	mana = 1;
	money = 0;

	//effects
	invisibility = 0;
	slow = 0;
	soulArrow = 0;
	venemous = 0;
	paralyzed = 0;
	wolfPearl = false;

	//animation, etc
	walk = -1;
	rumble = -1;
	attackAnim = -1;
	dropMode = false;
	loaded = true;
	loaded2 = true;
	doubleJump = false;
	flying = false;
	chargeD = 0;
	ySpeed = 0;
	maskWidth = global_data->getValue(DATA_RACE, race, 0);
	maskHeight = global_data->getValue(DATA_RACE, race, 1);
	line = DATA_NONE;
	summonEffectTimer = 0;

	//randomly initialize animation bob
	bob = (std::rand() % 100) * 0.01f;
	if (bob > 0.5)
		bobD = -1;
	else
		bobD = 1;
}

creature::creature(sf::Packet *loadFrom, unsigned short type) : type(type)
{
	//appearance
	float uRace, uColorNum, uGlow;
	(*loadFrom) >> uRace >> uColorNum >> uGlow;
	race = (unsigned int) uRace;
	glow = (unsigned int) uGlow;
	for (unsigned int i = 0; i < (unsigned int) uColorNum; i++)
	{
		float uR, uG, uB;
		(*loadFrom) >> uR >> uG >> uB;
		colors.push_back(sf::Color((unsigned int) uR, (unsigned int) uG, (unsigned int) uB));
	}
	int numParts = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM);
	for (int i = 0; i < numParts; i++)
	{
		float uPart;
		(*loadFrom) >> uPart;
		parts.push_back((unsigned int) uPart);
	}
	(*loadFrom) >> gender;

	//stats
	float uMoveSpeed, uMaxHealth, uBaseAttack, uBaseDefense;
	(*loadFrom) >> uMoveSpeed >> uMaxHealth >> uBaseAttack >> uBaseDefense >> knockbackImmune >> interruptImmune >> hurtSelf;
	baseMoveSpeed = (unsigned int) uMoveSpeed;
	maxHealth = (unsigned int) uMaxHealth;
	baseAttack = (unsigned int) uBaseAttack;
	baseDefense = (unsigned int) uBaseDefense;

	//experience stuff
	float uExp, uLevel, uPerkPoints;
	(*loadFrom) >> uExp >> uLevel >> uPerkPoints;
	experience = (unsigned int) uExp;
	level = (unsigned int) uLevel;
	perkPoints = (unsigned int) uPerkPoints;

	//inventory
	float uInvSize;
	(*loadFrom) >> uInvSize;
	for (unsigned int i = 0; i < (unsigned int) uInvSize; i++)
	{
		bool valid;
		(*loadFrom) >> valid;
		if (valid)
			inventory.push_back(new item(loadFrom));
		else
			inventory.push_back(NULL);
	}

	//other equip
	float uArmor, uArmorMaterial, uWeapon, uWeapon2, uWeaponMaterial, uWeaponMaterial2, uHat, uMagicItem;
	(*loadFrom) >> uArmor >> uArmorMaterial >> uWeapon >> uWeaponMaterial >> uWeapon2 >> uWeaponMaterial2 >> uHat >> uMagicItem;
	armor = (unsigned int) uArmor;
	armorMaterial = (unsigned int) uArmorMaterial;
	weapon = (unsigned int) uWeapon;
	weaponMaterial = (unsigned int) uWeaponMaterial;
	weapon2 = (unsigned int) uWeapon2;
	weaponMaterial2 = (unsigned int) uWeaponMaterial2;
	hat = (unsigned int) uHat;
	magicItem = (unsigned int) uMagicItem;

	miscInit();

	if (type == TYPE_PLAYER)
	{
		//perk stuff
		float uEfficiency, uMaxAmmo, uAmmo, uSoulArrow, uVenemous, uMoney;
		(*loadFrom) >> uEfficiency >> uMaxAmmo >> uAmmo >> uSoulArrow >> uVenemous >> wolfPearl >> uMoney;
		efficiency = (unsigned int) uEfficiency;
		maxAmmo = (unsigned int) uMaxAmmo;
		ammo = (unsigned int) uAmmo;
		soulArrow = (unsigned int) uSoulArrow;
		venemous = (unsigned int) venemous;
		money = (unsigned int) uMoney;
	}
	else if (type == TYPE_ALLY)
	{
		//npc stuff
		float uLine;
		(*loadFrom) >> uLine >> merchant;
		line = (unsigned int) uLine;
	}

	//load everything that the normal packet handling does
	receivePacketInner(loadFrom, true);
}

void creature::save (sf::Packet *saveTo)
{
	//appearance
	(*saveTo) << (float) race << (float) colors.size() << (float) glow;
	for (unsigned int i = 0; i < colors.size(); i++)
		(*saveTo) << (float) colors[i].r << (float) colors[i].g << (float) colors[i].b;
	for (unsigned int i = 0; i < parts.size(); i++)
		(*saveTo) << (float) parts[i];
	(*saveTo) << gender;

	//stats
	(*saveTo) << (float) baseMoveSpeed << (float) maxHealth << (float) baseAttack << (float) baseDefense << knockbackImmune << interruptImmune << hurtSelf;

	//level stuff
	(*saveTo) << (float) experience << (float) level << (float) perkPoints;

	//inventory
	(*saveTo) << (float) inventory.size();
	for (unsigned int i = 0; i < inventory.size(); i++)
	{
		(*saveTo) << (inventory[i] != NULL);
		if (inventory[i] != NULL)
			inventory[i]->save(saveTo);
	}

	//other equip
	(*saveTo) << (float) armor << (float) armorMaterial << (float) weapon << (float) weaponMaterial << (float) weapon2 << (float) weaponMaterial2 << (float) hat << (float) magicItem;

	if (type == TYPE_PLAYER) //perk stuff
		(*saveTo) << (float) efficiency << (float) maxAmmo << (float) ammo << (float) soulArrow << (float) venemous << wolfPearl << (float) money;
	else if (type == TYPE_ALLY) //npc stuff
		(*saveTo) << (float) line << merchant;

	//save everything that the normal packet handling does
	creature::sendPacket(saveTo, true);
}

bool creature::perkValid(unsigned int perk)
{
	unsigned int max = global_data->getValue(DATA_PERK, perk, 4);
	switch (global_data->getValue(DATA_PERK, perk, 2))
	{
	case 0:
		return baseAttack < max;
	case 1:
		return baseDefense < max;
	case 2:
		return maxHealth < max;
	case 3:
		return baseMoveSpeed < max;
	case 4:
		return efficiency < max;
	case 5:
		return maxAmmo < max;
	}
	return true;
}

void creature::perkApply(unsigned int perk)
{
	unsigned int add = global_data->getValue(DATA_PERK, perk, 3);
	switch (global_data->getValue(DATA_PERK, perk, 2))
	{
	case 0:
		baseAttack += add;
		break;
	case 1:
		baseDefense += add;
		break;
	case 2:
		maxHealth += add;
		health += add;
		break;
	case 3:
		baseMoveSpeed += add;
		break;
	case 4:
		efficiency += add;
		break;
	case 5:
		maxAmmo += add;
		break;
	}
}

unsigned int creature::toNextLevel()
{
	return DREWARD_EXPERIENCECOSTBASE + DREWARD_EXPERIENCECOSTRAMP * level + DREWARD_EXPERIENCECOSTEXP * level * level;
}

void creature::getEXP(unsigned int amount)
{
	if (type == TYPE_PLAYER)
	{
		experience += amount;
		mana += DREWARD_METER;
		if (mana > 1)
			mana = 1;

		while (experience >= toNextLevel())
		{
			experience -= toNextLevel();
			level += 1;
			perkPoints += 1;

			heal(100);
		}
	}
}

unsigned int creature::getPartFromList(unsigned int list, unsigned int lN)
{
	if (list == DATA_NONE)
		return DATA_NONE;

	unsigned int listSize = global_data->getEntrySize(lN, list);
	unsigned int pick = std::rand() % listSize;
	return global_data->getValue(lN, list, pick);
}

unsigned int creature::moveSpeed()
{
	float mod = 1;
	if (armor != DATA_NONE)
		mod = (global_data->getValue(DATA_ARMOR, armor, 6) + global_data->getValue(DATA_MATERIAL, armorMaterial, 2)) * 0.01f;
	if (slow > 0)
		mod *= (1 - global_data->getValue(DATA_EFFECT, 1, 2) * 0.01f);
	if (food == 0)
		mod *= FOOD_STARVINGSLOW;
	return (unsigned int) (baseMoveSpeed * mod);
}

float creature::attackInterval()
{
	float mod = 1;
	if (armor != DATA_NONE)
		mod = global_data->getValue(DATA_MATERIAL, armorMaterial, 2) * 0.01f;
	if (food == 0)
		mod *= FOOD_STARVINGATTACKSLOW;
	return global_data->getValue(DATA_WEAPON, weapon, 3) * 0.001f / mod; //low intervals are good, so divide by the mod
}

unsigned int creature::attack()
{
	unsigned int extraAdd = 0;
	if (invisibility > 0)
		extraAdd += global_data->getValue(DATA_EFFECT, 0, 3);
	unsigned int matAttack = global_data->getValue(DATA_MATERIAL, weaponMaterial, 0);
	if (matAttack == DATA_NONE)
		return DATA_NONE;
	return baseAttack + global_data->getValue(DATA_WEAPON, weapon, 2) + matAttack + extraAdd;
}

unsigned int creature::weaponMagic()
{
	return global_data->getValue(DATA_MATERIAL, weaponMaterial, 3);
}

unsigned int creature::defense()
{
	if (armor != DATA_NONE)
	{
		unsigned int matDefense = global_data->getValue(DATA_MATERIAL, armorMaterial, 0);
		if (matDefense == DATA_NONE)
			return DATA_NONE;
		return baseDefense + global_data->getValue(DATA_ARMOR, armor, 0) + matDefense;
	}
	return baseDefense;
}

bool creature::hasRanged()
{
	return !weaponMelee() ||
			(weapon2 != DATA_NONE && global_data->getValue(DATA_WEAPON, weapon2, 0) != 0);
}

void creature::takeHit (unsigned int damage, unsigned int attack, int knockDir, float hitX, float hitY, bool hitSound)
{
	if (attackAnim != -1 && !weaponMelee() && !interruptImmune)
		attackAnim = -1;

	if (damage == 0)
		return; //don't bother

	float mult;
	if (attack == DATA_NONE || defense() == DATA_NONE)
		mult = 1;
	else
	{
		int dif = (int) attack - (int) defense();
		if (abs(dif) < 2)
			mult = 1.0f;
		else if (dif > 4)
			mult = 3.0f;
		else if (dif > 0)
			mult = 2.0f;
		else if (dif < -4)
			mult = 0.1f;
		else
			mult = 0.5f;
	}

	unsigned int modDamage = (unsigned int) std::ceil(mult * damage); //round up, so it's always at least one damage

	if (type == TYPE_PLAYER && health == maxHealth && modDamage >= health)
		modDamage = health - 1; //players should never be one-hit by anything, if at full health

	if (modDamage >= health)
	{
		health = 0;

		if (type == TYPE_ENEMY)
			giantPoof();
	}
	else
		health -= modDamage;

	if (modDamage > 1 && !knockbackImmune && knockDir != 0)
	{
		float healthF = KNOCK_HEALTHF * maxHealth;
		float damageF = KNOCK_DAMAGEF * modDamage;
		float knock = (damageF + healthF) / healthF;
		if (knock > 0)
			knockback += knock * KNOCK_FSTRENGTH * knockDir;
	}

	if (makeDamageNum())
	{
		unsigned int hS;
		if (hitSound)
			hS = global_data->getValue(DATA_RACE, race, 5);
		else
			hS = DATA_NONE;
		global_map->addDamageNumber(this, modDamage, hitX, hitY, hS);
	}
}

void creature::giantPoof()
{
	//make two particle sources, to make it rectangular
	//this won't work if it's very angular
	unsigned int numParticles = (unsigned int) ((DSMOKE_SMOKEPERAREA * maskWidth * maskHeight) * (1 + DSMOKE_LEVELRAMP * level));
	if (maskWidth < maskHeight)
	{
		global_map->addParticles(0, numParticles / 2, x + maskWidth / 2, y + maskWidth / 2, maskWidth * 0.5f);
		global_map->addParticles(0, numParticles / 2, x + maskWidth / 2, y + maskHeight - maskWidth / 2, maskWidth * 0.5f);
	}
	else
	{
		global_map->addParticles(0, numParticles / 2, x + maskHeight / 2, y + maskHeight / 2, maskHeight * 0.5f);
		global_map->addParticles(0, numParticles / 2, x + maskWidth - maskHeight / 2, y + maskHeight / 2, maskHeight * 0.5f);
	}

	if (makeDamageNum())
		global_map->forceSound(1);
}

bool creature::getItem(item *it)
{
	//pay for it
	if (it->getCost() > 0 && projectileInteract())
		global_map->forceSound(6);
	money -= it->getCost();
	it->payFor();

	if (it->ammo() > 0)
	{
		addAmmo(it->ammo());
		it->removed = true;
		it->merged = true;
		return true;
	}

	for (unsigned int i = 0; i < INVENTORYSIZE; i++)
		if (inventory[i] != NULL && it->merge(inventory[i]))
			break;

	bool added = false;
	if (!it->merged)
	{
		for (unsigned int i = 0; i < INVENTORYSIZE; i++)
			if (inventory[i] == NULL)
			{
				added = true;
				inventory[i] = it;
				break;
			}
	}

	if (added || it->merged)
	{
		it->removed = true;
		return true;
	}
	else
		return false;
}

void creature::bloodEffect(float bloodX, float bloodY, unsigned int magnitude)
{
	//bound position
	if (bloodX < x)
		bloodX = x;
	else if (bloodX > x + maskWidth)
		bloodX = x + maskWidth;
	if (bloodY < y)
		bloodY = y;
	else if (bloodY > y + maskHeight)
		bloodY = y + maskHeight;

	global_map->addParticles(global_data->getValue(DATA_RACE, race, 4), BLOODBASE + BLOODRAMP * magnitude, bloodX, bloodY, BLOODRADIUS);
	rumble = RUMBLELENGTH;
}

void creature::applyEffect(unsigned int effect)
{
	if (effect == DATA_NONE)
		return;
	float duration = global_data->getValue(DATA_EFFECT, effect, 0) * 0.01f;
	switch(effect)
	{
	case 0:
		invisibility = duration;
		break;
	case 1:
		slow = duration;
		break;
	case 2:
		soulArrow = (unsigned int) duration;
		break;
	case 3:
		venemous += (unsigned int) duration;
		break;
	case 4:
	case 5:
		paralyzed = duration;
		break;
	case 6:
		wolfPearl = true;
		break;
	}
}

bool creature::addAmmo(unsigned int amount)
{
	if (ammo < maxAmmo)
	{
		ammo += amount;
		if (ammo > maxAmmo)
			ammo = maxAmmo;
		return true;
	}
	return false;
}

void creature::switchWeapon()
{
	if (weapon2 == DATA_NONE)
		return;

	//halt any animation
	if (attackAnim != -1)
		attackAnim = -1;

	//switch out
	bool loadedT = loaded;
	unsigned int weaponT = weapon;
	unsigned int weaponMaterialT = weaponMaterial;
	loaded = loaded2;
	weapon = weapon2;
	weaponMaterial = weaponMaterial2;
	loaded2 = loadedT;
	weapon2 = weaponT;
	weaponMaterial2 = weaponMaterialT;
}

void creature::rangedPow(float mult)
{
	unsigned int damage = global_data->getValue(DATA_WEAPON, weapon, 1);
	if (mult < 1)
	{
		damage = (unsigned int) (damage * mult);
		if (damage < 1)
			damage = 1;
	}
	if (soulArrow > 0)
		damage = (unsigned int) (damage * global_data->getValue(DATA_EFFECT, 2, 5) * 0.01);

	bool ven = false;
	if (venemous > 0 && projectileInteract())
	{
		venemous -= 1;
		ven = true;
	}

	forceShoot(damage, attack(), global_data->getValue(DATA_WEAPON, weapon, 6), weaponMagic(), ven);

	unsigned int sound = global_data->getValue(DATA_WEAPON, weapon, 10);
	if (sound != DATA_NONE)
		global_map->forceSound(sound);

	if (type == TYPE_PLAYER)
	{
		if (soulArrow > 0)
			soulArrow -= 1;
		else
			ammo -= 1;
	}

	if (hurtSelf && !dead())
		health -= 1;
}

float creature::projectileStartX()
{
	return x + maskWidth / 2 + global_data->getValue(DATA_RACE, race, 2) * facingX;
}

float creature::projectileStartY()
{
	return y + maskHeight + global_data->getValue(DATA_RACE, race, 3);
}

void creature::forceShoot (unsigned int damage, unsigned int attack, unsigned int projectileType, unsigned int projectileMagic, bool ven, bool aim)
{
	if (projectileInteract()) //slaves can't fire arrows
	{
		float xDir = (float) facingX;
		float yDir = 0;
		float startX = projectileStartX();
		float startY = projectileStartY();

		if (aim)
		{
			//find the best angle
			float bestValue = DATA_NONE;
			unsigned int numCreatures = global_map->getNumCreatures();
			for (unsigned int i = 0; i < numCreatures; i++)
			{
				creature *target = global_map->getCreature(i);
				float targetX = target->x + target->maskWidth / 2;
				float targetY = target->y + target->maskHeight / 2;

				if (!target->dead() && !target->invisible() && !target->isInvincible() &&
					abs(targetX - startX) < SCREENWIDTH / 2 &&
					((facingX == 1 && targetX > x + maskWidth / 2) ||
					(facingX == -1 && targetX < x + maskWidth / 2)) &&
					((target->type == TYPE_ENEMY && type == TYPE_PLAYER) ||
					(target->type == TYPE_PLAYER && type == TYPE_ENEMY)))
				{
					float xDif = targetX - startX;
					float yDif = targetY - startY;
					float dis = sqrt(xDif * xDif + yDif * yDif);
					float thisXDir = xDif / dis;
					float thisYDir = yDif / dis;

					float angle = atan2(thisXDir, thisYDir);
					float angleDif = (float) abs(abs(angle) - M_PI / 2);
					float value = dis * (1 + angleDif);
					if (value < bestValue && angleDif <= MAXANGLEDIF)
					{
						bestValue = value;
						xDir = thisXDir;
						yDir = thisYDir;
					}
				}
			}
		}

		if (projectileType == 0 && soulArrow > 0)
			projectileType = global_data->getValue(DATA_EFFECT, 2, 2);

		global_map->addProjectile(startX, startY, xDir, yDir, projectileType, damage, attack, type, projectileMagic, ven);
	}

	//cancel invisibility
	invisibility = 0;
}

unsigned int creature::weaponReach()
{
	bool sw = false;
	if (!weaponMelee())
	{
		sw = true;
		switchWeapon();
	}
	unsigned int reach = global_data->getValue(DATA_WEAPON, weapon, 6);
	if (sw)
		switchWeapon();
	return reach;
}

bool creature::canMeleeHit(creature *other)
{
	if (!weaponMelee())
		return false; //can't hit someone with a melee weapon if you don't have one on, obviously
	sf::FloatRect attackBox = sf::FloatRect(x + maskWidth * 0.5f, (float) y, maskWidth * 0.5f + weaponReach(), (float) maskHeight);
	if (facingX == -1)
		attackBox.left -= attackBox.width;
	sf::FloatRect theirBox = sf::FloatRect((float) other->x, (float) other->y, (float) other->maskWidth, (float) other->maskHeight);
	return (!other->dead() && !other->isInvincible() && //can't hit dead people
			((other->type == TYPE_ENEMY && type == TYPE_PLAYER) || //can't hit teammates
			(other->type == TYPE_PLAYER && type == TYPE_ENEMY)) &&
			attackBox.intersects(theirBox) //have to actually hit them
			);
}

void creature::meleePow()
{
	unsigned int targetSide;
	if (type == TYPE_PLAYER)
		targetSide = TYPE_ENEMY;
	else
		targetSide = TYPE_PLAYER;

	//check to see who you can hit
	unsigned int damage = global_data->getValue(DATA_WEAPON, weapon, 1);
	bool hitSomeone = false;
	for (unsigned int i = 0; i < global_map->getNumCreatures(); i++)
		if (canMeleeHit(global_map->getCreature(i)))
		{
			creature *cr = global_map->getCreature(i);
			cr->takeHit(damage, attack(), facingX, x + maskWidth / 2, y + maskHeight / 2);
			if (weaponMagic() != DATA_NONE && projectileInteract())
				global_map->magic(cr, weaponMagic(), targetSide);
			unsigned int armorMagic = global_data->getValue(DATA_MATERIAL, cr->armorMaterial, 3);
			if (armorMagic != DATA_NONE && projectileInteract()) //magic yourself too
				global_map->magic(this, armorMagic, type);
			unsigned int armorSelfEffect = global_data->getValue(DATA_MATERIAL, cr->armorMaterial, 6); //magic the enemy
			if (armorSelfEffect != DATA_NONE && cr->projectileInteract())
				global_map->magic(cr, armorSelfEffect, type);
			if (venemous > 0 && projectileInteract())
			{
				global_map->magic(cr, global_data->getValue(DATA_EFFECT, 3, 2), targetSide);
				venemous -= 1;
			}
			hitSomeone = true;
		}

	unsigned int weaponSelfEffect = global_data->getValue(DATA_MATERIAL, weaponMaterial, 6);
	if (!hitSomeone && weaponSelfEffect != DATA_NONE)
		global_map->magic(this, weaponSelfEffect, targetSide);
	if (!hitSomeone && wolfPearl && health == maxHealth && food > 0)
	{
		//make a wolf pearl projectile
		forceShoot(global_data->getValue(DATA_EFFECT, 6, 3), DATA_NONE, global_data->getValue(DATA_EFFECT, 6, 2), DATA_NONE, false, false);
		foodCost(global_data->getValue(DATA_EFFECT, 6, 4) * 0.001f);
	}

	//cancel invisibility
	invisibility = 0;

	if (hurtSelf && hitSomeone && !dead())
		health -= 1;
}

void creature::boundInner()
{
	if (x < global_camera->x)
		x = global_camera->x;
	else if (x + maskWidth > global_camera->x + SCREENWIDTH)
		x = global_camera->x + SCREENWIDTH - maskWidth;
	if (y < global_camera->y)
		y = global_camera->y;
	else if (y + maskHeight > global_camera->y + SCREENHEIGHT)
		y = global_camera->y + SCREENHEIGHT - maskHeight;
}

bool creature::moveInner(float amount, float *axis)
{
	unsigned int chunks = (unsigned int) (abs(amount) / MOVECHUNK);
	if (chunks <= 1)
	{
		int yDir = 0;
		if (axis == &y)
		{
			if (amount > 0)
				yDir = 1;
			else if (amount < 0)
				yDir = -1;
		}

		*axis += amount;

		//check to see if you fell in a bottomless pit
		if (y + maskHeight >= global_map->getHeight() ||
			(type == TYPE_PLAYER && y + maskHeight >= global_camera->y + SCREENHEIGHT))
		{
			//stop immediately and deal with this
			health = 0;
			if (type == TYPE_PLAYER)
				global_map->teleportToSafety(this); //move to safety
			else
			{
				//clear your inventory so that it doesn't waste time dropped down there in bottomless pit hell
				for (unsigned int i = 0; i < inventory.size(); i++)
					delete inventory[i];
				inventory.clear();
			}
			return false;
		}

		if (collideAt(x, y, yDir))
		{
			//find out what you hit, and how to get flush against it
			if (amount > 0)
			{
				unsigned int thick;
				if (axis == &y)
					thick = maskHeight;
				else
					thick = maskWidth;

				unsigned int t = (unsigned int) ((*axis + thick) / TILESIZE);
				*axis = t * TILESIZE * 1.0f - thick;
			}
			else
			{
				*axis -= amount;
				unsigned int t = (unsigned int) (*axis / TILESIZE);
				*axis = t * TILESIZE * 1.0f;
			}

			return false;
		}
		if (type == TYPE_PLAYER)
			boundInner();
	}
	else
	{
		//break the operation into pieces
		chunks += 1;
		for (unsigned int i = 0; i < chunks; i++)
			if (!moveInner(amount / chunks, axis))
				return false;
	}
	return true;
}

bool creature::flyMove (float xDir, float yDir, float elapsed)
{
	if (knockback != 0 || isParalyzed())
		return false;

	float mult = 1;
	if (attackAnim != -1 && !weaponMelee())
		mult *= ANIMSLOW;
	x += xDir * moveSpeed() * mult * elapsed;
	y += yDir * moveSpeed() * mult * elapsed;
	return true;
}

void creature::heal (unsigned int amount)
{
	int oldHealth = health;
	health += amount;
	if (health > maxHealth)
		health = maxHealth;
	global_map->addDamageNumber(this, oldHealth - health, x + maskWidth / 2, y + maskHeight / 2, 0);
}

void creature::useMagic()
{
	if (magicItem != DATA_NONE)
	{
		float cost = (global_data->getValue(DATA_MAGICITEM, magicItem, 1) - efficiency) * 0.01f - 0.001f;
		if (mana >= cost)
		{
			mana -= cost;
			unsigned short targetType;
			if (type == TYPE_PLAYER)
				targetType = TYPE_ENEMY;
			else
				targetType = TYPE_PLAYER;
			global_map->magic(this, global_data->getValue(DATA_MAGICITEM, magicItem, 0), targetType);
		}
	}
}

bool creature::move(int xDir, float elapsed)
{
	if (knockback != 0 || isParalyzed())
		return false;

	movingX = xDir;
	if (!attacking())
		facingX = xDir;
	float mult = 1;
	if (attackAnim != -1 && !weaponMelee())
		mult *= ANIMSLOW;
	if (flying)
	{
		x += xDir * moveSpeed() * elapsed * mult;
		return true;
	}
	else if (!collideAt(x + xDir, y, 0) && moveInner(xDir * elapsed * moveSpeed() * mult, &x))
	{
		foodCost(FOOD_WALKCOST * elapsed);

		//walk animation
		if (walk == -1)
			walk = 0;
		walk += elapsed * WALKRATE;
		if (walk > 1)
			walk -= 1;
		return true;
	}
	else
		return false;
}

bool creature::collideAt(float x, float y, int yDir)
{
	if (maskWidth > TILESIZE && ( //special checks for huge creatures
		global_map->tileSolid(x + maskWidth / 3, y, yDir, y + maskHeight - 3, dropMode) ||
		global_map->tileSolid(x + maskWidth / 3, y + maskHeight - 1, yDir, y + maskHeight / 2, dropMode) ||
		global_map->tileSolid(x + maskWidth * 2 / 3, y, yDir, y + maskHeight - 3, dropMode) ||
		global_map->tileSolid(x + maskWidth * 2 / 3, y + maskHeight - 1, yDir, y + maskHeight / 2, dropMode)
		))
		return true;
	return (global_map->tileSolid(x, y, yDir, y + maskHeight - 3, dropMode) ||
			global_map->tileSolid(x + maskWidth - 1, y, yDir, y + maskHeight / 2, dropMode) ||
			global_map->tileSolid(x, y + maskHeight - 1, yDir, y + maskHeight / 2, dropMode) ||
			global_map->tileSolid(x + maskWidth - 1, y + maskHeight - 1, yDir, y + maskHeight - 3, dropMode));
}

bool creature::weaponMelee()
{
	return global_data->getValue(DATA_WEAPON, weapon, 0) == 0 || global_data->getValue(DATA_WEAPON, weapon, 0) == 3;
}

bool creature::grounded()
{
	return collideAt(x, y + 1, 1);
}

void creature::startAttack()
{
	if (attackAnim == -1 && !isParalyzed())
	{
		foodCost(FOOD_ATTACKCOST);
		if (loaded && global_data->getValue(DATA_WEAPON, weapon, 0) == 1)
		{
			loaded = false;
			rangedPow(); //shoot!
		}
		else
		{
			if (weaponMelee())
			{
				unsigned int sound = global_data->getValue(DATA_WEAPON, weapon, 10);
				if (sound != DATA_NONE)
					global_map->forceSound(sound);
			}
			attackAnim = 0;
		}
	}
}

void creature::endAttack()
{
	if (attackAnim != -1 && global_data->getValue(DATA_WEAPON, weapon, 0) == 2)
	{
		//launch the attack
		if (attackAnim > AANIM_BOW_MINDRAW)
			rangedPow((attackAnim - AANIM_BOW_MINDRAW) / (1 - AANIM_BOW_MINDRAW));

		//stop attacking
		attackAnim = -1;
	}
}

bool creature::canHoldAttack()
{
	return global_data->getValue(DATA_WEAPON, weapon, 0) != 1;
}

void creature::jump(float mult)
{
	if (attackAnim != -1 && global_data->getValue(DATA_WEAPON, weapon, 0) == 1)
		return; //can't jump while reloading a crossbow

	if (grounded())
	{
		ySpeed = JUMPFORCE * mult;
		foodCost(FOOD_JUMPCOST);
	}
	else if (doubleJump && type == TYPE_PLAYER) //only the player can double-jump
	{
		ySpeed = JUMPFORCE * mult;
		doubleJump = false;
		foodCost(FOOD_JUMPCOST);
	}
}

void creature::foodCost(float cost)
{
	if (type == TYPE_PLAYER)
	{
		food -= cost;
		if (food < 0)
			food = 0;
	}
}

void creature::updateAnimOnly(float elapsed)
{
	//bob
	//which is the only animation that is safe to update in this phase, since it has no gameplay effect
	bob += bobD * elapsed * BOBRATE;
	if (bob > 1)
	{
		bob = 1;
		bobD *= -1;
	}
	else if (bob < -1)
	{
		bob = -1;
		bobD *= -1;
	}
}

void creature::update(float elapsed)
{
	if (dead())
	{
		//reset effects
		invisibility = 0;
		slow = 0;
		venemous = 0;
		paralyzed = 0;

		if (experience > 0 && type == TYPE_ENEMY)
		{
			//experience reward
			global_map->deathReward(experience);
			experience = 0;
		}
		else if (experience > 0 && type == TYPE_PLAYER)
			experience = 0; //lose all current experience

		drop();
		attackAnim = -1; //your attack animation is interrupted, obviously
		rumble = -1;
	}

	if (rumble != -1)
	{
		rumble -= elapsed;
		if (rumble <= 0)
			rumble = -1;
	}

	if (chargeD != 0)
	{
		//for now charge damage is enemy-only
		unsigned int numCreatures = global_map->getNumCreatures();
		sf::FloatRect me (x, y, (float) maskWidth, (float) maskHeight);
		for (unsigned int i = 0; i < numCreatures; i++)
		{
			creature *cr = global_map->getCreature(i);
			if (cr->getType() == TYPE_PLAYER && cr->knockback == 0 && cr->projectileInteract() && !cr->dead() && !cr->isInvincible())
			{
				sf::FloatRect them (cr->x, cr->y, (float) cr->maskWidth, (float) cr->maskHeight);
				if (me.intersects(them))
				{
					//hurt them using your base attack ONLY
					cr->takeHit(chargeD, baseAttack, facingX, x + maskWidth / 2, y + maskHeight);
				}
			}
		}
	}

	if (summonEffectTimer > 0)
	{
		summonEffectTimer -= elapsed;
		if (summonEffectTimer <= 0)
		{
			giantPoof();
			summonEffectTimer = 0;
		}
	}

	//progress effects
	if (invisibility > 0)
	{
		invisibility -=	elapsed;
		if (invisibility < 0)
			invisibility = 0;
	}
	if (slow > 0)
	{
		slow -= elapsed;
		if (slow < 0)
			slow = 0;
	}
	if (paralyzed > 0)
	{
		paralyzed -= elapsed;
		if (paralyzed <= 0)
		{
			paralyzed = 0;
			//cancel the attack HERE, so that they are stuck in the animation while paralyzed
			attackAnim = -1;
		}
	}

	if (!flying || knockback != 0 || isParalyzed())
	{
		if (y > global_map->getHeight() + TILESIZE)
			return; //you fell down a bottomless pit; don't waste your time falling any more

		if (ySpeed != 0)
		{
			if (moveInner(elapsed * ySpeed, &y))
				ySpeed += GRAVITY * elapsed;
			else
				ySpeed = 0;
			if (ySpeed > TERMINALVEL)
				ySpeed = TERMINALVEL;
		}
		else if (!grounded())
			ySpeed = 1;
		else
			doubleJump = true;
	}

	//knockback
	if (knockback != 0)
	{
		moveInner(knockback * elapsed, &x);

		int knockDir;
		if (knockback > 0)
			knockDir = 1;
		else
			knockDir = -1;
		float knockReduce = KNOCK_DECAY * elapsed;
		if (abs(knockback) <= knockReduce)
			knockback = 0;
		else
			knockback -= knockReduce * knockDir;
	}

	//animation control

	//attack
	if (attackAnim != -1 && !isParalyzed())
	{
		float oldAA = attackAnim;
		unsigned int wType = global_data->getValue(DATA_WEAPON, weapon, 0);
		if (wType != 3)
			attackAnim += elapsed / attackInterval();
		switch(wType)
		{
		case 0: //melee
			if (oldAA < AANIM_MELEE_POWPOINT && attackAnim >= AANIM_MELEE_POWPOINT)
				meleePow();
			if (attackAnim >= 1)
				attackAnim = -1;
			break;
		case 1: //crossbow
			if (attackAnim >= 1)
			{
				loaded = true;
				attackAnim = -1;
			}
			break;
		case 2: //bow
			if (attackAnim >= 1)
				attackAnim = 1;
			break;
		case 3: //boss tool
			break;
		}
	}

	if (!isParalyzed() && !dead())
		updateAnimOnly(elapsed);

	//walk
	if (flying && !isParalyzed())
	{
		if (walk == -1)
			walk = 0;
		walk += elapsed * WALKRATE;
		if (walk > 1)
			walk -= 1;
	}
	else if (lastX == x && !isParalyzed())
		walk = -1;
}

bool creature::isOnscreen()
{
	return (x + maskWidth >= global_camera->x &&
			y + maskHeight >= global_camera->y &&
			x <= global_camera->x + SCREENWIDTH &&
			y <= global_camera->y + SCREENHEIGHT);
}

void creature::render(drawer *draw, float xAt, float yAt)
{
	//check to see if you are offscreen
	if (xAt == -DATA_NONE && !isOnscreen())
		return;

	//check to see if you are a dead enemy
	if (type == TYPE_ENEMY && (dead() || invisibility > 0))
		return;

	if (summonEffectTimer > 0)
	{
		unsigned int slS = global_data->getValue(DATA_LIST, 0, 7);
		float slX = x + maskWidth / 2 - global_camera->x;
		unsigned int slH = global_data->getValue(DATA_SHEET, slS, 2);
		float summonM = slH * summonEffectTimer * SUMMONEFFECTSPEED;
		while (summonM > slH)
			summonM -= slH;
		float slY = y + maskHeight - global_camera->y;
		unsigned int colorN = ((unsigned int) (summonEffectTimer / SUMMONEFFECTSTROBE)) % 2;
		sf::Color color;
		if (colorN == 0)
			color = SUMMONEFFECTSTROBEC1;
		else
			color = SUMMONEFFECTSTROBEC2;
		unsigned int frame = 0;
		if (summonEffectTimer < SUMMONEFFECTFADE)
			frame = 1;
		draw->renderSprite(slS + 1, frame, slX, slY, false, color);
		slY -= summonM;
		while (slY > 0)
		{
			draw->renderSprite(slS, frame, slX, slY, false, color);
			slY -= slH;
		}
		return;
	}

	bool grnd;
	if (xAt == -DATA_NONE)
	{
		xAt = x;
		yAt = y;
		if (flying && parts.size() < 2) //this is a check for small flying enemies like the copter, but will make humanoids dramatically float
			grnd = true;
		else
			grnd = grounded();
	}
	else
		grnd = true;

	//add a light if necessary
	if (glow != DATA_NONE)
		draw->renderSprite(global_data->getValue(DATA_LIST, 0, 2), 0,
								xAt + maskWidth / 2 - global_camera->x,
								yAt + maskHeight / 2 - global_camera->y, false, makeColor(glow));

	//apply rumble NOW, so that the light won't rumble
	if (rumble != -1)
	{
		xAt += rand() % (RUMBLESTRENGTH * 2 + 1) - RUMBLESTRENGTH;
		yAt += rand() % (RUMBLESTRENGTH * 2 + 1) - RUMBLESTRENGTH;
	}

	unsigned int weaponType;
	if (weapon == DATA_NONE)
		weaponType = 0;
	else
		weaponType = global_data->getValue(DATA_WEAPON, weapon, 0);
	int numParts = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM);
	sf::Vector2f *positions = new sf::Vector2f[numParts];
	for (int i = 0; i < numParts; i++)
	{
		unsigned int partBase = parts[i];
		int part = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM + 1 + i);

		//determine position
		int relativeTo = global_data->getValue(DATA_PARTFORM, part, 1);
		int hMoveXLean = global_data->getValue(DATA_PARTFORM, part, 4);
		if (x < lastX)
			hMoveXLean *= -1;
		else if (x == lastX)
			hMoveXLean = 0;
		float partX = global_data->getValue(DATA_PARTFORM, part, 2) * facingX + hMoveXLean
						 + global_data->getValue(DATA_PARTFORM, part, 5) * bob * facingX;
		float partY = global_data->getValue(DATA_PARTFORM, part, 3) + global_data->getValue(DATA_PARTFORM, part, 6) * bob;
		if (relativeTo == DATA_NONE)
		{
			//relative to your position
			partX += xAt + maskWidth / 2;
			partY += yAt + maskHeight;
		}
		else
		{
			//relative to something else
			partX += positions[relativeTo].x;
			partY += positions[relativeTo].y;
		}

		//record position for future reference
		positions[i].x = partX;
		positions[i].y = partY;

		//if (partBase != DATA_NONE)
		{
			//get the frame add
			unsigned int frameAdd = 0;
			unsigned int baseFrameAdd = 0;
			
			if (global_data->getValue(DATA_PARTFORM, part, 11) && gender)
				frameAdd += 1;

			if (global_data->getValue(DATA_PARTFORM, part, 7))
			{
				//handle leg animation
				if (knockback != 0 || !grnd)
					frameAdd += 3;
				else if (walk >= 0 && walk < 0.5)
					frameAdd += 1;
				else if (walk >= 0.5)
					frameAdd += 2;
			}
			else if (global_data->getValue(DATA_PARTFORM, part, 8))
			{
				if (weapon == DATA_NONE)
				{
					baseFrameAdd = AANIM_NONE_STARTF;
				}
				else
				{
					//handle arm animation
					switch(weaponType)
					{
					case 0: //melee
						baseFrameAdd = AANIM_MELEE_STARTF;
						if (attackAnim == -1)
							frameAdd += AANIM_MELEE_IDLEF;
						else if (attackAnim < AANIM_MELEE_PHASE1E)
							frameAdd += AANIM_MELEE_PHASE1F;
						else if (attackAnim < AANIM_MELEE_PHASE2E)
							frameAdd += AANIM_MELEE_PHASE2F;
						else if (attackAnim < AANIM_MELEE_PHASE3E)
							frameAdd += AANIM_MELEE_PHASE3F;
						else
							frameAdd += AANIM_MELEE_PHASE4F;
						break;
					case 1: //crossbow
						baseFrameAdd = AANIM_CROSSBOW_STARTF;
						if (attackAnim != -1)
							frameAdd += AANIM_CROSSBOW_LOADINGF;
						else if (loaded)
							frameAdd += AANIM_CROSSBOW_LOADEDF;
						else
							frameAdd += AANIM_CROSSBOW_IDLEF;
						break;
					case 2: //bow
						baseFrameAdd = AANIM_BOW_STARTF;
						if (attackAnim == -1)
							frameAdd += AANIM_BOW_IDLEF;
						else if (attackAnim < 0.5)
							frameAdd += AANIM_BOW_DRAW1F;
						else if (attackAnim < 1)
							frameAdd += AANIM_BOW_DRAW2F;
						else
							frameAdd += AANIM_BOW_FULLF;
						break;
					case 3: //boss tool
						break;
					}
				}
			}

			if (weaponType == 3 && attackAnim != -1)
			{
				//access boss tool data
				//note that this SETS the frameadd if it's nonzero, instead of adding to it
				//that's so that if you use this while moving the leg animation won't fuck things up, etc
				unsigned int bossToolData = (unsigned int) ((attackAnim + BOSSTOOLM / 2) / BOSSTOOLM);
				unsigned int bTD = global_data->getValue(DATA_BOSSTOOLSTATE, bossToolData, i);
				if (bTD != 0)
					frameAdd = bTD;
			}

			partX -= global_camera->x;
			partY -= global_camera->y;

			unsigned int invisA = 255;
			if (invisibility > 0)
			{
				invisA = (unsigned int) (global_data->getValue(DATA_EFFECT, 0, 2) * 255 * 0.01f);
				if (invisibility < 1)
					invisA = (unsigned int) (invisA * invisibility + (1 - invisibility) * 255);
			}

			//draw it
			if (partBase != DATA_NONE && frameAdd < DATA_NONE)
			{
				unsigned int cN = global_data->getValue(DATA_PARTFORM, part, 12);
				sf::Color c;
				if (dead())
					c = DEADCOLOR;
				else if (cN == DATA_NONE)
					c = sf::Color::White;
				else
					c = colors[cN];
				c.a = invisA;
				draw->renderSprite(global_data->getValue(DATA_PARTFORM, part, 0), partBase + frameAdd + baseFrameAdd, partX, partY,
									facingX < 0, c);

			if (global_data->getValue(DATA_PARTFORM, part, 13) != DATA_NONE)
				draw->renderSprite(global_data->getValue(DATA_PARTFORM, part, 13), partBase + frameAdd + baseFrameAdd, partX, partY,
								facingX < 0, c);
			}

			if (global_data->getValue(DATA_PARTFORM, part, 9))
			{
				//draw armor
				unsigned int armorSheet = global_data->getValue(DATA_ARMOR, armor, 1);
				if (armorSheet != DATA_NONE)
				{
					unsigned int armorPart = global_data->getValue(DATA_ARMOR, armor, 2);
					sf::Color armorColor = makeColor(global_data->getValue(DATA_MATERIAL, armorMaterial, 1));
					armorColor.a = invisA;
					draw->renderSprite(armorSheet, armorPart + frameAdd + baseFrameAdd, partX, partY, facingX < 0, armorColor);
				}
			}
			if (weapon != DATA_NONE && global_data->getValue(DATA_PARTFORM, part, 10))
			{
				//draw weapon
				unsigned int weaponSheet = global_data->getValue(DATA_WEAPON, weapon, 4);
				if (weaponSheet != DATA_NONE)
				{
					unsigned int weaponPart = global_data->getValue(DATA_WEAPON, weapon, 5);
					sf::Color weaponColor = makeColor(global_data->getValue(DATA_MATERIAL, weaponMaterial, 1));
					weaponColor.a = invisA;
					draw->renderSprite(weaponSheet, weaponPart + frameAdd, partX, partY, facingX < 0, weaponColor);
				}
			}
			if (hat != DATA_NONE && global_data->getValue(DATA_PARTFORM, part, 14))
			{
				//draw hat
				unsigned int hatSheet = global_data->getValue(DATA_HAT, hat, 0);
				if (hatSheet != DATA_NONE)
				{
					unsigned int hatPart = global_data->getValue(DATA_HAT, hat, 1);
					sf::Color hatColor = makeColor(global_data->getValue(DATA_HAT, hat, 2));
					hatColor.a = invisA;
					draw->renderSprite(hatSheet, hatPart + frameAdd, partX, partY, facingX < 0, hatColor);
				}
			}
		}
	}
	delete[] positions;
}

void creature::givePlayerInventory (unsigned int cType)
{
	unsigned int dropList = global_data->getValue(DATA_CTYPE, cType, 14);
	unsigned int dropListSize = global_data->getEntrySize(DATA_LIST, dropList);
	for (unsigned int i = 0; i < dropListSize / 3; i++)
	{
		item *it = new item(global_data->getValue(DATA_LIST, dropList, i * 3),  global_data->getValue(DATA_LIST, dropList, i * 3 + 1),
			1, global_data->getValue(DATA_LIST, dropList, i * 3 + 2), 0, 0, global_map->getIdentifier());
		inventory[i] = it;
	}
}

void creature::sendPacket(sf::Packet *outPacket, bool always)
{
	if (!always && type == TYPE_ENEMY)
	{
		(*outPacket) << active();
		if (!active())
			return;
	}

	//everything is converted into floats instead of Uint16's because... the Uint16's stopped working?
	//it's way too early for me to give a fuck

	//position data
	(*outPacket) << x << y << ySpeed << (float) facingX << (float) movingX << walk << dropMode << knockback << flying;
	//resources
	(*outPacket) << (float) health;
	//attack data
	(*outPacket) << attackAnim << (float) weapon << loaded;
	//misc animation data
	(*outPacket) << walk;
	//effects
	bool hasEffects = invisibility > 0 || slow > 0 || paralyzed > 0;
	(*outPacket) << hasEffects;
	if (hasEffects)
		(*outPacket) << invisibility << slow << paralyzed;

	if (type == TYPE_PLAYER)
	{
		//player-relevant stuff
		(*outPacket) << food << mana;
	}
	else if (type == TYPE_ENEMY)
	{
		//enemy-relevant stuff
		(*outPacket) << (float) chargeD;
	}
}

void creature::receivePacketInner(sf::Packet *inPacket, bool always)
{
	if (!always && type == TYPE_ENEMY)
	{
		bool active;
		(*inPacket) >> active;
		if (!active)
		{
			//quickly set some default values, to ensure that inactive AIs don't end up walking around
			dropMode = false;
			walk = -1;
			attackAnim = -1;
			return;
		}
	}

	//position data
	float uFacingX, uMovingX;
	(*inPacket) >> x >> y >> ySpeed >> uFacingX >> uMovingX >> walk >> dropMode >> knockback >> flying;
	facingX = (int) uFacingX;
	movingX = (int) uMovingX;
	//resources
	float uHealth;
	(*inPacket) >> uHealth;
	health = (unsigned int) uHealth;
	//attack data
	float uWeapon;
	(*inPacket) >> attackAnim >> uWeapon >> loaded;
	if ((unsigned int) uWeapon != weapon)
		switchWeapon();
	//misc animation data
	(*inPacket) >> walk;
	//effects
	bool hasEffects;
	(*inPacket) >> hasEffects;
	if (hasEffects)
		(*inPacket) >> invisibility >> slow >> paralyzed;
	else
	{
		if (paralyzed > 0)
			attackAnim = -1; //cancel the attack animation here too, to make sure
		invisibility = 0;
		slow = 0;
		paralyzed = 0;
	}

	if (type == TYPE_PLAYER)
	{
		//player-relevant stuff
		(*inPacket) >> food >> mana;
	}
	else if (type == TYPE_ENEMY)
	{
		//enemy-relevant stuff
		float uChargeD;
		(*inPacket) >> uChargeD;
		chargeD = (unsigned int) uChargeD;
	}
}

void creature::pollCamera (unsigned int *checkCounter)
{
	if (type == TYPE_PLAYER)
	{
		global_camera->x += x + maskWidth / 2;
		global_camera->y += y + maskHeight / 2;
		(*checkCounter) += 1;
	}
}