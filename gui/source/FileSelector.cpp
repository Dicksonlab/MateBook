#include "FileSelector.hpp"
#include <QToolButton>
#include <QFileDialog>
#include <QStyle>

FileSelector::FileSelector(QWidget* parent) : QLineEdit(parent)
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

void FileSelector::resizeEvent(QResizeEvent*)
{
	QSize sz = selectorButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	selectorButton->move(rect().right() - frameWidth - sz.width(), (rect().bottom() + 1 - sz.height()) / 2);
}

void FileSelector::selectDirectory()
{
	QString newFile = QFileDialog::getOpenFileName(0, tr("Set new file"), text());
	if (!newFile.isNull()) {
		setText(newFile);
	}
}