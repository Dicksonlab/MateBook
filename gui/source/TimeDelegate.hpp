#ifndef TimeDelegate_hpp
#define TimeDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class TimeDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	TimeDelegate(QWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
};

#endif
