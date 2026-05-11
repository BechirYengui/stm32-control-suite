#include "DataModel.h"
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>

DataModel::DataModel(QObject *parent)
    : QObject(parent)
    , m_maxDataPoints(500)  // Par défaut: 500 points d'historique
{
    qDebug() << "[DataModel] Initialized with max" << m_maxDataPoints << "data points";
}

void DataModel::addTemperaturePoint(float temperature)
{
    QMutexLocker locker(&m_mutex);
    
    DataPoint point(QDateTime::currentDateTime(), temperature);
    addDataPoint(m_temperatureHistory, point);
    
    emit temperatureDataAdded(point);
}

void DataModel::addVoltagePoint(float voltage)
{
    QMutexLocker locker(&m_mutex);
    
    DataPoint point(QDateTime::currentDateTime(), voltage);
    addDataPoint(m_voltageHistory, point);
    
    emit voltageDataAdded(point);
}

void DataModel::addPwmPoint(uint8_t pwmDuty)
{
    QMutexLocker locker(&m_mutex);
    
    DataPoint point(QDateTime::currentDateTime(), pwmDuty);
    addDataPoint(m_pwmHistory, point);
    
    emit pwmDataAdded(point);
}

void DataModel::addDataPoint(QVector<DataPoint> &history, const DataPoint &point)
{
    history.append(point);
    
    // Limite la taille de l'historique
    if (history.size() > m_maxDataPoints) {
        history.removeFirst();
    }
}

QVector<DataModel::DataPoint> DataModel::getTemperatureHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_temperatureHistory;
}

QVector<DataModel::DataPoint> DataModel::getVoltageHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_voltageHistory;
}

QVector<DataModel::DataPoint> DataModel::getPwmHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_pwmHistory;
}

void DataModel::setMaxDataPoints(int maxPoints)
{
    QMutexLocker locker(&m_mutex);
    
    if (maxPoints < 10) maxPoints = 10;
    if (maxPoints > 10000) maxPoints = 10000;
    
    m_maxDataPoints = maxPoints;
    
    // Ajuste les historiques existants
    while (m_temperatureHistory.size() > m_maxDataPoints)
        m_temperatureHistory.removeFirst();
    
    while (m_voltageHistory.size() > m_maxDataPoints)
        m_voltageHistory.removeFirst();
    
    while (m_pwmHistory.size() > m_maxDataPoints)
        m_pwmHistory.removeFirst();
    
    qDebug() << "[DataModel] Max data points set to" << m_maxDataPoints;
}

double DataModel::getTemperatureAverage() const
{
    QMutexLocker locker(&m_mutex);
    return calculateAverage(m_temperatureHistory);
}

double DataModel::getVoltageAverage() const
{
    QMutexLocker locker(&m_mutex);
    return calculateAverage(m_voltageHistory);
}

double DataModel::getTemperatureMin() const
{
    QMutexLocker locker(&m_mutex);
    return findMin(m_temperatureHistory);
}

double DataModel::getTemperatureMax() const
{
    QMutexLocker locker(&m_mutex);
    return findMax(m_temperatureHistory);
}

double DataModel::calculateAverage(const QVector<DataPoint> &data) const
{
    if (data.isEmpty())
        return 0.0;
    
    double sum = 0.0;
    for (const auto &point : data) {
        sum += point.value;
    }
    
    return sum / data.size();
}

double DataModel::findMin(const QVector<DataPoint> &data) const
{
    if (data.isEmpty())
        return 0.0;
    
    auto minIt = std::min_element(data.begin(), data.end(),
        [](const DataPoint &a, const DataPoint &b) {
            return a.value < b.value;
        });
    
    return minIt->value;
}

double DataModel::findMax(const QVector<DataPoint> &data) const
{
    if (data.isEmpty())
        return 0.0;
    
    auto maxIt = std::max_element(data.begin(), data.end(),
        [](const DataPoint &a, const DataPoint &b) {
            return a.value < b.value;
        });
    
    return maxIt->value;
}

void DataModel::clearHistory()
{
    QMutexLocker locker(&m_mutex);
    
    m_temperatureHistory.clear();
    m_voltageHistory.clear();
    m_pwmHistory.clear();
    
    emit historyCleared();
    
    qDebug() << "[DataModel] History cleared";
}
