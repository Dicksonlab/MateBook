#include "FoldableGroupBox.hpp"

FoldableGroupBox::FoldableGroupBox(const QString& title, QWidget* parent) : QGroupBox(title, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(clicked(bool)), this, SLOT(setChecked(bool)));
}

void FoldableGroupBox::setChecked(bool checked)
{
	if (checked) {
		foreach (QWidget* child, findChildren<QWidget*>()) {
			child->show();
		}
	} else {
		foreach (QWidget* child, findChildren<QWidget*>()) {
			child->hide();
		}
	}

	QGroupBox::setChecked(checked);
}
