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
    QString b64str = ba.toBase64();

    QJsonObject postJson;
    QJsonDocument  doc;

    postJson.insert("image", b64str);
    postJson.insert("image_type", "BASE64");
    postJson.insert("face_field", "age,expression,face_shape,gender,glasses,emotion,face_type,mask,beauty");

    doc.setObject(postJson);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    emit resultReady(postData, childThread);

    buffer.close();

}
