/*
 *    Copyright 2014 Kai Pastor
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

#include "../src/settings.h"


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
: QObject(parent),
  autosave_interval(0.02) // 0.02 min = 1.2 s
{
	// Set distinct application name in order to use distinct settings.
	// Autosave objects must not be constructed before this!
	QCoreApplication::setApplicationName(QString::fromLatin1("Tests"));
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
	Settings::getInstance().setSetting(Settings::General_AutosaveInterval, autosave_interval);
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
}

void AutosaveTest::temporaryFailureTest()
{
	AutosaveTestDocument doc(Autosave::TemporaryFailure);
	
	// Enable and trigger Autosave
	Settings::getInstance().setSetting(Settings::General_AutosaveInterval, autosave_interval);
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger once more quickly
	QThread::msleep(10000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 3);
}

void AutosaveTest::permanentFailureTest()
{
	AutosaveTestDocument doc(Autosave::PermanentFailure);
	
	// Enable and trigger Autosave
	Settings::getInstance().setSetting(Settings::General_AutosaveInterval, autosave_interval);
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does not trigger again too early
	QThread::msleep(autosave_interval * 50000); 
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger again once before too late
	QThread::msleep(autosave_interval * 20000);
	QCoreApplication::processEvents();
	QCOMPARE(doc.autosaveCount(), 2);
}

void AutosaveTest::autosaveStopTest()
{
	{
		AutosaveTestDocument doc(Autosave::Success);
		
		// Enable and trigger Autosave
		Settings::getInstance().setSetting(Settings::General_AutosaveInterval, autosave_interval);
		doc.setAutosaveNeeded(true);
		
		// Sleep some time
		QThread::msleep(autosave_interval * 20000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 0);
		
		// Verify that Autosave does not trigger after setAutosaveNeeded(false)
		doc.setAutosaveNeeded(false);
		QThread::msleep(autosave_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 0);
	}
	
	{
		AutosaveTestDocument doc(Autosave::TemporaryFailure);
		
		// Test stopping autosave in temporary failure mode
		doc.setAutosaveNeeded(true);
		QThread::msleep(autosave_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
		
		doc.setAutosaveNeeded(false);
		QThread::msleep(autosave_interval * 20000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
	}
	
	{
		AutosaveTestDocument doc(Autosave::PermanentFailure);
		
		// Test stopping autosave in permament failure mode
		doc.setAutosaveNeeded(true);
		QThread::msleep(autosave_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
		
		doc.setAutosaveNeeded(false);
		QThread::msleep(autosave_interval * 70000); 
		QCoreApplication::processEvents();
		QCOMPARE(doc.autosaveCount(), 1);
	}
}

/*
 * We select a non-standard QPA because we don't need a real GUI window.
 * 
 * Normally, the "offscreen" plugin would be the correct one.
 * However, it bails out with a QFontDatabase error (cf. QTBUG-33674)
 */
auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");


QTEST_MAIN(AutosaveTest)
