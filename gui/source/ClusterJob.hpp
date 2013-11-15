#ifndef ClusterJob_hpp
#define ClusterJob_hpp

#include <QString>
#include <QStringList>
#include <QDir>
#include "Job.hpp"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

/**
  * @class  ClusterJob
  * @brief  a processing job that is executed on the cluster and monitored until it finishes
  */
class ClusterJob : public Job {
	Q_OBJECT

public:
	ClusterJob(const QString& sshClient, const QString& sshTransferHost, const QString& sshUsername, const QString& sshPrivateKey, const QString& sshEnvironment, int pollingIntervalSeconds, const QString& executable, const QStringList& arguments, const QDir& workingDirectory);
	virtual ~ClusterJob();

	virtual float getCpuLoad() const;

	virtual void start();
	virtual void stop();
	virtual QString getDescription() const;

protected slots:
	virtual void processStarted();
	virtual void processFinished();
	virtual void processError(QProcess::ProcessError error);

private slots:
	void readyReadStandardOutput();
	void readyReadStandardError();
	void checkJobStatus();	// create an ssh process starting a qstat
	void checkJobStatusStarted();	// qstat started
	void checkJobStatusFinished();	// qstat finished
	void checkJobStatusError(QProcess::ProcessError error);	// qstat failed

private:
	Q_DISABLE_COPY(ClusterJob);
	QTimer* timer;
	QString sshClient;
	QString sshTransferHost;
	QString sshUsername;
	QString sshPrivateKey;
	QString sshEnvironment;
	int pollingIntervalMilliseconds;
	QString executable;
	QStringList arguments;
	QDir workingDirectory;
	QStringList sshParameters;
	unsigned int id;	// server side job id
};

#endif
