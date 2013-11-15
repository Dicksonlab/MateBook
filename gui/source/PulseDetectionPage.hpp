#ifndef PulseDetectionPage_hpp
#define PulseDetectionPage_hpp

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

class PulseDetectionPage : public ConfigPage {
	Q_OBJECT

public:
	PulseDetectionPage(Settings& trackerSettings, QWidget* parent = 0);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	std::map<QString, float> getOptions() const;
	void setOptions(std::map<QString, float> options) const;

private slots:
	void loadOptions();
	void saveOptions();

private:
	std::vector<QWidget*> allOptions;

	QPushButton* btnLoadOptions;
	QPushButton* btnSaveOptions;
};

#endif
