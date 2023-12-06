#ifndef WIDGET_H
#define WIDGET_H

#define MAINWIN_H 1200
#define MAINWIN_W 700

#define LABEL_H 900
#define LABEL_W 500

#define TIME_OUT 10

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

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
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
private:
    Ui::Widget *ui;

    QCamera *m_camera;
    QCameraViewfinder* m_viewfinder;
    QCameraImageCapture* m_imageCapture;

    QHBoxLayout* m_hLayout;
    QVBoxLayout* m_vLayout_l,*m_vLayout_r;

    QTimer* m_refreshTimer;
    QNetworkAccessManager* m_tokenManager;
    QUrl* m_url;
    QUrlQuery* m_query;
    QSslConfiguration sslConfig;
    QNetworkRequest* m_networkReq;
    QByteArray replyData;
    QString m_access_token;


};
#endif // WIDGET_H
