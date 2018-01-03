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
	
}



/**
 * A template class which provides an operator() returning the first index
 * block for a particular OCD entity index.
 * 
 * The generic template is not meant to be used but will trigger an error.
 * Specializations must be provided for entity types to be supported.
 * 
 * @param F: the type defining the file format version type
 * @param T: the entity type (string, symbol, object)
 */
template< class F, class T >
class FirstIndexBlock
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

/**
 * A template class which provides an operator() returning the first string
 * index block.
 * 
 * @param F: the type defining the file format version type
 */
template< class F >
class FirstIndexBlock<F, Ocd::ParameterString>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

/**
 * A template class which provides an operator() returning the first symbol
 * index block.
 * 
 * @param F: the type defining the file format version type
 */
template< class F >
class FirstIndexBlock<F, typename F::BaseSymbol>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

/**
 * A template class which provides an operator() returning the first object
 * index block.
 * 
 * @param F: the type defining the file format version type
 */
template< class F >
class FirstIndexBlock<F, typename F::Object>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};



/**
 * A template class which provides an iterator for OCD entity indices.
 * 
 * @param F: the type defining the file format version type
 * @param T: the entity type (string, symbol, object)
 * @param E: the index entry type
 */
template< class F, class T, class E >
class OcdEntityIndexIterator
{
public:
	using FileFormat = F;
	using EntityType = T;
	using EntryType  = E;
	using IndexBlock = Ocd::IndexBlock<E>;
	
	OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block);
	
	const OcdEntityIndexIterator& operator++();
	
	const E& operator*() const;
	
	const E* operator->() const;
	
	bool operator==(const OcdEntityIndexIterator<F,T,E>& rhs) const;
	
	bool operator!=(const OcdEntityIndexIterator<F,T,E>& rhs) const;
	
private:
	const OcdFile<F>* file;
	const IndexBlock* block;
	std::size_t index;
};


/**
 * A template class which provides an iterator for OCD entity indices, 
 * specialized for the case where the index entry is just the quint32 file
 * position of the entity data.
 * 
 * @param F: the type defining the file format version type
 * @param T: the entity type (string, symbol, object)
 */
template< class F, class T >
class OcdEntityIndexIterator<F, T, quint32 >
{
public:
	using FileFormat = F;
	using EntityType = T;
	using EntryType  = quint32;
	using IndexBlock = Ocd::IndexBlock<quint32>;
	
	OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block);
	
	const OcdEntityIndexIterator& operator++();
	
	const T& operator*() const;
	
	const T* operator->() const;
	
	bool operator==(const OcdEntityIndexIterator<F, T, quint32>& rhs) const;
	
	bool operator!=(const OcdEntityIndexIterator<F, T, quint32>& rhs) const;
	
private:
	const OcdFile<F>* file;
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
	
	/** The index iterator type. */
	using const_iterator = OcdEntityIndexIterator<F, T, EntryType>;
	
	/**
	 * Constructs an entity index object.
	 * 
	 * You must call setData() before using the container interface.
	 */
	OcdEntityIndex();
	
	/**
	 * Destroys the object.
	 */
	~OcdEntityIndex();
	
	/**
	 * Sets the raw file data to which the object provides access.
	 * 
	 * The data is not copied and must not be deleted as long as the index
	 * and its iterators are in use.
	 */
	void setData(const OcdFile<F>* file);
	
	/**
	 * Returns a forward iterator to the beginning.
	 */
	const_iterator begin() const;
	
	/**
	 * Returns a forward iterator to the end.
	 */
	const_iterator end() const;
	
private:
	const OcdFile<F>* file;
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
	 * Constructs a new object for the file contents given by data.
	 * 
	 * We try to avoid copying the data by using the implicit sharing provided
	 * by QByteArray.
	 */
	OcdFile(const QByteArray& data);
	
	/**
	 * Destructs the object.
	 */
	~OcdFile() {}
	
	/**
	 * Returns the raw data.
	 */
	const QByteArray& byteArray() const;
	
	/**
	 * Returns a const reference to the byte specified by file_pos.
	 */
	const char& operator[](quint32 file_pos) const;
	
	/**
	 * Returns a pointer to the file header.
	 */
	const FileHeader* header() const;
	
	/**
	 * Returns a const reference to the parameter string index.
	 */
	const StringIndex& strings() const;
	
	/**
	 * Returns the raw data of the string.
	 */
	const QByteArray operator[](const typename StringIndex::EntryType& string) const;
	
	/**
	 * Returns a const reference to the symbol index.
	 */
	const SymbolIndex& symbols() const;
	
	/**
	 * Returns a const reference to the object index.
	 */
	const ObjectIndex& objects() const;
	
	/**
	 * Returns a const referenc to the object referenced by an object index iterator.
	 */
	const typename F::Object& operator[](const typename ObjectIndex::EntryType& object_entry) const;
	
private:
	QByteArray byte_array;
	StringIndex string_index;
	SymbolIndex symbol_index;
	ObjectIndex object_index;
};



// ### FirstIndexBlock implementation ###

template< class F >
quint32 FirstIndexBlock<F,Ocd::ParameterString>::operator()(const OcdFile<F>* file) const
{
	return (file->header()->version < 8) ? 0 : file->header()->first_string_block;
}

template< class F >
quint32 FirstIndexBlock<F,typename F::BaseSymbol>::operator()(const OcdFile<F>* file) const
{
	return file->header()->first_symbol_block;
}

template< class F >
quint32 FirstIndexBlock<F,typename F::Object>::operator()(const OcdFile<F>* file) const
{
	return file->header()->first_object_block;
}


// ### OcdEntityIndexIterator implementation ###

template< class F, class T, class E >
OcdEntityIndexIterator<F,T,E>::OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block)
 : file(nullptr)
 , block(nullptr)
 , index(0)
{
	if (file && first_block)
	{
		this->file = file;
		block = first_block;
		if (file && block->entries[index].pos == 0)
			operator++();
	}
}

template< class F, class T, class E >
const OcdEntityIndexIterator<F,T,E>& OcdEntityIndexIterator<F,T,E>::operator++()
{
	if (file)
	{
		do
		{
			++index;
			if (index == 256)
			{
				index = 0;
				quint32 next_block = block->next_block;
				if (next_block == 0)
				{
					block = nullptr;
					file = nullptr;
				}
				else if (Q_UNLIKELY(next_block >= (unsigned int)file->byteArray().size()))
				{
					qWarning("OcdEntityIndexIterator: Next index block is out of bounds");
					block = nullptr;
					file = nullptr;
				}
				else
				{
					block = reinterpret_cast<const IndexBlock*>(&(*file)[next_block]);
				}
			}
		}
		while (block && !block->entries[index].pos);
	}
	return *this;
}

template< class F, class T, class E >
const E& OcdEntityIndexIterator<F,T,E>::operator*() const
{
	return block->entries[index];
}

template< class F, class T, class E >
const E* OcdEntityIndexIterator<F,T,E>::operator->() const
{
	return &(block->entries[index]);
}

template< class F, class T, class E >
bool OcdEntityIndexIterator<F,T,E>::operator==(const OcdEntityIndexIterator<F,T,E>& rhs) const
{
	return (file == rhs.file && block == rhs.block && index == rhs.index);
}

template< class F, class T, class E >
bool OcdEntityIndexIterator<F,T,E>::operator!=(const OcdEntityIndexIterator<F,T,E>& rhs) const
{
	return (file != rhs.file || block != rhs.block || index != rhs.index);
}



// ### OcdEntityIndexIterator specialization ###

template< class F, class T >
OcdEntityIndexIterator<F,T,quint32>::OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block)
 : file(nullptr),
   block(nullptr),
   index(0)
{
	if (file && first_block)
	{
		this->file = file;
		block = first_block;
		if (file && block->entries[index] == 0)
			operator++();
	}
}

template< class F, class T >
const OcdEntityIndexIterator<F,T,quint32>& OcdEntityIndexIterator<F,T,quint32>::operator++()
{
	if (file)
	{
		do
		{
			++index;
			if (index == 256)
			{
				index = 0;
				quint32 next_block = block->next_block;
				if (next_block)
				{
					block = reinterpret_cast<const IndexBlock*>(&(*file)[next_block]);
				}
				else
				{
					block = nullptr;
					file = nullptr;
				}
			}
		}
		while ( file && !block->entries[index] );
	}
	return *this;
}

template< class F, class T >
const T& OcdEntityIndexIterator<F,T,quint32>::operator*() const
{
	return reinterpret_cast<const T&>((*file)[block->entries[index]]);
}

template< class F, class T >
const T* OcdEntityIndexIterator<F,T,quint32>::operator->() const
{
	return reinterpret_cast<const T*>(&(*file)[block->entries[index]]);
}

template< class F, class T >
bool OcdEntityIndexIterator<F,T,quint32>::operator==(const OcdEntityIndexIterator<F,T,quint32>& rhs) const
{
	return (file == rhs.file && block == rhs.block && index == rhs.index);
}

template< class F, class T >
bool OcdEntityIndexIterator<F,T,quint32>::operator!=(const OcdEntityIndexIterator<F,T,quint32>& rhs) const
{
	return (file != rhs.file || block != rhs.block || index != rhs.index);
}



// ### OcdEntityIndex implementation ###

template< class F, class T >
OcdEntityIndex<F,T>::OcdEntityIndex()
 : file(nullptr)
{
}

template< class F, class T >
OcdEntityIndex<F,T>::~OcdEntityIndex()
{
}

template< class F, class T >
void OcdEntityIndex<F,T>::setData(const OcdFile< F >* file)
{
	this->file = file;
}

template< class F, class T >
typename OcdEntityIndex<F,T>::const_iterator OcdEntityIndex<F,T>::begin() const
{
	Q_ASSERT(file);
	
	quint32 file_pos = FirstIndexBlock<F,T>()(file);
	const_iterator it(file, reinterpret_cast<const typename const_iterator::IndexBlock*>(file_pos ? &(*file)[file_pos] : nullptr));
	return it;
}

template< class F, class T >
typename OcdEntityIndex<F,T>::const_iterator OcdEntityIndex<F,T>::end() const
{
	static const_iterator it(nullptr, nullptr);
	return it;
}



// ### OcdFile implementation ###

template< class F >
OcdFile<F>::OcdFile(const QByteArray& data)
{
	byte_array = data;
	Q_ASSERT(byte_array.constData() == data.constData()); // No deep copy
	string_index.setData(this);
	symbol_index.setData(this);
	object_index.setData(this);
}

template< class F >
const QByteArray& OcdFile<F>::byteArray() const
{
	return byte_array;
}

template< class F >
const char& OcdFile<F>::operator[](quint32 file_pos) const
{
	return *(byte_array.constData() + file_pos);
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
const QByteArray OcdFile<F>::operator[](const typename OcdFile<F>::StringIndex::EntryType& string) const
{
	return QByteArray::fromRawData(&(*this)[string.pos], string.size);
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

template< class F >
const typename F::Object& OcdFile<F>::operator[](const typename OcdFile<F>::ObjectIndex::EntryType& object_entry) const
{
	return reinterpret_cast<const typename F::Object&>((*this)[object_entry.pos]);
}

#endif // OPENORIENTEERING_OCD_TYPES_H
