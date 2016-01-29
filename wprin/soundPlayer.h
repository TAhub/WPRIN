#pragma once

#include <SFML/Audio.hpp>

class soundPlayer
{
	std::vector<sf::SoundBuffer *> buffers;
	std::vector<sf::Sound *> sounds;
public:
	soundPlayer();
	~soundPlayer();

	void play (unsigned int id);

	bool valid;
};