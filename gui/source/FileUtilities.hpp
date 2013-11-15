#ifndef FileUtilities_hpp
#define FileUtilities_hpp

#include <QDir>

class FileUtilities {
public:
	static void removeDirectory(const QDir& directory);
	static void copyDirectory(const QDir& source, const QDir& target);
	static void moveDirectory(QDir& source, QDir& target);

private:
};

#endif
