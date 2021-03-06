
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#include "qxtpipe_p.h"
#include <QList>
#include <QQueue>
#include <QMutableListIterator>

/*!
 * \class  QxtPipe
 * \inmodule QxtCore
 * \brief The QxtPipe class provides a pipeable QIODevice
 *
 * pipes can be connected to other pipes, to exchange data
 * The default implementation uses a buffer.
 * Reimplement to make your custom class able to be connected into a pipe chain.
 *
 * Example usage:
 * \code
 * QxtPipe p1;
 * QxtPipe p2;
 * QxtPipe p3;
 * p1|p2|p3;
 * p1.write("hi. how are you?");
 * qDebug()<<p3.readAll();
 * p3.write("I'm fine, thanks.");
 * qDebug()<<p1.readAll();
 * \endcode


    <h4>Subclassing</h4>
    When implementing your own pipe element, like a de/encoder or something, you have to reimplement receiveData() and call sendData() whenever you have something to send to the pipe network.

    If you want to the user to be able to read from the device directly via the QIODevice facility you have to call enqueuData() too

    If you don't want to the user to be able to write to the device directly via the QIODevice facility (that would be fatal for a decoder, for example),
    then reimplement the functions readData() and writeData() and return 0.


 \sa QxtDeplex
*/


/*!
 * Contructs a new QxtPipe with \a parent.
 */
QxtPipe::QxtPipe(QObject * parent): QIODevice(parent)
{
    QXT_INIT_PRIVATE(QxtPipe);
    setOpenMode(QIODevice::ReadWrite);

}


/*!\reimp*/
bool QxtPipe::isSequential() const
{
    return true;
}

/*!\reimp*/
qint64 QxtPipe::bytesAvailable() const
{
    return qxt_d().q.count();
}

/*!
   Pipes the output of this instance to the \a other  QxtPipe using the given \a mode and \a connectionType.
   Returns \c true if succeeds. Connection pipes with this function can be considered thread safe.

   Example usage:
   \code
   QxtPipe p1;
   QxtPipe p2;
   p1.connect(&p2,QIODevice::ReadOnly);

   //this data will go nowhere. p2 is connected to p1, but not p2 to p1.
   p1.write("hello");

   //while this data will end up in p1
   p2.write("world");

   qDebug()<<p1.readAll();

   \endcode
 */
bool  QxtPipe::connect(QxtPipe * other , QIODevice::OpenMode mode, Qt::ConnectionType connectionType)
{

    ///tell the other pipe to write into this
    if (mode & QIODevice::ReadOnly)
    {
        other->connect(this, QIODevice::WriteOnly, connectionType);
    }


    Connection c;
    c.pipe = other;
    c.mode = mode;
    c.connectionType = connectionType;
    qxt_d().connections.append(c);

    return true;
}

/*!
 * Cuts the connection to the \a other QxtPipe and returns \c true if succeeds.
 */
bool QxtPipe::disconnect(QxtPipe * other)
{
    bool e = false;

    QMutableListIterator<Connection> i(qxt_d().connections);
    while (i.hasNext())
    {
        i.next();
        if (i.value().pipe == other)
        {
            i.remove();
            e = true;
            other->disconnect(this);
        }
    }

    return e;
}

/*!
 * Convenience function for QxtPipe::connect().
 * Pipes the output of this instance to the \a target QxtPipe in readwrite mode with auto connection.
 */
QxtPipe & QxtPipe::operator | (QxtPipe & target)
{
    connect(&target);
    return *this;
}

/*!\reimp*/
qint64 QxtPipe::readData(char * data, qint64 maxSize)
{
    QQueue<char> * q = &qxt_d().q;

    qint64 i = 0;
    for (;i < maxSize;i++)
    {
        if (q->isEmpty())
            break;
        (*data++) = q->dequeue();
    }
    return i;
}

/*!\reimp*/
qint64 QxtPipe::writeData(const char * data, qint64 maxSize)
{
    sendData(QByteArray(data, maxSize));
    return maxSize;
}

/*!
Call this from your subclass to write \a data to the pipe network.
All write connected pipes will be invoked with receiveData
In this case this is called from receiveData, the sender will be excluded from the receiver list.
*/

void   QxtPipe::sendData(QByteArray data) const
{
    foreach(const Connection& c, qxt_d().connections)
    {


        //don't write back to sender
        if (c.pipe == qxt_d().lastsender)
            continue;



        if (!(c.mode & QIODevice::WriteOnly))
            continue;


        bool r = QMetaObject::invokeMethod(&c.pipe->qxt_d(), "push", c.connectionType,
                                           Q_ARG(QByteArray, data), Q_ARG(const QxtPipe *, this));

#ifdef QT_NO_DEBUG
        Q_UNUSED(r);
#else
        if (!r)
        {
            QObject::connect(this, SIGNAL(readyRead()), &c.pipe->qxt_d(), SLOT(push(QByteArray, const QxtPipe *)), c.connectionType);
            qFatal("metacall failed. see debug output of QObject::connect above");
        }
#endif

    }
    qxt_d().lastsender = 0;

}
/*!
Call this from your subclass to make \a datab available to the QIODevice::read facility
*/
void   QxtPipe::enqueData(QByteArray datab)
{
    QQueue<char> * q = &qxt_d().q;

    const char * data = datab.constData();
    qint64 maxSize = datab.size();

    qint64 i = 0;
    for (;i < maxSize;i++)
        q->enqueue(*data++);
    if (i > 0)
        emit(readyRead());
}

/*!
This function is called from any connected pipe to input \a datab from \a sender into this instance.
Reimplement this function to handle data from the pipe network.

The default implementation calls enqueData() and sendData().
*/
void QxtPipe::receiveData(QByteArray datab , const QxtPipe * sender)
{
    enqueData(datab);
    qxt_d().lastsender = sender;
    sendData(datab);
}

void QxtPipePrivate::push(QByteArray data, const QxtPipe * sender)
{
    (&qxt_p())->receiveData(data, sender);
}
