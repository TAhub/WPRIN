#pragma once

//data file constants
#define DATA_SHEET 0
#define DATA_PARTFORM 1
#define DATA_RACE 2
#define DATA_TILE 3
#define DATA_TILESET 4
#define DATA_ENCOUNTERDATA 5
#define DATA_WEAPON 6
#define DATA_ARMOR 7
#define DATA_COLORLIST 8
#define DATA_LIST 9
#define DATA_COLOR 10
#define DATA_MATERIAL 11
#define DATA_PROJECTILE 12
#define DATA_CTYPE 13
#define DATA_HAT 14
#define DATA_BACKGROUNDLAYER 15
#define DATA_AI 16
#define DATA_PARTICLE 17
#define DATA_ITEM 18
#define DATA_RECIPIE 19
#define DATA_MAGIC 20
#define DATA_MAGICITEM 21
#define DATA_PERK 22
#define DATA_ITEMCATEGORY 23
#define DATA_EFFECT 24
#define DATA_TRAP 25
#define DATA_DOOR 26
#define DATA_MAPMOD 27
#define DATA_BOSSDATA 28
#define DATA_BOSSTOOLSTATE 29
#define DATA_TOWNDATA 30
#define DATA_TOWNSHAPE 31
#define DATA_DECORATION 32
#define DATA_SPECIALCHARACTER 33
#define DATA_SONG 34
#define DATA_SOUND 35

//important entries
#define DATA_RACE_PARTNUM 6
#define DATA_TILESET_LAYERNUM 14

#define DATA_INVALID 9999
#define DATA_NONE 9998

class database
{
	std::vector<std::vector<std::vector<int>>> data;
	std::vector<std::string> lines;

public:
	database();
	~database();
	bool valid;
	int getValue(unsigned int category, unsigned int entry, unsigned int value);
	unsigned int getCategorySize(unsigned int category);
	unsigned int getEntrySize(unsigned int category, unsigned int entry);
	std::string getLine(unsigned int lineNum);
};