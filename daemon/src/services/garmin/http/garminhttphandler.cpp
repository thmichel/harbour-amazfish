#include "garminhttphandler.h"
#include "lib/src/weather/currentweather.h"

#include "../protobuftools.h"
#include "garminjson.h"

#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

// Helper function
QByteArray zlibCompress(const QByteArray &data, int level = Z_DEFAULT_COMPRESSION)
{
    if (data.isEmpty()) return QByteArray();

    // Estimate the compressed size (zlib utility function)
    uLongf compressedSize = compressBound(data.size());
    QByteArray compressedData(compressedSize, 0); // Pre-allocate buffer

    int result = compress2(
        (Bytef*)compressedData.data(),
        &compressedSize,
        (const Bytef*)data.constData(),
        data.size(),
        level
    );

    if (result == Z_OK) {
        compressedData.resize(compressedSize); // Trim to actual size
        return compressedData;
    } else {
        qWarning() << "Zlib compression failed with error:" << result;
        return QByteArray();
    }
}

GarminHttpHandler::GarminHttpHandler(CommunicatorV2* parent):mCommunicator(parent)
{
    mNetworkManager = new QNetworkAccessManager();
}

void GarminHttpHandler::parse(const QByteArray& data, int msgId) {
    GarminHttpMessage *msg = new GarminHttpMessage(mCommunicator);
    msg->parse(data);
    handle(msg,msgId);
}

void GarminHttpHandler::handle(GarminHttpMessage *msg, int msgId) {
    qDebug() << Q_FUNC_INFO;
    GarminHttpRequest *request;
    switch (msg->getType())
    {
        case HttpRequestType::webRequest:
            request = new GarminHttpRequest(msg->getWebRequest(),msgId);
            handleWebRequest(request,msgId);
            break;
        case HttpRequestType::webResponse:
            qDebug() << Q_FUNC_INFO << "Garmin: Handling webRsponse not yet implemented";
            break;
        case HttpRequestType::rawRequest:
            request = new GarminHttpRequest(msg->getRawRequest(),msgId);
            handleWebRequest(request,msgId);
            break;
        case HttpRequestType::rawResponse:
            qDebug() << Q_FUNC_INFO << "Garmin: Handling rawResponse not yet implemented";
            break;
    }
}

void  GarminHttpHandler::handleWebRequest(GarminHttpRequest* msg, int msgId) {
    qDebug() << Q_FUNC_INFO;
    QUrl url(msg->getUri());
    if ((url.host()=="api.gcs.garmin.com") && (url.path().startsWith("/weather/v"))) {
        qDebug() << Q_FUNC_INFO<< "Found Weather request - not yet implemented";
        //handleWeatherRequest(msg,msgId);
        return;
    }
    // handle all other requests
    handleGenericRequest(msg, msgId);
}

void GarminHttpHandler::handleWeatherRequest(GarminHttpRequest* msg, int msgId) {
    Q_UNUSED(msg);
    Q_UNUSED(msgId);
    qDebug() << Q_FUNC_INFO;
 }

QByteArray GarminHttpHandler::createSuccessResponse(GarminHttpRequest *req, GarminHttpResponse resp) {
    qDebug() << Q_FUNC_INFO;
    if (req->getIsRaw())
        return createRawResponse(req,resp);
    return createWebResponse(req, resp);
}

QByteArray GarminHttpHandler::createErrorResponse(GarminHttpRequest* req, HttpResponseStatus status) {
    GarminHttpResponse resp;
    resp.setStatus(status);
    if (req->getIsRaw())
        return createRawResponse(req,resp);
    return createWebResponse(req,resp);

}

QByteArray GarminHttpHandler::createWebResponse(GarminHttpRequest *req, GarminHttpResponse resp) {
    qDebug() << Q_FUNC_INFO;
    WebRequest request = req->getWebRequest();
    if (resp.getBody().size() > int(req->getWebRequest().maxResponseLength)) {
        qDebug() << Q_FUNC_INFO << "Garmin: HTTP Response too large.";
        //TODO: send back response too large
        return QByteArray();
    }
    if (resp.getHeaders().value("content-type").contains("application/json", Qt::CaseInsensitive)) {
        QString tmpBody = resp.getBody();
        QByteArray jsonData = tmpBody.toUtf8();
        QByteArray body;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isObject() && !doc.isArray()) {
            qDebug() << Q_FUNC_INFO << "Response contains no json object";
            return QByteArray();
        }
        else {
            if (doc.isObject()) {
                QJsonObject jsonObject = doc.object();
                body = jsonEncode(jsonObject);
            } else if (doc.isArray()) {
                QJsonArray jsonArray = doc.array();
                body = jsonEncode(jsonArray);
            }
        }
        // Now build the Response Prototbuf string
        QByteArray proto;
        encodeFieldKey(proto,1,0);
        encodeVarint(proto,quint64(HttpResponseStatus::OK));
        encodeFieldKey(proto,2,0);
        encodeVarint(proto,quint64(resp.getStatus()));
        encodeFieldKey(proto,3,2);
        encodeVarint(proto,quint64(body.size()));
        proto.append(body);
        if (request.httpHeadersInResponse) {
            QMap<QString, QString> h = resp.getHeaders();
            if (!h.isEmpty()) {
                QJsonObject json;
                encodeFieldKey(proto,4,2);
                for (auto it=h.begin();it != h.end();++it)
                {
                   json[QString(it.key().toUtf8())] = QString(it.value().toUtf8());
                }
                QByteArray encodedHeaders=jsonEncode(json);
                encodeVarint(proto,quint64(encodedHeaders.size()));
                proto.append(encodedHeaders);
            }
        }
        encodeFieldKey(proto,5,0);
        encodeVarint(proto,quint64(0)); // non-compressed
        encodeFieldKey(proto,6,0);
        encodeVarint(proto,quint64(HttpResponseType::JSON));
        return proto;
    } else qDebug() << Q_FUNC_INFO << "Garmin: No JSON content.";
    //Non-json not handled
    return QByteArray();
}

QByteArray GarminHttpHandler::createRawResponse(GarminHttpRequest *req, GarminHttpResponse resp) {
    QList<QByteArray> responseHeaders;
    RawRequest request = req->getRawRequest();
    for (auto i=resp.getHeaders().begin();i!=resp.getHeaders().end();++i) {
        // create Protobuf for headers
        QByteArray h;
        encodeFieldKey(h,1,2);
        encodeVarint(h,quint64(i.key().size()));
        h.append(i.key());
        encodeFieldKey(h,2,2);
        encodeVarint(h,quint64(i.value().size()));
        h.append(i.value());
        responseHeaders.append(h);
    }
    if (request.useDataXfer) {
        // Date will be returned using data_xfer
        // not implemented yet!
        qDebug() << Q_FUNC_INFO << "Garmin: Http Data Xfer not yet implemented for request " << req->getUri();
        //return QByteArray();
    }

    QByteArray responseBody;
    if (req->getHeaders()["accept-encoding"]=="gzip")
    {
        qDebug() << Q_FUNC_INFO << "Garmin: compressing HTTP Response";
        QByteArray h;
        QByteArray key = QString("Content-Encoding").toUtf8();
        QByteArray value = QString("gzip").toUtf8();

        encodeFieldKey(h,1,2);
        encodeVarint(h,quint64(key.size()));
        h.append(key);
        encodeFieldKey(h,2,2);
        encodeVarint(h,quint64(value.size()));
        h.append(value);
        responseHeaders.append(h);
        QByteArray uncompressed = resp.getBody().toUtf8();
        responseBody = zlibCompress(uncompressed);
    }
    else responseBody = resp.getBody().toUtf8();

    // Now build Protobuf
    QByteArray proto;
    encodeFieldKey(proto,1,0);
    encodeVarint(proto,quint64(HttpResponseStatus::OK));
    encodeFieldKey(proto,2,0);
    encodeVarint(proto,resp.getStatus());
    encodeFieldKey(proto,3,2);
    encodeVarint(proto,responseBody.size());
    proto.append(responseBody);
    encodeFieldKey(proto,4,0);
    encodeVarint(proto,request.useDataXfer);
    QString it;
    while (!responseHeaders.isEmpty()){
        it = responseHeaders.takeFirst();
        encodeFieldKey(proto,5,2);
        encodeVarint(proto,quint64(it.size()));
        proto.append(it);
    }

    return proto;
}

void GarminHttpHandler::handleGenericRequest(GarminHttpRequest*  msg, int msgId) {
    // handle non-raw Web Request
    qDebug() << Q_FUNC_INFO;
    QUrl dest(msg->getUri());

    if ( dest.host().endsWith("garmin.com") || dest.host().endsWith("dciwx.com")) {
        // For now, we explicitly block all requests to Garmin domains, even if the user whitelists them.
        // Due to fake OAuth, most of these will include invalid authentication credentials, and needs
        // further investigation
        qDebug() << Q_FUNC_INFO <<"Garmin:Blocking request to Garmin url: " <<dest.host();
        QByteArray responseData = createErrorResponse(msg,HttpResponseStatus::UNKNOWN_STATUS);
        sendProtobufMessage(responseData,msgId);
        return;
    }
    QNetworkRequest htmlRequest;
    QUrl url(msg->getUri());
    htmlRequest.setUrl(url);
    QMap<QString,QString> headers = msg->getHeaders();
    for ( auto it=headers.begin();it!=headers.end();++it) {
        htmlRequest.setRawHeader(it.key().toUtf8(),it.value().toUtf8());
    }
    switch (msg->getMethod()) {
        case HttpMethod::PUT:
            qDebug() << Q_FUNC_INFO << "Garmin: HTTP Put not yet implemented";
        break;
        case HttpMethod::GET:
        case HttpMethod::POST:
            {
                QNetworkReply *htmlReply;
                QJsonDocument doc = QJsonDocument::fromJson(msg->getBodyToSend());
                if (msg->getMethod()==HttpMethod::GET) {
                    QUrlQuery queryData;
                    QJsonObject json = doc.object();
                    for (auto it = json.begin(); it != json.end(); ++it) {
                        if (it.value().isString())
                            queryData.addQueryItem(it.key(),it.value().toString());
                        else if (it.value().isDouble())
                            queryData.addQueryItem(it.key(),QString::number(it.value().toDouble()));
                        else if (it.value().isBool())
                            queryData.addQueryItem(it.key(),it.value().toBool() ? QString("?1") : QString("?0"));
                    }
                    QUrl url(msg->getUri());
                    url.setQuery(queryData);
                    htmlRequest.setUrl(url);
                    qDebug() << Q_FUNC_INFO << "Garmin: Sending HTTP get " << url;
                    htmlReply = mNetworkManager->get(htmlRequest);
                } else { // POST
                    htmlRequest.setRawHeader("content-type","application/json");
                    qDebug() << Q_FUNC_INFO << "Garmin: Sending HTTP post " << htmlRequest.url() <<doc.toJson();
                    htmlReply = mNetworkManager->post(htmlRequest,doc.toJson());
                }
                QObject::connect(htmlReply,&QNetworkReply::finished, this, [=]() {
                    GarminHttpResponse response;
                    if (htmlReply->error() != QNetworkReply::NoError)
                    {
                        qDebug() << Q_FUNC_INFO << "Garmin: HTTP Request error" << htmlReply->errorString() << " for " << htmlRequest.url();
                        QByteArray responseData = createErrorResponse(msg,HttpResponseStatus::DATA_TRANSFER_ITEM_FAILURE);
                        sendProtobufMessage(responseData,msgId);
                    }
                    else {
                        QByteArray data = htmlReply->readAll();
                        qDebug() << Q_FUNC_INFO << "Garmin: Got response " << QString(data) << " for " << htmlRequest.url();
                        response.setBody(QString(data));
                        QList<QNetworkReply::RawHeaderPair> headers = htmlReply->rawHeaderPairs();
                        for (auto it : headers) {
                            response.addHeader(QString(it.first).toLower(),QString(it.second));
                        }
                        response.setComplete(true);
                        response.setStatus(htmlReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
                        QByteArray responseData = createSuccessResponse(msg,response);
                        if (responseData.isEmpty()) // e.g. non json
                            responseData = createErrorResponse(msg,HttpResponseStatus::DATA_TRANSFER_ITEM_FAILURE);
                        // send  protobuf message
                        sendProtobufMessage(responseData,msgId);
                    }
                    htmlReply->deleteLater();
                });
            }
            break;
        default:
            {
                qDebug() << Q_FUNC_INFO << "Garmin: HTTP Method not yet implemented " << msg->getMethod();
                QByteArray responseData = createErrorResponse(msg,HttpResponseStatus::DATA_TRANSFER_ITEM_FAILURE);
                sendProtobufMessage(responseData,msgId);
            }
    }
}

void GarminHttpHandler::sendProtobufMessage(QByteArray data, quint16 msgId) {
    qDebug() << Q_FUNC_INFO << "Garmin: sending response for Message ID" << msgId;
    int offset = 0;
    const int protobufLength = data.size();
    while (offset < protobufLength)
    {
        qDebug() << Q_FUNC_INFO << "Garmin: sending response for Message ID" << msgId << "offset "<< offset;
        QByteArray message;
        writeU16le(message,msgId);
        // dataOffset LE32 = 0
        writeU32le(message,offset);
        // totalProtobufLength LE32
        writeU32le(message,protobufLength);
        // protobufDataLength LE32
        int chunkLen = std::min(375,protobufLength-offset);
        writeU32le(message,chunkLen);
        // protobuf bytes
        QByteArray chunk = data.mid(offset, chunkLen);
        message.append(chunk);
        // Wrap in Gfdi and send
        QByteArray watchResponse = wrapInGfdiEnvelope(MessageId::ProtobufResponse, message);
        if (mCommunicator) mCommunicator->sendMessage("HTTP RESPONSE",watchResponse);
        offset +=chunkLen;
    }
    /*
    //Prepare payload
    QByteArray message;
    writeU16le(message,msgId);
    // dataOffset LE32 = 0
    writeU32le(message,0);
    const quint32 protobufLength = quint32(data.size());
    // totalProtobufLength LE32
    writeU32le(message,protobufLength);
    // protobufDataLength LE32
    writeU32le(message,protobufLength);
    // protobuf bytes
    message.append(data);
    // Wrap in Gfdi and send
    QByteArray watchResponse = wrapInGfdiEnvelope(MessageId::ProtobufResponse, message);
    if (mCommunicator) mCommunicator->sendMessage("HTTP RESPONSE",watchResponse);
    */
}



