/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_UTIL_MATRIX_H
#define OPENORIENTEERING_UTIL_MATRIX_H

#include <cstring>

#include <QtGlobal>

class QString;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {


/** Dynamically sized matrix of doubles. */
class Matrix
{
public:
	
	/** Constructs a 0x0 matrix. */
	inline Matrix() : d(nullptr), n(0), m(0) {}
	
	/** Copy constructor. */
	Matrix(const Matrix& other)
	{
		n = other.n;
		m = other.m;
		d = new double[n * m];
		memcpy(d, other.d, n * m * sizeof(double));
	}
	
	/** Constructs a nxm matrix. */
	inline Matrix(int n, int m) : n(n), m(m)
	{
		d = new double[n*m];
		memset(d, 0, m*n*sizeof(double));
	}
	
	/** Destructs the matrix. */
	~Matrix()
	{
		delete[] d;
	}
	
	
	/** Saves the matrix in xml format with the given value of the role attribute. */
	void save(QXmlStreamWriter& xml, const QString& role) const;
	/** Loads the matrix in xml format. */
 	void load(QXmlStreamReader& xml);
	
	/** Returns the number of rows. */
	inline int getRows() const {return n;}
	/** Returns the number of columns. */
	inline int getCols() const {return m;}
	
	/** Assignment */
	Matrix& operator=(const Matrix& other)
	{
		if (this == &other)
			return *this;
		
		delete[] d;
		n = other.n;
		m = other.m;
		d = new double[n * m];
		memcpy(d, other.d, n * m * sizeof(double));
		return *this;
	}
	
	/**
	 * Changes the size of the matrix. If the new size is different to the old,
	 * all matrix elements will be reset to zero.
	 */
	void setSize(int n, int m)
	{
		if (this->n == n && this->m == m)
			return;
		
		this->n = n;
		this->m = m;
		delete[] d;
		d = new double[n*m];
		memset(d, 0, m*n*sizeof(double));
	}
	
	/** Sets all matrix elements to v. */
	void setTo(double v)
	{
		for (int i = 0; i < n*m; ++i)
			d[i] = v;
	}
	
	/** Sets a matrix element. */
	inline void set(int i, int j, double v)
	{
		d[i*m + j] = v;
	}
	/** Returns a matrix element. */
	inline double get(int i, int j) const
	{
		return d[i*m + j];
	}
	
	/** Exchanges the rows with indices a and b. */
	void swapRows(int a, int b)
	{
		Q_ASSERT(a != b);
		for (int i = 0; i < m; ++i)
		{
			double temp = get(a, i);
			set(a, i, get(b, i));
			set(b, i, temp);
		}
	}
	
	/** Component-wise subtraction. */
	void subtract(const Matrix& b, Matrix& out) const
	{
		Q_ASSERT(n == b.n && m == b.m);
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] - b.d[i];
	}
	/** Component-wise addition. */
	void add(const Matrix& b, Matrix& out) const
	{
		Q_ASSERT(n == b.n && m == b.m);
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] + b.d[i];
	}
	/** Multiplication with scalar factor. */
	void multiply(double b, Matrix& out) const
	{
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] * b;
	}
	/** Matrix multiplication. */
	void multiply(const Matrix& b, Matrix& out) const
	{
		Q_ASSERT(m == b.n);
		out.setSize(n, b.m);
		out.setTo(0);
		
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < b.m; ++j)
				for (int k = 0; k < m; ++k)
					out.set(i, j, out.get(i,j) + get(i, k) * b.get(k, j));
	}
	/** Matrix transpose. */
	void transpose(Matrix& out)
	{
		Q_ASSERT(this != &out);
		out.setSize(m, n);
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				out.set(j, i, get(i, j));
	}
	
	/** Calculates the determinant. */
	double determinant() const;
	
	/** Tries to inverts the matrix. Returns true if successful. */
	bool invert(Matrix& out) const;
	
	/** Outputs the matrix to stdout for debugging purposes. */
	void print() const;
	
private:
	
	double* d;
	int n;
	int m;
};


}  // namespace OpenOrienteering
#endif
