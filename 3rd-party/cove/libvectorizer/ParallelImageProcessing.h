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

#ifndef COVE_PARALLELIMAGEPROCESSING_H
#define COVE_PARALLELIMAGEPROCESSING_H

#include <algorithm>
#include <cstddef>

#include <QImage>
#include <QThreadPool>

#include "Concurrency.h"

namespace cove {

class ProgressObserver;

/**
 * Use concurrent image processing of horizontal stripes.
 */
struct HorizontalStripes
{
	/// Creates a QImage stripe, meant for read-only access.
	static QImage makeStripe(const QImage& original, int scanline, int stripe_height);
	
	/// Creates am InplaceImage stripe, meant for modification of the original image.
	static QImage makeStripe(QImage& original, int scanline, int stripe_height);
	
	/// A structure to describe data to a job which modifies a target image stripe in-place.
	struct InplaceStripeData
	{
		const QImage& source;
		int source_scanline;
		int source_height;
		QImage& target;
		int target_scanline;
		int target_height;
	};
	
	/// Creates concurrent jobs.
	template <typename ResultType, typename Functor>
	static Concurrency::JobList<ResultType> makeJobs(const Functor& functor, const QImage& source, QImage& target)
	{
		Concurrency::JobList<ResultType> jobs;
		
		auto const num_jobs = std::max(1, QThreadPool::globalInstance()->maxThreadCount());
		jobs.reserve(std::size_t(num_jobs));
		
		auto const image_height = source.height();
		auto const source_stripe_height = (image_height + num_jobs - 1) / num_jobs;
		auto const output_stripe_height = (target.height() + num_jobs - 1) / num_jobs;
		for (int i = 0, j = 0; i < image_height; i += source_stripe_height, j += output_stripe_height)
		{
			// If we construct the stripes here in the main thread, just before
			// concurrent processing, there might be more than a single QImage
			// referencing the same stripe when the worker thread modifies the
			// stripe for the first time. In this situation, the modified QImage
			// would detach, and the output would not reach the original target.
			// That's why we build the stripes inside the concurrent job where
			// it is going to be used, by using the following wrapper.
			auto runner = [](const Functor& functor, InplaceStripeData& data, ProgressObserver& observer) -> ResultType {
				auto const input = HorizontalStripes::makeStripe(data.source, data.source_scanline, data.source_height);
				auto output = HorizontalStripes::makeStripe(data.target, data.target_scanline, data.target_height);
				return functor(input, output, observer);
			};
			// Only a limited number of parameters can be passed to QtConcurrent::run.
			auto data = InplaceStripeData { source, i, source_stripe_height, target, j, output_stripe_height };
			jobs.emplace_back(Concurrency::run<ResultType>(runner, functor, data));
		}
		
		return jobs;
	}
};


}  // namespace cove

#endif  // COVE_PARALLELIMAGEPROCESSING_H
