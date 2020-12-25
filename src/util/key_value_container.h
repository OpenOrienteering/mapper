/*
 *    Copyright 2020 Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef OPENORIENTEERING_KEY_VALUE_CONTAINER_H
#define OPENORIENTEERING_KEY_VALUE_CONTAINER_H

#include <utility>
#include <vector>

#include <QString>

namespace OpenOrienteering {

struct KeyValue
{
	QString key;
	QString value;
};

bool operator==(const KeyValue& lhs, const KeyValue& rhs);

inline
bool operator!=(const KeyValue& lhs, const KeyValue& rhs) { return !(lhs == rhs); }


/**
 * A container for QString pairs with an interface similar to std::map,
 * but retaining the order of insertion.
 * 
 * This container does not uses hashes but is based on std::vector and linear
 * searching. Thus it has runtime properties similar to std::vector with regard
 * to complexity of operations and to invalidation of iterators. For a small
 * number of elements, there should be no disadvantage.
 * 
 * Unique keys are maintained only when modifying the container using the
 * functions implemented in this class, on top of std::vector.
 * 
 * \see https://en.cppreference.com/w/cpp/container/map,
 *      https://en.cppreference.com/w/cpp/container/vector
 */
class KeyValueContainer : public std::vector<KeyValue>
{
public:
	// Type aliases as in std::map
	using key_type = decltype(KeyValue::key);
	using value_type = KeyValue;
	using mapped_type = decltype(KeyValue::value);
	
	// Default constructors/destructor are okay.
	
	// Element access
	const mapped_type& at(const key_type& key) const;
	mapped_type& at(const key_type& key);
	mapped_type& operator[](const key_type& key);
	
	// Lookup
	const_iterator find(const key_type& key) const;
	iterator find(const key_type& key);
	bool contains(const key_type& key) const;
	
	// Modifiers
	/** Inserts a new item if it doesn't already exist. */
	std::pair<iterator, bool> insert(const value_type& value);
	/** Inserts a new item if it doesn't already exist, or updates the existing one. */
	std::pair<iterator, bool> insert_or_assign(const key_type& key, const mapped_type& object);
	/** Inserts a new item if it doesn't already exist, or updates the existing one. */
	iterator insert_or_assign(iterator hint, const key_type& key, const mapped_type& object);
	
private:
	using std::vector<KeyValue>::operator[];
};

}  // namespace OpenOrienteering


#endif
