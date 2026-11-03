#include "garminweathermessage.h"
#include "weather/currentweather.h"

void writeFloat32(QByteArray &out, double value)
{
    const float f = float(value);
    quint32 bits;
    static_assert(sizeof(bits) == sizeof(f), "unexpected float size");
    memcpy(&bits, &f, sizeof(bits));
    writeU32le(out, bits);
}

qint32 degreesToSemicircles(double degrees)
{
    const double semicircles = degrees * (2147483648.0 / 180.0);
    return qint32(std::clamp(semicircles, -2147483648.0, 2147483647.0));
}

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
    int size = int(sizeof(CURRENT_FIELDS) / sizeof (CURRENT_FIELDS[0]));
    QByteArray message = generateRecordDefinition(0,CURRENT_FIELDS,size);// local type is one as we do not (yet) provide hourly
    size = int(sizeof(DAILY_FIELDS) / sizeof (DAILY_FIELDS[0]));
    message.append(generateRecordDefinition(1,DAILY_FIELDS,size));
    return message;
}

QByteArray GarminWeatherMessage::generateRecordDefinition(quint8 localType, const WeatherField* fields, int size)
{
    qDebug() << Q_FUNC_INFO << "type " << localType;
    // This creates a payload to tell the watch which wheather fields we actually deliver
    qDebug() << Q_FUNC_INFO;
    QByteArray payload;
    payload.append(char(0x40 | localType)); // record header: definition, this local type
    payload.append(char(0));                // reserved
    payload.append(char(0));                // byte order: 0 = little endian
    writeU16le(payload, FIT_MESSAGE_WEATHER);
    payload.append(char(size));
    for (int i = 0; i < size; ++i) {
        payload.append(char(fields[i].number));
        payload.append(char(fields[i].size));
        payload.append(char(fields[i].baseType));
    }
    return payload;
}

QByteArray GarminWeatherMessage::encodeForecast(const CurrentWeather::Forecast forecast) {
    QDateTime forecastDay = QDateTime::fromMSecsSinceEpoch(forecast.dateTime()*1000);
    int dayOfWeek= forecastDay.date().dayOfWeek();
    qDebug() << Q_FUNC_INFO << " forecast for " << forecastDay.date().toString()  << "DOW " << dayOfWeek;
    QByteArray payload;
    quint64 current = QDateTime::currentMSecsSinceEpoch()/1000;
    quint32 timestamp = current-GARMIN_TIME_OFFSET;
    payload.append(char(1)); // we don't do hourly forcasts now so hard-code to 1 (otherwise 2)
    payload.append(REPORT_DAILY_FORECAST);
    writeU32le(payload, timestamp);
    payload.append(char(forecast.minTemperature()-273.15));
    payload.append(char(forecast.maxTemperature()-273.15));
    payload.append(mapWeatherCode(forecast.weatherCode()));
    payload.append(char(int(forecast.precipProbability())));
    payload.append(char(dayOfWeek));
    payload.append(char(forecast.humidity()));
    writeU16le(payload,quint16(forecast.windMaxSpeed()));
    writeU16le(payload,quint16(forecast.windDirection()));
    writeFloat32(payload,0); //UV Index not part of forecast
    writeU32le(payload,forecast.pressure());
    return payload;
}

QByteArray GarminWeatherMessage::encodeCurrentWeather(const CurrentWeather *weather) {
    qDebug() << Q_FUNC_INFO;
    QByteArray payload;
    quint64 current = QDateTime::currentMSecsSinceEpoch()/1000;
    quint32 timestamp = current-GARMIN_TIME_OFFSET;
    payload.append(char(LOCAL_TYPE_CURRENT)); // record header: plain data record
    payload.append(char(REPORT_CURRENT));
    payload.append(char(qint8(weather->temperature()-273.15)));
    payload.append(mapWeatherCode(weather->weatherCode()));
    writeU16le(payload,quint16(weather->windDeg()));
    writeU16le(payload,quint16(weather->windSpeed()));
    payload.append(char(0xff)); // No precipPropability in CurrentWeather
    payload.append(char(qint8(weather->temperature()-273.15))); // feels like tempeerature
    payload.append(char(weather->humidity()));
    writeU32le(payload, timestamp);
    writeU32le(payload,degreesToSemicircles(weather->city()->latitude())); //latitude
    writeU32le(payload,degreesToSemicircles(weather->city()->longitude())); //longitude
    payload.append(char(weather->maxTemperature()-273.15));
    payload.append(char(weather->minTemperature()-273.15));
    payload.append(char(0)); //dew_point, to be fixed
    writeU32le(payload,0);//pressure
    writeU32le(payload, timestamp);
    return payload;
}

QByteArray GarminWeatherMessage::generateDataPayload(const CurrentWeather *weather)
{
    qDebug() << Q_FUNC_INFO;
    QByteArray payload=encodeCurrentWeather(weather);

    // hourly forecast to be implemented later

    // "today" is always present (built from current-conditions fields, like
    // Gadgetbridge's todayDailyForecast), even when the API gave us no
    // future daily forecasts at all.

    CurrentWeather::Forecast f;
    quint64 current = QDateTime::currentMSecsSinceEpoch()/1000;
    f.setDateTime(current);
    f.setMaxTemperature(weather->maxTemperature());
    f.setMinTemperature(weather->minTemperature());
    f.setWeatherCode(weather->weatherCode());
    f.setHumidity(weather->humidity());
    f.setWindMaxSpeed(weather->windSpeed());
    f.setWindDirection(weather->windDeg());
    f.setPrecipProbability(0);

    payload.append(encodeForecast(f));


    for (int i=0;i < weather->forecastCount()-1;++i) // last one seems duplicate, hence -1;
    {
        CurrentWeather::Forecast forecast = weather->forecast(i);
        payload.append(encodeForecast(forecast));
    }
    return payload;
}
