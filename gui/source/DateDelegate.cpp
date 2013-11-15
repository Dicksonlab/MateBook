#include <QDateTimeEdit>

#include "DateDelegate.hpp"

DateDelegate::DateDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}

QWidget* DateDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QDateTimeEdit* editor = new QDateTimeEdit(parent);
	editor->setDisplayFormat("yyyy-MM-dd");
	return editor;
}

void DateDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QDateTimeEdit* dateEdit = static_cast<QDateTimeEdit*>(editor);
	dateEdit->setDateTime(index.data(Qt::EditRole).toDateTime());
}