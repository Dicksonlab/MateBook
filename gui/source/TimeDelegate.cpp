#include <QLineEdit>

#include "TimeDelegate.hpp"

TimeDelegate::TimeDelegate(QWidget* parent) : QStyledItemDelegate(parent)
{
}

QWidget* TimeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QLineEdit* editor = new QLineEdit(parent);
	editor->setInputMask("00:00:00"); 
	return editor;
}

void TimeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QLineEdit* timeEdit = static_cast<QLineEdit*>(editor);
	timeEdit->setText(index.data(Qt::EditRole).toString());
}