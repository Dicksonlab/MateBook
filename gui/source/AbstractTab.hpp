#ifndef AbstractTab_hpp
#define AbstractTab_hpp

#include <QWidget>
#include <QList>
#include <QModelIndex>
#include <boost/shared_ptr.hpp>

QT_BEGIN_NAMESPACE
class QTreeView;
class QSortFilterProxyModel;
class QSettings;
QT_END_NAMESPACE

class Item;
class Project;

class AbstractTab : public QWidget
{
	Q_OBJECT

public:
	AbstractTab(QWidget* parent = 0);
	virtual ~AbstractTab();

	virtual void readGuiSettings(const QSettings& settings);
	virtual void writeGuiSettings(QSettings& settings);
	virtual void readHeaderSettings();
	virtual void writeHeaderSettings();

	virtual void setProject(Project* project) = 0;
	virtual void setCurrentItem(Item* item = NULL) = 0;

signals:
	void showStatusMessage(const QString& text);
	void projectWasModified();

public slots:
	virtual void enter() = 0;
	virtual void leave() = 0;

	virtual void cut() = 0;
	virtual void copy() = 0;
	virtual void paste() = 0;
	virtual void del() = 0;

	virtual void viewWasModified();

//	virtual void toBeDeleted(Item* item) = 0;
//	virtual void toBeClosed(boost::shared_ptr<Project> currentProject) = 0;

//	virtual void save() = 0;

private slots:

private:
};

#endif
