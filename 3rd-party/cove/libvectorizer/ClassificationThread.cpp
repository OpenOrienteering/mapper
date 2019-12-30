/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ClassificationThread.h"

#include "libvectorizer/Vectorizer.h"

class QObject;

namespace cove {
//@{
/*! \class ClassificationThread
  \brief Subclass of QThread, used for running classification process. */
/*! \var Vectorizer* ClassificationThread::v
  Vectorizer on which to run performClassification. */
/*! \var ProgressObserver* ClassificationThread::p
  ProgressObserver, e.g. a UI dialog, to be passed to performClassification. */

/*! Constructor.
  \param[in] v Vectorizer on which to run performClassification.
  \param[in] p UIProgressDialog to be passed to performClassification. Can be 0.
  */
ClassificationThread::ClassificationThread(Vectorizer& v, ProgressObserver* const p, QObject* parent)
    : QThread(parent)
    , v(v)
    , p(p)
{
}

//! Reimplemented Thread method, calls Vectorizer::performClassification.
void ClassificationThread::run()
{
	v.performClassification(p);
}
} // cove

//@}
