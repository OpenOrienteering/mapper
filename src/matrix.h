/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _WORDS_MATRIX_H_
#define _WORDS_MATRIX_H_

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

#ifdef _MSC_VER
	#define isnan _isnan
#endif

class Matrix
{
public:
	
	inline Matrix() : d(NULL), n(0), m(0) {}
	Matrix(const Matrix& other)
	{
		n = other.n;
		m = other.m;
		d = new double[n * m];
		memcpy(d, other.d, n * m * sizeof(double));
	}
	inline Matrix(int n, int m) : n(n), m(m)
	{
		d = new double[n*m];
		memset(d, 0, m*n*sizeof(double));
	}
	~Matrix()
	{
		delete[] d;
	}
	
	void save(QFile* file);
	void load(QFile* file);
	
	inline int getRows() const {return n;}
	inline int getCols() const {return m;}
	
	void operator=(const Matrix& other)
	{
		delete[] d;
		n = other.n;
		m = other.m;
		d = new double[n * m];
		memcpy(d, other.d, n * m * sizeof(double));
	}
	
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
	void setTo(double v)
	{
		for (int i = 0; i < n*m; ++i)
			d[i] = v;
	}
	
	inline void set(int i, int j, double v)
	{
		d[i*m + j] = v;
	}
	inline double get(int i, int j) const
	{
		return d[i*m + j];
	}
	
	void swapRows(int a, int b)
	{
		assert(a != b);
		for (int i = 0; i < m; ++i)
		{
			double temp = get(a, i);
			set(a, i, get(b, i));
			set(b, i, temp);
		}
	}
	
	void subtract(const Matrix& b, Matrix& out) const
	{
		assert(n == b.n && m == b.m);
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] - b.d[i];
	}
	void add(const Matrix& b, Matrix& out) const
	{
		assert(n == b.n && m == b.m);
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] + b.d[i];
	}
	void multiply(double b, Matrix& out) const
	{
		out.setSize(n, m);
		for (int i = 0; i < n*m; ++i)
			out.d[i] = d[i] * b;
	}
	void multiply(const Matrix& b, Matrix& out) const
	{
		assert(m == b.n);
		out.setSize(n, b.m);
		out.setTo(0);
		
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < b.m; ++j)
				for (int k = 0; k < m; ++k)
					out.set(i, j, out.get(i,j) + get(i, k) * b.get(k, j));
	}
	void transpose(Matrix& out)
	{
		assert(this != &out);
		out.setSize(m, n);
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				out.set(j, i, get(i, j));
	}
	double determinant() const
	{
		Matrix a = Matrix(*this);
		
		double result = 1;
		for (int i = 1; i < n; ++i)
		{
			// Pivot search
			if (a.get(i - 1, i - 1) <= 0.001)
			{
				double highest = a.get(i - 1, i - 1);
				int highest_pos = i - 1;
				for (int k = i; k < n; ++k)
				{
					double v = a.get(k, i - 1);
					if (v > highest)
					{
						highest = v;
						highest_pos = k;
					}
				}
				if (highest == 0)
					return 0;
				if (i - 1 != highest_pos)
				{
					a.swapRows(i - 1, highest_pos);
					result = -result;
				}
			}
			
			result *= a.get(i-1, i-1);
			
			for (int k = i; k < n; ++k)
			{
				double factor = -a.get(k, i - 1) / a.get(i - 1, i - 1);
				for (int j = i; j < m; ++j)
					a.set(k, j, a.get(k, j) + factor * a.get(i - 1, j));
			}
		}
		result *= a.get(n-1, n-1);
		
		if (isnan(result))
		{
			print();
			assert(false);
		}
		
		return result;
	}
	bool invert(Matrix& out) const
	{
		Matrix a = Matrix(*this);
		out.setSize(n, m);
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j)
				out.set(i, j, (i == j) ? 1 : 0);
		
		for (int i = 1; i < n; ++i)
		{
			// Pivot search
			if (a.get(i - 1, i - 1) <= 0.001)
			{
				double highest = a.get(i - 1, i - 1);
				int highest_pos = i - 1;
				for (int k = i; k < n; ++k)
				{
					double v = a.get(k, i - 1);
					if (v > highest)
					{
						highest = v;
						highest_pos = k;
					}
				}
				if (highest == 0)
					return false;
				if (i - 1 != highest_pos)
				{
					a.swapRows(i - 1, highest_pos);
					out.swapRows(i - 1, highest_pos);
				}
			}
			
			for (int k = i; k < n; ++k)
			{
				double factor = -a.get(k, i - 1) / a.get(i - 1, i - 1);
				for (int j = 0; j < m; ++j)
				{
					a.set(k, j, a.get(k, j) + factor * a.get(i - 1, j));
					out.set(k, j, out.get(k, j) + factor * out.get(i - 1, j));
				}
			}
		}
		for (int i = n - 2; i >= 0; --i)
		{
			for (int k = i; k >= 0; --k)
			{
				double factor = -a.get(k, i + 1) / a.get(i + 1, i + 1);
				for (int j = 0; j < m; ++j)
				{
					a.set(k, j, a.get(k, j) + factor * a.get(i + 1, j));
					out.set(k, j, out.get(k, j) + factor * out.get(i + 1, j));
				}
			}
		}
		for (int i = 0; i < n; ++i)
		{
			double factor = 1 / a.get(i, i);
			for (int j = 0; j < m; ++j)
				out.set(i, j, out.get(i, j) * factor);
		}
		
		return true;
	}
	
	void print() const
	{
		for (int i = 0; i < n; ++i)
		{
			printf("( ");
			for (int j = 0; j < m; ++j)
			{
				printf("%f ", get(i, j));
			}
			printf(")\n");
		}
		printf("\n");
	}
	
private:
	
	double* d;
	int n;
	int m;
};

#endif
