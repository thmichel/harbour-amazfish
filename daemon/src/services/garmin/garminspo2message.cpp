#include "garminspo2message.h"

#include <QDateTime>
#include <QDebug>


GarminSpo2Message::GarminSpo2Message(CommunicatorV2* parent) : mCommunicator(parent)
{

}
void GarminSpo2Message::onConnect() {}
void GarminSpo2Message::onClose() {}
void GarminSpo2Message::onMessage(const QByteArray& data) {
    quint8 spo2 = data[0];
    mCommunicator->setSpo2(spo2);
}
