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

#ifndef PARALLELIMAGEPROCESSING_H
#define PARALLELIMAGEPROCESSING_H

#include <algorithm>
#include <iosfwd>

#include <QtGlobal>
#include <QImage>
#include <QThreadPool>

#include "Concurrency.h"

namespace cove {

/**
 * A QImage that will always use the original image's bits, even if copied.
 */
class InplaceImage : public QImage
{
public:
	InplaceImage(const QImage& original, uchar* bits, int width, int height, int bytes_per_line, QImage::Format format);
	explicit InplaceImage(QImage& source);
	InplaceImage(QImage&& source) = delete;
};


/**
 * Use concurrent image processing of horizontal stripes.
 */
struct HorizontalStripes
{
	/// Creates a QImage stripe, meant for read-only access.
	static QImage makeStripe(const QImage& original, int scanline, int stripe_height);
	
	/// Creates am InplaceImage stripe, meant for modification of the original image.
	static InplaceImage makeStripe(QImage& original, int scanline, int stripe_height);
	
	/// Creates concurrent jobs.
	template <typename ResultType, typename Functor>
	static Concurrency::JobList<ResultType> makeJobs(const Functor& functor, const QImage& source, InplaceImage target)
	{
		Concurrency::JobList<ResultType> jobs;
		
		auto const num_jobs = std::max(1, QThreadPool::globalInstance()->maxThreadCount());
		jobs.reserve(std::size_t(num_jobs));
		
		auto const image_height = source.height();
		auto const source_stripe_height = (image_height + num_jobs - 1) / num_jobs;
		auto const output_stripe_height = (target.height() + num_jobs - 1) / num_jobs;
		for (int i = 0, j = 0; i < image_height; i += source_stripe_height, j += output_stripe_height)
		{
			jobs.emplace_back(Concurrency::run<ResultType>(
			                      functor,
			                      HorizontalStripes::makeStripe(source, i, source_stripe_height),
			                      HorizontalStripes::makeStripe(target, j, output_stripe_height)
			));
		}
		
		return jobs;
	}
};


}  // namespace cove

#endif // PARALLELIMAGEPROCESSING_H
