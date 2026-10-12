#include "garminfitdefinitionmessage.h"
#include "garminweathermessage.h"


void parseIncoming(const QByteArray& data, int& pos, bool hasDeveloperData) {
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

void GarminFitDefinitionMessage::parse(const QByteArray& data)
{
    qDebug() << Q_FUNC_INFO;
    // Multiple Fit Types can be requested in one message
    // Extract the types and handle them individually
    int pos = 0;
    bool hasDeveloperData;

    while (pos < data.size()) {
        quint8 header = data[pos];
        bool hasDeveloperData = true;

        if ((header & 0x80) == 0x80) { //compressed timestamp
             // This is not a definition, not yet handled
            bool definition = false;
            hasDeveloperData = false;
            int localMessageType = (header >> 5) & 0x3;
            int timeOffset = header & 0x1f;
            break;
        } else if ((header & 0x40) == 0x40) {
             //it's a header - store local Message Type
             mDefinitionList.append(quint16(header & 0xf));
             qDebug() << Q_FUNC_INFO << "Garmin: fit definition requested for " << quint16(header & 0xf);
             // Now we eed to parse the buffer
             hasDeveloperData = ((header & 0x20) == 0x20);
             parseIncoming(data, pos,hasDeveloperData);
        }
    }
    if (mCommunicator) mCommunicator->sendMessage("FIT DATA", generateOutgoing());
}

QByteArray GarminFitDefinitionMessage::generateOutgoing() {
    qDebug() << Q_FUNC_INFO;
    QByteArray data;
    for (int i =0; i < mDefinitionList.size();++i) {
        switch (int(mDefinitionList[i])) {
            case FitDefinitions::WEATHER:
            {
                GarminWeatherMessage *message = new GarminWeatherMessage(mCommunicator);
                CurrentWeather* weather = new CurrentWeather();
                data.append(message->generateDataPayload(weather));
                break;
            }
            default:
                break;
        }
    }
    return wrapInGfdiEnvelope(MessageId::FitData,data);
}
