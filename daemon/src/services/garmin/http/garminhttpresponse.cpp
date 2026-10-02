#include "garminhttpresponse.h"

GarminHttpResponse::GarminHttpResponse()
{

}


bool GarminHttpResponse::isComplete() {
        return mComplete;
    }

void GarminHttpResponse::setComplete(const bool complete) {
        mComplete = complete;
    }

int GarminHttpResponse::getStatus() {
        return mStatus;
    }

void GarminHttpResponse::setStatus(const int status) {
        mStatus = status;
    }

QMap<QString, QString> GarminHttpResponse::getHeaders() {
        return mHeaders;
    }

void GarminHttpResponse::addHeader(QString header, QString value) {
    mHeaders.insert(header,value);
}

QString GarminHttpResponse::getBody() {
        return mBody;
    }

void GarminHttpResponse::setBody(const QString body) {
        mBody = body;
    }

auto GarminHttpResponse::getOnDataSuccessfullySentListener() {
        return mOnDataSuccessfullySentListener;
    }

void GarminHttpResponse::setOnDataSuccessfullySentListener(void(*listener)()) {
        mOnDataSuccessfullySentListener = listener;
    }
