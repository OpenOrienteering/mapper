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
#include <QMetaObject>
#include <QString>

#include "settings.h"


namespace {

/// Converts from autosave interval unit (minutes) to QTest::qWait unit (milliseconds)
int msecs(double minutes)
{
	return qRound(minutes * 60000);
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
	// Use distinct QSettings
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1(metaObject()->className()));
	QVERIFY2(QDir::home().exists(), "The home dir must be writable in order to use QSettings.");
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
	QTest::qWait(msecs(0.8 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger within 2 seconds after timeout
	QTest::qWait(msecs(0.2 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 1, 2000);
	
	// Verify that Autosave does not trigger again too early
	QTest::qWait(msecs(0.8 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger once again within 2 seconds
	QTest::qWait(msecs(0.2 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 2, 2000);
}

void AutosaveTest::temporaryFailureTest()
{
	AutosaveTestDocument doc(Autosave::TemporaryFailure);
	
	// Enable and trigger Autosave
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QTest::qWait(msecs(0.8 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger within 2 seconds after timeout
	QTest::qWait(msecs(0.2 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 1, 2000);
	
	// Verify that Autosave does not trigger once more too early
	QTest::qWait(4000);
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger once more quickly
	QTest::qWait(1000);
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 2, 2000);
	
	// NOTE: The following check used to fail frequently on macOS with our
	//       standard timing of 0.8 * autosave_interval. Given our extremely
	//       short autosave_interval in this test, this is probably a timer
	//       precision issue, not a bug in the Autosave unit.
	//       The less strict timing of 0.7 * autosave_interval seems to reduce
	//       the rate of failing runs to an acceptable level. The extra time
	//       is moved to the subsequent step of waiting.
	
	// Verify that Autosave does not trigger again too early
	QTest::qWait(msecs(0.7 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 2);
	
	// Verify that Autosave does trigger again once before too late
	QTest::qWait(msecs(0.3 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 3, 2000);
}

void AutosaveTest::permanentFailureTest()
{
	AutosaveTestDocument doc(Autosave::PermanentFailure);
	
	// Enable and trigger Autosave
	doc.setAutosaveNeeded(true);
	
	// Verify that Autosave does not trigger too early
	QTest::qWait(msecs(0.8 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 0);
	
	// Verify that Autosave does trigger exactly once before too late
	QTest::qWait(msecs(0.2 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 1, 2000);

	// Verify that Autosave does not trigger again too early
	QTest::qWait(msecs(0.8 * autosave_interval));
	QCOMPARE(doc.autosaveCount(), 1);
	
	// Verify that Autosave does trigger again once before too late
	QTest::qWait(msecs(0.2 * autosave_interval));
	QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 2, 2000);
}

void AutosaveTest::autosaveStopTest()
{
	{
		AutosaveTestDocument doc(Autosave::Success);
		
		// Enable and trigger Autosave
		doc.setAutosaveNeeded(true);
		
		// Sleep some time
		QTest::qWait(msecs(0.3 * autosave_interval));
		QCOMPARE(doc.autosaveCount(), 0);
		
		// Verify that Autosave does not trigger after setAutosaveNeeded(false)
		doc.setAutosaveNeeded(false);
		QTest::qWait(msecs(autosave_interval) + 2000);
		QCOMPARE(doc.autosaveCount(), 0);
	}
	
	{
		AutosaveTestDocument doc(Autosave::TemporaryFailure);
		
		// Test stopping autosave in temporary failure mode
		doc.setAutosaveNeeded(true);
		QTest::qWait(msecs(autosave_interval));
		QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 1, 2000);

		doc.setAutosaveNeeded(false);
		QTest::qWait(msecs(autosave_interval) + 7000);
		QCOMPARE(doc.autosaveCount(), 1);
	}
	
	{
		AutosaveTestDocument doc(Autosave::PermanentFailure);
		
		// Test stopping autosave in permanent failure mode
		doc.setAutosaveNeeded(true);
		QTest::qWait(msecs(autosave_interval));
		QTRY_COMPARE_WITH_TIMEOUT((doc.autosaveCount()), 1, 2000);

		doc.setAutosaveNeeded(false);
		QTest::qWait(msecs(autosave_interval) + 2000);
		QCOMPARE(doc.autosaveCount(), 1);
	}
}

/*
 * We don't need a real GUI window.
 */
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(AutosaveTest)
