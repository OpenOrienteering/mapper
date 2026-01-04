/*
 *    Copyright 2017, 2019, 2024, 2026 Kai Pastor
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
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStringList>
#include <QStyle>
#include <QTextDocumentFragment>
#include <QToolButton>
#if defined(Q_OS_ANDROID)
#include <QUrl>
#endif
#include <QVariant>
#include <QWidget>

#include "mapper_config.h"
#include "settings.h"
#include "gui/text_browser_dialog.h" // IWYU pragma: keep


namespace OpenOrienteering {

DoubleValidator::DoubleValidator(double bottom, double top, QObject* parent, int decimals)
: QDoubleValidator(bottom, top, decimals, parent)
{
	// Don't cause confusion, accept only English formatting
	setLocale(QLocale(QLocale::English));
}


DoubleValidator::~DoubleValidator() = default;


QValidator::State DoubleValidator::validate(QString& input, int& pos) const
{
	// Transform thousands group separators into decimal points to avoid confusion
	input.replace(QLatin1Char(','), QLatin1Char('.'));
	
	return QDoubleValidator::validate(input, pos);
}



namespace Util {
	
#if 0
	// Implementation moved to settings.cpp
	qreal mmToPixelPhysical(qreal millimeters);
	qreal pixelToMMPhysical(qreal pixels);
	qreal mmToPixelLogical(qreal millimeters);
	qreal pixelToMMLogical(qreal pixels);
	bool isAntialiasingRequired(qreal ppi);
	bool isAntialiasingRequired();
#endif
	
	
	void showHelp(QWidget* dialog_parent, const char* filename_latin1, const char* anchor_latin1)
	{
		showHelp(dialog_parent, QString::fromLatin1(filename_latin1) + QLatin1Char('#') + QString::fromLatin1(anchor_latin1));
	}
	
	void showHelp(QWidget* dialog_parent, const char* file_and_anchor_latin1)
	{
		showHelp(dialog_parent, QString::fromLatin1(file_and_anchor_latin1));
	}
	
	void showHelp(QWidget* dialog_parent, const QString& file_and_anchor)
	{
	#if defined(Q_OS_ANDROID)
		const QString manual_path = QLatin1String("doc:/manual/") + file_and_anchor;
		const QUrl help_url = QUrl::fromLocalFile(manual_path);
		TextBrowserDialog help_dialog(help_url, dialog_parent);
		help_dialog.exec();
		return;
	#else
		const auto base_url = QLatin1String("qthelp://" MAPPER_HELP_NAMESPACE "/manual/");
		
		static QProcess assistant_process;
		if (assistant_process.state() == QProcess::Running)
		{
			QString command = QLatin1String("setSource ") + base_url + file_and_anchor + QLatin1Char('\n');
			assistant_process.write(command.toLatin1());
		}
		else
		{
			auto help_collection = QFileInfo{ QString::fromUtf8("doc:Mapper " APP_VERSION_FILESYSTEM " Manual.qhc") };
			auto compiled_help = QFileInfo{ QString::fromUtf8("doc:Mapper " APP_VERSION_FILESYSTEM " Manual.qch") };
			if (!help_collection.exists() || !compiled_help.exists())
			{
				QMessageBox::warning(dialog_parent, QApplication::translate("OpenOrienteering::Util", "Error"), QApplication::translate("OpenOrienteering::Util", "Failed to locate the help files."));
				return;
			}
			
	#if defined(Q_OS_MACOS)
			const auto assistant = QString::fromLatin1("Assistant");
	#else
			const auto assistant = QString::fromLatin1("assistant");
	#endif
			auto assistant_path = QStandardPaths::findExecutable(assistant, { QCoreApplication::applicationDirPath() });
	#if defined(ASSISTANT_DIR)
			if (assistant_path.isEmpty())
				assistant_path = QStandardPaths::findExecutable(assistant, { QString::fromLatin1(ASSISTANT_DIR) });
	#endif
			if (assistant_path.isEmpty())
				assistant_path = QStandardPaths::findExecutable(assistant);
			if (assistant_path.isEmpty())
			{
				QMessageBox::warning(dialog_parent, QApplication::translate("OpenOrienteering::Util", "Error"), QApplication::translate("OpenOrienteering::Util", "Failed to locate the help browser (\"Qt Assistant\")."));
				return;
			}
			
			// Try to start the Qt Assistant process
			QStringList args;
			args << QLatin1String("-collectionFile")
				 << QDir::toNativeSeparators(help_collection.absoluteFilePath())
				 << QLatin1String("-showUrl")
				 << (base_url + file_and_anchor)
				 << QLatin1String("-enableRemoteControl");
			
			if ( QGuiApplication::platformName() == QLatin1String("xcb") ||
				 QGuiApplication::platformName().isEmpty() )
			{
				// Use the modern 'fusion' style instead of the default "windows"
				// style on X11.
				args << QLatin1String("-style") << QLatin1String("fusion");
			}
			
	#if defined(Q_OS_LINUX)
			auto env = QProcessEnvironment::systemEnvironment();
			env.insert(QLatin1String("QT_SELECT"), QLatin1String("5")); // #541
			env.insert(QLatin1String("LANG"), Settings::getInstance().getSetting(Settings::General_Language).toString());
			assistant_process.setProcessEnvironment(env);
	#endif
			
			assistant_process.start(assistant_path, args);
			
			// FIXME: Calling waitForStarted() from the main thread might cause the user interface to freeze.
			if (!assistant_process.waitForStarted())
			{
				QMessageBox msg_box;
				msg_box.setIcon(QMessageBox::Warning);
				msg_box.setWindowTitle(QApplication::translate("OpenOrienteering::Util", "Error"));
				msg_box.setText(QApplication::translate("OpenOrienteering::Util", "Failed to launch the help browser (\"Qt Assistant\")."));
				msg_box.setStandardButtons(QMessageBox::Ok);
				auto details = assistant_process.readAllStandardError();
				if (! details.isEmpty())
					msg_box.setDetailedText(QString::fromLocal8Bit(details));
				
				msg_box.exec();
			}
		}
	#endif
	}
	
	
	
	QString makeWhatThis(const char* reference_latin1)
	{
		//: This "See more" is displayed as a link to the manual in What's-this tooltips.
		return QStringLiteral("<a href=\"%1\">%2</a>").arg(
				 QString::fromLatin1(reference_latin1), QApplication::translate("OpenOrienteering::Util", "See more...") );
	}
	
	
	
	QString InputProperties<MapCoordF>::unit()
	{
		return QCoreApplication::translate("OpenOrienteering::UnitOfMeasurement", "mm", "millimeters");
	}
	
	QString InputProperties<RealMeters>::unit()
	{
		return QCoreApplication::translate("OpenOrienteering::UnitOfMeasurement", "m", "meters");
	}
	
	QString InputProperties<RotationalDegrees>::unit()
	{
		return QCoreApplication::translate("OpenOrienteering::UnitOfMeasurement", "\u00b0", "degrees");
	}
	
	
	
	QLabel* Headline::create(const QString& text)
	{
		return new QLabel(QLatin1String("<b>") + text + QLatin1String("</b>"));
	}
	
	QLabel* Headline::create(const char* text_utf8)
	{
		return create(QString::fromUtf8(text_utf8));
	}
	
	
	
	namespace Marker
	{
		void drawCenterMarker(QPainter* painter, const QPointF& center)
		{
			const auto saved_hints = painter->renderHints();
			painter->setRenderHint(QPainter::Antialiasing, Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool());
			
			const auto larger_radius = mmToPixelPhysical(1.1);
			const auto smaller_radius = larger_radius*3/4;
			
			auto pen = QPen(Qt::white);
			pen.setWidthF(larger_radius - smaller_radius);
			painter->setPen(pen);
			painter->setBrush(Qt::NoBrush);
			painter->drawEllipse(center, smaller_radius, smaller_radius);
			pen.setColor(Qt::black);
			painter->setPen(pen);
			painter->drawEllipse(center, larger_radius, larger_radius);
			
			painter->setRenderHints(saved_hints);
		}
	}  // namespace Marker



	QSpacerItem* SpacerItem::create(const QWidget* widget)
	{
		const int spacing = widget->style()->pixelMetric(QStyle::PM_LayoutTopMargin);
		return new QSpacerItem(spacing, spacing);
	}
	
	
	namespace SpinBox
	{
		
#ifndef NDEBUG
		namespace {
		
		/**
		 * Counts the number of digits for the given number and locale.
		 */
		template <class T>
		int countDigits(T value, const QLocale& locale)
		{
			return locale.toString(value).remove(locale.groupSeparator()).length();
		}
		
		/**
		 * Returns the maximum number of spinbox digits which is regarded as
		 * acceptable. Exceeding this number in Util::SpinBox::create() will
		 * print a warning at runtime.
		 */
		constexpr int max_digits()
		{
			return 13;
		}
		
		}  // namespace
#endif
		
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
			if (countDigits(min, box->locale()) > max_digits())
				qWarning("Util::SpinBox::create() will create "
				         "a very large widget because of min=%s",
				         QByteArray::number(min).constData());
			if (countDigits(max, box->locale()) > max_digits())
				qWarning("Util::SpinBox::create() will create "
				         "a very large widget because of max=%s",
				         QByteArray::number(max).constData());
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
			if (countDigits(min, box->locale()) > max_digits())
				qWarning("Util::SpinBox::create() will create "
				         "a very large widget because of min=%s",
				         QByteArray::number(min, 'f', decimals).constData());
			if (countDigits(max, box->locale()) > max_digits())
				qWarning("Util::SpinBox::create() will create "
				         "a very large widget because of max=%s",
				         QByteArray::number(max, 'f', decimals).constData());
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
			maybe_markup = QTextDocumentFragment::fromHtml(maybe_markup).toPlainText();
		}
		return maybe_markup;
	}


	namespace ToolButton
	{
		/**
		 * Returns a new QToolButton with a unified appearance.
		 */
		QToolButton* create(const QIcon& icon, const QString& text, const char* whats_this)
		{
			auto* button = new QToolButton();
			button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			button->setToolTip(text);
			button->setIcon(icon);
			button->setText(text);
			if (whats_this)
				button->setWhatsThis(OpenOrienteering::Util::makeWhatThis(whats_this));
			return button;
		}
	}  // namespace ToolButton


}  // namespace Util


}  // namespace OpenOrienteering
