#include "Job.hpp"

#include <iostream>

Job::Job()
{
	//TODO: call them directly from derived classes instead of using signal/slots
	connect(this, SIGNAL(jobStarted(Job*)), this, SLOT(started()));
	connect(this, SIGNAL(jobFinished(Job*)), this, SLOT(finished()));
	connect(this, SIGNAL(jobError(Job*)), this, SLOT(error()));
}

Job::~Job()
{
	logFile.close();
}

void Job::setLogFile(const QString& logFileName)
{
	logFile.close();
	logFile.setFileName(logFileName);
	logFile.open(QIODevice::WriteOnly);
}

void Job::log(const QByteArray& data)
{
	logFile.write(data);
}

void Job::started()
{
	startTime = QDateTime::currentDateTime();
}

void Job::finished()
{
	finishTime = QDateTime::currentDateTime();
}

void Job::error()
{
	errorTime = QDateTime::currentDateTime();
}
