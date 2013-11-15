#ifndef ConfigDialog_hpp
#define ConfigDialog_hpp

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
QT_END_NAMESPACE

class Settings;
class ArenaItem;
class ArenaDetectionPage;
class BehaviorAnalysisPage;
class PulseDetectionPage;
class SongAnalysisPage;
class SystemPage;

class ConfigDialog : public QDialog {
	Q_OBJECT

public:
	ConfigDialog(Settings& trackerSettings, QWidget* parent = 0, Qt::WindowFlags flags = 0);

	//TODO: size hints do not work here. Why?
//	QSize minimumSizeHint() const;
//	QSize sizeHint() const;

	void readGuiSettings(const QSettings& settings);
	void writeGuiSettings(QSettings& settings) const;

	// ArenaDetectionPage
	float getArenaDiameter() const;	// for custom arenas added by the user

	// BehaviorAnalysisPage
	std::vector<QImage> getHeatmaps(const std::vector<ArenaItem*>& arenaItems) const;

	//TODO: change this ugly interface?
	std::map<QString, float> getPulseDetectionSettings() const;
	std::map<QString, float> getSongAnalysisSettings() const;

	// SystemPage
	QString getMatlabExecutable() const;
	QString getTrackerExecutable() const;
	QString getArgumentPrefix() const;
	bool isClusterProcessingEnabled() const;
	QString getSshClient() const;
	QString getSshTransferHost() const;
	QString getSshUsername() const;
	QString getSshPrivateKey() const;
	QString getSshEnvironment() const;
	int getPollingInterval() const;

public slots:
	void changePage(QListWidgetItem* current, QListWidgetItem* previous);

private:
	ArenaDetectionPage* arenaDetectionPage;
	BehaviorAnalysisPage* behaviorAnalysisPage;
	PulseDetectionPage* pulseDetectionPage;
	SongAnalysisPage* songAnalysisPage;
	SystemPage* systemPage;

	QListWidget* contentsWidget;
	QStackedWidget* pagesWidget;
};

#endif
