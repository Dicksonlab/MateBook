#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

#include "FileItem.hpp"
#include "ItemMenu.hpp"

ItemMenu::ItemMenu(QWidget* parent) : FancyMenu(parent)
{
	createActions();
}

ItemMenu::~ItemMenu()
{
}

QAction* ItemMenu::exec(const QPoint& p, const QModelIndex& currentIndex, const QModelIndexList& selection, QAction* action)
{
	if(currentIndex.isValid()){
		openResultDirectoryAction->setEnabled(item = dynamic_cast<FileItem*>(Item::getItem(currentIndex)));
		showSettingsUsedAction->setEnabled(item = dynamic_cast<FileItem*>(Item::getItem(currentIndex)));
	}
	return FancyMenu::exec(p, currentIndex, selection, action);
}

void ItemMenu::openResultDirectory()
{
	if (item != 0) {
		QDesktopServices::openUrl(QUrl(QString("file:///") + item->absoluteDataDirectory().absolutePath()));
	}
}

void ItemMenu::showSettingsUsed()
{
	if (!item) {
		return;
	}

	std::map<QString, float> detOptions = item->getPulseDetectionOptions();
	std::map<QString, float> staOptions = item->getSongStatisticsOptions();

	std::stringstream sstr;
	sstr << "<table width='200'>";
	sstr << "<tr><td><b>Pulse Detection Settings:</b></td></tr>";
	if (detOptions.empty()) {
		sstr << "<tr><td>none</td></tr>";
	} else {
		for (std::map<QString, float>::iterator iter = detOptions.begin(); iter != detOptions.end(); ++iter) {
			sstr << "<tr><td>" << (*iter).first.toStdString() << ":"  << "</td><td align='right'>" << (*iter).second << "</td></tr>";
		}
	}
	sstr << "<tr></tr><tr><td><b>Song Analysis Settings:</b></td></tr>";
	if (staOptions.empty()) {
		sstr << "<tr><td>none</td></tr>";
	} else {
		for (std::map<QString, float>::iterator iter = staOptions.begin(); iter != staOptions.end(); ++iter) {
			sstr << "<tr><td>" << (*iter).first.toStdString() << ":"  << "</td><td align='right'>" << (*iter).second << "</td></tr>";
		}
	}
	sstr << "</table>";

	QMessageBox msgBox;
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText(QString::fromStdString(sstr.str()));
	msgBox.exec();
}

void ItemMenu::createActions()
{
	openResultDirectoryAction = new QAction(tr("Open Result Directory"), this);
	openResultDirectoryAction->setToolTip(tr("Opens a file browser to display the directory containing the raw processing results"));
	connect(openResultDirectoryAction, SIGNAL(triggered()), this, SLOT(openResultDirectory()));
	addAction(openResultDirectoryAction);

	showSettingsUsedAction = new QAction(tr("Show Settings Used"), this);
	showSettingsUsedAction->setToolTip(tr("Displays the settings that were used while processing this item"));
	connect(showSettingsUsedAction, SIGNAL(triggered()), this, SLOT(showSettingsUsed()));
	addAction(showSettingsUsedAction);
}
