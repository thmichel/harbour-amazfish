#include "garminhttpmessage.h"
#include "protobuftools.h"

#include <QDebug>



bool GarminHttpMessage::getRequestData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;

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
                    mRequest.url=fieldData;
                    break;
                case 2:
                    mRequest.method=HttpMethod(value);
                    break;
                case 3:
                    mRequest.headers=fieldData;
                    break;
                case 4:
                    mRequest.body=fieldData;
                    break;
                case 5:
                    mRequest.maxResponseLength=value;
                    break;
                case 6:
                    mRequest.httpHeadersInResponse= (value==0)? true:false;
                    break;
                case 7:
                    mRequest.compressResponseBody= (value==0)? true:false;
                    break;
                case 8:
                    mRequest.responseType=value;
                    break;
                case 9:
                    mRequest.version=value;
                    break;
                default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    return false;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    qDebug() << Q_FUNC_INFO << "HTTP request for " << mRequest.url;
    return true;
}


bool GarminHttpMessage::getResponseData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;

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
                    mResponse.status=value;
                    break;
                case 2:
                    mResponse.httpStatus=value;
                    break;
                case 3:
                    mResponse.body=fieldData;
                    break;
                case 4:
                    mResponse.headers=fieldData;
                    break;
                case 5:
                    mResponse.size=value;
                    break;
                case 6:
                    mResponse.responseType=value;
                    break;
                default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    return false;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    return true;
}

bool GarminHttpMessage::getRawRequestData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;

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
                    mRawRequest.url=fieldData;
                    break;
                case 3:
                    mRawRequest.method=HttpMethod(value);
                    break;
                case 5:
                    mRawRequest.header.append(getHeaderData(fieldData));
                    break;
                case 6:
                    mRawRequest.useDataXfer=value;
                    break;
                case 7:
                    mRawRequest.rawBody=fieldData;
                    break;
               default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    return false;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    qDebug() << Q_FUNC_INFO << "Raw HTTP request for " << mRawRequest.url     ;
    return true;
}

bool GarminHttpMessage::getRawResponseData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;

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
                    mRawResponse.status=value;
                    break;
                case 2:
                    mRawResponse.httpStatus=value;
                    break;
                case 3:
                    mRawResponse.body=fieldData;
                    break;
                case 4:
                    getXferData(fieldData);
                    break;
                case 5:
                    getHeaderData(fieldData);
                    break;
                default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    return false;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    return true;
}

bool GarminHttpMessage::getXferData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;

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
                    mRawResponse.xferData.id=value;
                    break;
                case 2:
                    mRawResponse.xferData.size=value;
                    break;
                  default:
                    qDebug() << Q_FUNC_INFO << "Unknown Garmin Http Message field:" << fieldNum;
                    return false;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parse Http Request: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    return true;
}

HttpHeader GarminHttpMessage::getHeaderData(QByteArray request) {
    // Parse request fields
    int cursor = 0;
    int loopCount = 0;
    HttpHeader h;

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
                    h.key=value;
                    break;
                case 2:
                    h.value=value;
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
    return h;
}

bool GarminHttpMessage::processWebRequest(QByteArray data) {
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
                case HttpRequestType::webRequest:
                    mType=HttpRequestType::webRequest;
                    return getRequestData(fieldData);
                   break;
                case HttpRequestType::webResponse:
                    mType=HttpRequestType::webResponse;
                    return getResponseData(fieldData);
                    break;
                case HttpRequestType::rawRequest:
                    mType=HttpRequestType::rawRequest;
                    return getRawRequestData(fieldData);
                   break;
                case HttpRequestType::rawResponse:
                    mType=HttpRequestType::rawResponse;
                    return getRawResponseData(fieldData);
                    break;                default:
                    qDebug() << Q_FUNC_INFO << "Unknown GarminHttpMessage field:" << fieldNum;
                    break;
                }
            }
            cursor = nextCursor;
            if (cursor == oldCursor) {
                qDebug() << Q_FUNC_INFO << "parseHttpRequest: cursor not advancing in request body";
                return false;
                break;
            }
        } else {
            return false;
            break;
        }
    }
    return false;
}

void GarminHttpMessage::parse(const QByteArray& data) {
    qDebug() << Q_FUNC_INFO << "Garmin: parsing http message";
    processWebRequest(data);

}

