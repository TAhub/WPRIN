#pragma once
#include "characterDesigner.h"
#include "creature.h"
#include "drawer.h"
#include "projectile.h"
#include "damageNumber.h"
#include "particle.h"
#include "trap.h"
#include <SFML/Network.hpp>

#define TILESIZE 50
#define CAMERASPEED 500

//multiplayer stuff
#define MULTIP_EXPMOD 0.75f
#define MULTIP_FOODMOD 1.5f

//generator constants
#define GENERATOR_MAPMODCHANCE 65
#define GENERATOR_TILESETINTERVAL 8
#define GENERATOR_HMULT 2
#define GENERATOR_PLATFORMSTART 10
#define GENERATOR_PLATFORMMIN 5
#define GENERATOR_PLATFORMMAX 15
#define GENERATOR_JUMPMINV 2
#define GENERATOR_JUMPMAXV 4
#define GENERATOR_JUMPMINH 1
#define GENERATOR_JUMPMAXH 2
#define GENERATOR_CEILYADD 5
#define GENERATOR_CEILXADD 5
#define GENERATOR_LONGJUMPCHANCE 45
#define GENERATOR_LONGJUMPLENGTH 2
#define GENERATOR_TOPBORDER 10
#define GENERATOR_TOPBORDERSOLID 3
#define GENERATOR_SIDEBORDER 1
#define GENERATOR_BOTTOMBORDER 2
#define GENERATOR_MAXTRIES 100000
#define GENERATOR_NUMDOORS 4
#define GENERATOR_NUMTOWNDOORS 2
#define GENERATOR_TOWNWIDTH 12
#define GENERATOR_TOWNCHUNKWIDTH 2
#define GENERATOR_TOWNHEIGHT 3
#define GENERATOR_SHOPITEMS 4
#define GENERATOR_SHOPXOFF 40
#define GENERATOR_DECORATIONSPERPLATFORM 0.65f

//event log stuff
#define EVENT_LOG_PROJECTILEMAKE 0.0f
#define EVENT_LOG_PROJECTILEDESTROY 1.0f
#define EVENT_LOG_DAMAGE 2.0f
#define EVENT_LOG_MAPCHANGE 3.0f
#define EVENT_LOG_MAPDATA 4.0f
#define EVENT_LOG_ITEMDROP 5.0f
#define EVENT_LOG_PICKUPREQUEST 6.0f
#define EVENT_LOG_GETITEM 7.0f
#define EVENT_LOG_REMOVEITEM 8.0f
#define EVENT_LOG_PLAYERUPDATE 9.0f
#define EVENT_LOG_MAGIC 10.0f
#define EVENT_LOG_DEACTIVATETRAP 11.0f
#define EVENT_LOG_SUMMON 12.0f
#define EVENT_LOG_DIALOGUE 13.0f
#define EVENT_LOG_MUSIC 14.0f
#define EVENT_LOG_SOUND 15.0f
#define EVENT_LOG_END 99.0f

#define SAFETYTELEPORTWIDTH 20

//network stuff
#define NETWORK_PORT_TCP 53000
//#define NETWORK_PORT_UDP_SERVER 53001
//#define NETWORK_PORT_UDP_CLIENT 53002

struct pulse
{
	float time, maxTime;
	unsigned int type, targets, pulses;
	creature *center;
};

struct decoration
{
	void save (sf::Packet *saveTo);
	decoration (sf::Packet *loadFrom);
	decoration(unsigned int x, unsigned int y, unsigned int id);
	bool onScreen();
	bool isForge();
	void render (drawer *draw);
	void update (float elapsed);
	unsigned int x, y, id;
	float timer;
};

struct door
{
	void save (sf::Packet *saveTo);
	door (sf::Packet *loadFrom);
	door (unsigned int oldNum, unsigned int oldType, unsigned int x, unsigned int y);
	unsigned int tileset, x, y, mapMod, type, num;
	bool faction;
	void render (drawer *draw);
};

class map
{
	//stats
	unsigned int width, height, tileset, mapNum, type;
	bool faction;

	//contents
	unsigned int *tiles;
	std::vector<creature *> creatures;
	std::vector<projectile *> projectiles;
	std::vector<damageNumber *> dNums;
	std::vector<particle *> particleList;
	std::vector<item *> items;
	std::vector<trap *> traps;
	std::vector<pulse *> pulses;
	std::vector<door *> doors;
	std::vector<decoration *> decorations;
	sf::Packet eventLog;

	//transitory things
	characterDesigner *cD;
	bool disconnected;

	//ending data
	bool killedWprin;

	//netcode
	sf::TcpSocket *socket;
	//sf::UdpSocket fastSocket;
	//sf::IpAddress udpTo;
	//unsigned short udpPortTo;
	bool master;
	bool single;
	unsigned int identifier;

	void mapShadow(unsigned int x, unsigned int y, drawer *draw);
	void backgroundLayer(unsigned int layer, drawer *draw);

	//map handling
	door *usedDoor;
	void mapClear(bool sparePlayers);
	bool mapGenerateInner(std::vector<creature *> savedCreatures, unsigned int mapMod);
	void mapGenerate();
	void mapRetrieve(sf::Packet *loadFrom = NULL);
	void mapSend(sf::Packet *sendTo = NULL);
	std::vector<unsigned int> mapChange;

	void receivePacket();

	//utility functions
	unsigned int toX(unsigned int i) {return i % width;}
	unsigned int toY(unsigned int i) {return i / width;}
	unsigned int toI(unsigned int x, unsigned int y) {return x + y * width;}
	void addCreature(creature *cr);
	void pollCamera(float elapsed);
	void makeEventLog();
	void readEventLog(sf::Packet *log);
	void killProjectile(unsigned int idToKill);
	unsigned int numThings (float thingsPer);
	void freePlace(unsigned int trapType, unsigned int creatureType, std::vector<std::vector<int>> *platforms, unsigned int finalPlatform);
	bool generatePlatforms(std::vector<std::vector<int>> *platforms, std::vector<std::vector<int>> *jumpGaps, unsigned int desiredPlatforms);
	void townNPCs(unsigned int townShape, bool top);
	void townDecorations(unsigned int townShape, bool top);
	sf::Vector2u townPos(unsigned int i, bool top);
	sf::Vector2u townPosOf(unsigned int id, unsigned int townShape);
	void generateTown(std::vector<std::vector<int>> *platforms, std::vector<std::vector<int>> *jumpGaps, unsigned int townShape);
	bool generateEnemy(std::vector<std::vector<int>> *platforms, std::vector<std::vector<unsigned int>> *enemiesP, unsigned int list, unsigned int bossN, float thingsPer);
	bool generateTrap(std::vector<std::vector<int>> *platforms, std::vector<std::vector<unsigned int>> *trapsP, unsigned int list, float thingsPer);
	bool generateDecoration(std::vector<std::vector<int>> *platforms, std::vector<unsigned int> *decorationsP, unsigned int list, bool forge);
	bool lastMap () { return type == 1 && (mapNum + 1) == GENERATOR_TILESETINTERVAL * 4; }
	void checkWprinKill();
public:
	//main functions
	map(sf::TcpSocket *socket, bool master);
	~map();
	void update(float elapsed);
	void networkAction();
	//void networkUpdateReceive();
	//void networkUpdateSend();
	void render(drawer *draw);

	//player functions
	void playerUpdate();

	//projectile functions
	void addProjectile(float x, float y, float xDir, float yDir, unsigned int type, unsigned int damage, unsigned int attack, unsigned int side, unsigned int magic, bool venemous);

	//damage number functions
	void addDamageNumber(creature *cr, int amount, float hitX, float hitY, unsigned int sound);
	
	//particle functions
	void addParticles(unsigned int type, unsigned int particles, float x, float y, float radius);
	void addParticles(unsigned int type, unsigned int particles, sf::FloatRect *customArea);

	//item functions
	void dropItem(item *item, float x, float y);
	void pickupItem(unsigned int i);

	//trap functions
	void deactivateTrap(trap *toDeactivate);

	//query functions
	bool tileSolid(float x, float y, int yDir = 0, float refY = -1, bool dropMode = false);

	//magic functions
	void magic(creature *center, unsigned int type, unsigned short targets, bool original = true, bool pulsed = false);

	//death reward functions
	void deathReward(unsigned int amount);

	//summoning functions
	void summon(float x, float y, unsigned int cType, bool boss, bool loud);

	//dialogue functions
	void forceDialogue(unsigned int id);

	//sound functions
	void forceMusic(unsigned int id);
	void forceSound(unsigned int id);

	//misc accessors
	bool isNetworked() { return !single; }
	creature *getCreature(unsigned int i) { return creatures[i]; }
	unsigned int getNumCreatures() { return creatures.size(); }
	item *getItem(unsigned int i) { return items[i]; }
	unsigned int getNumItems() { return items.size(); }
	float getHeight() { return height * TILESIZE * 1.0f; }
	float getWidth() { return width * TILESIZE * 1.0f; }
	bool mapEnd(float x, float y);
	bool ended() { for (unsigned int i = 0; i < mapChange.size(); i++) {if (mapChange[i] == identifier) return true; } return false; }
	unsigned int getIdentifier() { return identifier; }
	void teleportToSafety(creature *cr);
	bool ready() { return cD == NULL; }
	unsigned int getTileset() { return tileset; }
	bool isTown() { return type == 2; }
	bool isBoss() { return type == 1; }
	bool isMultiplayer() { return !single; }
	bool dialogueActive() { return creatures[identifier]->dialogueActive(); }
	bool forgeAt(float x, float y);
	unsigned int endState();
};