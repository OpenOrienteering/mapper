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

#include "PatternGetter.h"

#include <cmath>
#include <cstdlib>
#include <ctime>

#include "MapColor.h"
#include "Vectorizer.h"

namespace cove {
//@{
//!\ingroup libvectorizer

/*! \class PatternGetterDataMembers
  \brief Data members common to RandomPatternGetter and SequentialPatternGetter
  and their common initialization.
  */

/*! \var const QImage& PatternGetterDataMembers::image
  Source image. */
/*! \var MapColor* PatternGetterDataMembers::retval
  Prototype of the MapColor returned by getPattern. getPattern calls
  setRGBTriplet and returns this pointer. */
/*! \var int PatternGetterDataMembers::width
  Width of source image. */
/*! \var int PatternGetterDataMembers::height
  Height of source image. */

/*! Constructor initializing image from i, retval from mc, width and height from
  i.
  \param[in] i Image to be used.
  \param[in] mc MapColor subclass instance to be used. */
PatternGetterDataMembers::PatternGetterDataMembers(const QImage& i,
												   MapColor* mc)
	: image(i)
	, retval(dynamic_cast<MapColor*>(mc->clone()))
	, width(image.width())
	, height(image.height())
{
}

/*! Destructor, deletes retval. */
PatternGetterDataMembers::~PatternGetterDataMembers()
{
	delete retval;
}

// - RANDOM PATTERN GETTER -------------------
/*! \class RandomPatternGetter
  \brief Returns pixels from source image in a random manner.
  */

/*! Constructor.
 \param[in] im Image to be operated on.
 \param[in] mc Instance of a MapColor subclass that is used in classification.
 */
RandomPatternGetter::RandomPatternGetter(const QImage& im, MapColor* mc)
	: PatternGetterDataMembers(im, mc)
{
	srand(time(nullptr));
}

/*! Destructor, does nothing.  Needed to avoid compiler warnings. */
RandomPatternGetter::~RandomPatternGetter() = default;

/*! Get next pattern (pixel) in random order. */
const OrganizableElement* RandomPatternGetter::getPattern()
{
	int rx = rand() / (RAND_MAX / (width - 1)),
		ry = rand() / (RAND_MAX / (height - 1));
	// TODO find a better way to avoid rx & ry run out of bounds
	if (rx >= width)
	{
		rx /= 2;
	}
	if (ry >= height)
	{
		ry /= 2;
	}
	retval->setRGBTriplet(image.pixel(rx, ry));
	return retval;
}

// - SEQUENTIAL PATTERN GETTER ---------------
/*! \class SequentialPatternGetter
  \brief Returns pixels from source image in a sequential manner.
  */
/*! \var QImage SequentialPatternGetter::classifiedImage
Image holding classes of pixels as its index of palette.  After filling the
palette with moments, classified image appears. */
/*! \var ProgressObserver* SequentialPatternGetter::progressObserver
 Progress observer to be notified. */
/*! \var int SequentialPatternGetter::x
 X-coordinate of the current element. */
/*! \var int SequentialPatternGetter::y
 Y-coordinate of the current element. */
/*! \var int SequentialPatternGetter::nChanges
 Number of pixel class reassignments since last reset. */

/*! Constructor.
 \param[in] im Image to be operated on.
 \param[in] mc Instance of a MapColor subclass that is used in classification.
 \param[in] progressObserver Progress observer. */
SequentialPatternGetter::SequentialPatternGetter(
	const QImage& im, MapColor* mc, ProgressObserver* progressObserver)
	: PatternGetterDataMembers(im, mc)
	, progressObserver(progressObserver)
	, nChanges(0)
{
	classifiedImage = QImage(width, height, QImage::Format_Indexed8);
	classifiedImage.setColorCount(256);
	reset();
}

/*! Destructor, does nothing.  Needed to avoid compiler warnings. */
SequentialPatternGetter::~SequentialPatternGetter() = default;

/*! Get next pattern (pixel) in sequential order. */
const OrganizableElement* SequentialPatternGetter::getPattern()
{
	if (y < height)
	{
		x++;
		if (x >= width)
		{
			x = 0;
			y++;
			// this is to avoid comparison if(y < height-1) that involves
			// variable
			// decrement in the innermost loop
			if (y >= height)
			{
				if (progressObserver)
					progressObserver->percentageChanged(
						100 - static_cast<int>(
								  100 * pow(static_cast<float>(nChanges) /
												(width * height),
											0.2)));

				return nullptr;
			}

			if (progressObserver && progressObserver->getCancelPressed())
				return nullptr;
		}

		retval->setRGBTriplet(image.pixel(x, y));
		return retval;
	}
	else
	{
		return nullptr;
	}
}

/*! Get class of the last element got. \sa getPattern */
int SequentialPatternGetter::getLastElementClass() const
{
	return classifiedImage.pixelIndex(x, y);
}

/*! Set class of the last element got. Also updates number of changes. \sa
 * getPattern */
void SequentialPatternGetter::setLastElementClass(int classNumber)
{
	if (getLastElementClass() != classNumber) nChanges++;
	classifiedImage.setPixel(x, y, classNumber);
}

/*! Reset the pattern getter into initial state. */
void SequentialPatternGetter::reset()
{
	if (progressObserver && progressObserver->getCancelPressed())
	{
		y = height;					// do not allow to restart
		classifiedImage = QImage(); // image is a null one
	}
	else
	{
		x = -1;
		y = 0;
	}

	nChanges = 0;
}

/*! Returns number of class assignment changes happened in
 * setLastElementClass(int) since the last call to reset.
 */
int SequentialPatternGetter::numberOfChanges()
{
	return nChanges;
}

/*! Return image created by batch learning. */
QImage* SequentialPatternGetter::getClassifiedImage()
{
	return &classifiedImage;
}
} // cove

//@}
