/*
 *    Copyright 2013 Kai Pastor
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

#ifndef _OPENORIENTEERING_OCD_TYPES_
#define _OPENORIENTEERING_OCD_TYPES_

#include <QtGlobal>
#include <QtEndian>
#include <QByteArray>
#include <QChar>


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
	template< std::size_t N >
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
	template< std::size_t N >
	struct Utf8PascalString
	{
		unsigned char length;
		char data[N];
	};
	
	/**
	 * The generic header at the beginning of all supported OCD file formats.
	 */
	struct FileHeaderGeneric
	{
		quint16 vendor_mark;
		quint16 RESERVED_MEMBER;
		quint16 version;
		quint16 subversion;
	};
	
	/**
	 * An IndexBlock collects 256 index entries, and the file position of the
	 * next index block if more index entries exist.
	 */
	template< class E >
	struct IndexBlock
	{
		typedef E IndexEntryType;
		
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
		qint32 type;
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
		typedef ParameterStringIndexEntry IndexEntryType;
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
	 * A generic OCD file format trait.
	 *
	 * It is suitable for detecting the actual format.
	 */
	struct FormatGeneric
	{
		static inline int version() { return -1; };
		
		typedef FileHeaderGeneric FileHeader;
		
		struct BaseSymbol { typedef quint32 IndexEntryType; };
		
		struct Object { typedef quint32 IndexEntryType; };
	};
	
	/**
	 * A OCD file format trait for selecting the old version 8 importer.
	 */
	struct FormatLegacyImporter
	{
		static inline int version() { return 8; };
		
		typedef FileHeaderGeneric FileHeader;
	};
}



// Forward declaration, needed for class FirstIndexBlock.
template< class F >
class OcdFile;



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
	typedef F FileFormat;
	typedef T EntityType;
	typedef E EntryType;
	typedef Ocd::IndexBlock<E> IndexBlock;
	
	OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block);
	
	const OcdEntityIndexIterator& operator++();
	
	const E& operator*() const;
	
	const E* operator->() const;
	
	bool operator==(const OcdEntityIndexIterator<F,T,E>& rhs) const;
	
	bool operator!=(const OcdEntityIndexIterator<F,T,E>& rhs) const;
	
private:
	const OcdFile<F>* data;
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
	typedef F FileFormat;
	typedef T EntityType;
	typedef quint32 EntryType;
	typedef Ocd::IndexBlock<quint32> IndexBlock;
	
	OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block);
	
	const OcdEntityIndexIterator& operator++();
	
	const T& operator*() const;
	
	const T* operator->() const;
	
	bool operator==(const OcdEntityIndexIterator<F, T, quint32>& rhs) const;
	
	bool operator!=(const OcdEntityIndexIterator<F, T, quint32>& rhs) const;
	
private:
	const OcdFile<F>* data;
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
	typedef F FileFormat;
	
	/** The actual entity type. */
	typedef T EntityType;
	
	/** The index entry type for the entity type. */
	typedef typename T::IndexEntryType EntryType;
	
	/** The index iterator type. */
	typedef OcdEntityIndexIterator<F, T, EntryType> iterator;
	
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
	iterator begin() const;
	
	/**
	 * Returns a forward iterator to the end.
	 */
	iterator end() const;
	
private:
	const OcdFile<F>* data;
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
	typedef F Format;
	
	/** The actual file header type. */
	typedef typename F::FileHeader FileHeader;
	
	/** The actual string index type. */
	typedef OcdEntityIndex< F, Ocd::ParameterString > StringIndex;
	
	/** The actual symbol index type. */
	typedef OcdEntityIndex< F, typename F::BaseSymbol > SymbolIndex;
	
	/** The actual object index type. */
	typedef OcdEntityIndex< F, typename F::Object > ObjectIndex;
	
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
	~OcdFile() {};
	
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
	 * Returns the raw data of the string referenced by a string index iterator.
	 */
	const QByteArray operator[](const typename StringIndex::iterator& it) const;
	
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
	const typename F::Object& operator[](const typename ObjectIndex::iterator& it) const;
	
private:
	QByteArray byte_array;
	StringIndex string_index;
	SymbolIndex symbol_index;
	ObjectIndex object_index;
};



// ### FirstIndexBlock implementation ###

// Unknown entity type T: Return 0.
template< class F, class T >
quint32 FirstIndexBlock<F,T>::operator()(const OcdFile<F>* file) const
{
	Q_UNUSED(file);
	
	T* valid_entity_type = NULL;
	Q_ASSERT(valid_entity_type);
	
	return 0;
}

template< class F >
quint32 FirstIndexBlock<F,Ocd::ParameterString>::operator()(const OcdFile<F>* file) const
{
	return file->header()->first_string_block;
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
 : data(NULL),
   block(NULL),
   index(0)
{
	if (file && first_block)
	{
		data  = file;
		block = first_block;
	}
}

template< class F, class T, class E >
const OcdEntityIndexIterator<F,T,E>& OcdEntityIndexIterator<F,T,E>::operator++()
{
	if (data)
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
					block = reinterpret_cast<const IndexBlock*>(&(*data)[next_block]);
				}
				else
				{
					block = NULL;
					data = NULL;
				}
			}
		}
		while (block && !block->entries[index].pos);
	}
	return *this;
}

template< class F, class T, class E >
inline
const E& OcdEntityIndexIterator<F,T,E>::operator*() const
{
	return block->entries[index];
}

template< class F, class T, class E >
inline
const E* OcdEntityIndexIterator<F,T,E>::operator->() const
{
	return &(block->entries[index]);
}

template< class F, class T, class E >
inline
bool OcdEntityIndexIterator<F,T,E>::operator==(const OcdEntityIndexIterator<F,T,E>& rhs) const
{
	return (data == rhs.data && block == rhs.block && index == rhs.index);
}

template< class F, class T, class E >
inline
bool OcdEntityIndexIterator<F,T,E>::operator!=(const OcdEntityIndexIterator<F,T,E>& rhs) const
{
	return (data != rhs.data || block != rhs.block || index != rhs.index);
}



// ### OcdEntityIndexIterator specialization ###

template< class F, class T >
OcdEntityIndexIterator<F,T,quint32>::OcdEntityIndexIterator(const OcdFile<F>* file, const IndexBlock* first_block)
 : data(NULL),
   block(NULL),
   index(0)
{
	if (file && first_block)
	{
		data  = file;
		block = first_block;
	}
}

template< class F, class T >
const OcdEntityIndexIterator<F,T,quint32>& OcdEntityIndexIterator<F,T,quint32>::operator++()
{
	if (data)
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
					block = reinterpret_cast<const IndexBlock*>(&(*data)[next_block]);
				}
				else
				{
					block = NULL;
					data = NULL;
				}
			}
		}
		while ( data && !block->entries[index] );
	}
	return *this;
}

template< class F, class T >
inline
const T& OcdEntityIndexIterator<F,T,quint32>::operator*() const
{
	return reinterpret_cast<const T&>((*data)[block->entries[index]]);
}

template< class F, class T >
inline
const T* OcdEntityIndexIterator<F,T,quint32>::operator->() const
{
	return reinterpret_cast<const T*>(&(*data)[block->entries[index]]);
}

template< class F, class T >
inline
bool OcdEntityIndexIterator<F,T,quint32>::operator==(const OcdEntityIndexIterator<F,T,quint32>& rhs) const
{
	return (data == rhs.data && block == rhs.block && index == rhs.index);
}

template< class F, class T >
inline
bool OcdEntityIndexIterator<F,T,quint32>::operator!=(const OcdEntityIndexIterator<F,T,quint32>& rhs) const
{
	return (data != rhs.data || block != rhs.block || index != rhs.index);
}



// ### OcdEntityIndex implementation ###

template< class F, class T >
OcdEntityIndex<F,T>::OcdEntityIndex()
 : data(NULL)
{
}

template< class F, class T >
OcdEntityIndex<F,T>::~OcdEntityIndex()
{
}

template< class F, class T >
void OcdEntityIndex<F,T>::setData(const OcdFile< F >* file)
{
	data = file;
}

template< class F, class T >
typename OcdEntityIndex<F,T>::iterator OcdEntityIndex<F,T>::begin() const
{
	Q_ASSERT(data != NULL);
	
	quint32 file_pos = FirstIndexBlock<F,T>()(data);
	iterator it(data, reinterpret_cast<const typename iterator::IndexBlock*>(file_pos ? &(*data)[file_pos] : NULL));
	return it;
}

template< class F, class T >
typename OcdEntityIndex<F,T>::iterator OcdEntityIndex<F,T>::end() const
{
	static iterator it(NULL, NULL);
	return it;
}



// ### OcdFile implementation ###

template< class F >
inline
OcdFile<F>::OcdFile(const QByteArray& data)
{
	byte_array = data;
	Q_ASSERT(byte_array.constData() == data.constData()); // No deep copy
	string_index.setData(this);
	symbol_index.setData(this);
	object_index.setData(this);
}

template< class F >
inline
const QByteArray& OcdFile<F>::byteArray() const
{
	return byte_array;
}

template< class F >
inline
const char& OcdFile<F>::operator[](quint32 file_pos) const
{
	return *(byte_array.constData() + file_pos);
}

template< class F >
inline
const typename F::FileHeader* OcdFile<F>::header() const
{
	return (byte_array.size() < (int)sizeof(FileHeader)) ? NULL : (const FileHeader*)byte_array.constData();
}

template< class F >
inline
const typename OcdFile<F>::StringIndex& OcdFile<F>::strings() const
{
	return string_index;
}

template< class F >
inline
const QByteArray OcdFile<F>::operator[](const typename OcdFile<F>::StringIndex::iterator& it) const
{
	return QByteArray::fromRawData(&(*this)[it->pos], it->size);
}

template< class F >
inline
const typename OcdFile<F>::SymbolIndex& OcdFile<F>::symbols() const
{
	return symbol_index;
}

template< class F >
inline
const typename OcdFile<F>::ObjectIndex& OcdFile<F>::objects() const
{
	return object_index;
}

template< class F >
inline
const typename F::Object& OcdFile<F>::operator[](const typename OcdFile<F>::ObjectIndex::iterator& it) const
{
	return reinterpret_cast<const typename F::Object&>((*this)[it->pos]);
}

#endif
