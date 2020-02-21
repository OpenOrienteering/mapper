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

#include "Vectorizer.h"

#include <algorithm>
#include <cstdlib>
#include <iosfwd>
#include <iterator>
#include <numeric>
#include <utility>
#include <type_traits>

#include <Qt>
#include <QtGlobal>
#include <QColor>
#include <QException>
#include <QImage>
#include <QVector>

#include "AlphaGetter.h"
#include "Concurrency.h"
#include "ProgressObserver.h"
#include "KohonenMap.h"
#include "MapColor.h"
#include "Morphology.h"
#include "ParallelImageProcessing.h"
#include "PatternGetter.h"

namespace cove {

/*! \class Vectorizer
 * \ingroup libvectorizer
 * \brief Facade for vectorization process.
 *
 * This class provides facade for underlying classes MapColor (and its
 * derivates),
 * KohonenMap, Morphology and several others. The usual way how to cope with
 * this class is
 * - construct Vectorizer with an QImage (Vectorizer(QImage& im))
 * - set number of recognized colors, learning method and parameters (or leave
 *   them untouched when you are happy with them)
 * - call performClassification()
 * - get the resulting colors with getClassifiedColors()
 * - obtain the classified image with getClassifiedImage()
 * - after selecting the colors for separation call getBWImage()
 * - and in case of need thin the lines in the image with getThinnedImage()
 */

/*!
 * \enum Vectorizer::ColorSpace
 * Type of color space used.
 */
/*! \var Vectorizer::ColorSpace Vectorizer::COLSPC_RGB
 * Colors are from RGB space.
 */
/*! \var Vectorizer::ColorSpace Vectorizer::COLSPC_HSV
 * Colors are from HSV space.
 */
/*! \enum Vectorizer::LearningMethod
 * Learning method to be used.
 */
/*! \var Vectorizer::LearningMethod Vectorizer::KOHONEN_CLASSIC
 * Classic Kohonen learning with random pattern selection and stepwise lowered
 * speed of learning.
 */
/*! \var Vectorizer::LearningMethod Vectorizer::KOHONEN_BATCH
 * Kohonens' batch learning equivalent to ISODATA.
 */
/*! \enum Vectorizer::AlphaStrategy
 Possible alpha selection strategies for KOHONEN_CLASSIC learning. */
/*! \var Vectorizer::AlphaStrategy Vectorizer::ALPHA_CLASSIC
 Alpha starts at initAlpha, does E cycles of learning, gets multiplied by q and
 does and returns to the previous step (E cycles) until it falls below
 minAlpha. \sa ClassicAlphaGetter */
/*! \var Vectorizer::AlphaStrategy Vectorizer::ALPHA_NEUQUANT
 Not implemented yet. */

/*! \enum Vectorizer::PatternStrategy
 Selection of pixels from source image. */
/*! \var Vectorizer::PatternStrategy Vectorizer::PATTERN_RANDOM
 Random selection, used with KOHONEN_CLASSIC learning. */
/*! \var Vectorizer::PatternStrategy Vectorizer::PATTERN_NEUQUANT
 Not implemented yet. */

/*! \enum Vectorizer::MorphologicalOperation
 Enum of different possible morphological operations. */
/*! \var Vectorizer::MorphologicalOperation Vectorizer::EROSION
 *  Perform erosion on BW image.*/
/*! \var Vectorizer::MorphologicalOperation Vectorizer::DILATION
 *  Perform dilation on BW image. */
/*! \var Vectorizer::MorphologicalOperation Vectorizer::THINNING_ROSENFELD
 *  Perform thinning on BW image.  */
/*! \var Vectorizer::MorphologicalOperation Vectorizer::PRUNING
 *  Perform pruning on BW image, i.e. line endpoint removal.  */

/*! \enum Vectorizer::VectorizerState
  Enum of different facade states. */
/*! \var Vectorizer::VectorizerState Vectorizer::NO_IMAGE
   No image has been set.  This state cannot be skipped, use Vectorizer(QImage&
   im) constructor. */
/*! \var Vectorizer::VectorizerState Vectorizer::IMAGE
   Source image gets classified by running performClassification, then proceeds
   to CLASSIFIED_IMAGE. */
/*! \var Vectorizer::VectorizerState Vectorizer::CLASSIFIED_IMAGE
   Classified image has been created. This state cannot be skipped, use
   getBWImage to create an BW image. */
/*! \var Vectorizer::VectorizerState Vectorizer::BW_IMAGE
   BW Image gets thinned and proceeds to THINNED_IMAGE. */
/*! \var Vectorizer::VectorizerState Vectorizer::THINNED_IMAGE
   Final state. */
/*! \var Vectorizer::VectorizerState Vectorizer::state
   Data member holding the state of this facade.  */

/*! \var QImage Vectorizer::sourceImage
 Source image to be operated on. */
/*! \var QImage Vectorizer::classifiedImage
 Image created by classification. \sa getClassifiedImage */
/*! \var QImage Vectorizer::bwImage
 BW Image created from classifiedImage by selectinn colors. \sa getBWImage */
/*! \var QImage Vectorizer::thinnedBWImage
 (bad member name) BW Image after morphological operation. \sa
 getTransformedImage */
/*! \var MapColor** Vectorizer::sourceImageColors
 Classified color cluster moments. */
/*! \var MapColor* Vectorizer::mc
 Prototype of mapcolor. */
/*! \var int Vectorizer::E
 Parameter E of ClassicAlphaGetter. \sa setE */
/*! \var double Vectorizer::initAlpha
 Initial alpha of ClassicAlphaGetter. \sa setInitAlpha */
/*! \var double Vectorizer::q
 Parameter q of ClassicAlphaGetter. \sa setQ */
/*! \var double Vectorizer::minAlpha
 Minimal alpha of ClassicAlphaGetter. \sa setMinAlpha */
/*! \var double Vectorizer::p
 Minkowski metrics parameter. \sa setP */
/*! \var double Vectorizer::quality
 Quality of learning as returned by getClassifiedImage. \sa getClassifiedImage
 */
/*! \var LearningMethod Vectorizer::learnMethod
 Selected learning method. \sa setClassificationMethod */
/*! \var ColorSpace Vectorizer::colorSpace
 Selected color space. \sa setColorSpace */
/*! \var AlphaStrategy Vectorizer::alphaStrategy
 Selected alpha selection strategy. \sa setAlphaStrategy */
/*! \var PatternStrategy Vectorizer::patternStrategy
 Selected pattern selection strategy. \sa setPatternStrategy */

Vectorizer::Vectorizer()
	: mc(std::make_unique<MapColorRGB>(2.0))
	, E(100000)
	, initAlpha(0.1)
	, q(0.5)
	, minAlpha(1e-6)
	, p(2)
	, learnMethod(KOHONEN_CLASSIC)
	, colorSpace(COLSPC_RGB)
	, alphaStrategy(ALPHA_CLASSIC)
	, patternStrategy(PATTERN_RANDOM)
{
}

//! Constructs Vectorizer with im as sourceImage.
Vectorizer::Vectorizer(QImage& im)
	: Vectorizer()
{
	sourceImage = im;
}

void Vectorizer::deleteColorsTable()
{
	sourceImageColors.resize(0);
}

/*! Choose classification method to be used.
\sa LearningMethod */
void Vectorizer::setClassificationMethod(LearningMethod learnMethod)
{
	this->learnMethod = learnMethod;
}

/*! Set color space in which the classification will be performed. */
void Vectorizer::setColorSpace(ColorSpace colorSpace)
{
	this->colorSpace = colorSpace;
	switch (colorSpace)
	{
	case COLSPC_RGB:
		mc = std::make_unique<MapColorRGB>(p);
		break;
	case COLSPC_HSV:
		mc = std::make_unique<MapColorHSV>(p);
		break;
	default:
		qWarning("UNIMPLEMENTED colorspace");
	}
}

/*! Set Minkowski metrics parameter.
  \sa MapColor */
void Vectorizer::setP(double p)
{
	this->p = p;
	mc->setP(p);
}

/*! Choose alpha selection method to be used.
\sa AlphaStrategy */
void Vectorizer::setAlphaStrategy(AlphaStrategy alphaStrategy)
{
	this->alphaStrategy = alphaStrategy;
}

/*! Choose pattern selection method to be used.
\sa PatternStrategy */
void Vectorizer::setPatternStrategy(PatternStrategy patternStrategy)
{
	this->patternStrategy = patternStrategy;
}

/*! Set initial alpha for ClassicAlphaGetter.
\sa ClassicAlphaGetter */
void Vectorizer::setInitAlpha(double initAlpha)
{
	this->initAlpha = initAlpha;
}

/*! Set minimal alpha for ClassicAlphaGetter.
\sa ClassicAlphaGetter */
void Vectorizer::setMinAlpha(double minAlpha)
{
	this->minAlpha = minAlpha;
}

/*! Set q for ClassicAlphaGetter.
\sa ClassicAlphaGetter */
void Vectorizer::setQ(double q)
{
	this->q = q;
}

/*! Set E for ClassicAlphaGetter.
\sa ClassicAlphaGetter */
void Vectorizer::setE(int E)
{
	this->E = E;
}

/*! Set number of colors to be recognized in the picture.
  */
void Vectorizer::setNumberOfColors(int nColors)
{
	deleteColorsTable();
	sourceImageColors.resize(nColors);
}

/*! set initial colors for learning.  Sets random colors in case initColors is
 * 0. */
void Vectorizer::setInitColors(const std::vector<QRgb>& initColors)
{
	for (std::size_t i = 0; i < sourceImageColors.size(); i++)
	{
		sourceImageColors[i] =
			std::shared_ptr<MapColor>(dynamic_cast<MapColor*>(mc->clone()));

		if (!initColors.empty())
		{
			// Get provided colors
			sourceImageColors[i]->setRGBTriplet(initColors[i]);
		}
		else
		{
			// Random colors at beginning
			sourceImageColors[i]->setRGBTriplet(
				qRgb(rand() / (RAND_MAX / 255),    // NOLINT
				     rand() / (RAND_MAX / 255),    // NOLINT
				     rand() / (RAND_MAX / 255)));  // NOLINT
		}
	}
}

/*! Runs classfication.  The progress observer is called during work if set.
 * \param[in] progressObserver Pointer to class implementing ProgressObserver.
 * In case it is null pointer no calls take place.
 */
bool Vectorizer::performClassification(ProgressObserver* progressObserver)
{
	// invalidate previous images
	bwImage = classifiedImage = QImage();
	std::unique_ptr<KohonenPatternGetter> pg;

	// if no initial colors have been set, select random ones
	if (sourceImageColors.empty()) setInitColors({});

	std::vector<OrganizableElement*> classes(sourceImageColors.size());
	for (std::size_t i = 0; i < sourceImageColors.size(); i++)
		classes[i] = sourceImageColors[i].get();

	KohonenMap km;
	km.setClasses(classes);

	switch (learnMethod)
	{
	case KOHONEN_CLASSIC:
	{
		switch (patternStrategy)
		{
		case PATTERN_RANDOM:
			pg = std::make_unique<RandomPatternGetter>(sourceImage, mc.get());
			break;
		default:
			qWarning("UNIMPLEMENTED pattern getter");
		}

		std::unique_ptr<KohonenAlphaGetter> ag;
		switch (alphaStrategy)
		{
		case ALPHA_CLASSIC:
			ag = std::make_unique<ClassicAlphaGetter>(initAlpha, q, E, minAlpha,
													  progressObserver);
			break;
		default:
			qWarning("UNIMPLEMENTED alpha getter");
		}

		km.performLearning(*ag, *pg);
	}
	break;
	case KOHONEN_BATCH:
	{
		auto bg = std::make_unique<SequentialPatternGetter>(
			sourceImage, mc.get(), progressObserver);
		quality = km.performBatchLearning(*bg);
		classifiedImage = *bg->getClassifiedImage();
	}
	break;
	default:
		qWarning("UNIMPLEMENTED classification method");
	}

	bool cancel = progressObserver && progressObserver->isInterruptionRequested();
	if (cancel)
	{
		deleteColorsTable();
	}
	else
	{
		auto classes = km.getClasses();

		for (std::size_t i = 0; i < sourceImageColors.size(); i++)
			sourceImageColors[i] = std::shared_ptr<MapColor>(
				dynamic_cast<MapColor*>(classes[i]->clone()));
	}

	return !cancel;
}

/*! Returns colors found by classification process. */
std::vector<QRgb> Vectorizer::getClassifiedColors()
{
	if (!sourceImageColors.size()) return {};

	std::vector<QRgb> retval(sourceImageColors.size());
	for (std::size_t i = 0; i < sourceImageColors.size(); i++)
		retval[i] = sourceImageColors[i]->getRGBTriplet();

	return retval;
}

/*! Creates image where original pixel colors are replaced by "closest
 * momentum" colors.  Also computes quality of learning.
 * \param qualityPtr Pointer to double where quality of learning will be stored.
 * \param progressObserver Optional progress observer.
 * \return New image. */

class ClassificationMapper
{
	const std::vector<std::shared_ptr<MapColor>>& sourceImageColors;
	const KohonenMap& km;

public:
	ClassificationMapper(const std::vector<std::shared_ptr<MapColor>>& sic, KohonenMap& km)
		: sourceImageColors(sic)
		, km(km)
	{}

	double operator()(const QImage& sourceImage, QImage& outputImage, ProgressObserver& observer) const
	{
		auto const width = outputImage.width();
		auto const height = outputImage.height();

		auto color = std::unique_ptr<MapColor>(
			dynamic_cast<MapColor*>(sourceImageColors[0]->clone()));

		double quality = 0;
		for (int y = 0; y < height && !observer.isInterruptionRequested(); y++)
		{
			for (int x = 0; x < width; x++)
			{
				double distance;
				color->setRGBTriplet(sourceImage.pixel(x, y));
				int index = km.findClosest(*color, distance);
				outputImage.setPixel(x, y, index);
				quality += color->squares(*sourceImageColors[index]);
			}
			observer.setPercentage((100*y) / height);
		}
		return quality;
	}

	using concurrent_processing = HorizontalStripes;
};
Q_STATIC_ASSERT((Concurrency::supported<ClassificationMapper>::value));


QImage Vectorizer::getClassifiedImage(double* qualityPtr,
									  ProgressObserver* progressObserver)
{
	if (classifiedImage.isNull())
	{
		classifiedImage = QImage(sourceImage.size(), QImage::Format_Indexed8);
		classifiedImage.setColorCount(sourceImageColors.size());
		for (std::size_t i = 0; i < sourceImageColors.size(); i++)
		{
			classifiedImage.setColor(i, sourceImageColors[i]->getRGBTriplet());
		}

		std::vector<OrganizableElement*> classes(sourceImageColors.size());
		for (std::size_t i = 0; i < sourceImageColors.size(); i++)
			classes[i] = sourceImageColors[i].get();

		KohonenMap km;
		km.setClasses(classes);

		auto mapFunctor = ClassificationMapper(sourceImageColors, km);
		auto results = Concurrency::process<double>(progressObserver, mapFunctor, sourceImage, classifiedImage);
		if (progressObserver && progressObserver->isInterruptionRequested())
		{
			classifiedImage = QImage();
			quality = 0;
		}
		else
		{
			quality = std::accumulate(begin(results), end(results), 0.0, [](auto a, auto& b) { return a + b.future.result(); });
		}
	}

	if (qualityPtr) *qualityPtr = quality;

	return classifiedImage;
}

/*! Creates BW image where pixels are on positions where any of the selected
 * colors were.  Final image is also held in data member bwImage.
 \param[in] selectedColors Boolean array where true means color is selected and
 pixels of that color will be set to 1 in output image.
 \param[in] progressObserver Progress observer.
 */
class BWMapper
{
	const std::vector<bool>& selectedColors;

public:
	explicit BWMapper(const std::vector<bool>& sc)
	    : selectedColors(sc)
	{}

	void operator()(const QImage& source_image, QImage& output_image, ProgressObserver& progressObserver) const
	{
		auto const width = source_image.width();
		auto const height = source_image.height();
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				auto colorIndex = source_image.pixelIndex(x, y);
				output_image.setPixel(x, y, selectedColors[colorIndex]);
			}
			progressObserver.setPercentage((100*y) / height);
		}
	}

	using concurrent_processing = HorizontalStripes;
};
Q_STATIC_ASSERT((Concurrency::supported<BWMapper>::value));

QImage Vectorizer::getBWImage(std::vector<bool> selectedColors,
                              ProgressObserver* progressObserver)
{
	if (bwImage.isNull()
	    || !std::equal(begin(selectedColors), end(selectedColors),
	                   begin(this->selectedColors), end(this->selectedColors)))
	{
		this->selectedColors = selectedColors;
		bwImage = QImage(sourceImage.size(), QImage::Format_Mono);
		bwImage.setColorTable(
			QVector<QRgb>{QColor(Qt::white).rgb(), QColor(Qt::black).rgb()});

		auto mapFunctor = BWMapper(std::move(selectedColors));
		Concurrency::process(progressObserver, mapFunctor, classifiedImage, bwImage);
		if (progressObserver && progressObserver->isInterruptionRequested())
			bwImage = {};
	}

	return bwImage;
}

/*! Performs selected Morphological operation on data member bwImage.
 \sa MorphologicalOperation getTransformedImage(QImage bwImage,
 MorphologicalOperation mo, ProgressObserver* progressObserver) */
QImage Vectorizer::getTransformedImage(MorphologicalOperation mo,
									   ProgressObserver* progressObserver)
{
	QImage i = Vectorizer::getTransformedImage(bwImage, mo, progressObserver);
	if (!i.isNull()) bwImage = i;
	return i;
}

/*! Performs selected Morphological operation.
  \param[in] bwImage BW image to be processed.
  \param[in] mo Morphological operation to be performed.
  \param[in] progressObserver Progress observer.
  \return Transformed BW image.
 \sa MorphologicalOperation */
QImage Vectorizer::getTransformedImage(const QImage& bwImage,
                                       MorphologicalOperation mo,
                                       ProgressObserver* progressObserver)
{
	QImage outputImage;
	bool (Morphology::*operation)(ProgressObserver*) = nullptr;
	switch (mo)
	{
	case EROSION:
		operation = &Morphology::erosion;
		break;
	case DILATION:
		operation = &Morphology::dilation;
		break;
	case THINNING_ROSENFELD:
		operation = &Morphology::rosenfeld;
		break;
	case PRUNING:
		operation = &Morphology::pruning;
		break;
	}
	if (operation)
	{
		auto functor = [operation](const QImage& source_image, ProgressObserver& progressObserver) -> QImage {
			Morphology morphology(source_image);
			auto ok = (morphology.*operation)(&progressObserver);
			progressObserver.setPercentage(100);
			return ok ? morphology.getImage() : QImage{};
		};
		outputImage = Concurrency::process<QImage>(progressObserver, functor, bwImage);
	}

	return outputImage;
}


} // cove

//@}
