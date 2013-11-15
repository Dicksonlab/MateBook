#ifndef SongAnalysisPage_hpp
#define SongAnalysisPage_hpp

#include <QWidget>
#include <vector>
#include <map>
#include "ConfigPage.hpp"

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QString;
class QAbstractSpinBox;
QT_END_NAMESPACE

class Settings;

class SongAnalysisPage : public ConfigPage {
	Q_OBJECT

public:
	SongAnalysisPage(Settings& trackerSettings, QWidget* parent = 0);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	std::map<QString, float> getOptions();
	void setOptions(std::map<QString, float> options);

private slots:
	void loadOptions();
	void saveOptions();

private:
	std::vector<QAbstractSpinBox*> allOptions;

	QPushButton* btnLoadOptions;
	QPushButton* btnSaveOptions;
};

#endif
