#ifndef GARMINHTTPMESSAGE_H
#define GARMINHTTPMESSAGE_H

# pragma once
#include "communicator_v2.h"

#include <QByteArray>
#include <QUrl>

enum HttpResponseStatus : quint32  {
    UNKNOWN_STATUS=0,
    OK =100,
    NETWORK_REQUEST_TIMEOUT =200,
    FILE_TOO_LARGE = 300,
    DATA_TRANSFER_ITEM_FAILURE =  400,
};
enum HttpRequestType : quint32  {
    webRequest =1,
    webResponse =2,
    rawRequest = 5,
    rawResponse =  6,
};

enum HttpResponseType : quint32 {
  JSON = 0,
  URL_ENCODED = 1,
  PLAIN_TEXT = 2,
  XML = 3,
};

enum HttpVersion : quint32 {
  VERSION_1 = 0,
  VERSION_2 = 1,
};

enum HttpMethod :quint32 {
  UNKNOWN_METHOD = 0,
  GET = 1,
  PUT = 2,
  POST = 3,
  DELETE = 4,
  PATCH = 5,
  HEAD = 6,
};

struct HttpDataTransferItem {
  qint32 id;
  quint32 size;
};

struct HttpHeader {
  QString key;
  QString value;
};

struct WebRequest {
  QString url;
  quint32 method;
  QByteArray headers;
  QByteArray body;
  quint32 maxResponseLength;
  bool httpHeadersInResponse = true;
  bool compressResponseBody = false;
  quint32 responseType;
  quint32 version;
};

struct WebResponse {
  quint32 status;
  quint32 httpStatus;
  QByteArray body;
  QByteArray headers;
  quint32 size;
  quint32 responseType;
};

struct RawRequest {
   QUrl url;
   quint32 method;
   QList<HttpHeader> header;
   bool useDataXfer;
   QString rawBody;
 };

struct RawResponse {
   quint32 status ;
   quint32 httpStatus;
   QByteArray body;
   HttpDataTransferItem xferData;
   QList<HttpHeader > header;
 };


class GarminHttpMessage : public GarminGfdiMessage
{
    Q_OBJECT
public:
    GarminHttpMessage(CommunicatorV2 *parent) {
        mCommunicator = parent;
    };

    void parse(const QByteArray& data);
    WebRequest getWebRequest() {return mRequest;};
    HttpRequestType getType() {return mType;};

private:
    bool getRequestData(QByteArray request);
    bool getRawRequestData(QByteArray request);
    bool getResponseData(QByteArray request);
    bool getRawResponseData(QByteArray request);
    bool processWebRequest(QByteArray data);
    bool getXferData(QByteArray request);
    HttpHeader getHeaderData(QByteArray request);

    WebRequest mRequest;
    RawRequest mRawRequest;
    WebResponse mResponse;
    RawResponse mRawResponse;
    HttpRequestType mType;

signals:

};

#endif // GARMINHTTPMESSAGE_H
