#include "JobQueue.hpp"
#include "../../common/source/Singleton.hpp"
#include <iostream>

JobQueue::JobQueue(QObject* parent) : QObject(parent),
	maxJobs(1)
{
}

JobQueue::~JobQueue()
{
	abortLocalJobs();
}

void JobQueue::queueJob(Job* job)
{
	connect(job, SIGNAL(jobStarted(Job*)), this, SLOT(jobStartedSlot(Job*)));
	connect(job, SIGNAL(jobFinished(Job*)), this, SLOT(jobFinishedSlot(Job*)));
	connect(job, SIGNAL(jobError(Job*)), this, SLOT(jobErrorSlot(Job*)));

	if (job->getCpuLoad() == 0) {	//TODO: this is a hack: this way ClusterJobs (that have cpu load 0) never actually enter the local queue. We cannot yet schedule based on the load!
		runningJobs.insert(job);
		job->start();
	} else {
		queuedJobs.push_back(job);
		tryStartingJobs();
	}
}

void JobQueue::abortJob(Job* job)
{
	std::deque<Job*>::iterator queuedJob = std::find(queuedJobs.begin(), queuedJobs.end(), job);
	if (queuedJob != queuedJobs.end()) {
		queuedJobs.erase(queuedJob);
		(*queuedJob)->stop();
		delete *queuedJob;
	}

	std::set<Job*>::iterator runningJob = runningJobs.find(job);
	if (runningJob != runningJobs.end()) {
		runningJobs.erase(runningJob);
		(*runningJob)->stop();
		delete *runningJob;
	}
}

void JobQueue::abortAllJobs()
{
	while (!queuedJobs.empty()) {
		Job* jobToKill = queuedJobs.front();
		queuedJobs.pop_front();
		jobToKill->stop();
		delete jobToKill;
	}

	for (std::set<Job*>::iterator iter = runningJobs.begin(); iter != runningJobs.end();) {
		Job* jobToKill = (*iter);
		++iter;	// incremented here because the call below would invalidate the iterator
		runningJobs.erase(jobToKill);
		jobToKill->stop();
		delete jobToKill;
	}
}

void JobQueue::abortLocalJobs()
{
	while (!queuedJobs.empty()) {
		Job* jobToKill = queuedJobs.front();
		queuedJobs.pop_front();
		delete jobToKill;
	}

	for (std::set<Job*>::iterator iter = runningJobs.begin(); iter != runningJobs.end();) {
		Job* jobToKill = (*iter);
		++iter;	// incremented here because the call below would invalidate the iterator
		runningJobs.erase(jobToKill);
		delete jobToKill;
	}
}

size_t JobQueue::runningJobsCount() const
{
	return runningJobs.size();
}

size_t JobQueue::queuedJobsCount() const
{
	return queuedJobs.size();
}

void JobQueue::setMaxJobs(int maxJobs)
{
	this->maxJobs = (maxJobs < 0 ? 0 : maxJobs); 
	tryStartingJobs();
}

void JobQueue::jobStartedSlot(Job* job)
{
	emit jobStarted(job);
}

void JobQueue::jobFinishedSlot(Job* job)
{
	runningJobs.erase(job);
	emit jobFinished(job);
	job->deleteLater();	// we cannot delete it directly because as it is signalling us
	tryStartingJobs();
}

void JobQueue::jobErrorSlot(Job* job)
{
	std::cerr << "failed to run job: " << job->getDescription().toStdString() << std::endl;
	runningJobs.erase(job);
	emit jobError(job);
	job->deleteLater();	// we cannot delete it directly because as it is signalling us
	tryStartingJobs();
}

void JobQueue::tryStartingJobs()
{
	while (runningJobs.size() < maxJobs && !queuedJobs.empty()) {
		Job* jobToStart = queuedJobs.front();
		runningJobs.insert(jobToStart);
		queuedJobs.pop_front();
		jobToStart->start();
	}
}
