#ifndef EventsPage_hpp
#define EventsPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

class Settings;

class EventsPage : public ConfigPage {
public:
	EventsPage(Settings& trackerSettings, QWidget* parent = 0);

private:
};

#endif
