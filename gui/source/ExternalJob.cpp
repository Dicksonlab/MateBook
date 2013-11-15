#include "ExternalJob.hpp"
#include <iostream>

ExternalJob::ExternalJob(const QString& executable, const QStringList& arguments, const QDir& workingDirectory) : Job(),
	process(new QProcess),
	executable(executable),
	arguments(arguments),
	workingDirectory(workingDirectory)
{
	process->setWorkingDirectory(workingDirectory.path());
	connect(process, SIGNAL(started()), this, SLOT(processStarted()));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));

	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
	connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
}

ExternalJob::~ExternalJob()
{
	stop();
}

void ExternalJob::processStarted()
{
	setLogFile(workingDirectory.path() + "/" + QFileInfo(executable).fileName() + "_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".log");	//TODO: use PID instead of rand()
	emit jobStarted(this);
}

void ExternalJob::processFinished()
{
	emit jobFinished(this);
}

void ExternalJob::processError(QProcess::ProcessError error)
{
	emit jobError(this);
}

float ExternalJob::getCpuLoad() const
{
	return 1;
}

void ExternalJob::start()
{
	process->start(executable, arguments);
}

void ExternalJob::stop()
{
	process->terminate();
	process->kill();
}

QString ExternalJob::getDescription() const
{
	return executable;
}

void ExternalJob::readyReadStandardOutput()
{
	log(process->readAllStandardOutput());
}

void ExternalJob::readyReadStandardError()
{
	log(process->readAllStandardError());
}
