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
#include <QDoubleValidator>
#include <QString>
#include <QValidator>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QObject;
class QPainter;
class QPointF;
class QSpacerItem;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class MapCoordF;


/** Double validator for line edit widgets,
 *  ensures that only valid doubles can be entered. */
class DoubleValidator : public QDoubleValidator  // clazy:exclude=missing-qobject-macro
{
public:
	DoubleValidator(double bottom, double top = 10e10, QObject* parent = nullptr, int decimals = 20);
	
	~DoubleValidator() override;
	
	State validate(QString& input, int& pos) const override;
};



/**
 * A collection of GUI utility functions.
 */
namespace Util
{

	/**
	 * Converts millimeters to pixels using the physical dpi setting of
	 * Mapper's settings. This should be used to calculate sizes of map elements.
	 * @sa mmToPixelLogical()
	 */
	qreal mmToPixelPhysical(qreal millimeters);
	
	/** Inverse of mmToPixelPhysical(). */
	qreal pixelToMMPhysical(qreal pixels);
	
	
	/**
	 * Converts millimeters to pixels using the "logical" dpi setting of
	 * the operating system. This should be used to calculate sizes of UI
	 * elements.
	 * @sa mmToPixelPhysical()
	 */
	qreal mmToPixelLogical(qreal millimeters);
	
	/** Inverse of mmToPixelLogical(). */
	qreal pixelToMMLogical(qreal pixels);
	
	
	/** Returns true for low-dpi screens, false for high-dpi screens. */
	bool isAntialiasingRequired();
	
	/** Returns true for low-dpi screens, false for high-dpi screens. */
	bool isAntialiasingRequired(qreal ppi);
	
	
	
	/**
	 * Show the manual in Qt assistant.
	 * 
	 * @param filename_latin1 the name of the manual page html file
	 * @param anchor_latin1 the anchor in the specified file to jump to
	 */
	void showHelp(QWidget* dialog_parent, const char* filename_latin1, const char* anchor_latin1);
	
	/**
	 * Show the manual in Qt assistant.
	 * 
	 * The anchor may be left out or given with the filename.
	 * 
	 * @param file_and_anchor_latin1 the name of the manual page html file, optionally including an anchor
	 */
	void showHelp(QWidget* dialog_parent, const char* file_and_anchor_latin1 = "index.html");
	
	/**
	 * Show the manual in Qt assistant.
	 * 
	 * The anchor may be left out or given with the filename.
	 * 
	 * @param file_and_anchor the name of the manual page html file, optionally including an anchor
	 */
	void showHelp(QWidget* dialog_parent, const QString& file_and_anchor);
	
	
	
	/**
	 * Creates a What's-this text "See more" linking to the given page and
	 * fragment in the manual.
	 */
	QString makeWhatThis(const char* reference_latin1);
	
	
	
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
		static QString unit();
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
		static QString unit();
	};
	
	
	
	namespace Marker
	{
		/** Center marker sign for rotate and scale tools. */
		void drawCenterMarker(QPainter* painter, const QPointF& center);
	}  // namespace Marker


	namespace Headline
	{
		/**
		 * Creates a QLabel which is styled as a headline.
		 *
		 * This headline is intended for use in dialogs.
		 */
		QLabel* create(const QString& text);
		
		/**
		 * Creates a QLabel which is styled as a headline.
		 *
		 * This headline is intended for use in dialogs.
		 */
		QLabel* create(const char* text_utf8);
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
		QSpacerItem* create(const QWidget* widget);
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
	
	
	
	/**
	 * Remove any HTML markup from the input text.
	 * 
	 * \see QTextDocument::toPlainText
	 */
	QString plainText(QString maybe_markup);
	
}


}  // namespace OpenOrienteering
#endif
