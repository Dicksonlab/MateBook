#include "FileUtilities2.hpp"
#include <QFileInfo>
#include "RuntimeError.hpp"

void FileUtilities2::removeDirectory(const QDir& directory)
{
	if (directory.exists()) {
		foreach (QFileInfo entry, directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files)) {
			QString path = entry.absoluteFilePath();
			if (entry.isDir()) {
				removeDirectory(QDir(path));
			} else {
				QFile file(path);
				if (!file.remove()) {
					throw RuntimeError(QObject::tr("Failed to delete %1.").arg(path));
				}
			}
		}
		if (!directory.rmdir(directory.absolutePath())) {
			throw RuntimeError(QObject::tr("Failed to delete %1.").arg(directory.absolutePath()));
		}
	}
}

void FileUtilities2::copyDirectory(const QDir& source, const QDir& target)
{
	if (!source.exists()) {
		throw RuntimeError(QObject::tr("Failed to copy %1:\nSource directory doesn't exist.").arg(source.path()));
	}

	if (target.exists()) {
		throw RuntimeError(QObject::tr("Failed to copy %1:\nTarget directory exists already.").arg(source.path()));
	}

	QDir targetParent = target;
	targetParent.cdUp();
	if (!targetParent.mkdir(target.dirName())) {
		throw RuntimeError(QObject::tr("Failed to create destination directory %1.").arg(target.dirName()));
	}

	foreach (QFileInfo entry, source.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files)) {
		QString path = entry.absoluteFilePath();
		if (entry.isDir()) {
			copyDirectory(QDir(path), QDir(target.path() + "/" + entry.fileName()));
		} else if (!QFile::copy(path, target.path() + "/" + QFileInfo(path).fileName())) {
			throw RuntimeError(QObject::tr("Failed to copy %1.").arg(path));
		}
	}
}

void FileUtilities2::moveDirectory(QDir& source, QDir& target)
{
	if (!QDir::current().rename(source.path(), target.path())) {
		throw RuntimeError(QObject::tr("Failed to move %1 to %2.").arg(source.path()).arg(target.path()));
	}
}
