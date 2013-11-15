#include "MateBook.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <iostream>
#include <ctime>
#include "../../common/source/Singleton.hpp"
#include "global.hpp"
#include "../../common/source/Settings.hpp"
#include "../../mediawrapper/source/mediawrapper.hpp"
#include "../../common/source/debug.hpp"

int main(int argc, char *argv[])
{
	for (int i = 0; i < argc; ++i) {
		std::cout << argv[i] << std::endl;
	}

	//initMemoryLeakDetection();
	mw::initialize();
	qsrand(std::time(NULL));
	Q_INIT_RESOURCE(matebook);
	QApplication application(argc, argv);
	application.setQuitOnLastWindowClosed(true);
	global::executableFile = QFileInfo(application.argv()[0]).canonicalFilePath();
	global::executableDir = QFileInfo(application.argv()[0]).canonicalPath() + "/";
	MateBook mateBook;
	QObject::connect(&mateBook, SIGNAL(closeMateBook()), &application, SLOT(closeAllWindows()));
	mateBook.show();
	return application.exec();
}
