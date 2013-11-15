#ifndef GroupsTab_hpp
#define GroupsTab_hpp

#include <QModelIndex>
#include <QString>
#include <boost/shared_ptr.hpp>
#include "AbstractTab.hpp"
#include "Video.hpp"
#include "VideoPlayer.hpp"
#include "FileItem.hpp"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QTreeView;
class QSortFilterProxyModel;
class QAbstractItemDelegate;
QT_END_NAMESPACE

class FileItem;
class VideoPlayer;

class GroupsTab : public AbstractTab
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };
	enum DrawMode { DrawVideo, DrawBackground, DrawDifference };

	GroupsTab(QWidget* parent = 0);
	~GroupsTab();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

	void toBeDeleted(FileItem* fileItem);

private slots:
	void runPermutationTest();

private:
	Item* getItem(const QModelIndex& index) const;

	QLineEdit* searchLineEdit;
	QLineEdit* replaceLineEdit;
	QSpinBox* roundsSpinBox;

	QTreeView* groupsView;
	QTreeView* testsView;

	Project* currentProject;
};

#endif
