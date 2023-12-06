#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //设置摄像头功能
    m_camera = new QCamera;
    m_viewfinder = new QCameraViewfinder;
    m_imageCapture = new QCameraImageCapture(m_camera);


    m_viewfinder->show();
    m_camera->setViewfinder(m_viewfinder);


    m_camera->setCaptureMode(QCamera::CaptureStillImage); //静态图片
    m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
    m_camera->start();

    connect(m_imageCapture,&QCameraImageCapture::imageCaptured,this,&Widget::showCameraImage);
    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::takePicture);

    // 布局
    this->setMaximumSize(MAINWIN_H,MAINWIN_W);
    this->setMinimumSize(MAINWIN_H,MAINWIN_W);

    m_hLayout = new QHBoxLayout(this);

    m_vLayout_l = new QVBoxLayout;
    m_vLayout_l->addWidget(ui->label);
    m_vLayout_l->addWidget(ui->pushButton);

    m_vLayout_r = new QVBoxLayout;
    m_vLayout_r->addWidget(m_viewfinder);
    m_vLayout_r->addWidget(ui->textBrowser);

    m_hLayout->addLayout(m_vLayout_l);
    m_hLayout->addLayout(m_vLayout_r);
    this->setLayout(m_hLayout);

    ui->label->setScaledContents(true); //缩放
    ui->label->setMinimumSize(LABEL_H,LABEL_W);
    ui->label->setMaximumSize(LABEL_H,LABEL_W);

    //利用定时器，不断拍照
    m_refreshTimer = new QTimer;
    connect(m_refreshTimer,&QTimer::timeout,this,&Widget::takePicture);
    m_refreshTimer->start(TIME_OUT);

    m_tokenManager=new QNetworkAccessManager(this);
    qDebug()<<m_tokenManager->supportedSchemes();

    // 拼接url
    m_url = new QUrl;
    m_url->setUrl("https://aip.baidubce.com/oauth/2.0/token");

    m_query = new QUrlQuery;
    m_query->addQueryItem("client_id","hB1Sp4HeAGGBD95RKVCf9v07");
    m_query->addQueryItem("client_secret","MpnkvHtKxP7OMyIvFRAGPyLIZOhXlRqp");
    m_query->addQueryItem("grant_type","client_credentials");

    m_url->setQuery(*m_query);
    qDebug()<<m_url->url();


    // 配置ssl
    QSslSocket::supportsSsl()?qDebug()<<"支持ssl":qDebug()<<"不支持ssl";

    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2);

    // 组装请求
    m_networkReq = new QNetworkRequest;
    m_networkReq->setUrl(*m_url);
    m_networkReq->setSslConfiguration(sslConfig);

    m_tokenManager->get(*m_networkReq);
    connect(m_tokenManager,&QNetworkAccessManager::finished,this,&Widget::tokenReply);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::showCameraImage(int id, QImage preview)
{
    Q_UNUSED(id);
    ui->label->setPixmap(QPixmap::fromImage(preview));
}

void Widget::takePicture()
{
    // qDebug()<<__FILE__<<"\t"<<__FUNCTION__;
    m_imageCapture->capture();
}

void Widget::tokenReply(QNetworkReply *reply)
{
    // 错误处理
    if(reply->error() != QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return ;
    }

    // 正确应答
    replyData = reply->readAll();
    // qDebug()<<replyData;

    // json 解析
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(replyData,&jsonError);
    if(jsonError.error == QJsonParseError::NoError){
        // 解析成功
        QJsonObject obj = doc.object();
        if(obj.contains("access_token")){
               m_access_token=obj.take("access_token").toString();
        }
        ui->textBrowser->setText(m_access_token);
    }else{
        qDebug()<<"Json error  = "<<jsonError.errorString();
    }


}

