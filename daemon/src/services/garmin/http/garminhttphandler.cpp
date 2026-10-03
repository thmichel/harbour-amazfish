#include "garminhttphandler.h"
#include "lib/src/weather/currentweather.h"
#include "garminhttpresponse.h"
#include "../protobuftools.h"
#include "garminjson.h"

#include <QUrl>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    switch (msg->getType())
    {
        case HttpRequestType::webRequest:
            handleWebRequest(msg,msgId);
            break;
        case HttpRequestType::webResponse:
            break;
        case HttpRequestType::rawRequest:
            break;
        case HttpRequestType::rawResponse:
            break;
    }
}

void  GarminHttpHandler::handleWebRequest(GarminHttpMessage* msg, int msgId) {
    // handles the non-raw web requests, like weather requests.
    // for anything not handled, return null
    qDebug() << Q_FUNC_INFO;
    WebRequest r = msg->getWebRequest();
    QUrl url(r.url);
    if ((url.host()=="api.gcs.garmin.com") && (url.path().startsWith("/weather/v"))) {
        qDebug() << Q_FUNC_INFO<< "Found Weather request";
        handleWeatherRequest(msg,msgId);
        return;
    }
    // handle all other requests
    qDebug() << Q_FUNC_INFO << "Garmin: handling generic web request";
    handleGenericRequest(msg, msgId);
}

void GarminHttpHandler::handleWeatherRequest(GarminHttpMessage* msg, int msgId) {
    qDebug() << Q_FUNC_INFO;
    WebRequest request = msg->getWebRequest();
    QUrl url = request.url;
    QUrlQuery query(url.query());
    // Gadgetbridge handles weather requests based on versions and hour/day
    // For the moment, we ignore that and sned the forecast we get from anazfish library
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
    QByteArray responseData = createWebResponse(msg,r,msgId);
    QByteArray response = wrapInGfdiEnvelope(MessageId::ProtobufResponse, responseData);
    if (mCommunicator) mCommunicator->sendMessage("HTTP RESPONSE",response);

}

QByteArray GarminHttpHandler::createWebResponse (GarminHttpMessage *req, GarminHttpResponse resp, int msgId) {
    qDebug() << Q_FUNC_INFO;

    if (resp.getBody().size() > int(req->getWebRequest().maxResponseLength)) {
        qDebug() << Q_FUNC_INFO << "Garmin: HTTP Response too large.";
        //TODO: send back response too large
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
//        if (req->getWebRequest().httpHeadersInResponse) {
            QMap<QString, QString> h = resp.getHeaders();
            if (!h.isEmpty()) {
                QJsonObject json;
                encodeFieldKey(proto,4,0);
                for (auto it=h.begin();it != h.end();++it)
                {
                    json[it.key()] = it.value();
                }
                QByteArray encodedHeaders=jsonEncode(json);
                encodeVarint(proto,encodedHeaders.size());
                proto.append(encodedHeaders);
            }

  //      }
        encodeFieldKey(proto,5,2);
        encodeVarint(proto,0); // 0 for uncompressed
        return proto;
    } else qDebug() << Q_FUNC_INFO << "Garmin: No JSON content.";
    //Non-json not handled
    return QByteArray();
}


void GarminHttpHandler::handleGenericRequest(GarminHttpMessage*  msg, int msgId) {
    // handle non-raw Web Request
    qDebug() << Q_FUNC_INFO;
    WebRequest request = msg->getWebRequest();
    QUrl dest(request.url);
    if ( dest.host().endsWith("garmin.com") || dest.host().endsWith("dciwx.com")) {
        // For now, we explicitly block all requests to Garmin domains, even if the user whitelists them.
        // Due to fake OAuth, most of these will include invalid authentication credentials, and needs
        // further investigation
        qDebug() << Q_FUNC_INFO <<"Garmin:Blocking request to Garmin url: " <<dest.host();
        return;
    }
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest htmlRequest;
    htmlRequest.setUrl(request.url);
    htmlRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QJsonValue json = jsonDecode(request.headers);
    if (json.isObject()) {
        QJsonObject obj = json.toObject();

        for ( auto it = obj.begin();it != obj.end(); ++it) {
            if (it.value().isString()) {
                QByteArray value;
                value.append(it.value().toString().toUtf8());
                QByteArray key;
                key.append(it.key().toUtf8());
                htmlRequest.setRawHeader(key,value);
            }
        }
    }
    switch (request.method) {
        case HttpMethod::PUT:
            qDebug() << Q_FUNC_INFO << "Garmin: HTTP Put not yet implemented";
        break;
        case HttpMethod::GET:
            {
                QNetworkReply *htmlReply = manager->get(htmlRequest);
                QObject::connect(htmlReply,&QNetworkReply::finished, this, [=]() {
                    GarminHttpResponse response;
                    if (htmlReply->error())
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
                        QByteArray responseData = createWebResponse(msg,response,msgId);
                        // Generate protobuf payload
                        QByteArray message;
                        writeU16le(message,msgId);
                        // dataOffset LE32 = 0
                        for (int i = 0; i < 4; ++i) {
                            message.append(char(0));
                        }
                        const quint32 protobufLength = quint32(responseData.size());
                        // totalProtobufLength LE32
                        writeU32le(message,protobufLength);
                        // protobufDataLength LE32
                        writeU32le(message,protobufLength);
                        // protobuf bytes
                        message.append(responseData);
                        // now generate GFDI message
                        QByteArray watchResponse = wrapInGfdiEnvelope(MessageId::ProtobufResponse, message);
                        if (mCommunicator) mCommunicator->sendMessage("HTTP RESPONSE",watchResponse);
                    }
                    htmlReply->deleteLater();
                });
            }
        default: qDebug() << Q_FUNC_INFO << "Garmin: HTTP Method not yet implemented";
    }
}
