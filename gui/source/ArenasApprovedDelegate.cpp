#include <QStyle>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>

#include "ArenasApprovedDelegate.hpp"

ArenasApprovedDelegate::ArenasApprovedDelegate(QObject* parent) : QStyledItemDelegate(parent) {
}

void ArenasApprovedDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (option.state & QStyle::State_Selected){ //if cell is selected
        painter->fillRect(option.rect, QColor(198, 226, 255));
	}else{
		painter->fillRect(option.rect, qvariant_cast<QBrush>(index.data(Qt::BackgroundRole))); 
	}

	QStyledItemDelegate::paint(painter,option,index);

	int checked = index.model()->data(index, Qt::DisplayRole).toInt();

	QStyleOptionButton checkBoxOption;
	checkBoxOption.state |= QStyle::State_Enabled;
	switch(checked){
		case 0: 
			checkBoxOption.state |= QStyle::State_Off;
			break;
		case 1:
			checkBoxOption.state |= QStyle::State_NoChange;
			break;
		case 2:
			checkBoxOption.state |= QStyle::State_On;
			break;
		default:
			checkBoxOption.state |= QStyle::State_Off;
	}
	checkBoxOption.rect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option);

	QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);
}

bool ArenasApprovedDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
	if (event->type() == QEvent::MouseButtonRelease) { 
		QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event); // only allow left mouse button
		if (mouse_event->button() != Qt::LeftButton || !QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option).contains(mouse_event->pos())) {
			return false;
		}
	} else if (event->type() == QEvent::KeyPress) { // use space to select/deselect items
		if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
			return false;
		}
	} else {
		return false;
	}
	
	bool checked = index.model()->data(index, Qt::DisplayRole).toBool();
	return model->setData(index, !checked, Qt::EditRole);
}