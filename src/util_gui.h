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

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

/** A collection of GUI utility functions. */
namespace Util
{
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
			return box;
		}
	}
}

#endif
