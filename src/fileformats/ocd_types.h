/*
 *    Copyright 2013, 2015-2018 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_TYPES_H
#define OPENORIENTEERING_OCD_TYPES_H

#include <cstddef>
#include <iterator>
#include <type_traits>

#include <QtGlobal>
#include <QByteArray>
#include <QChar>

template<class F> class OcdFile;


// Helper macro
#define RESERVED_MEMBER_CONCAT2(A,B) A ## B
// Helper macro
#define RESERVED_MEMBER_CONCAT(A,B) RESERVED_MEMBER_CONCAT2(A,B)

/**
 * A macro which generates a unique member name of the format
 * reserved_member_LINE_NUMBER.
 */
#define RESERVED_MEMBER RESERVED_MEMBER_CONCAT( reserved_member_ , __LINE__ )


namespace Ocd
{
	/** OCD filetypes */
	enum FileType
	{
		Map                  = 0x0000001,
		CourseSetting        = 0x0000002
	};
	
// This pragma should be supported by msvc, gcc, clang [-fms-compatibility]
#pragma pack(push, 1)
	
	/**
	 * A trait for strings and file formats that use a custom 8 bit encoding.
	 */
	struct Custom8BitEncoding
	{
		// nothing
	};
	
	/**
	 * A trait for strings and file formats that use UTF-8 encoding.
	 */
	struct Utf8Encoding
	{
		// nothing
	};
	
	/** 
	 * A string of max. N characters with a pascal-style binary representation:
	 * the first byte indicates the length,
	 * the following N bytes contain the actual character data.
	 */
	template< unsigned char N >
	struct PascalString
	{
		unsigned char length;
		char data[N];
	};
	
	/** 
	 * A UTF-8-encoded string of max. N characters with a pascal-style binary representation:
	 * the first byte indicates the length,
	 * the following N bytes contain the actual character data.
	 */
	template< unsigned char N >
	struct Utf8PascalString
	{
		unsigned char length;
		char data[N];
	};
	
	/** 
	 * A UTF-16LE-encoded string of max. N characters, zero-terminated.
	 */
	template< std::size_t N >
	struct Utf16PascalString
	{
		QChar data[N];
	};
	
	/**
	 * The generic header at the beginning of all supported OCD file formats.
	 * 
	 * For implementation efficiency, this header is generalized in comparison
	 * to the upstream documentation:
	 * 
	 *  - Until V8, the format actually used a 16 bit file type field called
	 *    "section mark", but no file status field.
	 *  - Until V9, the format actually used a 16 bit subversion field, but no
	 *    subsubversion field.
	 */
	struct FileHeaderGeneric
	{
		quint16 vendor_mark;
		quint8  file_type;          /// aka "section mark" until V8
		quint8  file_status_V9;     /// \since V9
		quint16 version;
		quint8  subversion;
		quint8  subsubversion_V10;  /// \since V10
	};
	
	/**
	 * An IndexBlock collects 256 index entries, and the file position of the
	 * next index block if more index entries exist.
	 */
	template< class E >
	struct IndexBlock
	{
		using IndexEntryType = E;
		
		quint32 next_block;
		IndexEntryType entries[256];
	};
	
	/**
	 * An index entry for a parameter string.
	 */
	struct ParameterStringIndexEntry
	{
		quint32 pos;
		quint32 size;
		qint32  type;
		quint32 obj_index;
	};
	
	/**
	 * The parameter string trait.
	 * 
	 * OCD strings are raw data, so this is more a trait rather than an actual
	 * structure.
	 */
	struct ParameterString
	{
		using IndexEntryType = ParameterStringIndexEntry;
	};
	
	/**
	 * An index entry for a symbol.
	 */
	struct SymbolIndexEntry
	{
		quint32 pos;
	};
	
	/**
	 * The OCD file point data type.
	 * 
	 * The coordinates are not raw 32 bit signed integers but contain flags
	 * in the lowest 8 bits.
	 */
	struct OcdPoint32
	{
		qint32  x;
		qint32  y;
		
		// Flags in x coordinate
		enum XFlags
		{
			FlagCtl1 = 0x01,
			FlagCtl2 = 0x02,
			FlagLeft = 0x04
		};
		
		// Flags in Y coordinate
		enum YFlags
		{
			FlagCorner = 0x01,
			FlagHole   = 0x02,
			FlagRight  = 0x04,
			FlagDash   = 0x08
		};
	};
	
#pragma pack(pop)
	
	/**
	 * Symbol type values.
	 */
	enum SymbolType
	{
		SymbolTypePoint        = 1,
		SymbolTypeLine         = 2,
		SymbolTypeArea         = 3,
		SymbolTypeText         = 4,
		SymbolTypeRectangle_V8 = 5, /// Until V8
		SymbolTypeLineText  = 6, /// \since V9
		SymbolTypeRectangle_V9 = 7  /// \since V9
	};
	
	/**
	 * Status flags for symbols.
	 */
	enum SymbolStatus
	{
		SymbolNormal    = 0,
		SymbolProtected = 1,
		SymbolHidden    = 2
	};
	
	/**
	 * Status values for objects.
	 */
	enum ObjectStatus
	{
		ObjectDeleted = 0,
		ObjectNormal  = 1,
		ObjectHidden  = 2,
		ObjectDeletedForUndo = 3
	};
	
	/**
	 * Text alignment flags.
	 */
	enum TextAlignment
	{
		HAlignMask      = 0x03,
		HAlignLeft      = 0x00,
		HAlignCenter    = 0x01,
		HAlignRight     = 0x02,
		HAlignJustified = 0x03, /// All-line for line text symbols
		VAlignMask      = 0x0c, /// \since V10
		VAlignBottom    = 0x00, /// \since V10
		VAlignMiddle    = 0x04, /// \since V10
		VAlignTop       = 0x08  /// \since V10
	};
	
	/**
	 * Area hatch mode values.
	 */
	enum HatchMode
	{
		HatchNone   = 0,
		HatchSingle = 1,
		HatchCross  = 2
	};
	
	/**
	 * Area pattern structure values.
	 */
	enum StructureMode
	{
		StructureNone = 0,
		StructureAlignedRows = 1,
		StructureShiftedRows = 2
	};
	
	/**
	 *
	 */
	enum FramingMode
	{
		FramingNone      = 0,
		FramingShadow    = 1, /// Somehow different in older versions
		FramingLine      = 2, /// \since V7
		FramingRectangle = 3  /// \since V8; Not for line text symbols
	};
	
	
	/**
	 * Returns a pointer to the (index) block at pos inside byte_array.
	 * 
	 * This function returns nullptr if pos is zero, or if pos points
	 * to a block (at least partially) outside the byte_array.
	 */
	const void* getBlockChecked(const QByteArray& byte_array, quint32 pos, quint32 block_size);
	
}



/**
 * A template class which provides a forward iterator for OCD entity indices.
 * 
 * Dereferencing the iterator returns the index entry. The entity must be
 * fetched via the OcdFile<F>::operator[].
 * 
 * As with regular STL iterator, dereferencing and incrementing must only be
 * called when the iterator is dereferencable or incrementable state,
 * respectively.
 * 
 * @param V the index value type
 */
template< class V >
class OcdEntityIndexIterator : public std::iterator<std::input_iterator_tag, V, std::ptrdiff_t, void, V>
{
public:
	using value_type = V;
	using EntryType  = typename V::EntryType;
	using IndexBlock = const Ocd::IndexBlock<EntryType>;
	
	OcdEntityIndexIterator() noexcept = default;
	OcdEntityIndexIterator(const OcdEntityIndexIterator&) noexcept = default;
	OcdEntityIndexIterator(OcdEntityIndexIterator&&) noexcept = default;
	
	OcdEntityIndexIterator(const QByteArray& byte_array, const IndexBlock* first_block);
	
	OcdEntityIndexIterator& operator=(const OcdEntityIndexIterator&) noexcept = default;
	OcdEntityIndexIterator& operator=(OcdEntityIndexIterator&&) noexcept = default;
	
	OcdEntityIndexIterator& operator++();
	
	OcdEntityIndexIterator operator++(int);
	
	value_type operator*() const;
	
	bool operator==(const OcdEntityIndexIterator<V>& rhs) const;
	
	bool operator!=(const OcdEntityIndexIterator<V>& rhs) const;
	
private:
	const QByteArray* byte_array;
	const IndexBlock* block;
	std::size_t index;
};



/**
 * A template class for dealing with OCD entity indices.
 * 
 * Instances of this data do not actually copy any data, but rather provide
 * an STL container like interface for raw data. The interfaces allows for
 * forward iterating over the index for the given entity type.
 *
 * @param F: the type defining the file format version type
 * @param T: the entity type (string, symbol, object)
 */
template< class F, class T >
class OcdEntityIndex
{
public:
	/** The actual file format version type, reexported. */
	using FileFormat = F;
	
	/** The actual entity type. */
	using EntityType = T;
	
	/** The index entry type for the entity type. */
	using EntryType = typename T::IndexEntryType;
	
	struct value_type
	{
		using EntityType = T;
		using EntryType  = typename T::IndexEntryType;
		const EntryType*  entry;
		const EntityType* entity;
		
		template<class X = typename T::IndexEntryType, typename std::enable_if<std::is_same<X, Ocd::ParameterStringIndexEntry>::value, int>::type = 0>
		operator QByteArray() const;
	};
	
	/** The index iterator type. */
	using const_iterator = OcdEntityIndexIterator<value_type>;
	
	/**
	 * Constructs an entity index object.
	 */
	OcdEntityIndex(OcdFile<F>& file) noexcept;
	
	OcdEntityIndex(const OcdEntityIndex&) noexcept = default;
	
	/**
	 * Destroys the object.
	 */
	~OcdEntityIndex() = default;
	
	OcdEntityIndex& operator=(const OcdEntityIndex&) noexcept = default;
	
	/**
	 * Returns a forward iterator to the beginning of the index.
	 */
	const_iterator begin() const;
	
	/**
	 * Returns a forward iterator to the end.
	 */
	const_iterator end() const noexcept;
	
private:
	template< class X = EntryType, typename std::enable_if<std::is_same<X, typename Ocd::ParameterString::IndexEntryType>::value, int>::type = 0 >
	quint32 firstBlock() const;
	
	template< class X = EntryType, typename std::enable_if<std::is_same<X, typename F::BaseSymbol::IndexEntryType>::value, int>::type = 0 >
	quint32 firstBlock() const;
	
	template< class X = EntryType, typename std::enable_if<std::is_same<X, typename F::Object::IndexEntryType>::value, int>::type = 0 >
	quint32 firstBlock() const;
	
	OcdFile<F>& file;
};



/**
 * A template class for dealing with OCD files.
 * 
 * @param F: the type defining the actual file format version
 */
template< class F >
class OcdFile
{
public:
	/** The actual file format version type, reexported. */
	using Format = F;
	
	/** The actual file header type. */
	using FileHeader = typename F::FileHeader;
	
	/** The actual string index type. */
	using StringIndex = OcdEntityIndex< F, Ocd::ParameterString >;
	
	/** The actual symbol index type. */
	using SymbolIndex = OcdEntityIndex< F, typename F::BaseSymbol >;
	
	/** The actual object index type. */
	using ObjectIndex = OcdEntityIndex< F, typename F::Object >;
	
	/**
	 * Constructs a new object for the Ocd file contents given by data.
	 * 
	 * The data is not copied because of the implicit sharing provided
	 * by QByteArray. Const member functions in this class do not cause
	 * a deep copy.
	 */
	OcdFile(const QByteArray& data) noexcept;
	
	/**
	 * Destructs the object.
	 */
	~OcdFile() = default;
	
	/**
	 * Returns the raw data.
	 */
	const QByteArray& byteArray() const;
	
	/**
	 * Returns a pointer to the file header.
	 */
	const FileHeader* header() const;
	
	/**
	 * Returns a const reference to the parameter string index.
	 */
	const StringIndex& strings() const;
	
	/**
	 * Returns a const reference to the symbol index.
	 */
	const SymbolIndex& symbols() const;
	
	/**
	 * Returns a const reference to the object index.
	 */
	const ObjectIndex& objects() const;
	
private:
	QByteArray byte_array;
	StringIndex string_index;
	SymbolIndex symbol_index;
	ObjectIndex object_index;
};



// ### OcdEntityIndexIterator implementation ###

template< class V >
OcdEntityIndexIterator<V>::OcdEntityIndexIterator(const QByteArray& byte_array, IndexBlock* first_block)
: byte_array(&byte_array)
, block(first_block)
, index(0)
{
	if (block && block->entries[index].pos == 0)
		operator++();
}

template< class V >
OcdEntityIndexIterator<V>& OcdEntityIndexIterator<V>::operator++()
{
	do
	{
		++index;
		if (Q_UNLIKELY(index == 256))
		{
			index = 0;
			block = reinterpret_cast<IndexBlock*>(Ocd::getBlockChecked(*byte_array, block->next_block, sizeof(IndexBlock)));
			if (!block)
				break;
		}
	}
	while (!block->entries[index].pos);
	return *this;
}

template< class V >
OcdEntityIndexIterator<V> OcdEntityIndexIterator<V>::operator++(int)
{
	auto old_value(*this);
	operator++();
	return old_value;
}

template< class V >
typename OcdEntityIndexIterator<V>::value_type OcdEntityIndexIterator<V>::operator*() const
{
	return { &block->entries[index], reinterpret_cast<const typename value_type::EntityType*>(byte_array->data()+block->entries[index].pos) };
}

template< class V >
bool OcdEntityIndexIterator<V>::operator==(const OcdEntityIndexIterator<V>& rhs) const
{
	return (block == rhs.block && index == rhs.index);
}

template< class V >
bool OcdEntityIndexIterator<V>::operator!=(const OcdEntityIndexIterator<V>& rhs) const
{
	return !operator==(rhs);
}



// ### OcdEntityIndex implementation ###

template< class F, class T >
OcdEntityIndex<F,T>::OcdEntityIndex(OcdFile<F>& file) noexcept
: file(file)
{
	// nothing else
}


template< class F, class T >
template< class X, typename std::enable_if<std::is_same<X, Ocd::ParameterString::IndexEntryType>::value, int>::type >
quint32 OcdEntityIndex<F,T>::firstBlock() const
{
	return file.header()->version < 8 ? 0 : file.header()->first_string_block;
}


template< class F, class T >
template< class X, typename std::enable_if<std::is_same<X, typename F::BaseSymbol::IndexEntryType>::value, int>::type >
quint32 OcdEntityIndex<F,T>::firstBlock() const
{
	return file.header()->first_symbol_block;
}


template< class F, class T >
template< class X, typename std::enable_if<std::is_same<X, typename F::Object::IndexEntryType>::value, int>::type >
quint32 OcdEntityIndex<F,T>::firstBlock() const
{
	return file.header()->first_object_block;
}


template< class F, class T >
typename OcdEntityIndex<F,T>::const_iterator OcdEntityIndex<F,T>::begin() const
{
	auto pos = firstBlock<typename T::IndexEntryType>();
	const auto& byte_array = file.byteArray();
	auto first_block = Ocd::getBlockChecked(byte_array, pos, sizeof(typename const_iterator::IndexBlock));
	return { byte_array, reinterpret_cast<const typename const_iterator::IndexBlock*>(first_block) };
}


template< class F, class T >
typename OcdEntityIndex<F,T>::const_iterator OcdEntityIndex<F,T>::end() const noexcept
{
	return {};
}

template< class F, class T >
template<class X, typename std::enable_if<std::is_same<X, Ocd::ParameterStringIndexEntry>::value, int>::type>
OcdEntityIndex<F,T>::value_type::operator QByteArray() const
{
	return QByteArray::fromRawData(reinterpret_cast<const char*>(entity), int(entry->size));
}



// ### OcdFile implementation ###

template< class F >
OcdFile<F>::OcdFile(const QByteArray& data) noexcept
: byte_array(data)
, string_index(*this)
, symbol_index(*this)
, object_index(*this)
{
	// No deep copy during construction
	Q_ASSERT(byte_array.constData() == data.constData());
}

template< class F >
const QByteArray& OcdFile<F>::byteArray() const
{
	return byte_array;
}

template< class F >
const typename F::FileHeader* OcdFile<F>::header() const
{
	return (byte_array.size() < int(sizeof(FileHeader))) ? nullptr : reinterpret_cast<const FileHeader*>(byte_array.constData());
}

template< class F >
const typename OcdFile<F>::StringIndex& OcdFile<F>::strings() const
{
	return string_index;
}

template< class F >
const typename OcdFile<F>::SymbolIndex& OcdFile<F>::symbols() const
{
	return symbol_index;
}

template< class F >
const typename OcdFile<F>::ObjectIndex& OcdFile<F>::objects() const
{
	return object_index;
}

#endif // OPENORIENTEERING_OCD_TYPES_H
