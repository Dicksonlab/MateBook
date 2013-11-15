#ifndef JobQueue_hpp
#define JobQueue_hpp

#include <deque>
#include <set>
#include "Job.hpp"

/**
  * @class  JobQueue
  * @brief  a queue to manage processing jobs
  */
class JobQueue : public QObject {
	Q_OBJECT

public:
	JobQueue(QObject* parent = 0);
	~JobQueue();

	void queueJob(Job* job);
	void abortJob(Job* job);
	void abortAllJobs();
	void abortLocalJobs();

	size_t runningJobsCount() const;
	size_t queuedJobsCount() const;

public slots:
	void setMaxJobs(int maxJobs);

signals:
	void jobStarted(Job*);
	void jobFinished(Job*);
	void jobError(Job*);

private slots:
	void jobStartedSlot(Job*);
	void jobFinishedSlot(Job*);
	void jobErrorSlot(Job*);

private:
	void tryStartingJobs();
	std::deque<Job*> queuedJobs;
	std::set<Job*> runningJobs;
	unsigned int maxJobs;
};

#endif
