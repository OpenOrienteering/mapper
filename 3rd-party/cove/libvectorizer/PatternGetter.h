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

#ifndef COVE_PATTERNGETTER_H
#define COVE_PATTERNGETTER_H

#include <QImage>

#include "KohonenMap.h"

namespace cove {
class MapColor;
class ProgressObserver;

class PatternGetterDataMembers
{
protected:
	const QImage& image;
	MapColor* retval;
	int width, height;
	PatternGetterDataMembers(const QImage& i, MapColor* mc);
	~PatternGetterDataMembers();
};

class RandomPatternGetter : public KohonenPatternGetter,
                            protected PatternGetterDataMembers
{
private:
	RandomPatternGetter();

public:
	RandomPatternGetter(const QImage& im, MapColor* mc);
	~RandomPatternGetter() override;
	const OrganizableElement* getPattern() override;
};

class SequentialPatternGetter : public BatchPatternGetter,
                                protected PatternGetterDataMembers
{
protected:
	QImage classifiedImage;
	ProgressObserver* progressObserver;
	int x, y, nChanges;

private:
	SequentialPatternGetter();

public:
	SequentialPatternGetter(const QImage& im, MapColor* mc,
	                        ProgressObserver* progressObserver = nullptr);
	~SequentialPatternGetter() override;
	const OrganizableElement* getPattern() override;
	int getLastElementClass() const override;
	void setLastElementClass(int classNumber) override;
	void reset() override;
	int numberOfChanges() override;
	virtual QImage* getClassifiedImage();
};
} // cove

#endif
