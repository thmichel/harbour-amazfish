#include "garminfitdefinitionmessage.h"
#include "garminweathermessage.h"


void GarminFitDefinitionMessage::parseHeaderRecords(const QByteArray& data, int& pos, bool hasDeveloperData) {
    qDebug() << Q_FUNC_INFO;
    pos++; // ignore first byte
    bool isLittleEndian = (data[pos] == 0x01) ? false :true;
    pos++;
    // First get the message Number identifiying the FIT type
    int messageNumber;
    if (isLittleEndian) messageNumber = u16le(data,pos);
    else messageNumber = quint16(quint16(data[pos]) <<8) | (quint8(data[pos+1]));
    pos +=2;
    // Now get the number of fields requested for this type
    int numFields = data[pos];
    pos++;
    // Now we need to get the  fields themselves
    // We currently don't do anything with them as we only implement Weather FIT for now
    // Basically we need to collect these items and pass it to the message handler for the FIT type
    for (int i=0;i < numFields; i++) {
        int number = data[pos];
        pos++;
        int size = data[pos];
        pos++;
        int baseTypeIdentifier = data[pos];
        pos++;
    }

    // No get the developer data if any
    // Same as above - not used for now

    if (hasDeveloperData) {
        int numDevFields = data[pos];
        pos++;
        for (int i = 0; i < numDevFields; i++) {
            int number = data[pos];
            pos++;
            int size = data[pos];
            pos++;
            int developerDataIndex = data[pos];
            pos++;        }
    }
}

void GarminFitDefinitionMessage::parseIncoming(const QByteArray& data)
{
    // This is for incoming fit requests, e.g. the watch sends data to us
    qDebug() << Q_FUNC_INFO << " data " << data.toHex();
    int pos = 2; // first two bytes contain MessageType (FIT_DEFINITION)
    bool hasDeveloperData;

    while (pos < data.size()) {
        quint8 header = data[pos];
        qDebug() << Q_FUNC_INFO << "Header is " << header << " at pos " << pos;
        pos++;
        bool hasDeveloperData = true;
        bool isDefinition = false;

        if ((header & 0x80) == 0x80) { //compressed timestamp
             // This is not a definition, not yet handled

            isDefinition = false;
            hasDeveloperData = false;
            int localMessageType = (header >> 5) & 0x3;
            int timeOffset = header & 0x1f;

        } else  {
             if ((header & 0x40) == 0x40)
                 isDefinition = true;
             //store local Message Type
             mDefinitionList.append(quint16(header & 0xf));
             qDebug() << Q_FUNC_INFO << "Garmin: fit definition requested for " << quint16(header & 0xf);
             // Now we need to parse the buffer
             hasDeveloperData = ((header & 0x20) == 0x20);
             if (isDefinition) parseHeaderRecords(data, pos,hasDeveloperData);
        }
    }
    //Once parsed, we need to trnasfer this to the handler
    // Not implemented yet
}

QByteArray GarminFitDefinitionMessage::generateOutgoing() {
    // This should check which FIT type needs to be sent
    // For now we implemented only weather so just send weather data
    qDebug() << Q_FUNC_INFO;
    QByteArray data;

    // For testing send weather unconditionally
    GarminWeatherMessage *message = new GarminWeatherMessage(mCommunicator);
    data = message->generateDataPayload(mCommunicator->weather());
    return wrapInGfdiEnvelope(MessageId::FitData,data);;

    for (int i =0; i < mDefinitionList.size();++i) {
        switch (int(mDefinitionList[i])) {
            case FitDefinitions::WEATHER:
            {
                data.append(message->generateDataPayload(mCommunicator->weather()));
                break;
            }
            default:
                break;
        }
    }
    return wrapInGfdiEnvelope(MessageId::FitData,data);
}
