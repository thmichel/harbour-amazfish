#include "garminjson.h"
#include <cmath>   // For fmod()

#include <QList>
#include <QQueue>
#include <QMap>
#include <QDebug>

static QByteArray STRING_SECTION_MAGIC(QByteArray::fromHex("abcdabcd"));
static QByteArray DATA_SECTION_MAGIC(QByteArray::fromHex("da7ada7a"));

static constexpr quint8 TYPE_NULL = 0x00;
static constexpr quint8 TYPE_SINT32 = 0x01;
static constexpr quint8 TYPE_FLOAT = 0x02;
static constexpr quint8 TYPE_STRING = 0x03;
static constexpr quint8 TYPE_ARRAY = 0x05;
static constexpr quint8 TYPE_BOOL = 0x09;
static constexpr quint8 TYPE_MAP = 0x0b;
static constexpr quint8 TYPE_SINT64 = 0x0e;
static constexpr quint8 TYPE_DOUBLE = 0x0f;

QByteArray jsonEncode(QJsonObject object) {
    // First pass: collect all strings
    QStringList strings;
    collectStrings(object, strings);

    // Build string section
    QByteArray stringSection;
    quint32 currentOffset = 0;
    QMap<QString,quint32> offsets;

    for (const QString& str : strings) {
        offsets[str] = currentOffset;
        QByteArray strBytes = str.toUtf8();
        quint16 length = strBytes.size() + 1; // +1 for null terminator
        writeUint16BE(stringSection,length);
        stringSection.append(strBytes);
        stringSection.append(char(0x00));
        currentOffset += 2 + strBytes.size() + 1;
    }

    // Build data section (breadth-first)
    QByteArray dataSection;
    jsonEncodeValueBreadthFirst(object, dataSection, offsets);

    QByteArray output;

    // Write string section, if any
    if (!stringSection.isEmpty()) {
        output.append(STRING_SECTION_MAGIC);
        writeUint32BE(output, stringSection.size());
        output.append(stringSection);
    }

    // Write data section
    output.append(DATA_SECTION_MAGIC);
    writeUint32BE(output, dataSection.size());
    output.append(dataSection);

    return output;
  }

void jsonEncodeValueBreadthFirst( const QJsonValue& root, QByteArray& out, const QMap<QString, quint32>& stringOffsets) {
        QQueue<QueueItem> queue;
        queue.enqueue({QueueItem::JsonValue,root,{}});

        while (!queue.isEmpty()) {
            QueueItem item = queue.dequeue();
            if (item.type == QueueItem::StringValue)
            {
                out.append(char(TYPE_STRING));
                auto it = stringOffsets.find(item.string);
                if (it == stringOffsets.end())
                    qDebug() << Q_FUNC_INFO << "String not found";
                writeUint32BE(out,it.value());
                continue;
            }

            const QJsonValue& val = item.value;
            if (val.isNull() || val.isUndefined()) {
                out.append(TYPE_NULL);
            }
            else if (val.isBool()) {
                out.append(char(TYPE_BOOL));
                out.append(val.toBool() ? 0x01 : 0x00);
            }
            else if (val.isString()) {
                out.append(char(TYPE_STRING));
                QString str = val.toString();
                int offset = stringOffsets.value(str);
                writeUint32BE(out, offset);
            }
            else if (val.isDouble()) {
                double d = val.toDouble();
                qint64 i = static_cast<qint64>(d);
                bool isInteger = double(i) == d;
                if (!isInteger) {
                    float f = static_cast<float>(d);
                    if (double(f)==d) {
                        out.append(char(TYPE_FLOAT));
                        writeFloat(out, f);
                    }
                    else
                    {
                        out.append(char(TYPE_DOUBLE));
                        writeDouble(out,d);
                    }
                }
                else {
                    if (i >= INT32_MIN && i <= INT32_MAX) {
                        out.append(char(TYPE_SINT32));
                        writeUint32BE(out, static_cast<quint32>(i));
                    } else {
                        out.append(char(TYPE_SINT64));
                        writeUint64BE(out, static_cast<quint64>(i));
                    }
                }
            }

            else if (val.isArray()) {
                    out.append(char(TYPE_ARRAY));
                    QJsonArray arr = val.toArray();
                    writeUint32BE(out, arr.size());
                    // Add array elements to queue for breadth-first processing
                    for (const auto& element : arr) {
                        queue.enqueue({QueueItem::JsonValue,element,{}});
                    }
                }
            else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                out.append(TYPE_MAP);
                writeUint32BE(out, obj.size());
                for (auto entry = obj.begin(); entry != obj.end(); ++entry)  {
                    queue.enqueue({QueueItem::StringValue,{},entry.key()});
                    queue.enqueue({QueueItem::JsonValue,entry.value(),{}});
                }
            }
        }
    }

QJsonValue jsonDecode(const QByteArray& bytes) {
    if (bytes.size() < 9)
        return QJsonValue();

    qptrdiff pos = 0;

    QMap<quint32, QString> strings;
    QByteArray magic = bytes.mid(pos,4);
    pos +=4;

    if (magic == STRING_SECTION_MAGIC) {
        quint32 stringSectionLength = readUint32BE(bytes,pos);
        qptrdiff end = pos + stringSectionLength;
        qptrdiff start = pos;

        while (pos < end) {
            quint32 offset = pos - start;
            quint16 len = readUint16BE(bytes,pos);
            QByteArray utf8 = bytes.mid(pos, len-1);
            pos += len-1;
            pos++; // null terminator
            strings[offset]=QString::fromUtf8(utf8);
        }
        magic = bytes.mid(pos,4);
        pos +=4;

    }
    if (magic!=DATA_SECTION_MAGIC) {
            qDebug() << Q_FUNC_INFO <<" Invalid data section magic";
            return QJsonValue();
    }


    quint32 dataSectionLength = readUint32BE(bytes,pos);
    if (pos + dataSectionLength > bytes.size()) {
        qDebug() << Q_FUNC_INFO <<"Data section exceeds buffer";
        return QJsonValue();
    }


    DecodedValue root = jsonDecodeValue(bytes, pos, strings);

        // Process placeholders breadth-first using a queue
    QList<DecodedValue> queue;
    queue.append(root);

    QMutableListIterator<DecodedValue> it(queue);

    while (it.hasNext()) {
        DecodedValue& current = it.next();
        if (current.type == DecodedValue::Map) {
            auto& mp = current.mapPlaceholder;
            for (int i = 0; i <mp.size; i++) {
                DecodedValue key = jsonDecodeValue(bytes, pos, strings);
                DecodedValue val = jsonDecodeValue(bytes,pos, strings);

                QString keyString = key.jsonValue.toString();


                if (val.type==DecodedValue::Map) {
                    mp.object.insert(keyString,mp.object);
                    queue.append(val);
                }
                else if (val.type == DecodedValue::Array) {
                    mp.object.insert(keyString,val.arrayPlaceholder.array);
                    queue.append(val);
                }
                else
                {
                    mp.object.insert(keyString,val.jsonValue);
                }
            }
        }
        else if (current.type == DecodedValue::Array) {
            auto& ap = current.arrayPlaceholder;
            for (int i = 0; i < ap.length; i++) {
                DecodedValue val = jsonDecodeValue(bytes,pos,strings);

                if (val.type == DecodedValue::Map) {
                    ap.array.append(val.mapPlaceholder.object);
                    queue.append(val);
                }
                else if (val.type == DecodedValue::Array) {
                    ap.array.append(val.arrayPlaceholder.array);
                    queue.append(val);
                }
                else ap.array.append(val.jsonValue);
            }
       }
    }

    switch(queue.first().type) {
        case DecodedValue::Map:
            return queue.first().mapPlaceholder.object;
        case DecodedValue::Array:
            return queue.first().arrayPlaceholder.array;
        default:
            return queue.first().jsonValue;
    }

}

void collectStrings(const QJsonObject rootObj, QList<QString> &strings) {
    QQueue<QJsonValue> queue;
    queue.enqueue(rootObj);
    auto addUnique = [&] (const QString& s)
    {
        if (!strings.contains(s))
            strings.append(s);
    };

    while (!queue.isEmpty()) {
        QJsonValue val = queue.dequeue();
        if (val.isObject()) {
            QJsonObject obj=val.toObject();
            for ( auto it = obj.begin();it != obj.end(); ++it) {
                addUnique(it.key());
                if (it.value().isString()) {
                    addUnique(it.value().toString());
                } else if (it.value().isObject() || it.value().isArray()) {
                   queue.enqueue(it.value());
                }
            }
        }
        else if (val.isArray()) {
            for (const QJsonValue& element : val.toArray()) {
                if (element.isString()) {
                    addUnique(element.toString());
                } else if (element.isObject() || element.isArray()) {
                    queue.enqueue(element);
                }
            }
        }
        else {
            if (val.isString()) {
                addUnique(val.toString());
            }
        }
    }
}
DecodedValue jsonDecodeValue(const QByteArray& bytes, qptrdiff& pos, const QMap<quint32,QString> strings){
    quint8 type = static_cast<quint8>(bytes[int(pos++)]);
    DecodedValue result;

    switch (type) {
        case TYPE_NULL:
            result.jsonValue = QJsonValue(QJsonValue::Null);
            break;
        case TYPE_BOOL:
            result.jsonValue = QJsonValue(bytes[int(pos++)] != 0);
            break;
        case TYPE_SINT32:
            result.jsonValue = QJsonValue(static_cast<qint64>(readUint32BE(bytes,pos)));
            break;
        case TYPE_SINT64:
            result.jsonValue = QJsonValue(static_cast<qint64>(readUint64BE(bytes,pos)));
            break;
        case TYPE_FLOAT:
            result.jsonValue= QJsonValue(readFloat(bytes,pos));
            break;
        case TYPE_DOUBLE:
            result.jsonValue= QJsonValue(readDouble(bytes,pos));
            break;
        case TYPE_STRING:
            {
            quint32 offset = readUint32BE(bytes,pos);
            QString str = strings[offset];
            result.type = DecodedValue::Json;
            result.jsonValue=QJsonValue(str);
            break;
            }
        case TYPE_ARRAY:
            {
            result.type = DecodedValue::Array;
            result.arrayPlaceholder.length=static_cast<int>(readUint32BE(bytes,pos));
            break;
            }
        case TYPE_MAP:
            {
            result.type = DecodedValue::Map;
            result.mapPlaceholder.size=static_cast<int>(readUint32BE(bytes,pos));
            break;
            }
        default:
            qDebug() << Q_FUNC_INFO << "Unknown json type";
    }
    return result;
}

void writeUint16BE(QByteArray &out, const quint16 value) {
    out.append(char((value >> 8) & 0xFF));
    out.append(char(value & 0xFF));
}

void writeUint32BE(QByteArray &out, const quint32 value) {
    out.append(char((value >> 24) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char(value & 0xFF));
}

void writeUint64BE(QByteArray &out, const quint64 value) {
    out.append(char ((value >> 56) & 0xFF));
    out.append(char ((value >> 48) & 0xFF));
    out.append(char ((value >> 40) & 0xFF));
    out.append(char ((value >> 32) & 0xFF));
    out.append(char ((value >> 24) & 0xFF));
    out.append(char ((value >> 16) & 0xFF));
    out.append(char ((value >> 8) & 0xFF));
    out.append(char (value & 0xFF));
}

void writeFloat(QByteArray &out, float value) {
    uint32_t y;
    memcpy(&y, &value, 4);
    writeUint32BE(out,y);
}

void writeDouble(QByteArray &out, double value) {
    uint64_t y;
    memcpy(&y, &value, 8);
    writeUint64BE(out, y);
}

quint64 readUint64BE(const QByteArray& data, qptrdiff& pos) {
    quint64 v = 0;
    for (int i=0; i<8; i++)
    {
        v = (v <<8) | quint8(data[int(pos++)]);
    }
    return v;
}

quint32 readUint32BE(const QByteArray& data, qptrdiff& pos) {
    quint32 v =
            (quint32(data[int(pos)]) << 24) |
            (quint32(data[int(pos)+1]) << 16) |
            (quint32(data[int(pos)+2]) << 8) |
            quint32(data[int(pos)+3]);
    pos +=4;
    return v;
}

quint16 readUint16BE(const QByteArray& data, qptrdiff& pos) {
    quint16 v =
            (quint8(data[int(pos)]) << 8) |
            quint8(data[int(pos)+1]);
    pos +=2;
    return v;
}



float readFloat(const QByteArray& data, qptrdiff& pos) {
    quint32 bits = readUint32BE(data,pos);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

double readDouble(const QByteArray& data, qptrdiff& pos) {
    quint64 bits = readUint32BE(data,pos);
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
