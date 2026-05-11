#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QObject>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QMutex>

/**
 * Time-series store for live measurement plots. Reads happen from the UI
 * thread while writes come from the serial worker, so all accessors lock
 * m_mutex.
 */
class DataModel : public QObject
{
    Q_OBJECT

public:
    struct DataPoint {
        QDateTime timestamp;
        double value;

        DataPoint() : value(0.0) {}
        DataPoint(const QDateTime &ts, double val) : timestamp(ts), value(val) {}
    };

    explicit DataModel(QObject *parent = nullptr);

    void addTemperaturePoint(float temperature);
    void addVoltagePoint(float voltage);
    void addPwmPoint(uint8_t pwmDuty);

    QVector<DataPoint> getTemperatureHistory() const;
    QVector<DataPoint> getVoltageHistory() const;
    QVector<DataPoint> getPwmHistory() const;

    void setMaxDataPoints(int maxPoints);
    int maxDataPoints() const { return m_maxDataPoints; }

    double getTemperatureAverage() const;
    double getVoltageAverage() const;
    double getTemperatureMin() const;
    double getTemperatureMax() const;

    void clearHistory();

signals:
    void temperatureDataAdded(const DataPoint &point);
    void voltageDataAdded(const DataPoint &point);
    void pwmDataAdded(const DataPoint &point);
    void historyCleared();

private:
    void addDataPoint(QVector<DataPoint> &history, const DataPoint &point);
    double calculateAverage(const QVector<DataPoint> &data) const;
    double findMin(const QVector<DataPoint> &data) const;
    double findMax(const QVector<DataPoint> &data) const;

    QVector<DataPoint> m_temperatureHistory;
    QVector<DataPoint> m_voltageHistory;
    QVector<DataPoint> m_pwmHistory;

    int m_maxDataPoints;
    mutable QMutex m_mutex;
};

#endif // DATAMODEL_H
