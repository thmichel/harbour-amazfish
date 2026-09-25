#ifndef GARMINHTTPMESSAGE_H
#define GARMINHTTPMESSAGE_H
#include "communicator_v2.h"

#include <QObject>
#include <QByteArray>

class GarminHttpMessage : public GarminGfdiMessage
{
    Q_OBJECT
public:
    GarminHttpMessage(CommunicatorV2 *parent) {
        mCommunicator = parent;
    };

    void parse(const QByteArray& data);
signals:

};

#endif // GARMINHTTPMESSAGE_H
