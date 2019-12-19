/*
 * Copyright (c) 2005-2019 Libor Pecháček.
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

#include <QImage>
#include <QList>
#include <QSize>
#include <QThread>
#include <QtConcurrent>

namespace cove {
class ParallelImageProcessing
{
public:
	class Functor
	{
		Functor() = delete;

	protected:
		const QImage& sourceImage;
		QImage& outputImage;

		Functor(const QImage& si, QImage& oi)
			: sourceImage(si)
			, outputImage(oi)
		{
			Q_ASSERT(sourceImage.size() == outputImage.size());
		}

	public:
		struct ImagePart
		{
			int start, len;
		};

		virtual bool operator()(const ImagePart&) = 0;
		QSize imageSize()
		{
			return sourceImage.size();
		}
	};

	template <typename FunctorType> static void process(FunctorType& functor)
	{
		// use filter's parallel job scheduling capabilities
		QList<Functor::ImagePart> sequence;

		int nCores =
			QThread::idealThreadCount() > 1 ? QThread::idealThreadCount() : 1;

		int imageHeight = functor.imageSize().height();
		int stripeHeight = (imageHeight + nCores) / nCores;
		for (int i = 0; i < nCores; i++)
		{
			Functor::ImagePart piece;
			piece.start = i * stripeHeight;
			piece.len = piece.start < imageHeight - stripeHeight
							? stripeHeight
							: imageHeight - piece.start;
			sequence.append(piece);
		}

		QtConcurrent::blockingFilter(sequence, functor);
	}
};
} // cove

#endif // PARALLELIMAGEPROCESSING_H
