#include <QComboBox>
#include <QString>

#include "GenderDelegate.hpp"

GenderDelegate::GenderDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}

QWidget* GenderDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QComboBox* editor = new QComboBox(parent);
	editor->addItem("male");
	editor->addItem("female");
	
	return editor;
}

void GenderDelegate::setEditorData(QWidget* editor, const QModelIndex &index) const
{
	QComboBox* genderEdit = static_cast<QComboBox*>(editor);
	genderEdit->setCurrentIndex(genderEdit->findText(index.data(Qt::EditRole).toString()));
}

void GenderDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QComboBox *genderEdit = static_cast<QComboBox*>(editor);
	model->setData(index, genderEdit->currentText());
}