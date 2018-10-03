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

#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

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
	
	
	Q_STATIC_ASSERT(std::extent<decltype(IconV8::bits)>::value == IconV8::length());
	
	Q_STATIC_ASSERT(std::extent<decltype(IconV9::bits)>::value == IconV9::length());
	
	
	// uncompressed IconV8: Compare 11 bytes of each scanline, 12th byte unused.
	bool operator==(const IconV8& lhs, const IconV8& rhs)
	{
		auto equal = [](auto first1, auto last1, auto first2)
		{
			while (first1 != last1)
			{
				if (!std::equal(first1, first1+11, first2))
					return false;
				first1 += 12;
				first2 += 12;
			}
			return true;
		};
		using std::begin;
		using std::end;
		Q_ASSERT(end(lhs.bits) == begin(lhs.bits)+12*22);
		return equal(begin(lhs.bits), end(lhs.bits), begin(rhs.bits));
	}
	
	bool operator==(const IconV9& lhs, const IconV9& rhs)
	{
		using std::begin;
		using std::end;
		return std::equal(begin(lhs.bits), end(lhs.bits), begin(rhs.bits));
	}
	
	
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
	
	
	
	namespace
	{
	
	/**
	 * This struct refers to a sequence of bytes in existing data.
	 */
	struct ByteSequence {
		const quint8* start;
		unsigned int count;
	};
	
	
	/**
	 * This class allows to read from and write to streams of 9-bit code words.
	 * 
	 * The stream "starts" with the lowest bit, i.e. the lowest bit of the
	 * first byte of the stream is the lowest bit of the first code word.
	 * Consequently, the highest bit of the first 9-bit code word is found in
	 * the lowest bit of the second byte of the stream, and the lowest bit of
	 * the second code word is found in the second lowest bit of the second byte
	 * in the stream.
	 * 
	 * For writing, the Byte template parameter must not be const.
	 * 
	 * Synopsis:
	 *     using CodeStream = CodeStreamImplementation<quint8>;
	 *     auto stream = CodeStream{icon.bits};
	 *     stream.seek(16);
	 *     stream.put(0);
	 */
	template<class Byte>
	class CodeStreamImplementation
	{
		Q_STATIC_ASSERT(sizeof(Byte) == 1);
		Byte* data;
		unsigned byte_pos = 0;
		unsigned bit_pos = 0;
		unsigned end_pos;
		
	public:
		/**
		 * Special values in a compressed stream of icon data.
		 * 
		 * 0x100u could be "Clear code table". However, this application (9 bit
		 * code word size, 256 byte input size) shouldn't need it.
		 */
		enum SpecialValues
		{
			LastStaticCode   = 0x0ffu,
			Reserved_0x100   = 0x100u,
			EndOfData        = 0x101u,
			FirstDynamicCode = 0x102u,
		};
		
		template<class T, unsigned N = std::extent<T>::value>
		CodeStreamImplementation(T& data) noexcept : data{data}, end_pos(N) { Q_STATIC_ASSERT(N != 0); }
		
		bool atEnd() const noexcept { return byte_pos + 1 >= end_pos; } // One code word takes two bytes.
		
		unsigned pos() const noexcept { return byte_pos; }
		
		void seek(unsigned byte_pos) noexcept { this->byte_pos = byte_pos; bit_pos = 0; }
		
		quint16 get();
		
		void put(quint16 code);
		
	};
	
	
	/**
	 * Gets a 9-bit code word from the stream by filling its bits from the
	 * lowest to the highest one.
	 */
	template<class Byte>
	quint16 CodeStreamImplementation<Byte>::get()
	{
		// We are going to access two bytes of input.
		if (Q_UNLIKELY(atEnd()))
			throw std::length_error("Premature end of input data");
		
		quint16 code = data[byte_pos] >> bit_pos;
		code += (data[++byte_pos] << (8 - bit_pos)) & 0x1ff;
		++bit_pos;
		if (bit_pos == 8)
		{
			++byte_pos;
			bit_pos = 0;
		}
		return code;
	}
	
	/**
	 * Appends a code word to the stream, lowest bits first.
	 */
	template<class Byte>
	void CodeStreamImplementation<Byte>::put(quint16 code)
	{
		// We are going to access two bytes of output.
		if (Q_UNLIKELY(atEnd()))
			throw std::length_error("Too much output data");
		
		data[byte_pos] += (code << bit_pos) & 0xff;
		data[++byte_pos] = (code >> (8 - bit_pos)) & 0xff;
		++bit_pos;
		if (bit_pos == 8)
		{
			++byte_pos;
			bit_pos = 0;
		}
	}
	
	
	}  // namespace
	
	
	// Cf. http://giflib.sourceforge.net/whatsinagif/lzw_image_data.html
	// In IconV8::bits, compressed data starts at offset 16.
	// Our implementation leverages the facts that:
	// - the complete input is in local array `bits`, so
	//   the code word dictionary entries can refer to this data.
	// - the complete output is in 256 bytes of a local array `icon_v8.bits`,
	//   limiting the maximum number of dictionary entries.
	// The compressed data may exceed 256 bytes.
	IconV8 IconV9::compress() const
	{
		IconV8 icon_v8 = {};
		
		std::vector<ByteSequence> dict;
		dict.reserve((icon_v8.length() - 16) * 8 / 9);
		
		using CodeStream = CodeStreamImplementation<quint8>;
		auto stream = CodeStream(icon_v8.bits);
		stream.seek(16);
		
		// Searches for the sequence in the dictionary, and returns the matching
		// index, or -1 if not found.
		auto find_code = [&dict](const ByteSequence& sequence)->int {
			if (sequence.count == 1)
			{
				return *sequence.start;
			}
			for (quint8 i = 0; i < dict.size(); ++i)
			{
				const auto& entry = dict[i];
				if (entry.count == sequence.count
				    && 0 == memcmp(entry.start, sequence.start, sequence.count))
					return CodeStream::FirstDynamicCode + i;
			}
			return -1;
		};
		
		// Put the first input byte into the buffer, and loop over the rest.
		auto buffer = ByteSequence{ bits, 1 };
		for (auto i = 1u; i < length(); ++i)
		{
			// Determine code for current buffer + input
			auto new_buffer = ByteSequence{ bits + i - buffer.count, buffer.count + 1 };
			auto code = find_code(new_buffer);
			if (Q_UNLIKELY(code >= 0x200))
			{
				// Code word length exeeds 9 bits.
				throw std::length_error("Excess data during icon compression");
			}
			else if (code >= 0)
			{
				// Buffer + input was found in the code word table.
				buffer = new_buffer;
			}
			else
			{
				// Buffer + input was not found in code word table.
				dict.push_back(new_buffer);
				stream.put(quint16(find_code(buffer)));
				buffer = { bits + i, 1 };
			}
		}
		
		stream.put(quint16(find_code(buffer)));
		
		if (!stream.atEnd())
			stream.put(CodeStream::EndOfData);
		
		return icon_v8;
	}
	
	
	// Cf. http://giflib.sourceforge.net/whatsinagif/lzw_image_data.html
	// In IconV8::bits, compressed data starts at offset 16.
	// Our implementation leverages the facts that:
	// - the complete input is in 256 bytes of a local array `bits`,
	//   limiting the maximum number of dictionary entries.
	// - the complete output is in local array `icon_v9.bits`, so
	//   the code word dictionary entries can refer to this data.
	IconV9 IconV8::uncompress() const
	{
		IconV9 icon_v9 = {};
		
		std::vector<ByteSequence> dict;
		dict.reserve((length() - 16) * 8 / 9);
		
		using CodeStream = CodeStreamImplementation<const quint8>;
		auto stream = CodeStream(bits);
		stream.seek(16);
		
		// Initialization: handle first code word.
		auto pos = 0u;  // index in icon_v9.bits
		icon_v9.bits[pos++] = quint8(stream.get());
		auto buffer = ByteSequence{ icon_v9.bits, 1 };  // expansion of previous code word
		
		while (!stream.atEnd())
		{
			auto code = stream.get();
			
			// Handle code word
			if (code <= CodeStream::LastStaticCode)
			{
				// Code word gives a single output byte, code table not needed.
				if (Q_UNLIKELY(pos + 1 > icon_v9.length()))
					throw std::length_error("Too much input data during icon uncompression");
				icon_v9.bits[pos] = quint8(code);
				++buffer.count;
				dict.push_back(buffer);
				buffer = { icon_v9.bits + pos, 1 };
				++pos;
			}
			else if (code >= CodeStream::FirstDynamicCode)
			{
				// Code word needs code table mapping to multiple bytes.
				auto dict_index = code - unsigned(CodeStream::FirstDynamicCode);
				if (dict_index < dict.size())
				{
					// Known code word
					auto entry = dict[dict_index];
					if (Q_UNLIKELY(pos + entry.count > icon_v9.length()))
						throw std::length_error("Too much input data during icon uncompression");
					std::copy(entry.start, entry.start + entry.count, icon_v9.bits + pos);
					++buffer.count;
					dict.push_back(buffer);
					pos += entry.count;
					buffer = { icon_v9.bits + pos - entry.count, entry.count };
				}
				else if (dict_index == dict.size())
				{
					// New code word
					if (Q_UNLIKELY(pos + buffer.count + 1 > icon_v9.length()))
						throw std::length_error("Too much input data during icon uncompression");
					std::copy(buffer.start, buffer.start + buffer.count, icon_v9.bits + pos);
					icon_v9.bits[pos + buffer.count] = *buffer.start;
					++buffer.count;
					dict.push_back(buffer);
					pos += buffer.count;
					buffer = { icon_v9.bits + pos-buffer.count, buffer.count };
				}
				else
				{
					throw std::domain_error("Invalid data during icon uncompression");
				}
			}
			else if (code == CodeStream::EndOfData)
			{
				break;  // End of data
			}
			else
			{
				throw std::domain_error("Invalid data during icon uncompression");
			}
		}
		
		if (Q_UNLIKELY(pos != icon_v9.length()))
		{
			qDebug("pos:%d != last:%u", pos, icon_v9.length());
			throw std::domain_error("Premature end of input data during icon uncompression");
		}
		
		return icon_v9;
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
