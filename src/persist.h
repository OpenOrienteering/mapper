/*
 *    Copyright 2012 Thomas Schöps
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

#ifndef _OPENORIENTEERING_PERSIST_H
#define _OPENORIENTEERING_PERSIST_H

// Forward declaration
class Persistor;

/** Exception type for persistence errors.
 */
class PersistenceException : public std::exception {
public:
    PersistenceException(const char *message) : std::exception(), m(message)
    {
    }

    const char *what() const throw()
    {
        return m;
    }
private:
    const char *m;
};




/** Virtual base class for all objects that can be loaded and saved to a Persistor.
 */
class Persistable {
public:
    /** Abstract function that attempts to load an object of this type from the
     *  current state of the persistor. Returns successfully if the object was
     *  loaded, and throws a PersistenceException if the object could not be
     *  loaded. The parent object can catch this exception and decide whether or
     *  not to abort the load process, or try to continue loading the rest of the
     *  object graph.
     */
    virtual void load(Persistor &persistor) = 0;

    /** Abstract function that attempts to save this object to the persistor.
     *  Returns successfully if the object was saved, and throws a PersistenceException
     *  if the object could not be saved. As in the load() method, the parent can
     *  decide how it wants to handle such exceptions.
     */
    virtual void save(Persistor &persistor) const = 0;
};


/** Virtual base class for all persistence schemes. Instances are stateful and
 *  know how to persist various primitive and Qt types. Composite objects that
 *  implement Persistable are also handled, and can define their own schemes
 *  for persistence.
 *
 *  All methods should either return successfully, or throw a PersistenceException.
 */
class Persistor {
public:
    virtual void readBytes(char *bytes, size_t count) = 0;
    virtual QByteArray readBytes(size_t count) = 0;
    virtual bool readBool() = 0;
    virtual int readInt() = 0;
    virtual float readFloat() = 0;
    virtual QString readString() = 0;
    virtual QPointF readPoint() = 0;

    virtual void writeBytes(const char *bytes, size_t count) = 0;
    virtual void writeBytes(const QByteArray &bytes) = 0;
    virtual void writeBool(bool value) = 0;
    virtual void writeInt(int value) = 0;
    virtual void writeFloat(float value) = 0;
    virtual void writeString(const QString &value) = 0;
    virtual void writePoint(const QPointF &value) = 0;

    void readObject(Persistable &object);
    void writeObject(const Persistable &object);
};


/** Binary persistor - uses the existing omap scheme.
 */


/** String persistor - converts everything to QStrings
 *
 */

/** XML persistor - makes a pretty human-readable file. Paths, however, are compressed
 *  into single strings, the same way as SVG.
 */

class XMLPersistor {
    void readBytes(char *bytes, size_t count);
    QByteArray readBytes(size_t count);
    bool readBool();
    int readInt();
    float readFloat();
    QString readString();
    QPointF readPoint();

    void writeBytes(const char *bytes, size_t count);
    void writeBytes(const QByteArray &bytes);
    void writeBool(bool value);
    void writeInt(int value);
    void writeFloat(float value);
    void writeString(const QString &value);
    void writePoint(const QPointF &value);

    // Traversal
    XMLPersistor &child(const QString &name);
    XMLPersistor &attr(const QString &name);
    XMLPersistor &pop();
};


#endif // PERSIST_H
