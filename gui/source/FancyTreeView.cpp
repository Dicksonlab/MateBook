#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QWidgetAction>
#include <QHeaderView>
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include "FancyMenu.hpp"
#include "FancyTreeView.hpp"

FancyTreeView::FancyTreeView(FancyMenu* cellContextMenu, QWidget* parent) : QTreeView(parent),
	cellContextMenu(cellContextMenu),
	proxyModel(NULL),
	autoResize(true)
{
	headerContextMenu = new QMenu(this);

	header()->setContextMenuPolicy(Qt::CustomContextMenu);
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showHeaderContextMenu(const QPoint&)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showCellContextMenu(const QPoint&)));
}

FancyTreeView::~FancyTreeView()
{
	delete proxyModel;
}

void FancyTreeView::setModel(QAbstractItemModel* model)
{
	//TODO: if we don't jump through hoops here Qt crashes because it wants to access data in the new model using old indexes...file bug report?
	QTreeView::setModel(NULL);
	delete proxyModel; proxyModel = NULL;
	proxyModel = new QSortFilterProxyModel;
	proxyModel->setSourceModel(model);
	QTreeView::setModel(proxyModel);	
}

QModelIndex FancyTreeView::currentIndex() const
{
	return proxyModel->mapToSource(selectionModel()->currentIndex());
}

QModelIndexList FancyTreeView::getSelectedIndexes() const
{
	return proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
}

QModelIndex FancyTreeView::index(int row, int column, const QModelIndex & parent) const
{
	return proxyModel->mapToSource(QTreeView::model()->index(row, column, parent));
}

QModelIndex FancyTreeView::index(const QModelIndex& index) const
{
	return proxyModel->mapToSource(index);
}

QAbstractItemModel* FancyTreeView::model() const
{
	return proxyModel->sourceModel();
}

void FancyTreeView::setCurrentIndex(const QModelIndex& index, QItemSelectionModel::SelectionFlags command)
{
	QItemSelection previous = selectionModel()->selection();
	changeCurrent(index, proxyModel->mapToSource(selectionModel()->currentIndex()));
	currentChanged(proxyModel->mapFromSource(index), selectionModel()->currentIndex());
	selectionChanged(selectionModel()->selection(), previous);
}

// if the indexList contains indexes which do not have the same parent as the first selected one, those are removed from the list
void FancyTreeView::changeToValidSelection(QModelIndexList& indexList)
{
	int i = 0;
	foreach (QModelIndex current, indexList) {
		if (current.parent() != indexList.first().parent()) {
			selectionModel()->select(current, QItemSelectionModel::Deselect);
			indexList.removeAt(i);
			--i;
		}
		++i;
	}
}

void FancyTreeView::createHeaderContextMenu()	// context menu to hide columns and make them visible again
{
	QAction* autoResizeAction = new QAction("Auto Resize", this);
	autoResizeAction->setCheckable(true);
	autoResizeAction->setChecked(true);
	connect(autoResizeAction, SIGNAL(toggled(bool)), this, SLOT(setAutoResize(bool)));

	headerContextMenu->clear();
	headerContextMenu->addAction("Resize All Columns", this, SLOT(resizeAllColumns()));
	headerContextMenu->addAction(autoResizeAction);
	headerContextMenu->addSeparator();
	headerContextMenu->addAction("Show All", this, SLOT(showAllTreeColumns()));
	headerContextMenu->addAction("Hide All", this, SLOT(hideAllTreeColumns()));
	headerContextMenu->addSeparator();
	
	columnCheckBoxes.clear();
	columnCheckBoxes.reserve(model()->columnCount());
	for (int i = 0; i < model()->columnCount(); ++i) {
		QCheckBox* checkBox = new QCheckBox(headerContextMenu); // using checkboxes keeps context menu open
		checkBox->setText(model()->headerData(i, Qt::Horizontal).toString());
		checkBox->setProperty("index", QString::number(i));
		checkBox->setChecked(true);
		
		QWidgetAction* hideColumnAction = new QWidgetAction(headerContextMenu);
		hideColumnAction->setDefaultWidget(checkBox);
		columnCheckBoxes.push_back(checkBox);

		connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(hideshowTreeColumn(bool)));
		headerContextMenu->addAction(hideColumnAction);
	}
}

void FancyTreeView::setHeaderVisibility(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	header()->restoreState(file.read(file.size()));

	for (int i = 0; i < model()->columnCount(); ++i) {
		if (isColumnHidden(i)) {
			columnCheckBoxes[i]->setChecked(!isColumnHidden(i));
		}
	}
}

void FancyTreeView::saveHeaderVisibility(const QString& filePath) const
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(header()->saveState());
}

void FancyTreeView::cut() //TODO: works only for one cell (at the moment)
{
	QModelIndex currIndex = currentIndex();
	if (!currIndex.isValid()) {
		return;
	}
	QApplication::clipboard()->setText(model()->data(currIndex).toString());
	model()->setData(currIndex, QVariant());
}

void FancyTreeView::copy()
{
	QModelIndexList indexes = selectionModel()->selection().indexes();
	changeToValidSelection(indexes);

	if (indexes.size() > 0) {
		qSort(indexes.begin(), indexes.end()); //sorts by columns
		QString selected_text="";

		QModelIndex previous = indexes.first();
		selected_text.append(proxyModel->data(previous).toString());
		indexes.removeFirst();

		foreach (QModelIndex current, indexes) {
			if (current.row() != previous.row()) { 
				selected_text.append('\n'); //new row
			} else {
				selected_text.append('\t'); //new column
			}
			selected_text.append(proxyModel->data(current).toString());
			previous = current;
		}
		QApplication::clipboard()->setText(selected_text);
	}
}

void FancyTreeView::paste()
{
	QString clipboardString = QApplication::clipboard()->text();
	QModelIndexList indexes = selectionModel()->selection().indexes();
	if (indexes.size() > 1) {
		//allows pasting one value into whole treeView 
		if (clipboardString.size() > 1) {
			changeToValidSelection(indexes);
		}
		pasteIntoArea(indexes, clipboardString);
	} else {
		pasteIntoSingleCell(indexes, clipboardString);
	}
}

void FancyTreeView::clearSelection()
{
	selectionModel()->clear();
}

void FancyTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QTreeView::selectionChanged(selected, deselected);
	emit selectionHasChanged(proxyModel->mapSelectionToSource(selected), proxyModel->mapSelectionToSource(deselected));
}

void FancyTreeView::changeSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
	selectionModel()->select(proxyModel->mapSelectionFromSource(deselected), QItemSelectionModel::Deselect);
	selectionModel()->select(proxyModel->mapSelectionFromSource(selected), QItemSelectionModel::Select);
	QTreeView::selectionChanged(proxyModel->mapSelectionFromSource(selected), proxyModel->mapSelectionFromSource(deselected));
}

void FancyTreeView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	QTreeView::currentChanged(current, previous);
	emit currentHasChanged(proxyModel->mapToSource(current), proxyModel->mapToSource(previous));
}

void FancyTreeView::changeCurrent(const QModelIndex& current, const QModelIndex& previous)
{
	selectionModel()->setCurrentIndex(proxyModel->mapFromSource(current), QItemSelectionModel::Select);
	QTreeView::currentChanged(proxyModel->mapFromSource(current), proxyModel->mapFromSource(previous));
}

void FancyTreeView::showHeaderContextMenu(const QPoint& pos)
{
	headerContextMenu->exec(mapToGlobal(pos));
}

void FancyTreeView::showCellContextMenu(const QPoint& pos)
{
	if(cellContextMenu != 0){
		cellContextMenu->exec(mapToGlobal(pos), currentIndex(), getSelectedIndexes());
	}
}

// ContextMenu: sets all contextActions to checked and all columns to hidden
void FancyTreeView::showAllTreeColumns()
{
	for(int i = 0; i < columnCheckBoxes.size(); ++i) {
		columnCheckBoxes[i]->setChecked(true);
	}
	resizeAllColumns();
}

void FancyTreeView::hideAllTreeColumns()
{
	if(columnCheckBoxes.size() > 0){
		for(int i = 0; i < columnCheckBoxes.size(); ++i) {
			columnCheckBoxes[i]->setChecked(false);
		}
		columnCheckBoxes[0]->setChecked(true);
	}
}

void FancyTreeView::resizeAllColumns()
{
	for(int i = 0; i < columnCheckBoxes.size(); ++i) {
		if(columnCheckBoxes[i]->isChecked()){
			resizeColumnToContents(i);
		}
	}
}

// if column is hidden: set it to visible and the other way round
void FancyTreeView::hideshowTreeColumn(bool checked)
{
	QCheckBox* senderBox = qobject_cast<QCheckBox*>(sender());
	int columnIndex = senderBox->property("index").toInt();
	setColumnHidden(columnIndex, !checked);
	if(autoResize){
		resizeAllColumns();
	}
	emit columnsWereModified();
}

void FancyTreeView::setAutoResize(bool checked)
{
	autoResize = checked;
}

QItemSelectionModel* FancyTreeView::selectionModel() const
{
	return QTreeView::selectionModel();
}

void FancyTreeView::pasteIntoSingleCell(const QModelIndexList& indexes, const QString& clipboardString)
{
	if(indexes.size() > 0){
		// paste: the clipboard string is splitted into row and column lists.
		// the values are assigned to the cells next to the current selected index
		QStringList rows = clipboardString.split('\n');

		// text copied form excel has another '\n' at the end
		if (rows.size() > 1 && rows.last().isEmpty()) { 
			rows.pop_back();
		}

		int rowCount = rows.count();
		int colCount = rows.first().count('\t')+1;

		for(int i = 0; i< rowCount; ++i){
			QStringList columns = rows[i].split('\t');
			for (int j = 0; j < colCount; ++j){
				int row = indexes[0].row() + i;
				int col = indexes[0].column() + j;
				if (row < model()->rowCount(indexes[0].parent()) && col < model()->columnCount(indexes[0].parent())){
					QModelIndex index = indexes[0].sibling(row, col);
					if(index.isValid() && j < columns.size()){
						model()->setData(this->index(index), columns[j]);
					}
				}
			}
		}
	}
}

void FancyTreeView::pasteIntoArea(const QModelIndexList& indexes, const QString& clipboardString)
{
	if(indexes.size() > 0){
		QModelIndexList tempIndexes = indexes;
		qSort(tempIndexes.begin(), tempIndexes.end());

		QStringList list = clipboardString.split("\n");
		// text copied form excel has another '\n' at the end
		if(list.size() > 1 && list.last().isEmpty()){ 
			list.pop_back();
		}

		QList<QStringList> list2d;
		foreach(QString index, list){
			list2d.append(index.split("\t"));
		}

		int col = 1;
		int row = 0;
		QModelIndex previous = tempIndexes.first();
		model()->setData(this->index(previous), list2d[0][0]);
		tempIndexes.removeFirst();

		// as long as the row numbers are the same, the first row index of list2d stays
		// the same and only the column data is iterated over and over again until the
		// current column of the selected area is filled. if the row number (of ModelIndex) 
		// changes, the row index (list2d) is increased by one
		foreach (QModelIndex current, tempIndexes) {
			if (current.row() == previous.row() && current.parent() == previous.parent()) {
				int colSize = list2d[row].size();
				if (col >= colSize) {
					col = 0;
				}
			} else {
				++row;
				col = 0;
				if (row >= list2d.size()) {
					row = 0;
				}
			}
			model()->setData(this->index(current), list2d[row][col]);
			++col;
			previous = current;
		}
	}
}