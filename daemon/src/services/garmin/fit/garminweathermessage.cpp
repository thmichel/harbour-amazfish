#include "garminweathermessage.h"

#include "weather/currentweather.h"


quint8 mapWeatherCode(int code) {
    // Map OpenWeathermap codes to Garmin codes
    if (code >= 200 && code <=299) return 6; //Thunderstorms
    if (code == 500) return 16; //Light rain
    if (code >= 501 && code <=521) return 3; //Rain
    if (code >= 522 && code <=599) return 17; //Heavy Rain
    if (code >= 600) return 18; //Light snow
    if (code == 602) return 19; //Heavy Snow
    if (code >= 601 && code <=699) return 4; // Snow
    if (code == 741) return 8; //Fog
    if (code == 800) return 0; //Clear
    if (code >= 801 && code <=802) return 1; //Partly Clowdy
    if (code >= 803 && code <=804) return 22; // Clowdy
    return 0; // If we don't know better.
}

void GarminWeatherMessage::parse(const QByteArray& data) {
    qDebug() << Q_FUNC_INFO << "Garmin: parsing weather request";

    if (data.size() < 10) {
        return;
    }
    WeatherRequestMessage msg;
    msg.format = quint8(data[0]);
    msg.latitude = i32le(data, 1);
    msg.longitude = i32le(data, 5);
    msg.hoursOfForecast = quint8(data[9]);
    // We basically ignore above data for now - we always send the local weather
    QByteArray msgData = generateRecordDefinitions();
    if (mCommunicator) mCommunicator->sendMessage("WEATHER REQUEST DEFINITION", wrapInGfdiEnvelope(MessageId::FitDefinition,msgData));

}

QByteArray GarminWeatherMessage::generateRecordDefinitions() {
    qDebug() << Q_FUNC_INFO;
    QByteArray message = generateRecordDefinition(1,CURRENT_FIELDS);// local type is one as we do not (yet) provide hourly
    message.append(generateRecordDefinition(1,DAILY_FIELDS));
    return message;
}

QByteArray GarminWeatherMessage::generateRecordDefinition(quint8 localType, const WeatherField *fields)
{
    // This creates a payload to tell the watch wich whether fields we actually deliver
    qDebug() << Q_FUNC_INFO;
    QByteArray payload;
    int fieldCount = int(sizeof(*fields) / sizeof (fields[0]));
    payload.append(char(0x40 | localType)); // record header: definition, this local type
    payload.append(char(0));                // reserved
    payload.append(char(0));                // byte order: 0 = little endian
    writeU16le(payload, FIT_MESSAGE_WEATHER);
    payload.append(char(fieldCount));
    for (int i = 0; i < fieldCount; ++i) {
        payload.append(char(fields[i].number));
        payload.append(char(fields[i].size));
        payload.append(char(fields[i].baseType));
    }
    return payload;
}

QByteArray GarminWeatherMessage::encodeForecast(const CurrentWeather::Forecast forecast) {
    QByteArray payload;
    quint64 current = QDateTime::currentMSecsSinceEpoch()/1000;
    quint64 timestamp = current-GARMIN_TIME_OFFSET;

    payload.append(char(1)); // we don't do hourly forcasts now so hard-code to 1 (otherwise 2)
    payload.append(REPORT_DAILY_FORECAST);
    writeU32le(payload, quint32(timestamp));
    payload.append(char(forecast.minTemperature()));
    payload.append(char(forecast.maxTemperature()));
    payload.append(mapWeatherCode(forecast.weatherCode()));
    char dayOfWeek= QDateTime::fromMSecsSinceEpoch(forecast.dateTime()).date().dayOfWeek();
    payload.append(dayOfWeek);
    payload.append(char(forecast.humidity()));
    writeU16le(payload,quint16(forecast.windMaxSpeed()));
    writeU16le(payload,quint16(forecast.windDirection()));
    return payload;
}



QByteArray GarminWeatherMessage::encodeCurrentWeather(const CurrentWeather *weather) {
    QByteArray payload;
    quint64 current = QDateTime::currentMSecsSinceEpoch()/1000;
    quint64 timestamp = current-GARMIN_TIME_OFFSET;
    payload.append(char(LOCAL_TYPE_CURRENT)); // record header: plain data record
    payload.append(char(REPORT_CURRENT));
    payload.append(char(qint8(weather->temperature())));
    payload.append(mapWeatherCode(weather->weatherCode()));
    writeU16le(payload,quint16(weather->windDeg()));
    writeU16le(payload,quint16(weather->windSpeed()));
    payload.append(char(weather->humidity()));
    writeU32le(payload, quint32(timestamp));
    payload.append(char(weather->maxTemperature()));
    payload.append(char(weather->minTemperature()));
    writeU32le(payload, quint32(timestamp));
    return payload;
}

QByteArray GarminWeatherMessage::generateDataPayload(const CurrentWeather *weather)
{

    QByteArray payload=encodeCurrentWeather(weather);

    // hourly forecaste to be implemented later

    // "today" is always present (built from current-conditions fields, like
    // Gadgetbridge's todayDailyForecast), even when the API gave us no
    // future daily forecasts at all.
    CurrentWeather::Forecast f;
    f.setDateTime(QDateTime::currentMSecsSinceEpoch());
    f.setMaxTemperature(weather->maxTemperature());
    f.setMinTemperature(weather->minTemperature());
    f.setWeatherCode(weather->weatherCode());
    f.setHumidity(weather->humidity());
    f.setWindMaxSpeed(weather->windSpeed());
    f.setWindDirection(weather->windDeg());
    payload.append(encodeForecast(f));
    for (int f=0;f < weather->forecastCount();f++)
    {
        CurrentWeather::Forecast forecast = weather->forecast(f);
        payload.append(encodeForecast(forecast));

    }
    return payload;
}
