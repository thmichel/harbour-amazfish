#ifndef GARMINHTTPMESSAGE_H
#define GARMINHTTPMESSAGE_H


#include "communicator_v2.h"
#include "garmintypes.h"

#include <QByteArray>
#include <QUrl>

class GarminHttpMessage : public GarminGfdiMessage
{
    Q_OBJECT
public:
    GarminHttpMessage(CommunicatorV2 *parent) {
        mCommunicator = parent;
    };

    void parse(const QByteArray& data);
    WebRequest getWebRequest() {return mRequest;};
    RawRequest getRawRequest() {return mRawRequest;};
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
