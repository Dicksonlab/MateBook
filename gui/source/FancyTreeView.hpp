#ifndef FancyTreeView_hpp
#define FancyTreeView_hpp

#include <QTreeView>
#include <QString>
#include <vector>
#include "../../common/source/Settings.hpp"

QT_BEGIN_NAMESPACE
	class QSortFilterProxyModel;
	class QAbstractItemModel;
	class QModelIndex;
	class QCheckBox;
	class QMenu;
QT_END_NAMESPACE

class FancyMenu;

class FancyTreeView : public QTreeView
{
	Q_OBJECT

public:
	FancyTreeView(FancyMenu* cellContextMenu = 0, QWidget* parent = 0);
	~FancyTreeView();
	
	void setModel(QAbstractItemModel* model);

	QModelIndex currentIndex() const;
	QModelIndexList getSelectedIndexes() const;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	QModelIndex index(const QModelIndex& index) const;

	QAbstractItemModel* model() const;
	
	void setCurrentIndex(const QModelIndex& index, QItemSelectionModel::SelectionFlags command);
	void changeToValidSelection(QModelIndexList& indexList);
	void createHeaderContextMenu();

	void setHeaderVisibility(const QString& filePath);
	void saveHeaderVisibility(const QString& filePath) const;

	void cut();
	void copy();
	void paste();
	void clearSelection();

signals:
	void selectionHasChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void currentHasChanged(const QModelIndex& current, const QModelIndex& previous);
	void columnsWereModified();

private slots:
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void changeSelection(const QItemSelection& selected, const QItemSelection& deselected);
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
	void changeCurrent(const QModelIndex& current, const QModelIndex& previous);
	void showHeaderContextMenu(const QPoint& pos);
	void showCellContextMenu(const QPoint& pos);
	void showAllTreeColumns();
	void hideAllTreeColumns();
	void resizeAllColumns();
	void hideshowTreeColumn(bool checked);
	void setAutoResize(bool checked);

private:
	bool autoResize;
	QMenu* headerContextMenu;
	FancyMenu* cellContextMenu;
	std::vector<QCheckBox*> columnCheckBoxes;
	QSortFilterProxyModel* proxyModel;

	QItemSelectionModel* selectionModel() const; // to ensure that this function is never called

	void pasteIntoSingleCell(const QModelIndexList& indexes, const QString& clipboardString);
	void pasteIntoArea(const QModelIndexList& indexes, const QString& clipboardString);
};

#endif
