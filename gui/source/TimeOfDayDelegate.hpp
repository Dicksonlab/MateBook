#ifndef TimeOfDayDelegate_hpp
#define TimeOfDayDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class TimeOfDayDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	TimeOfDayDelegate(QWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
};

#endif
