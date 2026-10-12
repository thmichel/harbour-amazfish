#ifndef GARMINWEATHERMESSAGE_H
#define GARMINWEATHERMESSAGE_H

#include "../garmingfdimessage.h"
#include "../communicator_v2.h"

#include <QObject>

const quint8 REPORT_CURRENT = 0;
const quint8 REPORT_HOURLY_FORECAST = 1;
const quint8 REPORT_DAILY_FORECAST = 2;
const quint8 LOCAL_TYPE_CURRENT = 0;

const quint8 FIT_BASE_ENUM = 0x00;
const quint8 FIT_BASE_SINT8 = 0x01;
const quint8 FIT_BASE_UINT8 = 0x02;
const quint8 FIT_BASE_UINT16 = 0x84;
const quint8 FIT_BASE_SINT32 = 0x85;
const quint8 FIT_BASE_UINT32 = 0x86;
const quint8 FIT_BASE_FLOAT32 = 0x88;

const quint16 FIT_MESSAGE_WEATHER = 128;

struct WeatherField {
    quint8 number;
    quint8 size;
    quint8 baseType;
};

const WeatherField CURRENT_FIELDS[] = {
    { 0, 1, FIT_BASE_ENUM },     // weather_report
    { 1, 1, FIT_BASE_SINT8 },    // temperature
    { 2, 1, FIT_BASE_ENUM },     // condition
    { 3, 2, FIT_BASE_UINT16 },   // wind_direction
    { 4, 2, FIT_BASE_UINT16 },   // wind_speed
    { 7, 1, FIT_BASE_UINT8 },    // relative_humidity
    { 9, 4, FIT_BASE_UINT32 },   // observed_at_time
    { 13, 1, FIT_BASE_SINT8 },   // high_temperature
    { 14, 1, FIT_BASE_SINT8 },   // low_temperature
    { 253, 4, FIT_BASE_UINT32 }, // timestamp
};

const WeatherField HOURLY_FIELDS[] = {
    { 0, 1, FIT_BASE_ENUM },     // weather_report
    { 253, 4, FIT_BASE_UINT32 }, // timestamp
    { 1, 1, FIT_BASE_SINT8 },    // temperature
    { 2, 1, FIT_BASE_ENUM },     // condition
    { 3, 2, FIT_BASE_UINT16 },   // wind_direction
    { 4, 2, FIT_BASE_UINT16 },   // wind_speed
    { 6, 1, FIT_BASE_SINT8 },    // temperature_feels_like
};

const WeatherField DAILY_FIELDS[] = {
    { 0, 1, FIT_BASE_ENUM },     // weather_report
    { 253, 4, FIT_BASE_UINT32 }, // timestamp (always "now" - see appendDailyRecord)
    { 14, 1, FIT_BASE_SINT8 },   // low_temperature
    { 13, 1, FIT_BASE_SINT8 },   // high_temperature
    { 2, 1, FIT_BASE_ENUM },     // condition
    { 12, 1, FIT_BASE_ENUM },    // day_of_week
    { 4, 2, FIT_BASE_UINT16 },   // wind_speed
    { 3, 2, FIT_BASE_UINT16 },   // wind_direction
};

class GarminWeatherMessage : public GarminGfdiMessage
{
public:
    GarminWeatherMessage(CommunicatorV2 *parent) {
        mCommunicator = parent;
    }
    void parse(const QByteArray& data);
    QByteArray generateRecordDefinitions();
    QByteArray generateRecordDefinition(quint8 localType, const WeatherField *fields);
    QByteArray encodeCurrentWeather(const CurrentWeather *weather);
    QByteArray encodeToday(const CurrentWeather *weather);
    QByteArray encodeForecast(const CurrentWeather::Forecast forecast);
    QByteArray generateDataPayload(const CurrentWeather *weather);
};

#endif // GARMINWEATHERMESSAGE_H
