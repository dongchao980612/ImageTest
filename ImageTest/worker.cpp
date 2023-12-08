#include "worker.h"

Worker::Worker( QObject *parent) : QObject(parent)
{

}

void Worker::doWork(QImage img, QThread *childThread)
{
    //图片tobs64编码
    QByteArray ba;
    QBuffer buffer(&ba);
    //buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG"); // writes image into ba in PNG format
    QByteArray postData("image=") ;
    postData.append(ba.toBase64().toPercentEncoding());

    emit resultReady(postData, childThread);

    buffer.close();

}
