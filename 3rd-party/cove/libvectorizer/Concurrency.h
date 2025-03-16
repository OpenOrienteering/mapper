/*
 * Copyright 2020 Kai Pastor
 *
 * This file is part of CoVe.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COVE_CONCURRENCY_H
#define COVE_CONCURRENCY_H

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <QtConcurrent>
#include <QtGlobal>
#include <QAtomicInteger>
#include <QFuture>
#include <QThread>
#include <QThreadPool>

#include "ProgressObserver.h"

namespace cove {

namespace Concurrency {

/**
 * A class for the exchange of progress state during concurrent processing.
 * 
 * Copies of a Progress object share the same progress state.
 * The actual data is held via a const std::shared_ptr, enabling
 * shared ownership and safe concurrent access from multiple threads
 * (cf. std::shared_ptr documentation).
 * 
 * Assignment is not supported. It would require extra synchronization
 * for concurrent access to the same object.
 */
class Progress final : public ProgressObserver
{
private:
	struct Data
	{
		QAtomicInteger<int> percentage = 0;
		QAtomicInteger<int> canceled = 0;
	};
	std::shared_ptr<Data> const data;
	
public:
	Progress() : data(std::make_shared<Data>()) {}
	Progress(const Progress&) = default;
	Progress(Progress&& p) noexcept : Progress(static_cast<const Progress&>(p)) {};  // copy from const
	Progress& operator=(const Progress&) = delete;
	Progress& operator=(Progress&& p) = delete;
	~Progress() override = default;
	
	int getPercentage() const noexcept;
	void setPercentage(int percentage) final;
	
	bool isInterruptionRequested() const final;
	void requestInterruption() noexcept;
};


/**
 * A lightweight adapter for transforming progress, e.g. when aggregating multiple jobs.
 */
struct TransformedProgress : public ProgressObserver
{
	ProgressObserver& observer;  // NOLINT
	double factor = 1.0;  // NOLINT
	double offset = 0.0;  // NOLINT
	
	TransformedProgress(
	        ProgressObserver& observer,
	        double factor = 1.0,
	        double offset = 0.0)
	    : observer(observer), factor(factor), offset(offset)
	{}
	
	void setPercentage(int percentage) final;
	
	bool isInterruptionRequested() const final
	{
		return observer.isInterruptionRequested();
	}
};


/**
 * A class representing a job handled by this framework.
 * 
 * It features the `QFuture` which signals the thread's total state and result,
 * and `Progress` information which is shared with the worker activity.
 * 
 * Assignment is not supported, following the choice made for Progress.
 */
template <typename ResultType>
struct Job
{
	using result_type = ResultType;
	
	QFuture<ResultType> future;  // NOLINT
	Progress progress;           // NOLINT
	char padding[64 - (sizeof(future) - sizeof(progress)) % 64] = {};  // NOLINT
	
	Job() = delete;
	Job(Job const&) = default;
	Job(Job&& j) noexcept : Job(std::move(j.future), j.progress) {};
	Job(QFuture<ResultType>&& f, const Progress& p) noexcept : future(std::move(f)), progress(p) {}
	Job& operator=(Job const&) = delete;
	Job& operator=(Job&& job) = delete;
	~Job() = default;
};


/**
 * Starts a functor job in the background.
 * 
 * This is a wrapper around QtConcurrent::run. In addition to what
 * QtConcurrent::run already does, this class creates a progress observer
 * object, passes it by reference to the background function call, and
 * adds this object to the return value.
 */
template <typename ResultType, typename FunctorType, typename ... Input>
Job<ResultType> run(const FunctorType& functor, Input&& ... args)
{
	Progress progress;
	auto future = QtConcurrent::run(functor, &FunctorType::operator(), std::forward<Input>(args)..., progress);
	return {std::move(future), progress};
}


/**
 * A utility to handle progress information for a single job.
 * 
 * This function does a single exchange of cancellation and progress information
 * between the given observer and the job. It returns true as long as the job
 * is not in finished state.
 */
template <typename ResultType>
bool handleProgress(ProgressObserver* observer, Job<ResultType>& job)
{
	if (Q_LIKELY(observer))
	{
		if (observer->isInterruptionRequested())
			job.progress.requestInterruption();
		observer->setPercentage(job.progress.getPercentage());
		
	}
	return !job.future.isFinished();
}


/**
 * Waits for a background job or a for a list of jobs to be finished.
 * 
 * This function continuously exchanges progress with the observer and sends the
 * current thread to sleep for a short break until the job is finished.
 * Note that this function does not run the event loop: This is left to the
 * observer if needed.
 */
template <typename JobOrList>
void waitForFinished(ProgressObserver* observer, JobOrList& jobs)
{
	// This thread will sleep most of the time.
	QThreadPool::globalInstance()->releaseThread(); 
	while (handleProgress(observer, jobs))
	{
		QThread::msleep(100);
	}
	QThreadPool::globalInstance()->reserveThread();
}



// Generic functions for parallel processing

/**
 * A list of concurrent jobs.
 */
template <typename ResultType>
using JobList = std::vector<Job<ResultType>>;


/**
 * A utility to handle progress information for a list of jobs.
 * 
 * This function does a single exchange of cancellation and progress information
 * between the given observer and the jobs. It reports the average progress.
 * It returns true as long as not all jobs are in finished state.
 */
template <typename ResultType>
bool handleProgress(ProgressObserver* observer, JobList<ResultType>& jobs)
{
	auto const interruption_requested = observer && observer->isInterruptionRequested();
	bool finished = true;
	int progress = 0;
	for (auto& job : jobs)
	{
		finished &= job.future.isFinished();
		if (interruption_requested) job.progress.requestInterruption();
		progress += job.progress.getPercentage();
	}
	if (Q_LIKELY(observer))
		observer->setPercentage(progress / int(jobs.size()));
	return !finished;
}


/**
 * Runs a functor in one or more threads in the background,
 * using the explicitly given type of concurrency.
 * 
 * Type `ConcurrencyType` must implement a function template (!) like
 * 
 *     template <typename ResultType, typename Functor>
 *     static JobList<ResultType> makeJobs(const Functor& functor, ...)
 *     {
 * 	       Concurrency::JobList<ResultType> jobs;
 * 	       while(moreWork())
 *             jobs.emplace_back(Concurrency::run<ResultType>(functor, ...));
 *         return jobs;
 *     }
 */
template <typename ConcurrencyType, typename ResultType, typename FunctorType, typename ... Input>
JobList<ResultType> processConcurrent(ProgressObserver* observer, const FunctorType& function, Input&& ... args)
{
	auto jobs = ConcurrencyType::template makeJobs<ResultType>(function, std::forward<Input>(args)...);
	waitForFinished<JobList<ResultType>>(observer, jobs);
	return jobs;
}


/**
 * A utility to test if concurrent worker jobs creation support is defined
 * for the given functor type.
 * 
 * This is needed to automatically dispatch process() calls to either a single
 * background job, or to multiple jobs created by a user-provided job list
 * builder function.
 * 
 * For this meta property to evaluate to true, the Functor type must define a
 * member type `concurrent_processing` which implements the job list builder
 * function as required by processConcurrent().
 */
template<typename Functor, typename = void>
struct supported : std::false_type {};

template<typename Functor>
struct supported<Functor, typename std::enable_if<std::is_class<typename Functor::concurrent_processing>::value>::type> : std::true_type {};


/**
 * Runs a functor in one or more threads in the background,
 * deriving the type of concurrency from the functor type.
 * 
 * This is a combination of run() and waitForFinished() for Functors which
 * declare concurrent worker jobs creation.
 */
template <typename ResultType = void, typename FunctorType, typename ... Input>
typename std::enable_if_t<supported<FunctorType>::value, JobList<ResultType>>
 process(ProgressObserver* observer, const FunctorType& function, Input&& ... args)
{
	return processConcurrent<typename FunctorType::concurrent_processing, ResultType, FunctorType>(observer, function, std::forward<Input>(args)...);
}



// Generic fallback for absence of declared (supported) parallel processing

/**
 * Executes a functor in another thread and waits for its completion.
 * 
 * This is a combination of run() and waitForFinished() for Functors which do
 * not declare concurrent worker jobs creation.
 */
template <typename ResultType, typename FunctorType, typename ... Input>
typename std::enable_if_t<!supported<FunctorType>::value, ResultType>
 process(ProgressObserver* observer, const FunctorType& functor, Input&& ... args)
{
	auto job = run<ResultType>(functor, std::forward<Input>(args)...);
	waitForFinished(observer, job);
	return job.future.result();
}


}  // namespace Concurrency

}  // namespace cove

#endif // COVE_CONCURRENCY_H
