#ifndef DisplayPage_hpp
#define DisplayPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"
#include "ColorButton.hpp"

class Settings;

class DisplayPage : public ConfigPage {
public:
	DisplayPage(Settings& trackerSettings, QWidget* parent = 0);

private:
	ColorButton* fly0ColorButton;
	ColorButton* fly1ColorButton;
};

#endif
