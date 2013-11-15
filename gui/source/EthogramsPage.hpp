#ifndef EthogramsPage_hpp
#define EthogramsPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

class Settings;
class VerticalWidgetList;

class EthogramsPage : public ConfigPage {
public:
	EthogramsPage(Settings& trackerSettings, QWidget* parent = 0);

	std::string getString() const;	// a string describing all ethograms
	void fromString(const std::string& description);	// set ethograms from the string

private:
	VerticalWidgetList* ethogramAttributes;
};

#endif
