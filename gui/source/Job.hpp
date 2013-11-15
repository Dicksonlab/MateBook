#ifndef Job_hpp
#define Job_hpp

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QFile>

/**
  * @class  Job
  * @brief  a processing job to be put into a JobQueue
  */
class Job : public QObject {
	Q_OBJECT

public:
	Job();
	virtual ~Job();

	QDateTime getStartTime() const;
	QDateTime getFinishTime() const;
	QDateTime getErrorTime() const;

	virtual float getCpuLoad() const = 0;	// "1.0" means 1 CPU is needed; can be a fraction

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual QString getDescription() const = 0;

signals:
	void jobStarted(Job*);
	void jobFinished(Job*);
	void jobError(Job*);

protected:
	void setLogFile(const QString& logFileName);
	void log(const QByteArray& data);

private slots:
	void started();
	void finished();
	void error();

private:
	Q_DISABLE_COPY(Job);
	QFile logFile;
	QDateTime startTime;
	QDateTime finishTime;
	QDateTime errorTime;
};

#endif
