#ifndef DateDelegate_hpp
#define DateDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class DateDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	DateDelegate(QWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
};

#endif
