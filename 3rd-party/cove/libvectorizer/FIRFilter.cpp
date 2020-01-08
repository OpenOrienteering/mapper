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

#include "FIRFilter.h"

#include <memory>

#include <QImage>

#include "MapColor.h"
#include "ProgressObserver.h"

namespace cove {
//@{
//! \ingroup libvectorizer

/*! \class FIRFilter
  \brief FIR (Finite Image Response) filter that can be applied onto QImage.

  Implemented filters are Box and Binomic of arbitrary size.
 */

/*! \var unsigned FIRFilter::dimension
  FIR filter matrix dimension. */

/*! \var double** FIRFilter::matrix
  FIR filter matrix. */

/*! Constructor, allocates \a matrix.
  \param[in] radius Filter radius. 1 => matrix size 1x1, 2 => 3x3, 3 => 5x5, ...
  */
FIRFilter::FIRFilter(unsigned radius)
{
	int dimension = radius * 2 - 1;
	if (dimension > 0)
	{
		matrix.resize(dimension);
		for (int i = 0; i < dimension; i++)
			matrix[i].resize(dimension);
	}
}

/*! Fills binomic FIR filter values into \a matrix. */
FIRFilter& FIRFilter::binomic()
{
	unsigned dimension = matrix.size();
	std::vector<double> temprow(dimension + 1);
	temprow[0] = 0;
	temprow[dimension] = 0;

	for (unsigned i = 0; i < dimension; i++)
		matrix[0][i] = 0;
	matrix[0][dimension / 2] = 1;

	// create binomic coefficients
	while (matrix[0][0] == 0)
	{
		for (unsigned i = 1; i < dimension; i++)
			temprow[i] = matrix[0][i] + matrix[0][i - 1];
		for (unsigned i = 0; i < dimension; i++)
			matrix[0][i] = temprow[i] + temprow[i + 1];
	}

	// create other elements
	double divisor = 0;
	for (unsigned i = 0; i < dimension; i++)
		for (unsigned j = 0; j < dimension; j++)
		{
			matrix[i][j] = matrix[0][i] * matrix[0][j];
			divisor += matrix[i][j];
		}

	// normalize matrix so that sum of all its elements gives 1
	for (unsigned i = 0; i < dimension; i++)
		for (unsigned j = 0; j < dimension; j++)
			matrix[i][j] /= divisor;

	return *this;
}

/*! Fills box FIR filter values into \a matrix. */
FIRFilter& FIRFilter::box()
{
	unsigned dimension = matrix.size();
	double q = 1.0 / (dimension * dimension);

	for (unsigned i = 0; i < dimension; i++)
		for (unsigned j = 0; j < dimension; j++)
			matrix[i][j] = q;

	return *this;
}

//! Not implemented yet.
FIRFilter& FIRFilter::a(double /*center*/)
{
	return *this;
}

/*! Applies this FIR filter onto image and returns transformed image.
  \param[in] source Source image.
  \param[in] outOfBoundsColor Color that has the out-of-bounds area.
  \param[in] progressObserver Progress observer.  */
QImage FIRFilter::apply(const QImage& source, QRgb outOfBoundsColor,
						ProgressObserver* progressObserver)
{
	int imwidth = source.width(), imheight = source.height();
	bool cancel = false;
	int progressHowOften = (imheight > 100) ? imheight / 75 : 1;
	QImage retimage(imwidth, imheight, QImage::Format_RGB32);

	unsigned dimension = matrix.size();
	unsigned bordercut = dimension / 2;
	MapColorRGB c, p;
	for (int y = 0; !cancel && y < imheight; y++)
	{
		for (int x = 0; x < imwidth; x++)
		{
			c.setRGBTriplet(qRgb(0, 0, 0));
			for (unsigned i = 0; i < dimension; i++)
				for (unsigned j = 0; j < dimension; j++)
				{
					if (source.valid(x + i - bordercut, y + j - bordercut))
						p.setRGBTriplet(
							source.pixel(x + i - bordercut, y + j - bordercut));
					else
						p.setRGBTriplet(outOfBoundsColor);
					p.multiply(matrix[i][j]);
					c.add(p);
				}
			retimage.setPixel(x, y, c.getRGBTriplet());
		}
		if (progressObserver && !(y % progressHowOften))
		{
			progressObserver->setPercentage(y * 100 / imheight);
			cancel = progressObserver->isInterruptionRequested();
		}
	}
	return cancel ? QImage() : retimage;
}
} // cove

//@}
