#include "PathSelector.hpp"
#include <QToolButton>
#include <QFileDialog>
#include <QStyle>

PathSelector::PathSelector(QWidget* parent) : QLineEdit(parent)
{
	selectorButton = new QToolButton(this);
	selectorButton->setIcon(QIcon(":/mb/icons/open.png"));
	selectorButton->setCursor(Qt::ArrowCursor);
	selectorButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
	connect(selectorButton, SIGNAL(clicked()), this, SLOT(selectDirectory()));
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(selectorButton->sizeHint().width() + frameWidth + 1));
	QSize msz = minimumSizeHint();
	setMinimumSize(qMax(msz.width(), selectorButton->sizeHint().height() + frameWidth * 2 + 2),
	               qMax(msz.height(), selectorButton->sizeHint().height() + frameWidth * 2 + 2));
}

void PathSelector::resizeEvent(QResizeEvent*)
{
	QSize sz = selectorButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	selectorButton->move(rect().right() - frameWidth - sz.width(), (rect().bottom() + 1 - sz.height()) / 2);
}

void PathSelector::selectDirectory()
{
	QString newPath = QFileDialog::getExistingDirectory(0, tr("Set new path"), text());
	if (!newPath.isNull()) {
		setText(newPath);
	}
}