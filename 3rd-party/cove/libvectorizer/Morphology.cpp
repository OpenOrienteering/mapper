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

#include "Morphology.h"

#include <cmath>  // IWYU pragma: keep
#include <vector>

#include <QtGlobal>

#include "ProgressObserver.h"

namespace cove {
//@{
//!\ingroup libvectorizer

/*! \class Morphology
  \brief Performs basic morphological operations.

  I.e. erosion, dilation, thinning.
  */

/*! \var const QImage* Morphology::image
  Input image.
 */

/*! \var QImage* Morphology::thinnedImage
  Transformed image.
 */

//! Constructor
Morphology::Morphology(const QImage& img)
	: image(img)
	, thinnedImage(nullptr)
{
	if (image.depth() > 1) qWarning("Morphology:: can thin only 1bpp images");
}

//! Returns the resulting image.
QImage Morphology::getImage() const
{
	return thinnedImage;
}

//! Direction masks N S W E
unsigned int Morphology::masks[] = {0200, 0002, 0040, 0010};

/*! Conditional erosion

The neighborhood map is defined as an integer of bits abcdefghi with a
non-zero bit representing a non-zero pixel. The bit assignment for the
neighborhood is:

<pre>
 a b c
 d e f
 g h i
</pre>
*/

bool Morphology::todelete[512] = {
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  false, false, false, true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  false, false, false, false, false, false, false, true,  false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true,  false, false, false, true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  true,  false,
	false, false, false, false, false, false, true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  false, true,
	true,  true,  false, true,  false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  false, false,
	false, true,  false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  false, false, false, false, false, false, false,
	true,  false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, true,  false, false, false, true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	true,  true,  false, false, false, false, false, false, false, true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true,  true,  false, false, true,  true,  true,  true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  false, true,  true,  true,  false, true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, true,  true,  false, false, true,  true,  true,  true,  true,  true,
	false, false, true,  true,  false, false};

//! Rosenfeld thinning.
bool Morphology::rosenfeld(ProgressObserver* progressObserver)
{
	bool cancel = false; // whether the thinning was canceled
	unsigned int x, y;   // Pixel coordinates
	unsigned int count;  // Deleted pixel count
	unsigned int p, q;   // Neighborhood maps of adjacent cells

	thinnedImage = image;
	thinnedImage.detach();
	unsigned int xsize = thinnedImage.width();
	unsigned int ysize = thinnedImage.height();
	// Neighborhood maps of previous scanline
	std::vector<unsigned char> qb(xsize);

	qb[xsize - 1] = false; // Used for lower-right pixel

	do
	{ // Thin image lines until there are no deletions
		count = 0;

		for (auto const m : masks)
		{
			// Build initial previous scan buffer.
			p = !!thinnedImage.pixelIndex(0, 0);
			for (x = 0; x < xsize - 1; x++)
				qb[x] =
					(unsigned char)(p = ((p << 1) & 0006) |
										(!!thinnedImage.pixelIndex(x + 1, 0)));

			// Scan image for pixel deletion candidates.
			for (y = 0; y < ysize - 1; y++)
			{
				q = qb[0];
				p = ((q << 2) & 0330) | (!!thinnedImage.pixelIndex(0, y + 1));

				for (x = 0; x < xsize - 1; x++)
				{
					q = qb[x];
					p = ((p << 1) & 0666) | ((q << 3) & 0110) |
						(!!thinnedImage.pixelIndex(x + 1, y + 1));
					qb[x] = (unsigned char)p;
					if (((p & m) == 0) && todelete[p])
					{
						count++;
						thinnedImage.setPixel(x, y, 0); // delete the pixel
					}
				}

				// Process right edge pixel.
				p = (p << 1) & 0666;
				if ((p & m) == 0 && todelete[p])
				{
					count++;
					thinnedImage.setPixel(x, y, 0); // delete the pixel
				}
			}

			// Process bottom scan line.
			q = qb[0];
			p = ((q << 2) & 0330);

			for (x = 0; x < xsize; x++)
			{
				q = qb[x];
				p = ((p << 1) & 0666) | ((q << 3) & 0110);
				if ((p & m) == 0 && todelete[p])
				{
					count++;
					thinnedImage.setPixel(x, y, 0); // delete the pixel
				}
			}
		}
		if (progressObserver)
			progressObserver->setPercentage(
				100 -
				static_cast<int>(
					100 * std::pow(static_cast<float>(count) / (xsize * ysize),
								   0.2)));
	} while (
		count &&
		!(progressObserver && (cancel = progressObserver->isInterruptionRequested())));

	return !cancel;
}

//! Erosion table
bool Morphology::isDeletable[512] = {
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
	false, true,  true,  false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  false, false, true,
	true,  false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  false, false, true,  true,  false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	false, false, true,  true,  false, false};

//! Dilation table.
bool Morphology::isInsertable[512] = {
	false, false, true,  true,  false, false, true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	true,  true,  false, false, true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  false, false, true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true,  true,  false, false, true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  true,  false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  true,  false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  true,  false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  true,  false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  true,  true,
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
	true,  false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false};

//! Pruning table.
bool Morphology::isPrunable[512] = {
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  false, false,
	true,  true,  true,  true,  false, false, true,  true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, true,  true,  false, false, true,  true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  false, false, true,  true,
	true,  true,  false, false, true,  true,  false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  false, false, true,  true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, true,  true,  false, false, true,  true,  false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  false, false, true,  true,  false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, true,
	true,  false, false, true,  true,  true,  true,  false, false, true,  true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, true,  true,  false, false,
	true,  true,  false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, true,  true,  false,
	false, true,  true,  true,  true,  false, false, true,  true,  false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, true,  true,  false, false, true,  true,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, true,  true,  false, false, true,  true,  false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, true,  true,  false, false, true,  true,  false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false};

//! Performs erosion by calling runMorpholo with isDeletable table.
bool Morphology::erosion(ProgressObserver* progressObserver)
{
	return runMorpholo(isDeletable, false, progressObserver);
}

//! Performs dilation by calling runMorpholo with isInsertable table.
bool Morphology::dilation(ProgressObserver* progressObserver)
{
	return runMorpholo(isInsertable, true, progressObserver);
}

//! Performs pruning by calling runMorpholo with isPrunable table.
bool Morphology::pruning(ProgressObserver* progressObserver)
{
	return runMorpholo(isPrunable, false, progressObserver);
}

//! Prepares new thinnedImage and runs modifyImage once.
bool Morphology::runMorpholo(bool* table, bool insert,
							 ProgressObserver* progressObserver)
{
	thinnedImage = image;
	thinnedImage.detach();
	return modifyImage(table, insert, progressObserver) > -1;
}

/*! Modifies thinnedImage according to given table.  Builds 3x3 neighborhood for
  every pixel and sets/resets (according to insert) the pixel in case the table
  contains true.
  \param[in] table Neighborhood table, e.g. isDeletable or isInsertable
  \param[in] insert Whether the pixel should be set or reset when the table
  contains true.
  \param[in] progressObserver Progress observer.
  */
int Morphology::modifyImage(bool* table, bool insert,
							ProgressObserver* progressObserver)
{
	int xSize = thinnedImage.width();
	int ySize = thinnedImage.height();
	int progressHowOften = (ySize > 100) ? ySize / 75 : 1;
	std::vector<bool> topScanLine(xSize);
	std::vector<bool> midScanLine(xSize);
	std::vector<bool> botScanLine(xSize);
	int p, modifications;
	bool cancel = false;

	modifications = 0;
	// prepare buffers
	for (int x = 0; x < xSize; x++)
	{
		midScanLine[x] = false;
		botScanLine[x] = !!thinnedImage.pixelIndex(x, 0);
	}

	for (int y = 0; y < ySize - 1 && !cancel; y++)
	{
		// rotate buffers
		auto& temp = topScanLine;
		topScanLine = midScanLine;
		midScanLine = botScanLine;
		botScanLine = temp;
		// initialize bottom line
		botScanLine[0] = !!thinnedImage.pixelIndex(0, y + 1);
		// create neighborhood
		p = int(topScanLine[0]) << 6 | int(topScanLine[1]) << 5 | int(midScanLine[0]) << 3 |
			int(midScanLine[1]) << 2 | int(botScanLine[0]);

		// process line
		for (int x = 0; x < xSize - 1; x++)
		{
			botScanLine[x + 1] = !!thinnedImage.pixelIndex(x + 1, y + 1);
			p = ((p << 1) & 0666) | int(topScanLine[x + 1]) << 6 |
				int(midScanLine[x + 1]) << 3 | int(botScanLine[x + 1]);
			if (table[p])
			{
				thinnedImage.setPixel(x, y, insert ? 1 : 0); // set the pixel
				// TTR midScanLine[x] = false;
				modifications++;
			}
		}

		// right margin line processing
		p = (p << 1) & 0666;
		if (table[p])
		{
			thinnedImage.setPixel(xSize - 1, y,
								  insert ? 1 : 0); // set the pixel
			// TTR midScanLine[xSize-1] = false;
			modifications++;
		}

		if (progressObserver && !(y % progressHowOften))
		{
			progressObserver->setPercentage(y * 100 / ySize);
			cancel = progressObserver->isInterruptionRequested();
		}
	}

	// bottom margin line processing
	auto& temp = topScanLine;
	topScanLine = midScanLine;
	midScanLine = botScanLine;
	botScanLine = temp;
	p = int(topScanLine[0]) << 7 | int(topScanLine[1]) << 6 | int(midScanLine[0]) << 4 |
		int(midScanLine[1]) << 3;
	for (int x = 0; x < xSize - 1 && !cancel; x++)
	{
		p = ((p << 1) & 0666) | int(topScanLine[x + 1]) << 6 |
			int(midScanLine[x + 1]) << 3;
		if (table[p])
		{
			thinnedImage.setPixel(x, ySize - 1,
								  insert ? 1 : 0); // set the pixel
			// TTR midScanLine[x] = false;
			modifications++;
		}
	}

	// bottom right pixel
	p = (p << 1) & 0666;
	if (table[p])
	{
		thinnedImage.setPixel(xSize - 1, ySize - 1,
							  insert ? 1 : 0); // set the pixel
		// TTR midScanLine[xSize-1] = false;
		modifications++;
	}

	return cancel ? -1 : modifications;
}
} // cove

//@}
