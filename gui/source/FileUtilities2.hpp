#ifndef FileUtilities2_hpp
#define FileUtilities2_hpp

#include <QDir>

class FileUtilities2 {
public:
	static void removeDirectory(const QDir& directory);
	static void copyDirectory(const QDir& source, const QDir& target);
	static void moveDirectory(QDir& source, QDir& target);

private:
};

#endif
