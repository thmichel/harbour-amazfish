#include "garminhttprequest.h"
#include "garminjson.h"
#include "QJsonObject"

GarminHttpRequest::GarminHttpRequest(RawRequest raw, quint16 msgId) :
    mRawRequest(raw), mMessageId(msgId)
{
    mMethod=raw.method;
    mUrl = QUrl(raw.url);
    mHeaders=headersToMap(raw.header);
    mIsRaw=true;
}

GarminHttpRequest::GarminHttpRequest(WebRequest web, quint16 msgId) :
    mWebRequest(web), mMessageId(msgId)
{
    mMethod=web.method;
    mUrl = QUrl(web.url);
    QJsonValue json = jsonDecode(web.headers);
    if (json.isObject()) {
        QJsonObject obj = json.toObject();

        for ( auto it = obj.begin();it != obj.end(); ++it) {
            if (it.value().isString()) {
                QByteArray value;
                value.append(it.value().toString().toUtf8());
                QByteArray key;
                key.append(it.key().toUtf8());
                mHeaders.insert(key,value);
            }
        }
    }
}

quint16  GarminHttpRequest::getMessageId() {
    return mMessageId;
}

RawRequest GarminHttpRequest::getRawRequest() {
    return mRawRequest;
}

WebRequest GarminHttpRequest::getWebRequest() {
    return mWebRequest;
}

QString GarminHttpRequest::getUri() {
    return mUrl.url();
}

QString GarminHttpRequest::getDomain() {
    return mUrl.host();
}

QString GarminHttpRequest::getPath() {
    return mUrl.path();
}

QByteArray GarminHttpRequest::getBody() {
    return mIsRaw? mRawRequest.rawBody.toUtf8(): mWebRequest.body;
}

HttpMethod GarminHttpRequest::getMethod() {
    return mMethod;
}

QMap<QString, QString> GarminHttpRequest::getQuery() {
    QMap<QString,QString> queryItems;
    if (!mUrl.hasQuery()) return QMap<QString,QString>();
    QUrlQuery query(mUrl);

    for (auto it : query.queryItems()) {
        queryItems.insert(it.first, it.second);
    }
    return queryItems;
}

QMap<QString, QString> GarminHttpRequest::getHeaders() {
    return mHeaders;
}

bool GarminHttpRequest::getIsRaw() {
    return mIsRaw;
}
QMap<QString, QString> GarminHttpRequest::headersToMap(QList<HttpHeader> headers) {
    QMap<QString, QString> ret;
    for (auto header : headers) {
        ret.insert(header.key.toLower(), header.value);
    }
    return ret;
}

