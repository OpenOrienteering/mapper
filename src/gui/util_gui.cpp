/*
 *    Copyright 2017 Kai Pastor
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


#include "util_gui.h"

#include <cmath>

#include <Qt>
#include <QtGlobal>
#include <QCheckBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLatin1Char>
#include <QLocale>
#include <QSpinBox>
#include <QTextDocument>


namespace Util
{

	namespace SpinBox
	{
		
		QSpinBox* create(int min, int max, const QString &unit, int step)
		{
			auto box = new QSpinBox();
			box->setRange(min, max);
			const auto space = QLatin1Char{' '};
			if (unit.startsWith(space))
				box->setSuffix(unit);
			else if (unit.length() > 0)
				box->setSuffix(space + unit);
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
		
		QDoubleSpinBox* create(int decimals, double min, double max, const QString &unit, double step)
		{
			auto box = new QDoubleSpinBox();
			box->setDecimals(decimals);
			box->setRange(min, max);
			const auto space = QLatin1Char{' '};
			if (unit.startsWith(space))
				box->setSuffix(unit);
			else if (unit.length() > 0)
				box->setSuffix(space + unit);
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
		
	}
		
	namespace TristateCheckbox
	{
		void setDisabledAndChecked(QCheckBox* checkbox, bool checked)
		{
			Q_ASSERT(checkbox);
			checkbox->setEnabled(false);
			checkbox->setTristate(true);
			checkbox->setCheckState(checked ? Qt::PartiallyChecked : Qt::Unchecked);
		}
		
		void setEnabledAndChecked(QCheckBox* checkbox, bool checked)
		{
			Q_ASSERT(checkbox);
			checkbox->setEnabled(true);
			checkbox->setChecked(checked);
			checkbox->setTristate(false);
		}
	}
	
	
	
	QString plainText(QString maybe_markup)
	{
		if (maybe_markup.contains(QLatin1Char('<')))
		{
			QTextDocument doc;
			doc.setHtml(maybe_markup);
			maybe_markup = doc.toPlainText();
		}
		return maybe_markup;
	}
	
}
