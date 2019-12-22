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

#include "Vectorizer.h"

#include <cstdlib>
#include <iosfwd>

#include <QColor>
#include <QImage>
#include <QVector>
#include <Qt>
#include <QtGlobal>

#include "AlphaGetter.h"
#include "KohonenMap.h"
#include "MapColor.h"
#include "Morphology.h"
#include "ParallelImageProcessing.h"
#include "PatternGetter.h"

namespace cove {
//@{
/*! \defgroup libvectorizer Library for color classification and BW morphology
 */

/*! \class ProgressObserver
 * \ingroup libvectorizer
 * \brief Anyone who wants to monitor operation's progress must implement this
 * interface.
 *
 * This class provides interface for classes that would like to monitor
 * progress of long lasting operations like
 * Vectorizer::performClassification(ProgressObserver*).  The \a
 * percentageChanged(int)
 * method is called occasionally thtough the progress of the work and \a
 * getCancelPressed() is called to obtain information whether the process
 * should continue or not. The class implementing ProgressObserver will usually
 * be something like a progress dialog with progress bar and a cancel button.
 */

/*! \fn virtual void ProgressObserver::percentageChanged(int percentage) = 0;
 * This method is called occasionally with \a percentage equal toratio of work
 * done. The \a percentage is between 0 and 100 inclusive.
 * \param[in] percentage The amount of work done.
 */

/*! \fn virtual bool ProgressObserver::getCancelPressed() = 0;
 * Returns the boolean value indicating whether the work should progress or
 * finish as soon as possible. Once the ProgressObserver returns true it must
 * return true on all subsequent calls.
 * \return Value indicating whether the process should continue (false) or abort
 * (true).
 */

/*! Destroys ProgressObserver object.
 */
ProgressObserver::~ProgressObserver()
{
}

/*! \class Vectorizer
 * \ingroup libvectorizer
 * \brief Facade for vectorization process.
 *
 * This class provides facade for underlaying classes MapColor (and its
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
 does and returns to the previous step (E cycles) until it falls bellow
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
				qRgb(rand() / (RAND_MAX / 255), rand() / (RAND_MAX / 255),
					 rand() / (RAND_MAX / 255)));
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

	bool cancel = progressObserver && progressObserver->getCancelPressed();
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

class ClassificationMapper : public ParallelImageProcessing::Functor
{
	const std::vector<std::shared_ptr<MapColor>>& sourceImageColors;
	const KohonenMap& km;
	double globalQuality;
	bool& cancel;
	ProgressObserver* const progressObserver;

public:
	ClassificationMapper(const QImage& si, QImage& oi,
						 const std::vector<std::shared_ptr<MapColor>>& sic,
						 KohonenMap& km, bool& ca, ProgressObserver* po)
		: Functor(si, oi)
		, sourceImageColors(sic)
		, km(km)
		, globalQuality(0)
		, cancel(ca)
		, progressObserver(po)
	{
	}

	bool operator()(const ImagePart& p) override
	{
		int progressHowOften = (p.len > 40) ? p.len / 30 : 1;
		int width = outputImage.width();

		auto color = std::unique_ptr<MapColor>(
			dynamic_cast<MapColor*>(sourceImageColors[0]->clone()));

		for (int y = p.start; !cancel && y < p.start + p.len; y++)
		{
			double rowquality = 0;
			for (int x = 0; x < width; x++)
			{
				double distance;
				color->setRGBTriplet(sourceImage.pixel(x, y));
				int index = km.findClosest(*color, distance);
				outputImage.setPixel(x, y, index);
				rowquality += color->squares(*sourceImageColors[index]);
			}
			globalQuality += rowquality;

			// only one thread reports progress,
			// assuming all threads run at equal speed
			if (!p.start && progressObserver && !(y % progressHowOften))
			{
				progressObserver->percentageChanged(y * 100 / p.len);
				cancel = progressObserver->getCancelPressed();
			}
		}

		return false;
	}

	double getQuality()
	{
		return globalQuality;
	}
};

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

		bool cancel = false;

		auto mapFunctor = ClassificationMapper(sourceImage, classifiedImage,
											   sourceImageColors, km, cancel,
											   progressObserver);
		ParallelImageProcessing::process(mapFunctor);

		if (cancel)
		{
			classifiedImage = QImage();
			quality = 0;
		}
		else
		{
			quality = mapFunctor.getQuality();
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
class BWMapper : public ParallelImageProcessing::Functor
{
	const std::vector<bool> selectedColors;
	bool& cancel;
	ProgressObserver* const progressObserver;

public:
	BWMapper(const QImage& si, QImage& oi, const std::vector<bool>& sc,
			 bool& ca, ProgressObserver* po)
		: Functor(si, oi)
		, selectedColors(sc)
		, cancel(ca)
		, progressObserver(po)
	{
	}

	bool operator()(const ImagePart& p) override
	{
		int progressHowOften = (p.len > 40) ? p.len / 30 : 1;
		int width = outputImage.width();

		for (int y = p.start; !cancel && y < p.start + p.len; y++)
		{
			for (int x = 0; x < width; x++)
			{
				int colorIndex = sourceImage.pixelIndex(x, y);
				outputImage.setPixel(x, y, selectedColors[colorIndex]);
			}

			// only one thread reports progress,
			// assuming all threads run at equal speed
			if (!p.start && progressObserver && !(y % progressHowOften))
			{
				progressObserver->percentageChanged(y * 100 / p.len);
				cancel = progressObserver->getCancelPressed();
			}
		}

		return false;
	}
};

QImage Vectorizer::getBWImage(std::vector<bool> selectedColors,
							  ProgressObserver* progressObserver)
{
	if (bwImage.isNull())
	{
		bwImage = QImage(sourceImage.size(), QImage::Format_Mono);
		bwImage.setColorTable(
			QVector<QRgb>{QColor(Qt::white).rgb(), QColor(Qt::black).rgb()});

		bool cancel = false;

		auto mapFunctor = BWMapper(classifiedImage, bwImage, selectedColors,
								   cancel, progressObserver);

		ParallelImageProcessing::process(mapFunctor);

		if (cancel) bwImage = QImage();
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
QImage Vectorizer::getTransformedImage(QImage bwImage,
									   MorphologicalOperation mo,
									   ProgressObserver* progressObserver)
{
	Morphology thinner(bwImage);
	bool ok = false;
	switch (mo)
	{
	case EROSION:
		ok = thinner.erosion(progressObserver);
		break;
	case DILATION:
		ok = thinner.dilation(progressObserver);
		break;
	case THINNING_ROSENFELD:
		ok = thinner.rosenfeld(progressObserver);
		break;
	case PRUNING:
		ok = thinner.pruning(progressObserver);
		break;
	default:
		qWarning("UNIMPLEMENTED morphological operation");
	}
	if (ok) bwImage = thinner.getImage();

	return ok ? bwImage : QImage();
}
} // cove

//@}
