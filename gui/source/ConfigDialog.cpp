#include <QtGui>
#include "ConfigDialog.hpp"
#include "ArenaDetectionPage.hpp"
#include "FlyTrackingPage.hpp"
#include "EventsPage.hpp"
#include "EthogramsPage.hpp"
#include "BehaviorAnalysisPage.hpp"
#include "PulseDetectionPage.hpp"
#include "SongAnalysisPage.hpp"
#include "DisplayPage.hpp"
#include "SystemPage.hpp"
#include "../../common/source/Settings.hpp"
#include "../../common/source/debug.hpp"

ConfigDialog::ConfigDialog(Settings& trackerSettings, QWidget* parent, Qt::WindowFlags flags) : QDialog(parent, flags)
{
	resize(600, 400);

	contentsWidget = new QListWidget;
	contentsWidget->setMaximumWidth(128);
	
	pagesWidget = new QStackedWidget;

	{
		pagesWidget->addWidget(arenaDetectionPage = new ArenaDetectionPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Arena Detection"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(new FlyTrackingPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Fly Tracking"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(new EventsPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Events"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(new EthogramsPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Ethograms"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(behaviorAnalysisPage = new BehaviorAnalysisPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Behavior Analysis"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(pulseDetectionPage = new PulseDetectionPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Pulse Detection"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(songAnalysisPage = new SongAnalysisPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Song Analysis"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(new DisplayPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("Display"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}

	{
		pagesWidget->addWidget(systemPage = new SystemPage(trackerSettings));
		QListWidgetItem* button = new QListWidgetItem(contentsWidget);
		button->setIcon(QIcon(":/mb/icons/open.png"));
		button->setText(tr("System"));
		button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}
	
	QPushButton* closeButton = new QPushButton(tr("Close"));
	
	connect(contentsWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

	contentsWidget->setCurrentRow(0);
	
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(contentsWidget);
	horizontalLayout->addWidget(pagesWidget, 1);
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(closeButton);
	
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(horizontalLayout);
	mainLayout->addStretch(1);
	mainLayout->addSpacing(12);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);
	
	setWindowTitle(tr("MateBook Settings"));
}
/*
QSize ConfigDialog::minimumSizeHint() const
{
	return QSize(800, 600);
}

QSize ConfigDialog::sizeHint() const
{
	return QSize(800, 600);
}
*/
void ConfigDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current) {
		current = previous;
	}

	pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void ConfigDialog::readGuiSettings(const QSettings& settings)
{
	for (int pageIndex = 0; pageIndex != pagesWidget->count(); ++pageIndex) {
		assert_cast<ConfigPage*>(pagesWidget->widget(pageIndex))->readGuiSettings(settings);
	}
}

void ConfigDialog::writeGuiSettings(QSettings& settings) const
{
	for (int pageIndex = 0; pageIndex != pagesWidget->count(); ++pageIndex) {
		assert_cast<ConfigPage*>(pagesWidget->widget(pageIndex))->writeGuiSettings(settings);
	}
}

float ConfigDialog::getArenaDiameter() const
{
	return arenaDetectionPage->getArenaDiameter();
}

std::vector<QImage> ConfigDialog::getHeatmaps(const std::vector<ArenaItem*>& arenaItems) const
{
	return behaviorAnalysisPage->getHeatmaps(arenaItems);
}

std::map<QString, float> ConfigDialog::getPulseDetectionSettings() const
{
	return pulseDetectionPage->getOptions();
}

std::map<QString, float> ConfigDialog::getSongAnalysisSettings() const
{
	return songAnalysisPage->getOptions();
}

QString ConfigDialog::getMatlabExecutable() const
{
	return systemPage->getMatlabExecutable();
}

QString ConfigDialog::getTrackerExecutable() const
{
	return systemPage->getTrackerExecutable();
}

QString ConfigDialog::getArgumentPrefix() const
{
	return systemPage->getArgumentPrefix();
}

bool ConfigDialog::isClusterProcessingEnabled() const
{
	return systemPage->isClusterProcessingEnabled();
}

QString ConfigDialog::getSshClient() const
{
	return systemPage->getSshClient();
}

QString ConfigDialog::getSshTransferHost() const
{
	return systemPage->getSshTransferHost();
}

QString ConfigDialog::getSshUsername() const
{
	return systemPage->getSshUsername();
}

QString ConfigDialog::getSshPrivateKey() const
{
	return systemPage->getSshPrivateKey();
}

QString ConfigDialog::getSshEnvironment() const
{
	return systemPage->getSshEnvironment();
}

int ConfigDialog::getPollingInterval() const
{
	return systemPage->getPollingInterval();
}
