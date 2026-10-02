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
    std::optional<GarminHttpResponse>  handleWebRequest(GarminHttpMessage *msg, int msgId);
    std::optional<GarminHttpResponse> handleWeatherRequest(WebRequest msg, int msgId);
    std::optional<GarminHttpResponse> handleGenericRequest(WebRequest msg, int msgId);

signals:
private:
    CommunicatorV2 *mCommunicator;

};

#endif // GARMINHTTPHANDLER_H
