#include "garminhttphandler.h"
#include "lib/src/weather/currentweather.h"

#include "../protobuftools.h"
#include "garminjson.h"

#include <QUrl>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
        qDebug() << Q_FUNC_INFO<< "Found Weather request";
        handleWeatherRequest(msg,msgId);
        return;
    }
    // handle all other requests
    handleGenericRequest(msg, msgId);
}

void GarminHttpHandler::handleWeatherRequest(GarminHttpRequest* msg, int msgId) {
    qDebug() << Q_FUNC_INFO;
    QUrl url = msg->getUri();
    QUrlQuery query(url.query());
    // Gadgetbridge handles weather requests based on versions and hour/day
    // For the moment, we ignore that and send the forecast we get from anazfish library
    QJsonObject json;

    // Test with fixed data
    json["epochSeconds"] = QDateTime::currentDateTime().currentMSecsSinceEpoch()*1000;
    json["temperature"] = "[22,CELSIUS]";
    json["description"] ="Light Rain"; // ligh rain
    json["icon"] =17; //light rain;
    json["feelsLikeTemperature"] = "[24,CELSIUS]";
    json["dewPoint"] = "[2,CELSIUS]";
    json["relativeHumidity"] = 40;
    //json["wind"] = "";
    json["locationName"] = "Oberhausen";
    //json["visibility"] = "";
    //json["pressure"] = "";
    //json["pressureChange"] = "";
    //json["cloudCoverage"] = "";
    QJsonDocument doc;
    doc.setObject(json);
    QString body = doc.toJson();
    GarminHttpResponse r;
    r.setBody(body);
    r.setStatus(200);
    r.addHeader("content-type", "application/json");
    // now generate GFDI message
    QByteArray responseData = createWebResponse(msg,r);
    QByteArray response = wrapInGfdiEnvelope(MessageId::ProtobufResponse, responseData);
    if (mCommunicator) mCommunicator->sendMessage("HTTP RESPONSE",response);

}

QByteArray GarminHttpHandler::createSuccessResponse(GarminHttpRequest *req, GarminHttpResponse resp) {
    qDebug() << Q_FUNC_INFO;
    if (req->getIsRaw())
        return createRawResponse(req,resp);
    return createWebResponse(req, resp);
}

QByteArray GarminHttpHandler::createWebResponse(GarminHttpRequest *req, GarminHttpResponse resp) {
    qDebug() << Q_FUNC_INFO << resp.getBody();
    WebRequest request = req->getWebRequest();
    if (resp.getBody().size() > int(req->getWebRequest().maxResponseLength)) {
        qDebug() << Q_FUNC_INFO << "Garmin: HTTP Response too large.";
        //TODO: send back response too large
        return QByteArray();
    }

    QJsonObject jsonObject;

    if (QString::compare(resp.getHeaders().value("content-type"),"application/json", Qt::CaseInsensitive)==0) {
        QString tmpBody = resp.getBody();
        QByteArray jsonData = tmpBody.toUtf8();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        jsonObject = doc.object();
        qDebug() << Q_FUNC_INFO << "Got response body: " << doc.toJson();

        QByteArray body = jsonEncode(jsonObject);
        // Now build the Response Prototbuf string

        QByteArray proto;
        encodeFieldKey(proto,1,2);
        encodeVarint(proto,quint64(HttpResponseStatus::OK));
        encodeFieldKey(proto,2,2);
        encodeVarint(proto,resp.getStatus());
        encodeFieldKey(proto,3,0);
        encodeVarint(proto,body.size());
        proto.append(body);
       //Todo: Check if headers in response is set?
        if (request.httpHeadersInResponse) {
            QMap<QString, QString> h = resp.getHeaders();
            if (!h.isEmpty()) {
                QJsonObject json;
                encodeFieldKey(proto,4,0);
                for (auto it=h.begin();it != h.end();++it)
                {
                   json[QString(it.key().toUtf8())] = QString(it.value().toUtf8());
                }
                QByteArray encodedHeaders=jsonEncode(json);
                encodeVarint(proto,encodedHeaders.size());
                proto.append(encodedHeaders);
            }

        }
        encodeFieldKey(proto,5,2);
        encodeVarint(proto,0); // 0 for uncompressed
        encodeFieldKey(proto,6,2);
        encodeVarint(proto,HttpResponseType::JSON);
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
        //string key = 1;
        //string value = 2;
        encodeFieldKey(h,1,0);
        encodeVarint(h,i.key().size());
        h.append(i.key());
        encodeFieldKey(h,2,0);
        encodeVarint(h,i.value().size());
        h.append(i.value());
        responseHeaders.append(h);
    }
    if (request.useDataXfer) {
        // Date will be returned using data_xfer
        // not implemented yet!
        qDebug() << Q_FUNC_INFO << "Garmin: Http Data Xfer not yet implemented";
        return QByteArray();
    }

    QByteArray responseBody;
    if (req->getHeaders()["accept-encoding"]=="gzip")
    {
        qDebug() << Q_FUNC_INFO << "Garmin: compressing HTTP Response";
        QByteArray h;
        QByteArray key = QString("Content-Encoding").toUtf8();
        QByteArray value = QString("gzip").toUtf8();

        //string key = 1;
        //string value = 2;
        encodeFieldKey(h,1,0);
        encodeVarint(h,key.size());
        h.append(key);
        encodeFieldKey(h,2,0);
        encodeVarint(h,value.size());
        h.append(value);
        responseHeaders.append(h);
        QByteArray uncompressed = resp.getBody().toUtf8();
        responseBody = zlibCompress(uncompressed);
    }
    else responseBody = resp.getBody().toUtf8();

    // Now build Protobuf
    QByteArray proto;
    encodeFieldKey(proto,1,2);
    encodeVarint(proto,quint64(HttpResponseStatus::OK));
    encodeFieldKey(proto,2,2);
    encodeVarint(proto,resp.getStatus());
    encodeFieldKey(proto,3,0);
    encodeVarint(proto,responseBody.size());
    encodeFieldKey(proto,4,2);
    encodeVarint(proto,request.useDataXfer);
    QString it;
    while (!responseHeaders.isEmpty()){
        it = responseHeaders.takeFirst();
        encodeFieldKey(proto,5,0);
        encodeVarint(proto,it.size());
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
        return;
    }

    qDebug() << Q_FUNC_INFO << "Garmin: Handling web request for" << msg->getUri();

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest htmlRequest;
    htmlRequest.setUrl(msg->getUri());
    htmlRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QMap<QString,QString> headers = msg->getHeaders();
    for ( auto it=headers.begin();it!=headers.end();++it) {
        htmlRequest.setRawHeader(it.key().toUtf8(),it.value().toUtf8());
    }
    switch (msg->getMethod()) {
        case HttpMethod::PUT:
            qDebug() << Q_FUNC_INFO << "Garmin: HTTP Put not yet implemented";
        break;
        case HttpMethod::GET:
            {
                QNetworkReply *htmlReply = manager->get(htmlRequest);
                QObject::connect(htmlReply,&QNetworkReply::finished, this, [=]() {
                    GarminHttpResponse response;
                    if (htmlReply->error() != QNetworkReply::NoError)
                    {
                        qDebug() << Q_FUNC_INFO << "Garmin: HTTP Request error" << htmlReply->errorString();
                    }
                    else {                        
                        QByteArray data = htmlReply->readAll();
                        response.setBody(QString(data));
                        QList<QNetworkReply::RawHeaderPair> headers = htmlReply->rawHeaderPairs();
                        for (auto it : headers) {
                            response.addHeader(QString(it.first).toLower(),QString(it.second));
                        }
                        response.setComplete(true);
                        response.setStatus(htmlReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
                        QByteArray responseData = createSuccessResponse(msg,response);
                        // send  protobuf message
                        sendProtobufMessage(responseData,msgId);
                    }
                    htmlReply->deleteLater();
                });
            }
            break;
        default: qDebug() << Q_FUNC_INFO << "Garmin: HTTP Method not yet implemented";
    }
    manager->deleteLater();
}

void GarminHttpHandler::sendProtobufMessage(QByteArray data, quint16 msgId) {
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

}



