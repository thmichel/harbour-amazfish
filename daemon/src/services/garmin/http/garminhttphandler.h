#ifndef GARMINHTTPHANDLER_H
#define GARMINHTTPHANDLER_H

#pragma once

#include "../garmingfdimessage.h"
#include "garminhttpresponse.h"
#include "garminhttprequest.h"
#include "../garminhttpmessage.h"
#include "../communicator_v2.h"

#include <QObject>
#include <QNetworkReply>

class GarminHttpMessage;
class GarminHttpRequest;
struct WebRequest;


class GarminHttpHandler : public QObject
{
public:
    explicit GarminHttpHandler(CommunicatorV2 *parent=0);
    void parse(const QByteArray& data, int msgId);
    void handle(GarminHttpMessage *msg, int msgId);
    QByteArray createSuccessResponse (GarminHttpRequest* req, GarminHttpResponse resp);
    QByteArray createErrorResponse(GarminHttpRequest* req, HttpResponseStatus status);
    QByteArray createWebResponse (GarminHttpRequest* req, GarminHttpResponse resp);
    QByteArray createRawResponse (GarminHttpRequest* req, GarminHttpResponse resp);
    void handleWebRequest(GarminHttpRequest* msg, int msgId);
    void handleWeatherRequest(GarminHttpRequest* msg, int msgId);
    void handleGenericRequest(GarminHttpRequest* msg, int msgId);
private:
    void sendProtobufMessage(QByteArray data, quint16 msgId);
    CommunicatorV2 *mCommunicator;
    QNetworkAccessManager *mNetworkManager;
};

#endif // GARMINHTTPHANDLER_H
