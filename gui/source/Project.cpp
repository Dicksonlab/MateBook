#include "Project.hpp"
#include <stdexcept>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QTreeView>
#include "GroupTree.hpp"
#include "ItemTree.hpp"
#include "RuntimeError.hpp"
#include "FileUtilities.hpp"
#include "MateBook.hpp"
#include "../../common/source/serialization.hpp"
#include "Version.hpp"

#if defined(_DEBUG)
//	#include "../../modeltest/source/modeltest.h"
#endif

Project::Project(MateBook* mateBook) :
	version(Version::current),
	mateBook(mateBook),
	unnamed(true),
	groupTree(),
	itemTree()
{
	registerSettings();

	// create a new (!) subdirectory of the temp directory, to store the project files
	QString tempPath = QDir::temp().canonicalPath();
	QString dataDirName;
	do {
		dataDirName = QString("Temporary.") + QString::number(qrand()) + QString(".mbd");
	} while (!QDir(tempPath).mkdir(dataDirName));
	directory = QDir(tempPath + "/" + dataDirName);

	groupTree = new GroupTree(mateBook, this);
	itemTree = new ItemTree(mateBook, this);

	#if defined(_DEBUG)
//		new ModelTest(groupTree, this);
//		new ModelTest(itemTree, this);
	#endif

	connect(itemTree, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(itemTreeWasModified()));
}

Project::Project(MateBook* mateBook, const QString& filePath) :
	version(0),
	mateBook(mateBook),
	filePath(QFileInfo(filePath).canonicalFilePath()),
	unnamed(false),
	groupTree(),
	itemTree()
{
	registerSettings();

	// read the project file
	try {
		projectInfo.importFrom(filePath.toStdString());

		// handle project versions in different ways:
		if (version > Version::current) {
			// refuse to load and ask the user to upgrade the software
			throw RuntimeError(QObject::tr("MateBook needs to be updated to load that project.\nMateBook version: %1\nProject version: %2").arg(Version::current).arg(version));
		}
		if (version < Version::earliestResettableProject) {
			// refuse to load
			throw RuntimeError(QObject::tr("That project is too old to be loaded with this version of MateBook.\nMateBook version: %1\nProject version: %2").arg(Version::current).arg(version));
		}
		if (version < Version::earliestConvertibleProject) {
			//TODO: ask the user if it's ok to reset to "Recoding Finished"
		}
		if (version < Version::current) {
			//TODO: convert to Version::current (make backup?)
		}
	} catch (const std::bad_cast& e) {
		throw RuntimeError(QObject::tr("Could not parse %1.").arg(filePath));
	}

	// see if the data subdirectory exists and try to create it if it doesn't
	QString dataDirName;
	directory = QDir(QFileInfo(filePath).canonicalPath() + "/" + getFileName() + ".mbd");
	if (!directory.exists() && !directory.mkdir(".")) {
		throw RuntimeError(QObject::tr("Cannot create data directory %1.").arg(directory.canonicalPath()));
	}

	groupTree = new GroupTree(mateBook, this);
	itemTree = new ItemTree(mateBook, this);

	connect(itemTree, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(itemTreeWasModified()));
}

Project::~Project()
{
	if (unnamed) {
		try {
			FileUtilities::removeDirectory(directory);
		} catch (...) {
		}
	}
}

QString Project::getFileName() const
{
	return QFileInfo(filePath).fileName();
}

QString Project::getFilePath() const
{
	return filePath;
}

QDir Project::getDirectory() const
{
	// force the QDir object to update its status regarding the existence of directory
	if (!directory.exists()) {
		throw RuntimeError(QObject::tr("Project directory does not exist."));
	}

	return directory;
}

bool Project::addVideo(const QString& fileName)
{
	return itemTree->addVideo(fileName);
}

void Project::removeVideo(const QModelIndex& index)
{
	itemTree->removeRow(index.row(), index.parent());
}

void Project::removeItems(const QModelIndexList& indexes)
{
	std::map<std::pair<int, int>, QModelIndex> sortedRows;	// sorted by depth and then by row-index
	foreach (QModelIndex index, indexes) {
		// determine the depth of the item in the tree
		int depth = 0;
		for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent()) {
			++depth;
		}
		sortedRows[std::make_pair(depth, index.row())] = index;
	}
	// remove them back to front so we delete children before their parents and we don't invalidate indexes
	for (std::map<std::pair<int, int>, QModelIndex>::reverse_iterator iter = sortedRows.rbegin(); iter != sortedRows.rend(); ++iter) {
		removeVideo(iter->second);
	}
}

Item* Project::getItem(const QModelIndex& index) const
{
	return itemTree->getItem(index);
}

int Project::getHeader(const QString& name) const
{
	return itemTree->getColumn(name);
}

bool Project::isUnnamed() const
{
	return unnamed;
}

void Project::save()
{
	projectInfo.exportTo(filePath.toStdString());
	itemTree->serialize();
}

void Project::saveAs(const QString &filePath)
{
	// since we remove the target files if they exist (after asking the user of course),
	// we have to make sure they are not equal to the source files, otherwise we lose our data
	if (!unnamed && QFileInfo(filePath).canonicalFilePath() == getFilePath()) {
		save();
		return;
	}

	// hack to test if we can write to the destination and to make canonicalPath() below work
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		throw RuntimeError(QObject::tr("Cannot open file %1 for writing:\n%2.").arg(filePath).arg(file.errorString()));
	}
	file.close();

	//TODO: handle existing directories, access restrictions, etc...
	//TODO: cannot move the directory while jobs are being executed!!!
	// copy the project data directory
	QString newDirectoryString = QFileInfo(filePath).canonicalPath() + "/" + QFileInfo(filePath).fileName() + ".mbd";
	QDir newDirectory(newDirectoryString);
	if (unnamed) {
		try {
			FileUtilities::moveDirectory(directory, newDirectory);
		} catch (RuntimeError& e) {
			FileUtilities::copyDirectory(directory, newDirectory);
			try {
				FileUtilities::removeDirectory(directory);
			} catch (...) {
			}
		}
	} else {
		FileUtilities::copyDirectory(directory, newDirectory);
	}

	unnamed = false;
	this->filePath = QFileInfo(filePath).canonicalFilePath();
	this->directory = newDirectory;
	save();
}

GroupTree* Project::getGroupTree() const
{
	return groupTree;
}

ItemTree* Project::getItemTree() const
{
	return itemTree;
}

void Project::itemTreeWasModified()
{
	emit projectWasModified();
}

void Project::registerSettings()
{
	projectInfo.add("version", version);
}
