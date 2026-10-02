#ifndef GARMINHTTPRESPONSE_H
#define GARMINHTTPRESPONSE_H

#include <QMap>

class GarminHttpResponse
{
public:
    GarminHttpResponse();
    bool isComplete();
    void setComplete(const bool complete);
    int getStatus();
    void setStatus(const int status);
    QMap<QString,QString> getHeaders();
    void addHeader(QString header, QString value);
    QString getBody();
    void setBody(const QString body);
    auto  getOnDataSuccessfullySentListener();
    void setOnDataSuccessfullySentListener(void(*listener)());

private:
    bool mComplete = true;
    int mStatus = 100; //OK
    QMap<QString, QString> mHeaders;
    QString mBody;
    void (*mOnDataSuccessfullySentListener)(); // callback funtion

};

#endif // GARMINHTTPRESPONSE_H
