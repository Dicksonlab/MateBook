#ifndef ConfigPage_hpp
#define ConfigPage_hpp

/*
Base class for configuration pages.
Also contains a few helper functions needed for the ConfigDialog and *Page classes.
*/

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QString;
class QButtonGroup;
class QSettings;
class QDoubleSpinBox;
QT_END_NAMESPACE

class Settings;

class ConfigPage : public QWidget {
	Q_OBJECT

public:
	ConfigPage(Settings& trackerSettings, QWidget* parent = 0);

	virtual void readGuiSettings(const QSettings& settings);
	virtual void writeGuiSettings(QSettings& settings) const;

private:
};

QDoubleSpinBox* addRowToFormLayout(Settings& settings, QFormLayout* layout, const QString& key, const QString& labelText, double defaultValue, double minValue, double maxValue, const QString& unit = QString(), const QString& labelToolTip = QString(), const QString& fieldToolTip = QString());
void setCheckedId(QButtonGroup* buttonGroup, int id);

#endif
