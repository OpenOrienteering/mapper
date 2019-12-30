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

#ifndef COVE_ALPHAGETTER_H
#define COVE_ALPHAGETTER_H

#include "KohonenMap.h"

namespace cove {
class ProgressObserver;

class ClassicAlphaGetter : public KohonenAlphaGetter
{
	double alpha, minAlpha, q;
	unsigned int E;
	ProgressObserver* progressObserver;

public:
	ClassicAlphaGetter();
	ClassicAlphaGetter(ProgressObserver* progressObserver);
	ClassicAlphaGetter(double alpha, double q, unsigned int e, double minAlpha);
	ClassicAlphaGetter(double alpha, double q, unsigned int e, double minAlpha,
	                   ProgressObserver* progressObserver);
	double getAlpha() override;
	unsigned int getE() override;

	virtual void setAlpha(double alpha);
	virtual void setMinAlpha(double minAlpha);
	virtual void setQ(double alpha);
	virtual void setE(unsigned int alpha);
};
} // cove

#endif
