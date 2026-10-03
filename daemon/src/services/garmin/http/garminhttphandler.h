#ifndef GARMINHTTPHANDLER_H
#define GARMINHTTPHANDLER_H

#pragma once

#include "../garmingfdimessage.h"
#include "garminhttpresponse.h"
#include "../garminhttpmessage.h"
#include "../communicator_v2.h"

#include <QObject>

class GarminHttpMessage;
struct WebRequest;


class GarminHttpHandler : public QObject
{

public:
    explicit GarminHttpHandler(CommunicatorV2 *parent=0);
    void parse(const QByteArray& data, int msgId);
    void handle(GarminHttpMessage *msg, int msgId);
    QByteArray createWebResponse (GarminHttpMessage *req, GarminHttpResponse resp, int msgId);
    void handleWebRequest(GarminHttpMessage* msg, int msgId);
    void handleWeatherRequest(GarminHttpMessage* msg, int msgId);
    void handleGenericRequest(GarminHttpMessage* msg, int msgId);

signals:
private:
    CommunicatorV2 *mCommunicator;

};

#endif // GARMINHTTPHANDLER_H
