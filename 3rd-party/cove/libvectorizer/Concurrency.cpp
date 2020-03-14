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

#include "Concurrency.h"

#include <memory>

#include <QAtomicInteger>


namespace cove {

// ### ProgressObserver, from ProgressObserver.h ###

ProgressObserver::~ProgressObserver() = default;



namespace Concurrency {

// ### Concurrency::Progress ###

int Progress::getPercentage() const noexcept
{
	return data->percentage.load();
}

void Progress::setPercentage(int percentage)
{
	data->percentage.store(percentage);
}

bool Progress::isInterruptionRequested() const
{
	return bool(data->canceled.load());
}

void Progress::requestInterruption() noexcept
{
	data->canceled.store(int(true));
}


// ### Concurrency::TransformedProgress ###

void TransformedProgress::setPercentage(int percentage)
{
	observer.setPercentage(qBound(0, qRound(offset + factor * percentage), 100));
}


}  // namespace Concurrency

}  // cove

