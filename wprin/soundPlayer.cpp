#include "stdafx.h"

extern database* global_data;
extern unsigned int* global_soundVol;

soundPlayer::soundPlayer()
{
	valid = true;
	for (unsigned int i = 0; i < global_data->getCategorySize(DATA_SOUND); i++)
	{
		sf::SoundBuffer *buf = new sf::SoundBuffer();
		buffers.push_back(buf);
		if (!buf->loadFromFile("sound/" + global_data->getLine(global_data->getValue(DATA_SOUND, i, 0))))
		{
			std::cout << "Unable to load sound file " << global_data->getLine(global_data->getValue(DATA_SOUND, i, 0)) << std::endl;
			valid = false;
			return;
		}
	}
}

soundPlayer::~soundPlayer()
{
	for (unsigned int i = 0; i < buffers.size(); i++)
		delete buffers[i];
	for (unsigned int i = 0; i < sounds.size(); i++)
	{
		sf::Sound *snd = sounds[i];
		snd->stop();
		delete snd;
	}
}

void soundPlayer::play (unsigned int id)
{
	//clean up old sounds
	std::vector <sf::Sound *> newSounds;
	for (unsigned int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i]->getStatus() == sf::Sound::Playing)
			newSounds.push_back(sounds[i]);
		else
			delete sounds[i]; //clean up the memory
	}
	sounds = newSounds;

	//play the new sound
	sf::Sound *snd = new sf::Sound(*(buffers[id]));
	snd->setVolume(*global_soundVol * 1.0f);
	snd->play();
	sounds.push_back(snd);
}