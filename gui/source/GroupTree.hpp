#ifndef GroupTree_hpp
#define GroupTree_hpp

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QDir>

#include <map>
#include <vector>
#include "../../mediawrapper/source/mediawrapper.hpp"
#include "Accessor.hpp"

class AbstractGroupItem;
class MateBook;
class Project;
class ArenaItem;

/**
  * @class  GroupTree
  * @brief  holds the roots of the Item tree and serves as a model for QTreeViews
  *
  * When accessed through the QAbstractItemModel interface, it looks for the requested Item,
  * but delegates the actual data access to Accessors stored in a map that can be indexed using the column index.
  */
class GroupTree : public QAbstractItemModel {
	Q_OBJECT

public:
	GroupTree(MateBook* mateBook, Project* project, QObject* parent = 0);
	~GroupTree();

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

	bool addArenaItemToGroup(ArenaItem* arenaItem, const QString& groupName);
	void addHeader(const QString& header, Accessor<AbstractGroupItem>* accessor);

	AbstractGroupItem* getItem(const QModelIndex& index) const;
	int getColumn(QString header) const;
	int getRow(AbstractGroupItem* item) const;

	// here we're bypassing the model interface, but it's ready-only, so it should be okay
	// we're using this to make the implementation of the permutation test more straight-forward
	const std::vector<AbstractGroupItem*>& getGroups() const;
	const std::map<QString, size_t>& getGroupIndexes() const;

private slots:
	void itemChanged(AbstractGroupItem* item);

private:
	MateBook* mateBook;
	Project* project;
	std::vector<AbstractGroupItem*> groups;
	std::map<QString, size_t> groupIndexes;
	std::map<QString, int> headerToColumn;
	std::vector<QString> headers; // maps columns to headers
	std::vector<Accessor<AbstractGroupItem>* > accessors; // maps columns to accessors
};

#endif
