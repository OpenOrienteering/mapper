/**
 * This file is part of OpenOrienteering.
 *
 * This is a modified version of tst_qglobal.cpp from the Qt Toolkit 5.7.
 * You can redistribute it and/or modify it under the terms of
 * the GNU General Public License, version 3, as published by
 * the Free Software Foundation.
 *
 * OpenOrienteering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
 *
 * Changes:
 * 2016-03-25 Kai Pastor <dg0yt@darc.de>
 * - Adjustment of legal information
 * - Reduction to qOverload related tests
 * - Adaption to our testing framework
 */
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
****************************************************************************/


#include "tst_qglobal.h"

#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QString>

#include "util/backports.h"  // IWYU pragma: keep


// clazy:excludeall=function-args-by-ref
struct Overloaded
{
    void foo() {}
    void foo(QByteArray) {}
    void foo(QByteArray, const QString &) {}

    void constFoo() const {}
    void constFoo(QByteArray) const {}
    void constFoo(QByteArray, const QString &) const {}

    void mixedFoo() {}
    void mixedFoo(QByteArray) const {}
};

void freeOverloaded() {}
void freeOverloaded(QByteArray) {}
void freeOverloaded(QByteArray, const QString &) {}

void freeOverloadedGet(QByteArray) {}
QByteArray freeOverloadedGet() { return QByteArray(); }


void tst_QGlobal::testqOverload()
{
#ifdef Q_COMPILER_VARIADIC_TEMPLATES

    // void returning free overloaded functions
    QVERIFY(QOverload<>::of(&freeOverloaded) ==
             static_cast<void (*)()>(&freeOverloaded));

    QVERIFY(QOverload<QByteArray>::of(&freeOverloaded) ==
             static_cast<void (*)(QByteArray)>(&freeOverloaded));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&freeOverloaded)) ==
             static_cast<void (*)(QByteArray, const QString &)>(&freeOverloaded));

    // value returning free overloaded functions
    QVERIFY(QOverload<>::of(&freeOverloadedGet) ==
             static_cast<QByteArray (*)()>(&freeOverloadedGet));

    QVERIFY(QOverload<QByteArray>::of(&freeOverloadedGet) ==
             static_cast<void (*)(QByteArray)>(&freeOverloadedGet));

    // void returning overloaded member functions
    QVERIFY(QOverload<>::of(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::foo));

    QVERIFY(QOverload<QByteArray>::of(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)(QByteArray)>(&Overloaded::foo));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&Overloaded::foo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &)>(&Overloaded::foo));

    // void returning overloaded const member functions
    QVERIFY(QOverload<>::of(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)() const>(&Overloaded::constFoo));

    QVERIFY(QOverload<QByteArray>::of(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::constFoo));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&Overloaded::constFoo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &) const>(&Overloaded::constFoo));

    // void returning overloaded const AND non-const member functions
    QVERIFY(QNonConstOverload<>::of(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::mixedFoo));

    QVERIFY(QConstOverload<QByteArray>::of(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::mixedFoo));

#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304 // C++14

    // void returning free overloaded functions
    QVERIFY(qOverload<>(&freeOverloaded) ==
             static_cast<void (*)()>(&freeOverloaded));

    QVERIFY(qOverload<QByteArray>(&freeOverloaded) ==
             static_cast<void (*)(QByteArray)>(&freeOverloaded));

    QVERIFY((qOverload<QByteArray, const QString &>(&freeOverloaded) ==
             static_cast<void (*)(QByteArray, const QString &)>(&freeOverloaded)));

    // value returning free overloaded functions
    QVERIFY(qOverload<>(&freeOverloadedGet) ==
             static_cast<QByteArray (*)()>(&freeOverloadedGet));

    QVERIFY(qOverload<QByteArray>(&freeOverloadedGet) ==
             static_cast<void (*)(QByteArray)>(&freeOverloadedGet));

    // void returning overloaded member functions
    QVERIFY(qOverload<>(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::foo));

    QVERIFY(qOverload<QByteArray>(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)(QByteArray)>(&Overloaded::foo));

    QVERIFY((qOverload<QByteArray, const QString &>(&Overloaded::foo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &)>(&Overloaded::foo));

    // void returning overloaded const member functions
    QVERIFY(qOverload<>(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)() const>(&Overloaded::constFoo));

    QVERIFY(qOverload<QByteArray>(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::constFoo));

    QVERIFY((qOverload<QByteArray, const QString &>(&Overloaded::constFoo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &) const>(&Overloaded::constFoo));

    // void returning overloaded const AND non-const member functions
    QVERIFY(qNonConstOverload<>(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::mixedFoo));

    QVERIFY(qConstOverload<QByteArray>(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::mixedFoo));
#endif

#endif
}


QTEST_APPLESS_MAIN(tst_QGlobal)
