#include "splitPath.hpp"
#include <QFileInfo>

QStringList splitPath(QString path)
{
	QStringList ret;

	QString oldPath;
	do {
		QFileInfo fileInfo(path);
		ret.prepend(fileInfo.fileName());
		oldPath = path;
		path = fileInfo.path();
	} while (path != oldPath);
	ret.first() = path;

	return ret;
}
