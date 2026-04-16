#pragma execution_character_set("utf-8")
#include "SoundManager.h"
#include <iostream>
#include <filesystem>

SoundManager::SoundManager() {}

void SoundManager::loadAll(const std::string& assetsPath) {
	std::string soundPath = assetsPath + "/sounds/";
	loadSound(SoundEffect::PlaceStone, soundPath + "place_stone.wav");
	loadSound(SoundEffect::Win, soundPath + "win.wav");
	loadSound(SoundEffect::Lose, soundPath + "lose.wav");
	loadSound(SoundEffect::Click, soundPath + "click.wav");
	loadSound(SoundEffect::Error, soundPath + "error.wav");
	loadSound(SoundEffect::Start, soundPath + "start.wav");
	loadSound(SoundEffect::TimerWarning, soundPath + "timer_warning.wav");

	std::string musicPath = assetsPath + "/music/";
	m_menuMusic = std::make_unique<sf::Music>();
	if (!m_menuMusic->openFromFile(musicPath + "menu_bgm.wav") && !m_menuMusic->openFromFile(musicPath + "menu_bgm.ogg")) {
		std::cout << "[SoundManager] Failed to load menu BGM\n";
		m_menuMusic.reset();
	}
	else {
		m_menuMusic->setLooping(true);
	}

	m_gameMusic = std::make_unique<sf::Music>();
	if (!m_gameMusic->openFromFile(musicPath + "game_bgm.wav") && !m_gameMusic->openFromFile(musicPath + "game_bgm.ogg")) {
		std::cout << "[SoundManager] Failed to load game BGM\n";
		m_gameMusic.reset();
	}
	else {
		m_gameMusic->setLooping(true);
	}
}

void SoundManager::loadSound(SoundEffect effect, const std::string& path) {
	if (!std::filesystem::exists(path)) {
		std::cout << "[SoundManager] Sound not found: " << path << " (skipping)\n";
		return;
	}
	try {
		auto& data = m_sounds[static_cast<int>(effect)];
		if (!data.buffer.loadFromFile(path)) {
			std::cout << "[SoundManager] Failed to load: " << path << "\n";
			m_sounds.erase(static_cast<int>(effect));
			return;
		}
		data.sound = std::make_unique<sf::Sound>(data.buffer);
		data.sound->setVolume(m_volume);
		std::cout << "[SoundManager] Loaded: " << path << "\n";
	}
	catch (const sf::Exception& e) {
		std::cout << "[SoundManager] Failed to load: " << path << " (" << e.what() << ")\n";
	}
	catch (...) {
		std::cout << "[SoundManager] Failed to load: " << path << "\n";
	}
}

void SoundManager::playMenuMusic() {
	if (m_muted) {
		if (m_menuMusic) m_menuMusic->stop();
		if (m_gameMusic) m_gameMusic->stop();
		return;
	}
	if (m_gameMusic && m_gameMusic->getStatus() == sf::SoundStream::Status::Playing) {
		m_gameMusic->stop();
	}
	if (m_menuMusic && m_menuMusic->getStatus() != sf::SoundStream::Status::Playing) {
		m_menuMusic->setVolume(m_volume);
		m_menuMusic->play();
	}
}

void SoundManager::playGameMusic() {
	if (m_muted) {
		if (m_menuMusic) m_menuMusic->stop();
		if (m_gameMusic) m_gameMusic->stop();
		return;
	}
	if (m_menuMusic && m_menuMusic->getStatus() == sf::SoundStream::Status::Playing) {
		m_menuMusic->stop();
	}
	if (m_gameMusic && m_gameMusic->getStatus() != sf::SoundStream::Status::Playing) {
		m_gameMusic->setVolume(m_volume);
		m_gameMusic->play();
	}
}

void SoundManager::stopMusic() {
	if (m_menuMusic) m_menuMusic->stop();
	if (m_gameMusic) m_gameMusic->stop();
}

void SoundManager::play(SoundEffect effect) {
	if (m_muted) return;
	auto it = m_sounds.find(static_cast<int>(effect));
	if (it != m_sounds.end() && it->second.sound) {
		float vol = m_volume;
		if (effect == SoundEffect::Error) {
			vol = 200.f; // wash ur ears
		}
		it->second.sound->setVolume(vol);
		it->second.sound->play();
	}
}

void SoundManager::setVolume(float volume) {
	m_volume = std::clamp(volume, 0.f, 100.f);
	for (auto& [key, data] : m_sounds) {
		if (data.sound) data.sound->setVolume(m_volume);
	}
	if (m_menuMusic) m_menuMusic->setVolume(m_volume);
	if (m_gameMusic) m_gameMusic->setVolume(m_volume);
}

float SoundManager::getVolume() const { return m_volume; }

void SoundManager::setMuted(bool muted) {
	m_muted = muted;
	if (m_muted) {
		stopMusic();
	}
	else {
		// We might need to resume, but simple stop on mute is fine
	}
}
bool SoundManager::isMuted() const { return m_muted; }

