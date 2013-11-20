#include "ClusterJob.hpp"
#include <iostream>
#include <QTimer>
#include <QProcess>
#include "../../common/source/debug.hpp"

ClusterJob::ClusterJob(const QString& sshClient, const QString& sshTransferHost, const QString& sshUsername, const QString& sshPrivateKey, const QString& sshEnvironment, int pollingIntervalSeconds, const QString& executable, const QStringList& arguments, const QDir& workingDirectory) : Job(),
	timer(),
	sshClient(sshClient),
	sshTransferHost(sshTransferHost),
	sshUsername(sshUsername),
	sshPrivateKey(sshPrivateKey),
	sshEnvironment(sshEnvironment),
	pollingIntervalMilliseconds(pollingIntervalSeconds < 10 ? 10 * 1000 : pollingIntervalSeconds * 1000),
	executable(executable),
	arguments(),	// we add them below, after putting quotes around each of them
	workingDirectory(workingDirectory),
	id()
{
	#if defined(WIN32)
		if (!sshPrivateKey.isEmpty()) {
			sshParameters << "-i" << sshPrivateKey;
		}
		sshParameters
			<< "-batch"	// never prompt (for passwords, etc...)
			<< sshUsername + "@" + sshTransferHost
			<< sshEnvironment + " && "
		;
	#else
		sshParameters
            << "-o" << "BatchMode=yes"	// never prompt (for passwords, etc...)
            << "-o" << "UserKnownHostsFile=/dev/null"	// so we don't mess with known_hosts
            << "-o" << "StrictHostKeyChecking=no"	// makes us vulnerable to man-in-the-middle attacks, but is more convenient for users
			<< sshUsername + "@" + sshTransferHost
			<< sshEnvironment + " && "
		;
	#endif

	foreach (const QString& arg, arguments) {
		this->arguments << QString("\"") + arg + QString("\"");
	}

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkJobStatus()));
}

ClusterJob::~ClusterJob()
{
//	stop();
}

void ClusterJob::processStarted()
{
	setLogFile(workingDirectory.path() + "/" + QFileInfo(executable).fileName() + "_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".log");	//TODO: use PID instead of rand()
}

void ClusterJob::processFinished()
{
	QString standardOutput(assert_cast<QProcess*>(sender())->readAllStandardOutput());
	QRegExp regExp("Your job (\\d+) \\(\".*\"\\) has been submitted");
	if (regExp.indexIn(standardOutput) > -1) {
		id = regExp.cap(1).toUInt();
	}

	if (!id) {
		std::cerr << "failed to submit cluster job; cout and cerr:" << std::endl;
		std::cout << standardOutput.toStdString() << std::endl;
		QString standardError(assert_cast<QProcess*>(sender())->readAllStandardError());
		std::cerr << standardError.toStdString() << std::endl;
		emit jobError(this);
		return;
	}

	std::cout << "submitted job " << id << std::endl;

	// qsub was successful so we can start polling the job status
	timer->setInterval(pollingIntervalMilliseconds);	// milliseconds
	timer->start();
}

void ClusterJob::processError(QProcess::ProcessError error)
{
	emit jobError(this);
}

float ClusterJob::getCpuLoad() const
{
	return 0;
}

void ClusterJob::start()
{
	QProcess* process = new QProcess;
	process->setWorkingDirectory(workingDirectory.path());
	connect(process, SIGNAL(started()), this, SLOT(processStarted()));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
//	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
//	connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));

    QStringList strlist;
    strlist << sshParameters << executable << arguments;
    QString str = strlist.join(" ");
    process->start(sshClient, strlist);
}

void ClusterJob::stop()
{
	QProcess* process = new QProcess;
	process->setWorkingDirectory(workingDirectory.path());
	process->start(sshClient, QStringList() << sshParameters << "qdel " + QString::number(id));
}

QString ClusterJob::getDescription() const
{
	return executable;
}

void ClusterJob::readyReadStandardOutput()
{
	log(assert_cast<QProcess*>(sender())->readAllStandardOutput());
}

void ClusterJob::readyReadStandardError()
{
	log(assert_cast<QProcess*>(sender())->readAllStandardError());
}

void ClusterJob::checkJobStatus()
{
	QProcess* process = new QProcess;
	process->setWorkingDirectory(workingDirectory.path());
	connect(process, SIGNAL(started()), this, SLOT(checkJobStatusStarted()));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(checkJobStatusFinished()));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(checkJobStatusError(QProcess::ProcessError)));

	process->start(sshClient, QStringList() << sshParameters << "qalter -w v " + QString::number(id));
}

void ClusterJob::checkJobStatusStarted()
{
	// we can ignore this since we don't care about qstat starting
}

void ClusterJob::checkJobStatusFinished()
{
	//TODO: parse its stdout and emit jobStarted(this), jobFinished(this) or jobError(this) if the status changed
	//TODO: stop the timer if it's no longer running
	QString standardOutput(assert_cast<QProcess*>(sender())->readAllStandardOutput());
	if (standardOutput.contains("job \"" + QString::number(id) + "\" does not exist")) {
		timer->stop();
		emit jobFinished(this);
	} else if (standardOutput.contains("job is already running")) {
		//TODO: only emit if it just changed to running, i.e. keep the status as a member variable
		emit jobStarted(this);
	} else if (standardOutput.contains("found suitable queue")) {
		// the job should be showing up as queued anyway
	} else {
		std::cerr << "unknown job status reported for job " << id << ": " << standardOutput.toStdString() << std::endl;
	}

	sender()->deleteLater();
}

void ClusterJob::checkJobStatusError(QProcess::ProcessError error)
{
	std::cerr << "warning: could not check cluster job status" << std::endl;
	sender()->deleteLater();
}
