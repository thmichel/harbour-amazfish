#include "garminhttpmessage.h"
#include "protobuftools.h"

#include <QDebug>


enum HttpRequestType : quint32  {
    webRequest =1,
    webResponse =2,
    rawRequest = 5,
    rawResponst =  6,
};

enum ResponseType : quint32 {
  JSON = 0,
  URL_ENCODED = 1,
  PLAIN_TEXT = 2,
  XML = 3,
};

enum Version : quint32 {
  VERSION_1 = 0,
  VERSION_2 = 1,
};

enum Method :quint32 {
  UNKNOWN_METHOD = 0,
  GET = 1,
  PUT = 2,
  POST = 3,
  DELETE = 4,
  PATCH = 5,
  HEAD = 6,
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


void getRequestData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;
    WebRequest r;

    while (cursor < request.size()) {
        ++loopCount;
        if (loopCount > 100) {
            break;
        }

        const int oldCursor = cursor;
        quint32 fieldNum = 0;
        quint8 wireType = 0;
        QByteArray fieldData;
        int nextCursor = 0;

        if (parseField(request, cursor, fieldNum, wireType, fieldData, nextCursor)) {
            quint64 value;
            int valueLen;
            auto varintRes = decodeVarint(fieldData,value,valueLen);
            if (varintRes) {

                switch (fieldNum) {
                // URL
                case 1:
                    r.url=fieldData;
                    break;
                case 2:
                    r.method=value;
                    break;
                case 3:
                    r.headers=fieldData;
                    break;
                case 4:
                    r.body=fieldData;
                    break;
                case 5:
                    r.maxResponseLength=value;
                    break;
                case 6:
                    r.httpHeadersInResponse=value;
                    break;
                case 7:
                    r.compressResponseBody=value;
                    break;
                case 8:
                    r.responseType=value;
                    break;
                case 9:
                    r.version=value;
                    break;
                default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                break;
            }
        } else {
            break;
        }
    }
    qDebug() << Q_FUNC_INFO << "HTTP request for " << r.url     ;

}

void getWebRequest(QByteArray data) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;
    QString url;

    while (cursor < data.size()) {
        ++loopCount;
        if (loopCount > 100) {
            break;
        }

        const int oldCursor = cursor;
        quint32 fieldNum = 0;
        quint8 wireType = 0;
        QByteArray fieldData;
        int nextCursor = 0;

        if (parseField(data, cursor, fieldNum, wireType, fieldData, nextCursor)) {
            quint64 value;
            int valueLen;
            auto varintRes = decodeVarint(fieldData,value,valueLen);
            if (varintRes) {

                switch (fieldNum) {
                // HTTP Request
                case 1:
                    getRequestData(fieldData);
                    break;
                default:
                    qDebug() << Q_FUNC_INFO << "Unknown GarminHttpMessage field:" << fieldNum;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parseHttpRequest: cursor not advancing in request body";
                break;
            }
        } else {
            break;
        }
    }
}

void GarminHttpMessage::parse(const QByteArray& data) {
    qDebug() << Q_FUNC_INFO << "Garmin: parsing http message";
    getWebRequest(data);

}

