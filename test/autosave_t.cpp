/*
 *    Copyright 2014-2019 Kai Pastor
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

#include "autosave_t.h"

#include <QtGlobal>
#include <QtTest>
#include <QCoreApplication>
#include <QString>
#include <QThread>

#include "settings.h"


namespace {

/// Converts from autosave interval unit (minutes) to QThread::msleep unit (milliseconds)
unsigned long msecs(double minutes)
{
	return static_cast<unsigned long>(minutes * 60000);
}

}  // namespace



//### AutosaveTestDocument ###

AutosaveTestDocument::AutosaveTestDocument(Autosave::AutosaveResult expected_result)
: expected_result(expected_result),
  autosave_count(0)
{
	// nothing
}

Autosave::AutosaveResult AutosaveTestDocument::autosave()
{
	++autosave_count;
	
	Autosave::AutosaveResult result = expected_result;
	if (result == Autosave::TemporaryFailure)
		expected_result = Autosave::Success; // success on next call
	
	return result;
}

int AutosaveTestDocument::autosaveCount() const
{
	return autosave_count;
}


//### AutosaveTest ###

AutosaveTest::AutosaveTest(QObject* parent)
: QObject(parent)
, autosave_interval(0.02) // 0.02 min = 1.2 s
{
	// The default settings format for Windows (registry) needs an organization.
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	// Set distinct application name in order to use distinct settings.
	// Autosave objects must not be constructed before this!
	QCoreApplication::setApplicationName(QString::fromLatin1("Tests"));
}

void AutosaveTest::initTestCase()
{
	Settings::getInstance().setSetting(Settings::General_AutosaveInterval, autosave_interval);
	QCOMPARE(QSettings().status(), QSettings::NoError);
	QCOMPARE(Settings::getInstance().getSetting(Settings::General_AutosaveInterval).toDouble(), autosave_interval);
}

void AutosaveTest::autosaveTestDocumentTest()
{
	{
		AutosaveTestDocument doc(Autosave::Success);
		QCOMPARE(doc.autosaveCount(), 0);
		QCOMPARE(doc.autosave(), Autosave::Success);
		QCOMPARE(doc.autosaveCount(), 1);
	}
	
	{
		AutosaveTestDocument doc(Autosave::TemporaryFailure);
		QCOMPARE(doc.autosave(), Autosave::TemporaryFailure);
		QCOMPARE(doc.autosave(), Autosave::Success);
		QCOMPARE(doc.autosave(), Autosave::Success);
		QCOMPARE(doc.autosaveCount(), 3);
	}
	
	{
		AutosaveTestDocument doc(Autosave::PermanentFailure);
		QCOMPARE(doc.autosave(), Autosave::PermanentFailure);
		QCOMPARE(doc.autosave(), Autosave::PermanentFailure);
		QCOMPARE(doc.autosaveCount(), 2);
	}
}

void AutosaveTest::successTest()
{
	AutosaveTestDocument doc(Autosave::Success);
	
	// Enable and trigger Autosave
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
}

void AutosaveTest::temporaryFailureTest()
{
	AutosaveTestDocument doc(Autosave::TemporaryFailure);
	
	// Enable and trigger Autosave
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does not trigger once more too early
	QThread::msleep(4000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger once more quickly
	QThread::msleep(2000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 3);
}

void AutosaveTest::permanentFailureTest()
{
	AutosaveTestDocument doc(Autosave::PermanentFailure);
	
	// Enable and trigger Autosave
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(msecs(0.9 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(msecs(0.2 * autosave_interval));
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
}

void AutosaveTest::autosaveStopTest()
{
	{
		AutosaveTestDocument doc(Autosave::Success);
		
		// Enable and trigger Autosave
		doc.setAutosaveNeeded(true);
		
		// Sleep some time
		QThread::msleep(msecs(0.3 * autosave_interval));
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 0);
		
		// Verify that Autosave does not trigger after setAutosaveNeeded(false)
		doc.setAutosaveNeeded(false);
		QThread::msleep(msecs(1.1 * autosave_interval));
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 0);
	}
	
	{
		AutosaveTestDocument doc(Autosave::TemporaryFailure);
		
		// Test stopping autosave in temporary failure mode
		doc.setAutosaveNeeded(true);
		QThread::msleep(msecs(1.1 * autosave_interval));
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
		
		doc.setAutosaveNeeded(false);
		QThread::msleep(msecs(autosave_interval) + 6000);
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
	}
	
	{
		AutosaveTestDocument doc(Autosave::PermanentFailure);
		
		// Test stopping autosave in permament failure mode
		doc.setAutosaveNeeded(true);
		QThread::msleep(msecs(1.1 * autosave_interval));
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
		
		doc.setAutosaveNeeded(false);
		QThread::msleep(msecs(1.1 * autosave_interval));
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
	}
}

/*
 * We don't need a real GUI window.
 */
namespace {
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(AutosaveTest)
