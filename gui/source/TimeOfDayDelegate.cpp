#include <QDateTimeEdit>

#include "TimeOfDayDelegate.hpp"

TimeOfDayDelegate::TimeOfDayDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}

QWidget* TimeOfDayDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QDateTimeEdit* editor = new QDateTimeEdit(parent);
	editor->setDisplayFormat("hh:mm:ss");
	return editor;
}

void TimeOfDayDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QDateTimeEdit* dateEdit = static_cast<QDateTimeEdit*>(editor);
	dateEdit->setDateTime(index.data(Qt::EditRole).toDateTime());
}