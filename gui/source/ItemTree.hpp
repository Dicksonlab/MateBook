#ifndef ItemTree_hpp
#define ItemTree_hpp

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QDir>
#include <QReadWriteLock>

#include <map>
#include <vector>
#include "../../mediawrapper/source/mediawrapper.hpp"
#include "Accessor.hpp"

class Item;
class MateBook;
class Project;

/**
  * @class  ItemTree
  * @brief  holds the roots of the Item tree and serves as a model for QTreeViews
  *
  * When accessed through the QAbstractItemModel interface, it looks for the requested Item,
  * but delegates the actual data access to Accessors stored in a map that can be indexed using the column index.
  */
class ItemTree : public QAbstractItemModel {
	Q_OBJECT

public:
	ItemTree(MateBook* mateBook, Project* project, QObject* parent = 0);
	~ItemTree();

	void clear();

	// the following methods are required for read-only models
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;

	// tree models require these to be implemented
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex& index) const;

	// in addition to the ones above, an editable model has to provide these methods
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole);

	// resizable models require these to be implemented
//	bool insertRows(int position, int count, const QModelIndex& parent = QModelIndex());
	bool removeRows(int position, int count, const QModelIndex& parent = QModelIndex());
//	bool insertColumns(int position, int count, const QModelIndex& parent = QModelIndex());
//	bool removeColumns(int position, int count, const QModelIndex& parent = QModelIndex());

	bool addVideo(const QString& fileName);
	void addHeader(const QString& header, Accessor<Item>* accessor);

	Item* getItem(const QModelIndex& index) const;
	int getColumn(QString header) const;
	int getRow(Item* item) const;

	void serialize() const;

private slots:
	void itemChanged(Item* item);
	void beginInsertChildren(Item* parent, int first, int last);
	void endInsertChildren(Item* parent, int first, int last);
	void beginRemoveChildren(Item* parent, int first, int last);
	void endRemoveChildren(Item* parent, int first, int last);

private:
	MateBook* mateBook;
	Project* project;
	std::vector<Item*> files;
	std::map<QString, int> headerToColumn;
	std::vector<QString> headers; // maps columns to headers
	std::vector<Accessor<Item>*> accessors; // maps columns to accessors
};

#endif
