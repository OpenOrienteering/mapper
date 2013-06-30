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


#ifndef RESERVED_MEMBER
	
	// Helper macro
	#define RESERVED_MEMBER_CONCAT2(A,B) A ## B
	// Helper macro
	#define RESERVED_MEMBER_CONCAT(A,B) RESERVED_MEMBER_CONCAT2(A,B)
	
	/**
	 * A macro which generates a unique member name of the format
	 * reserved_member_LINE_NUMBER.
	 */
	#define RESERVED_MEMBER RESERVED_MEMBER_CONCAT( reserved_member_ , __LINE__ )
	
#endif // RESERVED_MEMBER


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
	 * A string of max. N characters with a pascal-style binary representation:
	 * the first byte indicates the length,
	 * the following N bytes contain the actual character data.
	 */
	template< unsigned int N >
	struct PascalString
	{
		unsigned char length;
		char data[N];
	};
	
	struct FileHeaderGeneric
	{
		quint16 vendor_mark;
		quint16 RESERVED_MEMBER;
		quint16 version;
		quint16 subversion;
	};
	
	struct FileHeaderV8
	{
		quint16 vendor_mark;
		quint16 section_mark;
		quint16 version;
		quint16 subversion;
		quint32 first_symbol_block;
		quint32 first_object_block;
		quint32 setup_pos;
		quint32 setup_size;
		quint32 info_pos;
		quint32 info_size;
		quint32 first_string_block;
		quint32 RESERVED_MEMBER;
		quint32 RESERVED_MEMBER;
		quint32 RESERVED_MEMBER;
	};
	
	template< class E >
	struct IndexBlock
	{
		typedef E IndexEntryType;
		
		quint32 next_block;
		IndexEntryType entries[256];
	};
	
	struct StringIndexEntry // V11
	{
		quint32 pos;
		quint32 size;
		qint32 type;
		quint32 obj_index;
	};
	
	struct String
	{
		typedef StringIndexEntry IndexEntryType;
	};
	
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
	
	/** Generic OCD file format trait. */
	struct FormatGeneric
	{
		static inline int version() { return -1; };
		
		typedef FileHeaderGeneric FileHeader;
		
		struct BaseSymbol { typedef quint32 IndexEntryType; };
		
		struct Object { typedef quint32 IndexEntryType; };
	};
	
	/** OCD file format version 8 trait. */
	struct FormatV8
	{
		static inline int version() { return 8; };
		
		typedef FileHeaderV8 FileHeader;
	};
	
// 	/** OCD file format version 9 trait. */
// 	struct Format9
// 	{
// 		static inline int version() { return 9; };
// 		
// 		typedef FileHeaderV9 FileHeader;
// 	};
	
}



template< class F >
class OcdFile;



template< class F, class T >
class FirstIndexBlock
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

template< class F >
class FirstIndexBlock<F, Ocd::String>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

template< class F >
class FirstIndexBlock<F, typename F::BaseSymbol>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};

template< class F >
class FirstIndexBlock<F, typename F::Object>
{
public:
	quint32 operator()(const OcdFile<F>* file) const;
};



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



template< class F, class T >
class OcdEntityIndex
{
public:
	typedef F FileFormat;
	typedef T EntityType;
	typedef typename T::IndexEntryType EntryType;
	typedef OcdEntityIndexIterator<F, T, EntryType> iterator;
	
	OcdEntityIndex();
	
	~OcdEntityIndex();
	
	void setData(const OcdFile<F>* file);
	
	iterator begin() const;
	
	iterator end() const;
	
private:
	const OcdFile<F>* data;
};



template< class F >
class OcdFile
{
public:
	typedef F Format;
	typedef typename F::FileHeader FileHeader;
	typedef OcdEntityIndex< F, Ocd::String > StringIndex;
	typedef OcdEntityIndex< F, typename F::BaseSymbol > SymbolIndex;
	typedef OcdEntityIndex< F, typename F::Object > ObjectIndex;
	
	OcdFile(const QByteArray& data);
	
	~OcdFile() {};
	
	const QByteArray& byteArray() const;
	
	const char& operator[](quint32 file_pos) const;
	
	const FileHeader* header() const;
	
	const StringIndex& strings() const;
	
	const QByteArray operator[](const typename StringIndex::iterator& it) const;
	
	const SymbolIndex& symbols() const;
	
	const ObjectIndex& objects() const;
	
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
quint32 FirstIndexBlock<F,Ocd::String>::operator()(const OcdFile<F>* file) const
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
