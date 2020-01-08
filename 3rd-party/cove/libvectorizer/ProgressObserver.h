/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 * Copyright 2020 Kai Pastor
 *
 * This file is part of CoVe 
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

#ifndef COVE_CONCURRENCY_OBSERVER_H
#define COVE_CONCURRENCY_OBSERVER_H

/* 
 * This is a simple separate header, because it is meant to be used often,
 * without needing to parse complex templates in STL or Concurrency headers.
 */

namespace cove {

/**
 * An interface for exchaning progress with, and signaling cancelation to;
 * long-running operations.
 *
 * This class provides an interface for classes that would like to monitor
 * the progress of long-running operations like
 * Vectorizer::performClassification(ProgressObserver*).
 * The operations must occasionally call \a setPercentage(int) and
 * \a isInterruptionRequested() during the progress of the work.
 * The class implementing ProgressObserver will usually be something like a
 * progress dialog with progress bar and a cancel button.
 */
class ProgressObserver
{
protected:
	ProgressObserver() = default;
	ProgressObserver(const ProgressObserver&) = default;
	ProgressObserver(ProgressObserver&&) = default;
	ProgressObserver& operator=(const ProgressObserver&) = default;
	ProgressObserver& operator=(ProgressObserver&&) = default;
	
public:
	virtual ~ProgressObserver();  // Defined in Concurrency.cpp.
	
	/** 
	 * Signals the ratio of completed work as a \a percentage.
	 * 
	 * The parameter needs to be in the range 0 and 100 inclusive.
	 */
	virtual void setPercentage(int percentage) = 0;
	
	/**
	 * Returns a boolean value indicating whether the work should be canceled.
	 * 
	 * Once the ProgressObserver returns true it must do so on all subsequent calls.
	 */
	virtual bool isInterruptionRequested() const = 0;
	
};

}  // namespace cove

#endif
