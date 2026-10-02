#ifndef GARMINJSON_H
#define GARMINJSON_H

#include <QByteArray>
#include <QMap>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

struct QueueItem {
    enum Type {
        JsonValue,
        StringValue
    };
    Type type;
    QJsonValue value;
    QString string;
};

struct MapPlaceholder
{
    QJsonObject object;
    int size;
};

struct ArrayPlaceholder
{
    QJsonArray array;
    int length;
};

struct DecodedValue {
    enum Type{
        Json,
        Map,
        Array
    };
    Type type = Json;

    QJsonValue jsonValue;
    MapPlaceholder mapPlaceholder;
    ArrayPlaceholder arrayPlaceholder;
};

QByteArray jsonEncode(QJsonObject object);

void jsonEncodeValueBreadthFirst( const QJsonValue& root, QByteArray& out, const QMap<QString, quint32>& stringOffsets);

QJsonValue jsonDecode(const QByteArray& bytes);
DecodedValue jsonDecodeValue(const QByteArray& bytes, qptrdiff& pos,const QMap<quint32,QString> strings);

void collectStrings(const QJsonObject rootObj, QList<QString> &strings);

void writeUint16BE(QByteArray &out, const quint16 value);
void writeUint32BE(QByteArray &out, const quint32 value);
void writeUint64BE(QByteArray &out, const quint64 value);
void writeFloat(QByteArray &out, float value);
void writeDouble(QByteArray &out, double value);
quint16 readUint16BE(const QByteArray& data, qptrdiff& pos);
quint32 readUint32BE(const QByteArray& data, qptrdiff& pos);
quint64 readUint64BE(const QByteArray& data, qptrdiff& pos);
float readFloat(const QByteArray& data, qptrdiff& pos);
double readDouble(const QByteArray& data, qptrdiff& pos);


#endif // GARMINJSON_H
