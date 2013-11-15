#ifndef Project_hpp
#define Project_hpp

#include <QString>
#include <QDir>
#include <QSortFilterProxyModel>

#include "../../common/source/Settings.hpp"

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

class Item;
class GroupTree;
class ItemTree;
class MateBook;
class Settings;

/**
  * @class  Project
  * @brief  stores project settings and grabs project resources (e.g. the data directory)
  */
class Project : public QObject {
	Q_OBJECT

public:
	Project(MateBook* mateBook);	// to create a new project
	Project(MateBook* mateBook, const QString& filePath);	// to open an existing project
	~Project();

	QString getFileName() const;
	QString getFilePath() const;
	QDir getDirectory() const;

	bool addVideo(const QString& fileName);
	void removeVideo(const QModelIndex& index);
	void removeItems(const QModelIndexList& indexes);

	Item* getItem(const QModelIndex& index) const;
	int getHeader(const QString& name) const;

	bool isUnnamed() const;

	void save();
	void saveAs(const QString& filePath);

	GroupTree* getGroupTree() const;
	ItemTree* getItemTree() const;

signals:
	void projectWasModified();

private slots:
	void itemTreeWasModified();

private:
	unsigned int version;
	MateBook* mateBook;
	QString filePath;
	QDir directory;
	bool unnamed;
	GroupTree* groupTree;
	ItemTree* itemTree;

	void registerSettings();
	Settings projectInfo;
};

#endif
