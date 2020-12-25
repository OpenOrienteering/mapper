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

#include "key_value_container.h"

#include <algorithm>
#include <stdexcept>


namespace OpenOrienteering {

bool operator==(const KeyValue& lhs, const KeyValue& rhs)
{
	return lhs.key == rhs.key && lhs.value == rhs.value;
}


const KeyValueContainer::mapped_type& KeyValueContainer::at(const key_type& key) const
{
	auto hint = find(key);
	if (hint == end())
		throw std::out_of_range("Invalid key");
	return hint->value;
}

KeyValueContainer::mapped_type& KeyValueContainer::at(const key_type& key)
{
	return const_cast<mapped_type&>(static_cast<const KeyValueContainer*>(this)->at(key));
}

KeyValueContainer::mapped_type& KeyValueContainer::operator[](const key_type& key)
{
	auto hint = find(key);
	if (hint == end())
		hint = std::vector<KeyValue>::insert(hint, {key, {}});
	return hint->value;
}


KeyValueContainer::const_iterator KeyValueContainer::find(const key_type& key) const
{
	return std::find_if(begin(), end(), [key](auto const& tag) { return tag.key == key; });
}

KeyValueContainer::iterator KeyValueContainer::find(const key_type& key)
{
	return std::find_if(begin(), end(), [key](auto const& tag) { return tag.key == key; });
}

bool KeyValueContainer::contains(const key_type& key) const
{
	return find(key) != cend();
}


std::pair<KeyValueContainer::iterator, bool> KeyValueContainer::insert(const value_type& value)
{
	auto hint = find(value.key);
	auto const at_end = hint == end();
	if (at_end)
		hint = std::vector<KeyValue>::insert(hint, value);
	return {hint, at_end};
}

std::pair<KeyValueContainer::iterator, bool> KeyValueContainer::insert_or_assign(const key_type& key, const mapped_type& object)
{
	auto hint = find(key);
	auto const at_end = hint == end();
	if (at_end)
		hint = std::vector<KeyValue>::insert(hint, {key, object});
	else
		hint->value = object;
	return {hint, at_end};
}

KeyValueContainer::iterator KeyValueContainer::insert_or_assign(iterator hint, const key_type& key, const mapped_type& object)
{
	auto const at_end = hint == end();
	if (at_end || hint->key != key)
		hint = std::vector<KeyValue>::insert(hint, {key, object});
	else
		hint->value = object;
	return hint;
}


}  // namespace OpenOrienteering
