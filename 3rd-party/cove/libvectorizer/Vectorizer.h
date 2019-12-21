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

#ifndef COVE_VECTORIZER_H
#define COVE_VECTORIZER_H

#include <memory>
#include <vector>
#include <QColor>
#include <QImage>

#include "libvectorizer/MapColor.h"

namespace cove {
class ProgressObserver
{
public:
	virtual ~ProgressObserver();
	virtual void percentageChanged(int percentage) = 0;
	virtual bool getCancelPressed() = 0;
};

class Vectorizer
{
public:
	enum ColorSpace
	{
		COLSPC_RGB,
		COLSPC_HSV
	};
	enum LearningMethod
	{
		KOHONEN_CLASSIC,
		KOHONEN_BATCH
	};
	enum AlphaStrategy
	{
		ALPHA_CLASSIC,
		ALPHA_NEUQUANT
	};
	enum PatternStrategy
	{
		PATTERN_RANDOM,
		PATTERN_NEUQUANT
	};
	enum MorphologicalOperation
	{
		EROSION,
		DILATION,
		THINNING_ROSENFELD,
		PRUNING
	};

protected:
	QImage sourceImage;
	QImage classifiedImage;
	QImage bwImage;
	QImage thinnedBWImage;
	std::vector<std::shared_ptr<MapColor>> sourceImageColors;
	std::unique_ptr<MapColor> mc;
	int E;
	double initAlpha, q, minAlpha, p, quality;
	LearningMethod learnMethod;
	ColorSpace colorSpace;
	AlphaStrategy alphaStrategy;
	PatternStrategy patternStrategy;

	void deleteColorsTable();

public:
	Vectorizer();
	Vectorizer(QImage& im);
	virtual void setClassificationMethod(LearningMethod learnMethod);
	virtual void setColorSpace(ColorSpace colorSpace);
	virtual void setP(double p);
	virtual void setAlphaStrategy(AlphaStrategy alphaStrategy);
	virtual void setPatternStrategy(PatternStrategy patternStrategy);
	virtual void setInitAlpha(double initAlpha);
	virtual void setMinAlpha(double minAlpha);
	virtual void setQ(double q);
	virtual void setE(int E);
	virtual void setNumberOfColors(int nColors);
	virtual void setInitColors(const std::vector<QRgb>& initColors);
	virtual bool performClassification(ProgressObserver* progressObserver = 0);
	std::vector<QRgb> getClassifiedColors();
	virtual QImage getClassifiedImage(double* qualityPtr = 0,
									  ProgressObserver* progressObserver = 0);
	virtual QImage getBWImage(std::vector<bool> selectedColors,
							  ProgressObserver* progressObserver = 0);
	virtual QImage getTransformedImage(MorphologicalOperation mo,
									   ProgressObserver* progressObserver = 0);
	static QImage getTransformedImage(QImage bwImage, MorphologicalOperation mo,
									  ProgressObserver* progressObserver = 0);
};
} // cove

#endif
