#pragma once

#include "drawer.h"
#include "creature.h"

#define MELEEDETECTH 60
#define MELEEDETECTV 30
#define ACQUIRERANGE 450
#define CLOSERANGE 30
#define MEDIUMRANGE 200
#define FORGETRANGE 600

#define BOSSMUSIC 4

//archer stuff
#define ARCHERFREQUENCY 1.5f

//manhack stuff
#define ORBITRANGEMAX 220
#define ORBITRANGEMIN 160
#define ORBITSPEEDMIN 1.0f
#define ORBITSPEEDMAX 1.2f
#define ORBITTIMERSPEED 0.3f

//flower stuff
#define FLOWERFREQUENCY 4.0f

//animal stuff
#define ANIMALDIRECTIONCHANGEMIN 0.4f
#define ANIMALDIRECTIONCHANGEMAX 1.2f

//copter stuff
#define COPTERBACKOFF 0.5f

//retreat stuff
#define RETREATPOINT 0.5f
#define RETREATDUR 1.5f
#define STOPRETREATDIS 60
#define RETREATMAGICTIMER 0.7f
#define RETREATSPEEDBONUS 1.2f

#define BIGJUMP 1.2f
#define HOP 0.8f

#define BOSSTOOLM 0.01f

class enemy: public creature
{
	//ai type stuff
	unsigned int AItype;
	bool canJump, defenseHops, retreatMagic, teleportMagic;
	unsigned int maxRange, magic;

	//misc stuff
	bool firstFrame;

	//AI variables
	float timer, angle, orbitRange, orbitSpeed, retreatTimer, bossTimer;
	int hopDir, wanderDir, retreatPhase, bossPhase, bossProb, bossResource;
	creature *target;

	//AI scripts
	void basicAIStart();
	void basicAI(float elapsed);
	void archerAIStart();
	void archerAI(float elapsed);
	void manhackAIStart();
	void manhackAI(float elapsed);
	void manhackAIOrbitDetails();
	void flowerAIStart();
	void flowerAI(float elapsed);
	void animalAIStart();
	void animalAI(float elapsed);
	void bigWolfAIStart();
	void bigWolfAI(float elapsed);
	void wolfMakerAIStart();
	void wolfMakerAI(float elapsed);
	void wolfMadeAIStart();
	void wolfMadeAI(float elapsed);
	void neoRollerAIStart();
	void neoRollerAI(float elapsed);
	void copterAIStart();
	void copterAI(float elapsed);
	void wprinAIStart();
	void wprinAI(float elapsed);
	void kingAIStart();
	void kingAI(float elapsed);

	//AI utility functions
	creature *inRange(unsigned int distance);
	void forceTarget();
	creature *sampleP();
	void manhackTargetAcquire();
	void bossToolAnimSlideScale(unsigned int startFrame, unsigned int endFrame, float progress);
public:
	void update (float elapsed);
	enemy(float x, float y, unsigned int cType, bool boss);
	bool active() { return target != NULL; }
	void drop();
	void render (drawer *draw, float xAt = -DATA_NONE, float yAt = -DATA_NONE);
};