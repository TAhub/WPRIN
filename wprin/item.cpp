#include "stdafx.h"

extern map *global_map;
extern database *global_data;
extern sf::Vector2f *global_camera;

unsigned int global_itemId = 0;

item::item (sf::Packet *loadFrom) : removed(false), merged(false)
{
	float uIdentifier, uID, uType, uNum, uMat, uCost;
	(*loadFrom) >> uIdentifier >> uType >> uID >> uNum >> uMat >> uCost;
	identifier = (unsigned int) uIdentifier;
	type = (unsigned short) uType;
	id = (unsigned short) uID;
	number = (unsigned short) uNum;
	material = (unsigned int) uMat;
	cost = (unsigned int) uCost;
}

item::item (unsigned short type, unsigned int id, unsigned int number, unsigned int material, float x, float y, unsigned int serverID) : x(x), y(y),
		type(type), id(id), number(number), material(material), removed(false), merged(false), cost(0)
{
	if (serverID != DATA_NONE)
	{
		identifier = serverID * MAXID + global_itemId;
		global_itemId = (global_itemId + 1) % MAXID;
	}
	else
		identifier = 0; //temporary item, so don't bother incrementing the itemID
}

void item::save (sf::Packet *saveTo)
{
	(*saveTo) << (float) identifier << (float) type << (float) id << (float) number << (float) material << (float) cost;
}

void item::calculateCost()
{
	cost = returnCost();
}

unsigned int item::returnSaleValue()
{
	unsigned int c = (unsigned int) (returnCost() * SELLVALUE);
	if (c < 1)
		return 1; //you can't get 0 for anything, ever
	else
		return c;
}

unsigned int item::returnCost()
{
	//get base cost
	unsigned int c;
	switch(type)
	{
	case 0:
		c = global_data->getValue(DATA_ITEM, id, 6);
		break;
	case 1:
		c = global_data->getValue(DATA_ARMOR, id, 5);
		break;
	case 2:
		c = global_data->getValue(DATA_WEAPON, id, 9);
		break;
	case 3:
		c = global_data->getValue(DATA_MAGICITEM, id, 5);
		break;
	}

	//get the modifier
	float mod = global_data->getValue(DATA_MATERIAL, material, 5) * 0.01f;
	if (mod != 0 && mod > -1)
		c = (unsigned int) (c * (1 + mod));
	if (number > 1)
		c *= number;
	return c;
}

void item::render (drawer *draw)
{
	unsigned int sheet = global_data->getValue(DATA_LIST, 0, 3);
	unsigned int frame;
	sf::Color color = makeColor(global_data->getValue(DATA_MATERIAL, material, 1));
	switch(type)
	{
	case 0:		
		frame = global_data->getValue(DATA_ITEM, id, 0);
		break;
	case 1:
		frame = global_data->getValue(DATA_ARMOR, id, 3);
		break;
	case 2:
		frame = global_data->getValue(DATA_WEAPON, id, 7);
		break;
	case 3:
		frame = global_data->getValue(DATA_MAGICITEM, id, 2);
		color = makeColor(global_data->getValue(DATA_MAGICITEM, id, 3));
		break;
	}

	if (frame == DATA_NONE)
		frame = frame = global_data->getValue(DATA_ITEM, 0, 0); //just a temporary thing, for bad drops

	draw->renderSprite(sheet, frame, x - global_camera->x, y - global_camera->y, false, color);
}

unsigned int item::ammo()
{
	if (type != 0)
		return 0;
	else
		return global_data->getValue(DATA_ITEM, id, 3);
}

unsigned int item::feeds()
{
	if (type != 0)
		return 0;
	else
		return global_data->getValue(DATA_ITEM, id, 4);
}

unsigned int item::effect()
{
	if (type != 0)
		return DATA_NONE;
	else
		return global_data->getValue(DATA_ITEM, id, 5);
}

unsigned int item::heals()
{
	if (type != 0)
		return 0;
	else
	{
		unsigned int baseH = global_data->getValue(DATA_ITEM, id, 2);
		if (baseH > 0)
			return baseH + global_data->getValue(DATA_MATERIAL, material, 0);
		else
			return 0;
	}
}

void item::useSome (unsigned int amount)
{
	if (number > amount)
		number -= amount;
	else
		number = 0;
}

bool item::canMerge (item *mergeWith)
{
	return (mergeWith->id == id && mergeWith->type == type && mergeWith->material == material && type == 0 &&
			mergeWith->number < ITEM_MAXSTACK);
}

bool item::merge (item *mergeWith)
{
	if (canMerge(mergeWith))
	{
		mergeWith->number += number;
		if (mergeWith->number > ITEM_MAXSTACK)
		{
			number = ITEM_MAXSTACK - mergeWith->number;
			mergeWith->number = ITEM_MAXSTACK;

			return false; //you didn't "really" merge, you just gave them some of your stuff
		}

		merged = true;
		return true;
	}
	else
		return false;
}

bool item::collision()
{
	return (global_map->tileSolid(x - ITEM_SIZE / 2, y - ITEM_SIZE / 2) ||
			global_map->tileSolid(x + ITEM_SIZE / 2, y - ITEM_SIZE / 2) ||
			global_map->tileSolid(x - ITEM_SIZE / 2, y + ITEM_SIZE / 2) ||
			global_map->tileSolid(x + ITEM_SIZE / 2, y + ITEM_SIZE / 2));
}

std::string item::getName()
{
	std::string baseName = global_data->getLine(getNameLine());
	if (type != 3)
	{
		unsigned int matName = global_data->getValue(DATA_MATERIAL, material, 4);
		if (matName != DATA_NONE)
			baseName = global_data->getLine(matName) + " " + baseName;
	}
	
	return baseName;
}

std::string item::getDescription()
{
	std::string baseDesc = global_data->getLine(getNameLine() + 1);
	std::string description = "";
	for (unsigned int i = 0; i < baseDesc.length(); i++)
	{
		char c = baseDesc.at(i);
		if (c == '#')
		{
			unsigned int matName = global_data->getValue(DATA_MATERIAL, material, 4);
			if (matName == DATA_NONE)
				description += "STUFF";
			else
				description += global_data->getLine(matName);
		}
		else if (c == '%')
		{
			unsigned int matName = global_data->getValue(DATA_MATERIAL, material, 4);
			if (matName == DATA_NONE)
				description += "STUFF";
			else
				description += global_data->getLine(matName + 1);
		}
		else
			description += c;
	}
	return description;
}

unsigned int item::getNameLine()
{
	switch(type)
	{
	case 0:
		return global_data->getValue(DATA_ITEM, id, 1);
	case 1:
		return global_data->getValue(DATA_ARMOR, id, 4);
	case 2:
		return global_data->getValue(DATA_WEAPON, id, 8);
	case 3:
		return global_data->getValue(DATA_MAGICITEM, id, 4);
	}
	return 0;
}

void item::update (float elapsed)
{
	if (ySpeed != 0)
	{
		ySpeed += elapsed * ITEM_GRAVITY;
		if (ySpeed == 0)
			ySpeed = 0.01f;

		y += ySpeed * elapsed;
		if (collision())
		{
			int dir = (int) (ySpeed / abs(ySpeed));
			//keep going back at intervals of one tilesize until you aren't colliding
			while (collision())
				y -= TILESIZE * dir * 1.0f;

			//keep going until it matches perfectly
			while (!collision())
				y += dir;
			y -= dir;
				
			//stop moving
			if (ySpeed > 0)
				ySpeed = 0;
			else
				ySpeed = 0.01f;
		}
	}
}