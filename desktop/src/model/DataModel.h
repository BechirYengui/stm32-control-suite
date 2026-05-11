#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QObject>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QMutex>

/**
 * @brief Modèle de données avec historique temporel
 * 
 * Gère l'historique des mesures pour l'affichage de graphiques temps réel.
 * Utilise un mutex pour la sécurité thread (accès depuis SerialWorker).
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
    
    // Ajout de données
    void addTemperaturePoint(float temperature);
    void addVoltagePoint(float voltage);
    void addPwmPoint(uint8_t pwmDuty);
    
    // Récupération des données (thread-safe)
    QVector<DataPoint> getTemperatureHistory() const;
    QVector<DataPoint> getVoltageHistory() const;
    QVector<DataPoint> getPwmHistory() const;
    
    // Configuration
    void setMaxDataPoints(int maxPoints);
    int maxDataPoints() const { return m_maxDataPoints; }
    
    // Statistiques
    double getTemperatureAverage() const;
    double getVoltageAverage() const;
    double getTemperatureMin() const;
    double getTemperatureMax() const;
    
    // Gestion
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
    mutable QMutex m_mutex;  // Protection pour accès multi-thread
};

#endif // DATAMODEL_H
