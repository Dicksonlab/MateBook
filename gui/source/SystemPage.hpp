#ifndef SystemPage_hpp
#define SystemPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QGroupBox;
QT_END_NAMESPACE

class Settings;

class SystemPage : public ConfigPage {
	Q_OBJECT

public:
	SystemPage(Settings& trackerSettings, QWidget* parent = 0);

	void readGuiSettings(const QSettings& settings);
	void writeGuiSettings(QSettings& settings) const;

	// local processing
	QString getMatlabExecutable() const;
	QString getTrackerExecutable() const;
	QString getArgumentPrefix() const;

	// cluster processing
	bool isClusterProcessingEnabled() const;
	QString getSshClient() const;
	QString getSshTransferHost() const;
	QString getSshHostKey() const;
	QString getSshUsername() const;
	QString getSshPrivateKey() const;
	QString getSshEnvironment() const;
	int getPollingInterval() const;

private slots:
	void commitSshHostKey();

private:
	QComboBox* matlabExecutable;
	QComboBox* trackerExecutable;
	QSpinBox* maxJobsSpinBox;
	QCheckBox* attachDebugger;
	QCheckBox* liveVisualization;
	QComboBox* sshClient;
	QLineEdit* sshTransferHost;
	QLineEdit* sshHostKey;
	QLineEdit* sshUsername;
	QComboBox* sshPrivateKey;
	QLineEdit* sshEnvironment;
	QSpinBox* pollingInterval;
	QGroupBox* clusterProcessing;
};

#endif
