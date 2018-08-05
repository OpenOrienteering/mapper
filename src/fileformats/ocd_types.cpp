/*
 *    Copyright 2013, 2016-2018 Kai Pastor
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

#include <limits>

#include "ocd_types.h"

#include "ocd_types_v8.h"
#include "ocd_types_v9.h"
#include "ocd_types_v10.h"
#include "ocd_types_v11.h"
#include "ocd_types_v12.h"
#include "ocd_types_v2018.h"


namespace Ocd
{
	// Verify at compile time that a double is 8 bytes big.
	
	Q_STATIC_ASSERT(sizeof(double) == 8);
	
	// Verify at compile time that data structures are packed, not aligned.
	
	Q_STATIC_ASSERT(sizeof(FileHeaderGeneric) == 8);
	
	Q_STATIC_ASSERT(sizeof(FormatV8::FileHeader) - sizeof(SymbolHeaderV8) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV9::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV10::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV11::FileHeader) == 48);
	
	Q_STATIC_ASSERT(sizeof(FormatV12::FileHeader) == 60);
	
	
	
	const void* getBlockCheckedRaw(const QByteArray& byte_array, quint32 pos, quint32 block_size)
	{
		if (pos == 0)
		{
			return nullptr;
		}
		else if (Q_UNLIKELY(quint64(pos) + block_size > std::numeric_limits<quint32>::max()
		                    || pos + block_size - 1 >= quint32(byte_array.size())))
		{
			qWarning("Ocd::getBlockChecked: Requested data block is out of bounds");
			return nullptr;
		}
		return byte_array.data() + pos;
	}
	
	
	QByteArray& addPadding(QByteArray& byte_array)
	{
		return byte_array.append("\0\0\0\0\0\0\0", (0x7ffffff8 - byte_array.size()) % 8);
	}
	
	
} // namespace Ocd



namespace {

	template< class File >
	void addSetupHeader(File& /*file*/) {}
	
	
	void addSetupHeader(OcdFile<Ocd::FormatV8>& file)
	{
		auto setup = Ocd::SetupV8{};
		auto& byte_array = Ocd::addPadding(file.byteArray());
		file.header()->setup_pos = quint32(byte_array.size());
		file.header()->setup_size = quint32(sizeof setup);
		Ocd::addPadding(byte_array).append(reinterpret_cast<const char*>(&setup), sizeof setup);
	}
	
} // namespace



// ### OcdEntityIndex implementation ###

template< class F, class T >
typename OcdEntityIndex<F,T>::EntryType& OcdEntityIndex<F,T>::insert(const QByteArray& entity_data, const EntryType& entry)
{
	auto& byte_array = Ocd::addPadding(file.byteArray());
	IndexBlock* block;
	auto next_block_pos = firstBlock<typename T::IndexEntryType>();
	auto block_pos = decltype(next_block_pos)(0);
	do
	{
		block_pos = next_block_pos;
		block = Ocd::getBlockChecked<IndexBlock>(byte_array, block_pos);
		if (Q_UNLIKELY(!block))
		{
			///  \todo Throw exception
			qFatal("OcdEntityIndexIterator: Next index block is out of bounds");
		}
		next_block_pos = block->next_block;
	}
	while (next_block_pos != 0);
	
	quint16 index = 0;
	while (index < 256 && block->entries[index].pos)
		++index;
	
	if (Q_UNLIKELY(index == 256))
	{
		block_pos = decltype(block->next_block)(byte_array.size());
		block->next_block = block_pos;
		auto new_block = IndexBlock {};
		byte_array.append(reinterpret_cast<const char*>(&new_block), sizeof(IndexBlock));
		block = reinterpret_cast<IndexBlock*>(byte_array.data() + block_pos);
		index = 0;
	}
	
	auto entity_pos = decltype(block->entries[index].pos)(byte_array.size());
	byte_array.append(entity_data); // May reallocate! Re-calculate block pointer:
	block = Ocd::getBlockChecked<IndexBlock>(byte_array, block_pos);
	Q_ASSERT(block);
	block->entries[index] = entry;
	block->entries[index].pos = entity_pos;
	return block->entries[index];
}



// ### OcdFile implementation ###

template<class F>
OcdFile<F>::OcdFile()
: string_index(*this)
, symbol_index(*this)
, object_index(*this)
{
	byte_array.reserve(1000000);
	
	{
		FileHeader header = {};
		header.version = F::version;
		if (F::version == 11)
			header.subversion = 3;
		byte_array.append(reinterpret_cast<const char*>(&header), sizeof header);
	}
	
	Q_ASSERT(header());
	
	addSetupHeader(*this);
	
	{
		header()->first_string_block = static_cast<decltype(header()->first_string_block)>(byte_array.size());
		using StringIndexBlock = Ocd::IndexBlock<Ocd::ParameterStringIndexEntry>;
		auto first_string_block = StringIndexBlock{};
		Ocd::addPadding(byte_array).append(reinterpret_cast<const char*>(&first_string_block), sizeof(StringIndexBlock));
		// Free stack from first_string_block
	}
	
	{
		header()->first_symbol_block = static_cast<decltype(header()->first_symbol_block)>(byte_array.size());
		using SymbolIndexBlock = Ocd::IndexBlock<typename F::BaseSymbol::IndexEntryType>;
		auto first_symbol_block = SymbolIndexBlock{};
		Ocd::addPadding(byte_array).append(reinterpret_cast<const char*>(&first_symbol_block), sizeof(SymbolIndexBlock));
		// Free stack from first_symbol_block
	}
	
	{
		header()->first_object_block = static_cast<decltype(header()->first_object_block)>(byte_array.size());
		using ObjectIndexBlock = Ocd::IndexBlock<typename F::Object::IndexEntryType>;
		auto first_object_block = ObjectIndexBlock{};
		Ocd::addPadding(byte_array).append(reinterpret_cast<const char*>(&first_object_block), sizeof(ObjectIndexBlock));
		// Free stack from first_object_block
	}
}



// Explicit instantiations

OCD_EXPLICIT_INSTANTIATION(template, Ocd::FormatV8)
OCD_EXPLICIT_INSTANTIATION(template, Ocd::FormatV9)
Q_STATIC_ASSERT((std::is_same<Ocd::FormatV10, Ocd::FormatV9>::value));
OCD_EXPLICIT_INSTANTIATION(template, Ocd::FormatV11)
OCD_EXPLICIT_INSTANTIATION(template, Ocd::FormatV12)
Q_STATIC_ASSERT((std::is_same<Ocd::FormatV2018, Ocd::FormatV12>::value));
