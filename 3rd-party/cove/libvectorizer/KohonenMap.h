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

#ifndef COVE_KOHONENMAP_H
#define COVE_KOHONENMAP_H

#include <memory>
#include <vector>

namespace cove {
class OrganizableElement
{
public:
	virtual ~OrganizableElement();
	virtual OrganizableElement* clone() const = 0;
	virtual double distance(const OrganizableElement& y) const = 0;
	virtual double squares(const OrganizableElement& y) const = 0;
	virtual void add(const OrganizableElement& y) = 0;
	virtual void subtract(const OrganizableElement& y) = 0;
	virtual void multiply(double y) = 0;
};

class KohonenAlphaGetter
{
public:
	virtual ~KohonenAlphaGetter();
	virtual double getAlpha() = 0;
	virtual unsigned int getE() = 0;
};

class KohonenPatternGetter
{
public:
	virtual ~KohonenPatternGetter();
	virtual const OrganizableElement* getPattern() = 0;
};

class BatchPatternGetter : public KohonenPatternGetter
{
public:
	~BatchPatternGetter() override;
	virtual int getLastElementClass() const = 0;
	virtual void setLastElementClass(int classNumber) = 0;
	virtual void reset() = 0;
	virtual int numberOfChanges() = 0;
};

class KohonenMap
{
	std::vector<std::unique_ptr<OrganizableElement>> classes;

public:
	void setClasses(const std::vector<OrganizableElement*>& newClasses);
	std::vector<std::unique_ptr<OrganizableElement>> getClasses();
	int findClosest(const OrganizableElement& v, double& bestDistance) const;
	void learn(const OrganizableElement& v, double alfa);
	void performLearning(KohonenAlphaGetter& alphaGetter,
	                     KohonenPatternGetter& patternGetter);
	double performBatchLearning(BatchPatternGetter& patternGetter);
};
} // cove

#endif
