#ifndef ExternalJob_hpp
#define ExternalJob_hpp

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDir>
#include "Job.hpp"

/**
  * @class  ExternalJob
  * @brief  a processing job that delegates to an external executable, launched in a separate process
  */
class ExternalJob : public Job {
	Q_OBJECT

public:
	ExternalJob(const QString& executable, const QStringList& arguments, const QDir& workingDirectory);
	virtual ~ExternalJob();

	virtual float getCpuLoad() const;

	virtual void start();
	virtual void stop();
	QString getDescription() const;

protected slots:
	virtual void processStarted();
	virtual void processFinished();
	virtual void processError(QProcess::ProcessError error);

private slots:
	void readyReadStandardOutput();
	void readyReadStandardError();

private:
	Q_DISABLE_COPY(ExternalJob);
	QProcess* process;
	QString executable;
	QStringList arguments;
	QDir workingDirectory;
};

#endif
