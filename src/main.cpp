#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include "SaveUtils.hpp"

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
	bool init(GJGameLevel * level, bool challenge) {

		if (!LevelInfoLayer::init(level, challenge)) return false;

		if (!Mod::get()->getSettingValue<bool>("enable-button")) return true;

		auto menu = this->getChildByID("left-side-menu");
		if (!menu) return true;

		auto timeSettings = CCMenuItemSpriteExtra::create(
			CCSprite::create("TTSettingsButton.png"_spr),
			this, menu_selector(MyLevelInfoLayer::onTimeSettings)
		);
		timeSettings->setID("time-settings"_spr);

		menu->addChild(timeSettings);
		menu->updateLayout();

		return true;
	}
	void onTimeSettings(CCObject*) {
		std::string description;
		std::vector<int> timeObj = Mod::get()->getSavedValue<std::vector<int>>(std::to_string(m_level->m_levelID.value()), std::vector<int>());

		if (timeObj.empty()) return FLAlertLayer::create("Time Played", "No time recorded", "OK")->show();

		bool hhmmssFormat = Mod::get()->getSettingValue<bool>("hhmmss-time-format");
		bool hoursFormat = Mod::get()->getSettingValue<bool>("hours-only-time-format");
		bool timeWithPaused = Mod::get()->getSettingValue<bool>("time-with-paused");
		bool timeWithoutPaused = Mod::get()->getSettingValue<bool>("time-without-paused");

		if (!(timeWithoutPaused || timeWithPaused)) return FLAlertLayer::create("Time Played", "You need to select either a 'total time' or 'excluding paused time' option in settings!", "OK")->show();

		if (timeWithPaused) {
			int seconds = timeObj[0];
			int minutes = seconds / 60;
			int hours = seconds / 3600;

			description += "<cy>Total Time</c>:\n";
			if (hhmmssFormat) {
				description += std::to_string(hours) + " hours, ";
				description += std::to_string(minutes % 60) + " minutes, ";
				description += std::to_string(seconds % 60) + " seconds\n";
			}
			if (hoursFormat) {
				char buffer[20];
				std::sprintf(buffer, "%.3f", seconds / 3600.0f);
				if (hhmmssFormat) description += "or ";
				description.append(buffer);
				description += " hours\n";
			}
		}
		if (timeWithoutPaused) {
			int seconds = timeObj[0] - timeObj[1];
			int minutes = seconds / 60;
			int hours = seconds / 3600;

			description += "<cy>Time Excluding Pause Menu</c>:\n";
			if (hhmmssFormat) {
				description += std::to_string(hours) + " hours, ";
				description += std::to_string(minutes % 60) + " minutes, ";
				description += std::to_string(seconds % 60) + " seconds\n";
			}
			if (hoursFormat) {
				char buffer[20];
				std::sprintf(buffer, "%.3f", seconds / 3600.0f);
				if (hhmmssFormat) description += "or ";
				description.append(buffer);
				description += " hours\n";
			}
		}
		FLAlertLayer::create("Time Played", description, "OK")->show();
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	struct Fields {
		std::chrono::steady_clock::time_point m_sessionStart = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point m_pausePoint = std::chrono::steady_clock::now();
		std::chrono::seconds m_pauseTime = std::chrono::seconds::zero();
		bool m_loggingPaused = false;
	};
	bool init(GJGameLevel * level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
		m_fields->m_sessionStart = std::chrono::steady_clock::now();
		return true;
	}
	void updatePauseTime() {
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration duration = now - m_pausePoint;
		std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
		m_pauseTime += seconds;
		m_loggingPaused = false;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration duration = now - m_fields->m_sessionStart;
		std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

		if (m_isPaused && m_fields->m_loggingPaused) MyPlayLayer::updatePauseTime();

		int secondsPlayed = seconds.count();
		int secondsPaused = m_fields->m_pauseTime.count();

		std::vector<int> times = {secondsPlayed, secondsPaused};
		SaveUtils::addTime(m_level, times);
	}
	void onQuit() {
		MyPlayLayer::updatePauseTime();
		PlayLayer::onQuit();
	}
	void pauseGame(bool bl) {
		m_fields->m_pausePoint = std::chrono::steady_clock::now();
		m_fields->m_loggingPaused = true;
		PlayLayer::pauseGame(bl);
	}
	void resume() {
		MyPlayLayer::updatePauseTime();
		PlayLayer::resume();
	}
};