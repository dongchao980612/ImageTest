#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QBuffer>
#include <QImage>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker( QObject *parent = nullptr);

signals:
    void resultReady(QByteArray postDatam, QThread *childThread);
public slots:
    void doWork(QImage img, QThread *childThread);

};

#endif // WORKER_H
