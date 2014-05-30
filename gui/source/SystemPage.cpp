#include <QtGui>
#include <boost/bind.hpp>
#include "SystemPage.hpp"
#include "global.hpp"
#include "JobQueue.hpp"
#include "../../common/source/Settings.hpp"
#include "../../common/source/Singleton.hpp"
#include "../../common/source/system.hpp"

SystemPage::SystemPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QVBoxLayout* processingLayout = new QVBoxLayout;

	QFormLayout* pathLayout = new QFormLayout();

	matlabExecutable = new QComboBox();
	matlabExecutable->setEditable(true);
	#if defined(WIN32)
		// try a few common MATLAB paths
		for (int year = 2009; year < 2099; ++year) {
			QString path = QFileInfo("C:/Program Files/MATLAB/R" + QString::number(year) + "a/bin/matlab.exe").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
			path = QFileInfo("C:/Program Files/MATLAB/R" + QString::number(year) + "b/bin/matlab.exe").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
			path = QFileInfo("C:/Program Files (x86)/MATLAB/R" + QString::number(year) + "a/bin/matlab.exe").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
			path = QFileInfo("C:/Program Files (x86)/MATLAB/R" + QString::number(year) + "b/bin/matlab.exe").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
		}
	#elif defined(__APPLE__)
		// try a few common MATLAB paths
		for (int year = 2009; year < 2099; ++year) {
			QString path = QFileInfo("/Applications/MATLAB_R" + QString::number(year) + "a.app/bin/matlab").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
			path = QFileInfo("/Applications/MATLAB_R" + QString::number(year) + "b.app/bin/matlab").canonicalFilePath();
			if (!path.isEmpty()) {
				matlabExecutable->addItem(path);
			}
		}
	#endif
	pathLayout->addRow(QString("MATLAB: "), matlabExecutable);
	pathLayout->itemAt(pathLayout->count() - 2)->widget()->setToolTip("Path to the MATLAB executable.");
	pathLayout->itemAt(pathLayout->count() - 1)->widget()->setToolTip("Path to the MATLAB executable.");

	trackerExecutable = new QComboBox();
	trackerExecutable->setEditable(true);
	#if defined(WIN32)
		#if defined(_DEBUG)
			trackerExecutable->addItem(QFileInfo(global::executableDir + "/../../../../tracker/binaries/Win32/Release/tracker.exe").canonicalFilePath());
			trackerExecutable->addItem(QFileInfo(global::executableDir + "/../../../../tracker/binaries/Win32/Debug/tracker.exe").canonicalFilePath());
		#else
			trackerExecutable->addItem(QFileInfo(global::executableDir + "/tracker.exe").canonicalFilePath());
		#endif
	#else
		trackerExecutable->addItem(QFileInfo(global::executableDir + "/tracker").canonicalFilePath());
	#endif
	pathLayout->addRow(QString("Tracker: "), trackerExecutable);
	pathLayout->itemAt(pathLayout->count() - 2)->widget()->setToolTip("Path to the tracker executable.");
	pathLayout->itemAt(pathLayout->count() - 1)->widget()->setToolTip("Path to the tracker executable.");

	maxJobsSpinBox = new QSpinBox(this);
	connect(maxJobsSpinBox, SIGNAL(valueChanged(int)), &(Singleton<JobQueue>::instance()), SLOT(setMaxJobs(int)));
	maxJobsSpinBox->setValue(1);
	pathLayout->addRow(QString("Maximum Number of Jobs: "), maxJobsSpinBox);
	pathLayout->itemAt(pathLayout->count() - 2)->widget()->setToolTip(QString("Run at most this many processing jobs concurrently in the background."));
	pathLayout->itemAt(pathLayout->count() - 1)->widget()->setToolTip(QString::number(maxJobsSpinBox->minimum()) + "..." + QString::number(maxJobsSpinBox->maximum()));

	attachDebugger = new QCheckBox(this);
	attachDebugger->setText("Attach Debugger");
	attachDebugger->setToolTip("Attaches the debugger to the tracker process.");
	trackerSettings.add<bool>("debug", boost::bind(&QCheckBox::isChecked, attachDebugger), boost::bind(&QCheckBox::setChecked, attachDebugger, _1));

	liveVisualization = new QCheckBox(this);
	liveVisualization->setText("Live Visualization");
	liveVisualization->setToolTip("Shows the videos being tracked in a separate window.");
	trackerSettings.add<bool>("visualize", boost::bind(&QCheckBox::isChecked, liveVisualization), boost::bind(&QCheckBox::setChecked, liveVisualization, _1));

	processingLayout->addLayout(pathLayout);
	processingLayout->addWidget(attachDebugger);
	processingLayout->addWidget(liveVisualization);

	QGroupBox* processing = new QGroupBox(tr("Local Processing"));
	processing->setCheckable(true);
	processing->setLayout(processingLayout);

	QFormLayout* clusterProcessingLayout = new QFormLayout;
	sshClient = new QComboBox(this);
	sshClient->setEditable(true);
	#if defined(WIN32)
		QString path = QFileInfo("C:\\Program Files (x86)\\PuTTY\\plink.exe").canonicalFilePath();
		if (!path.isEmpty()) {
			sshClient->addItem(path);
		}

		path = QFileInfo("C:\\Program Files\\PuTTY\\plink.exe").canonicalFilePath();
		if (!path.isEmpty()) {
			sshClient->addItem(path);
		}
	#endif
	sshClient->addItem("ssh");
	clusterProcessingLayout->addRow(QString("SSH Client: "), sshClient);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("The SSH client used when submitting jobs."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("Path to an executable."));

	sshTransferHost = new QLineEdit(this);
	clusterProcessingLayout->addRow(QString("Transfer Host: "), sshTransferHost);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("The machine to contact when submitting jobs."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("Host name or IP address."));

	sshHostKey = new QLineEdit(this);
	clusterProcessingLayout->addRow(QString("Host Key: "), sshHostKey);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("The key which guarantees the host identity."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("In PuTTY format as used at HKCU/Software/SimonTatham/PuTTY/SshHostKeys in the registry."));

	sshUsername = new QLineEdit(this);
	try {
		sshUsername->setText(QString::fromStdString(getUserName()));
	} catch (const std::runtime_error&) {
		// cannot get the username so we just leave the field empty
	}
	clusterProcessingLayout->addRow(QString("Username: "), sshUsername);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("The user for whom to launch jobs."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("UNIX username."));

	sshPrivateKey = new QComboBox(this);
	sshPrivateKey->setEditable(true);
	try {
		// sshPrivateKey->addItem(QString("\\\\gaudi\\gaudihome\\") + QString::fromStdString(getUserName()) + "\\keys\\id_rsa.ppk");
		// sshPrivateKey->addItem(QString("C:\\Users\\") + QString::fromStdString(getUserName()) + "\\Documents\\keys\\id_rsa.ppk");
	} catch (const std::runtime_error&) {
		// cannot get the username so we just leave the field empty
	}
	clusterProcessingLayout->addRow(QString("Private Key: "), sshPrivateKey);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("A file containing the private key with which one can connect to the transfer host."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("Overrides the password."));

	sshEnvironment = new QLineEdit(this);
	// sshEnvironment->setText("source /sw/lenny/etc/sge-aragon.bash");
	sshEnvironment->setText("source /sge/current/default/common/settings.sh");
	clusterProcessingLayout->addRow(QString("Remote Environment: "), sshEnvironment);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("The environment in which cluster jobs are submitted."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString("Code that will be executed remotely before submitting jobs."));

	pollingInterval = new QSpinBox(this);
	pollingInterval->setRange(10, 3600);
	pollingInterval->setValue(60);
	pollingInterval->setSuffix(" s");
	clusterProcessingLayout->addRow(QString("Polling Interval: "), pollingInterval);
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 2)->widget()->setToolTip(QString("How long to wait between cluster job status checks."));
	clusterProcessingLayout->itemAt(clusterProcessingLayout->count() - 1)->widget()->setToolTip(QString::number(pollingInterval->minimum()) + "..." + QString::number(pollingInterval->maximum()));

	clusterProcessing = new QGroupBox(tr("Cluster Processing"));
	clusterProcessing->setCheckable(true);
	clusterProcessing->setLayout(clusterProcessingLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(processing);
	mainLayout->addWidget(clusterProcessing);
	mainLayout->addStretch(1);
	setLayout(mainLayout);

	connect(sshHostKey, SIGNAL(editingFinished()), this, SLOT(commitSshHostKey()));
}

void SystemPage::readGuiSettings(const QSettings& settings)
{
	attachDebugger->setChecked(settings.value("attachDebugger", false).toBool());
	liveVisualization->setChecked(settings.value("liveVisualization", false).toBool());
	int maxJobs = settings.value("maxJobsSpinBox", QThread::idealThreadCount()).toInt();
	maxJobsSpinBox->setValue(maxJobs < 0 ? 1 : maxJobs);
	sshTransferHost->setText(settings.value("sshTransferHost", "albert").toString());
	sshHostKey->setText(settings.value("sshHostKey", "0x23,0xb8573b6e21dfd2b72fe747d43ccc5572f53c74f8e99230b8c81bef31f3be74ae4d5692c08a62ff2b72e0a9c2d0a3d7ee0220213f3954e53e5ea9131235e9d9e5111e0a9e5c1e8e5e126cab0394c4f3ca0d241f87b4f6acf0583146f9881a559b3e59f3aab0ece7623bb0ecc439048d9ddaa2f8847b8207769e4e6d90c3deebd8f6ce993a1d2e093706845685ef13af299005422992d8e284e3f427316b81ae4bef6c435afebce66d4e1e703061a8cf6410928416420d2aaf46dc588c33672cfca890a7fc2ad6ffd0fa471d93fe8543ee99ddc69f1d1cfe7dac633e9b09977d52e5ce0de288e22faf33d94e2ac0eb58ee60d8a106e985213b7e0eb549e1dd119d").toString());
	commitSshHostKey();
}

void SystemPage::writeGuiSettings(QSettings& settings) const
{
	settings.setValue("attachDebugger", attachDebugger->isChecked());
	settings.setValue("liveVisualization", liveVisualization->isChecked());
	settings.setValue("maxJobsSpinBox", maxJobsSpinBox->value());
	settings.setValue("sshTransferHost", sshTransferHost->text());
	settings.setValue("sshHostKey", sshHostKey->text());
}

QString SystemPage::getMatlabExecutable() const
{
	return matlabExecutable->currentText();
}

QString SystemPage::getTrackerExecutable() const
{
	return trackerExecutable->currentText();
}

QString SystemPage::getArgumentPrefix() const
{
	return "--";
}

bool SystemPage::isClusterProcessingEnabled() const
{
	return clusterProcessing->isChecked();
}

QString SystemPage::getSshClient() const
{
	return sshClient->currentText();
}

QString SystemPage::getSshTransferHost() const
{
	return sshTransferHost->text();
}

QString SystemPage::getSshHostKey() const
{
	return sshHostKey->text();
}

QString SystemPage::getSshUsername() const
{
	return sshUsername->text();
}

QString SystemPage::getSshPrivateKey() const
{
	return sshPrivateKey->currentText();
}

QString SystemPage::getSshEnvironment() const
{
	return sshEnvironment->text();
}

int SystemPage::getPollingInterval() const
{
	return pollingInterval->value();
}

void SystemPage::commitSshHostKey()
{
	#if defined(WIN32)
		QSettings settingsPuTTY("HKEY_CURRENT_USER\\Software\\SimonTatham\\PuTTY\\SshHostKeys", QSettings::NativeFormat);
		settingsPuTTY.setValue("rsa2@22:" + getSshTransferHost(), getSshHostKey());
	#endif
}
