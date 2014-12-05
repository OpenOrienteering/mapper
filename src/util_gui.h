/*
 *    Copyright 2012, 2013 Kai Pastor
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


#ifndef _OPENORIENTEERING_UTIL_GUI_H_
#define _OPENORIENTEERING_UTIL_GUI_H_

#include <cmath>

#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringBuilder>
#include <QStyle>

class MapCoordF;

/** A collection of GUI utility functions. */
namespace Util
{
	/**
	 * Provides information about the properties of Mapper types
	 * for the purpose of customizing input widgets.
	 * 
	 * The generic class is empty.
	 * Template specializations provide the actual values.
	 * See InputProperties<MapCoordF> for an example.
	 */
	template< class T >
	struct InputProperties
	{
		// intentionally left empty
	};
	
	/**
	 * Provides information about the properties of MapCoordF
	 * for the purpose of customizing input widgets.
	 */
	template< >
	struct InputProperties< MapCoordF >
	{
		/** The underlying fundamental type. */
		typedef double basetype;
		
		/** The minimum input value. */
		inline static double min() throw()   { return -99999999.99; }
		
		/** The maximum input value. */
		inline static double max() throw()   { return +99999999.99; }
		
		/** The spinbox step width. */
		inline static double step() throw()  { return 1.0; }
		
		/** The number of decimals. */
		inline static int decimals() throw() { return 2; }
		
		/** The unit of measurement, translated in context UnitOfMeasurement. */
		inline static QString unit()
		{
			return QCoreApplication::translate("UnitOfMeasurement", "mm", "millimeters");
		}
	};
	
	/** Identifies the type double representing real meters */
	struct RealMeters
	{
		// intentionally left empty
	};
	
	/**
	 * Provides information about the type double representing real meters
	 * for the purpose of customizing input widgets.
	 */
	template< >
	struct InputProperties< RealMeters >
	{
		/** The underlying fundamental type. */
		typedef double basetype;
		
		/** The minimum input value. */
		inline static double min() throw()   { return -99999999.99; }
		
		/** The maximum input value. */
		inline static double max() throw()   { return +99999999.99; }
		
		/** The spinbox step width. */
		inline static double step() throw()  { return 1.0; }
		
		/** The number of decimals. */
		inline static int decimals() throw() { return 2; }
		
		/** The unit of measurement, translated in context UnitOfMeasurement. */
		inline static QString unit()
		{
			return QCoreApplication::translate("UnitOfMeasurement", "m", "meters");
		}
	};
	
	namespace Headline
	{
		/** 
		 * Creates a QLabel which is styled as a headline.
		 *
		 * This headline is intended for use in dialogs.
		 */ 
		inline QLabel* create(const QString& text)
		{
			return new QLabel(QString("<b>") % text % "</b>");
		}
		
		/** 
		 * Creates a QLabel which is styled as a headline.
		 *
		 * This headline is intended for use in dialogs.
		 */ 
		inline QLabel* create(const char* text)
		{
			return new QLabel(QString("<b>") % text % "</b>");
		}
	}
	
	namespace SpacerItem
	{
		/** 
		 * Creates a QSpacerItem which takes up a style dependent width
		 * and height.
		 *
		 * This spacer item is intended for use with QFormLayout which
		 * does not offer a direct mean for extra spacing.
		 */ 
		inline QSpacerItem* create(const QWidget* widget)
		{
			const int spacing = widget->style()->pixelMetric(QStyle::PM_LayoutTopMargin);
			return new QSpacerItem(spacing, spacing);
		}
	}
	
	namespace SpinBox
	{
#ifndef NDEBUG
		/**
		 * Returns the maximum number of digits in a spinbox which is regarded
		 * as normal. Exceedings this number in Util::SpinBox::create() will
		 * print a runtime warning in development builds.
		 */
		inline int max_digits()
		{
			return 13;
		}
#endif
		
		/**
		 * Creates and initializes a QSpinBox.
		 * 
		 * This method allows to initialize the most frequent options of
		 * QSpinBox in a single call:
		 * the lower and upper bound of the valid range,
		 * the unit of measurement (optional),
		 * the step width of the spinbox buttons (optional).
		 */
		inline QSpinBox* create(int min, int max, const QString &unit = "", int step = 0)
		{
			QSpinBox* box = new QSpinBox();
			box->setRange(min, max);
			if (unit.startsWith(' '))
				box->setSuffix(unit);
			else if (unit.length() > 0)
				box->setSuffix(QString(' ') % unit);
			if (step > 0)
				box->setSingleStep(step);
#ifndef NDEBUG
			if (box->locale().toString(min).remove(box->locale().groupSeparator()).length() > max_digits())
				qDebug().nospace()
				  << "WARNING: Util::SpinBox::create() will create a very large widget because of min="
			      << box->locale().toString(min);
			if (box->locale().toString(max).remove(box->locale().groupSeparator()).length() > max_digits())
				qDebug().nospace()
				  << "WARNING: Util::SpinBox::create() will create a very large widget because of max="
			      << box->locale().toString(max);
#endif
			return box;
		}
		
		/**
		 * Creates and initializes a QDoubleSpinBox.
		 * 
		 * This method allows to initialize the most frequent options of
		 * QDoubleSpinBox in a single call:
		 * the number of decimals,
		 * the lower and upper bound of the valid range,
		 * the unit of measurement (optional),
		 * the step width of the spinbox buttons (optional; dependent on 
		 * the number of decimals if not specified).
		 */
		inline QDoubleSpinBox* create(int decimals, double min, double max, const QString &unit = "", double step = 0.0)
		{
			QDoubleSpinBox* box = new QDoubleSpinBox();
			box->setDecimals(decimals);
			box->setRange(min, max);
			if (unit.startsWith(' '))
				box->setSuffix(unit);
			else if (unit.length() > 0)
				box->setSuffix(QString(' ') % unit);
			if (step > 0.0)
				box->setSingleStep(step);
			else
			{
				switch (decimals)
				{
					case 0: 	box->setSingleStep(1.0); break;
					case 1: 	box->setSingleStep(0.1); break;
					default: 	box->setSingleStep(5.0 * pow(10.0, -decimals));
				}
			}
#ifndef NDEBUG
			if (box->textFromValue(min).remove(box->locale().groupSeparator()).length() > max_digits())
				qDebug().nospace()
				  << "WARNING: Util::SpinBox::create() will create a very large widget because of min="
			      << box->locale().toString(min, 'f', decimals);
			if (box->textFromValue(max).remove(box->locale().groupSeparator()).length() > max_digits())
				qDebug().nospace()
				  << "WARNING: Util::SpinBox::create() will create a very large widget because of max="
			      << box->locale().toString(max, 'f', decimals);
#endif
			return box;
		}
		
		/**
		 * Creates and initializes a QDoubleSpinBox.
		 * 
		 * This method allows to initialize the most frequent options of
		 * QDoubleSpinBox in a single call, determining the actual properties
		 * via InputProperties<T>.
		 */
		template< class T >
		inline QDoubleSpinBox* create()
		{
			typedef InputProperties<T> P;
			return create(P::decimals(), P::min(), P::max(), P::unit(), P::step());
		}
		
		/**
		 * @deprecated Transitional method.
		 * 
		 * Creates and initializes a QDoubleSpinBox.
		 * 
		 * This method allows to initialize the most frequent options of
		 * QDoubleSpinBox in a single call, determining the actual properties
		 * via InputProperties<T>.
		 * 
		 * The unit of measurement is taken from the actual parameter. This is
		 * meant to support the transition from code where the translation of
		 * units still exists in the context of the client code, instead of
		 * in the context UnitOfMeasurement.
		 */
		template< class T >
		inline QDoubleSpinBox* create(const QString& unit)
		{
			typedef InputProperties<T> P;
			return create(P::decimals(), P::min(), P::max(), unit, P::step());
		}
	}
	
	namespace TristateCheckbox
	{
		inline
		void setDisabledAndChecked(QCheckBox* checkbox, bool checked)
		{
			Q_ASSERT(checkbox);
			checkbox->setEnabled(false);
			checkbox->setTristate(true);
			checkbox->setCheckState(checked ? Qt::PartiallyChecked : Qt::Unchecked);
		}
		
		inline
		void setEnabledAndChecked(QCheckBox* checkbox, bool checked)
		{
			Q_ASSERT(checkbox);
			checkbox->setEnabled(true);
			checkbox->setChecked(checked);
			checkbox->setTristate(false);
		}
	}
}

#endif
