/*
 *    Copyright 2016 Kai Pastor
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

#include "printer_properties.h"

#include <QApplication>
#include <QDialog>
#include <private/qprintengine_win_p.h>
#include <qpa/qplatformnativeinterface.h>

namespace
{
	/** 
	 * A helper class which makes the QWin32PrintEnginePrivate member accessible.
	 */
	class AccessToPrintEnginePrivate : public QWin32PrintEngine
	{
	public:
		Q_DECLARE_PRIVATE(QWin32PrintEngine)
	};
	
}	


namespace PlatformPrinterProperties
{
	void save(const QPrinter* printer, std::shared_ptr<const void>& buffer)
	{
		Q_ASSERT(printer->outputFormat() == QPrinter::NativeFormat);
		auto engine = static_cast<QWin32PrintEngine *>(printer->printEngine());
		auto ep = static_cast<AccessToPrintEnginePrivate*>(engine)->d_func();
		if (ep->devMode && ep->hPrinter)
		{
			auto devmode_size = sizeof(DEVMODE) + ep->devMode->dmDriverExtra;
			auto devmode = static_cast<DEVMODE*>(malloc(devmode_size));
			if (devmode)
				memcpy(devmode, ep->devMode, devmode_size);
			
			buffer.reset(devmode);
		}
	}
	
	void restore(QPrinter* printer, const std::shared_ptr<const void> buffer)
	{
		Q_ASSERT(printer->outputFormat() == QPrinter::NativeFormat);
		
		auto engine = static_cast<QWin32PrintEngine *>(printer->printEngine());
		auto ep = static_cast<AccessToPrintEnginePrivate*>(engine)->d_func();
		if (ep->devMode && ep->hPrinter && buffer)
		{
			auto devmode = static_cast<const DEVMODE*>(buffer.get());
			if (wcsncmp(ep->devMode->dmDeviceName, devmode->dmDeviceName, CCHDEVICENAME) == 0
			    && ep->devMode->dmSpecVersion == devmode->dmSpecVersion
			    && ep->devMode->dmDriverVersion == devmode->dmDriverVersion
			    && ep->devMode->dmDriverExtra == devmode->dmDriverExtra)
			{
				auto devmode_size = sizeof(DEVMODE) + ep->devMode->dmDriverExtra;
				auto devmode_copy = LPDEVMODE(malloc(devmode_size));
				if (devmode_copy)
				{
					memcpy(devmode_copy, devmode, devmode_size);
					engine->setGlobalDevMode(nullptr, devmode_copy);
					ep->globalDevMode = nullptr;
					ep->ownsDevMode = true;
					GlobalUnlock(devmode_copy);
				}
			}
		}
	}
	
	bool dialogSupported()
	{
		return true;
	}
	
	int execDialog(QPrinter* printer, QWidget* parent)
	{
		if (!printer || !printer->outputFormat() == QPrinter::NativeFormat)
			return QDialog::Rejected;
		
		auto engine = static_cast<QWin32PrintEngine *>(printer->printEngine());
		auto ep = static_cast<AccessToPrintEnginePrivate*>(engine)->d_func();
		
		if (!ep->devMode || !ep->hPrinter)
			return QDialog::Rejected;
		
		HWND hwnd = nullptr;
		auto window = parent ? parent->window() : QApplication::activeWindow();
		if (auto window_handle = window->windowHandle())
			hwnd = (HWND)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window_handle);
		
		auto devmode_size = sizeof(DEVMODE) + ep->devMode->dmDriverExtra;
		auto devmode = LPDEVMODE(malloc(devmode_size));
		if (!devmode)
			return QDialog::Rejected;
		
		auto result = DocumentProperties(hwnd, ep->hPrinter, (LPWSTR)ep->m_printDevice.id().utf16(),
		                                 devmode, ep->devMode,
		                                 DM_IN_BUFFER | DM_IN_PROMPT | DM_OUT_BUFFER);
		if (result != IDOK)
			return QDialog::Rejected;
		
		engine->setGlobalDevMode(nullptr, devmode);
		ep->globalDevMode = nullptr;
		ep->ownsDevMode = true;
		GlobalUnlock(devmode);
		
		return QDialog::Accepted;
	}
}
