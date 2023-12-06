#ifndef WIDGET_H
#define WIDGET_H

#define MAINWIN_H 1200
#define MAINWIN_W 700

#define LABEL_H 900
#define LABEL_W 500

#define TIME_OUT 10

#define FACEDETECTSTR "https://aip.baidubce.com/rest/2.0/face/v3/detect"
#define TOKENSTR "https://aip.baidubce.com/oauth/2.0/token"


#include <QWidget>
#include <QDebug>

#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>


#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QTimer>

#include <QNetworkAccessManager>
#include <QUrl>
#include <QUrlQuery>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>


#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>


#include <QImage>
#include <QBuffer>

#include <QVariant>

#include <QMap>
#include <QThread>

#include <QPainter>

#include "worker.h"

const QMap<QString, QString> emo_map
{
    {"angry", "愤怒"},
    {"disgust", "厌恶"},
    {"fear", "恐惧"},
    {"happy", "高兴"},
    {"angry", "伤心"},
    {"surprise", "惊讶"},
    {"neutral", "无表情"},
    {"happy", "撅嘴"},
    {"grimace", "鬼脸"}
};


QT_BEGIN_NAMESPACE
namespace Ui
{
    class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
public slots:
    void showCameraImage(int id, QImage preview);
    void takePicture();
    void tokenReply(QNetworkReply *reply);
    void imgReply(QNetworkReply *reply);
    void beginFaceDetect(QByteArray postData);
    void preparePostData();
private:
    Ui::Widget *ui;

    QCamera *m_camera;
    QCameraViewfinder *m_viewfinder;
    QCameraImageCapture *m_imageCapture;

    QHBoxLayout *m_hLayout;
    QVBoxLayout *m_vLayout_l, *m_vLayout_r;

    QTimer *m_refreshTimer;
    QNetworkAccessManager *m_tokenManager;
    QNetworkAccessManager *m_imgManager;
    QUrl *m_url;
    QUrlQuery *m_query;
    QSslConfiguration m_sslConfig;
    QNetworkRequest *m_networkReq;
    QString m_access_token = "";
    QImage m_img;

    QThread *m_childThread;

    double m_left, m_top, m_width, m_height;


signals:
    void beginWork(QImage img);
};
#endif // WIDGET_H
