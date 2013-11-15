#ifndef ArenasApprovedDelegate_hpp
#define ArenasApprovedDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QAbstractItemModel>
#include <QPainter>
#include <QEvent>

// paints a checkbox instead of bool values in the column approved arenas

class ArenasApprovedDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	ArenasApprovedDelegate(QObject* parent = 0);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
};

#endif
