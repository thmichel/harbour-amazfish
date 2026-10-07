#ifndef GARMINHTTPREQUEST_H
#define GARMINHTTPREQUEST_H


#include <QUrl>
#include <QMap>
#include <QString>
#include <QByteArray>

#include "../garminhttpmessage.h"
#include "../garmintypes.h"


class GarminHttpRequest
{
public:
    GarminHttpRequest(RawRequest raw, quint16 msgId);
    GarminHttpRequest(WebRequest web, quint16 msgId);
    quint16 getMessageId();
    RawRequest getRawRequest();
    WebRequest getWebRequest();
    QString getUri();
    QString getDomain();
    QString getPath();
    QByteArray getBody();
    HttpMethod getMethod();
    QMap<QString,QString> getQuery();
    QMap<QString,QString> getHeaders();
    bool getIsRaw();


private:
    QMap<QString,QString> headersToMap(QList<HttpHeader> headers);
    RawRequest mRawRequest;
    WebRequest mWebRequest;

    quint16 mMessageId;

    HttpMethod mMethod;
    QUrl mUrl;
    QMap<QString, QString> mHeaders;
    bool mIsRaw=false;
};

#endif // GARMINHTTPREQUEST_H
