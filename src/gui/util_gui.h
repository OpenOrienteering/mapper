/*
 *    Copyright 2012, 2013, 2015, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_UTIL_GUI_H
#define OPENORIENTEERING_UTIL_GUI_H

#include <QtGlobal>
#include <QCoreApplication>
#include <QLabel>
#include <QSpacerItem>
#include <QString>
#include <QStyle>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDoubleSpinBox;
// IWYU pragma: no_forward_declare QLabel
class QSpacerItem;
class QSpinBox;
QT_END_NAMESPACE

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
		constexpr static double min() noexcept { return -99999999.99; }
		
		/** The maximum input value. */
		constexpr static double max() noexcept { return +99999999.99; }
		
		/** The spinbox step width. */
		constexpr static double step() noexcept { return 1.0; }
		
		/** The number of decimals. */
		constexpr static int decimals() noexcept { return 2; }
		
		/** The unit of measurement, translated in context UnitOfMeasurement. */
		static QString unit()
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
		static constexpr double min() noexcept { return -99999999.99; }
		
		/** The maximum input value. */
		constexpr static double max() noexcept { return +99999999.99; }
		
		/** The spinbox step width. */
		constexpr static double step() noexcept { return 1.0; }
		
		/** The number of decimals. */
		constexpr static int decimals() noexcept { return 2; }
		
		/** The unit of measurement, translated in context UnitOfMeasurement. */
		static QString unit()
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
		inline
		QLabel* create(const QString& text)
		{
			return new QLabel(QLatin1String("<b>") + text + QLatin1String("</b>"));
		}
		
		/**
		 * Creates a QLabel which is styled as a headline.
		 *
		 * This headline is intended for use in dialogs.
		 */
		inline
		QLabel* create(const char* text_utf8)
		{
			return create(QString::fromUtf8(text_utf8));
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
		inline
		QSpacerItem* create(const QWidget* widget)
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
		inline
		constexpr int max_digits()
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
		QSpinBox* create(int min, int max, const QString &unit = {}, int step = 0);
		
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
		QDoubleSpinBox* create(int decimals, double min, double max, const QString &unit = {}, double step = 0.0);
		
		/**
		 * Creates and initializes a QDoubleSpinBox.
		 * 
		 * This method allows to initialize the most frequent options of
		 * QDoubleSpinBox in a single call, determining the actual properties
		 * via InputProperties<T>.
		 */
		template< class T >
		QDoubleSpinBox* create()
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
		QDoubleSpinBox* create(const QString& unit)
		{
			typedef InputProperties<T> P;
			return create(P::decimals(), P::min(), P::max(), unit, P::step());
		}
	}
	
	namespace TristateCheckbox
	{
		void setDisabledAndChecked(QCheckBox* checkbox, bool checked);
		
		void setEnabledAndChecked(QCheckBox* checkbox, bool checked);
	}
	
}

#endif
