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

    connect(m_imageCapture, &QCameraImageCapture::imageCaptured, this, &Widget::showCameraImage);
    connect(ui->pushButton, &QPushButton::clicked, this, &Widget::beginFaceDetect);

    // 布局
    this->setMaximumSize(MAINWIN_H, MAINWIN_W);
    this->setMinimumSize(MAINWIN_H, MAINWIN_W);

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
    ui->label->setMinimumSize(LABEL_H, LABEL_W);
    ui->label->setMaximumSize(LABEL_H, LABEL_W);

    //利用定时器，不断拍照
    m_refreshTimer = new QTimer;
    connect(m_refreshTimer, &QTimer::timeout, this, &Widget::takePicture);
    m_refreshTimer->start(TIME_OUT);

    m_tokenManager = new QNetworkAccessManager(this);
    m_imgManager = new QNetworkAccessManager(this);
    qDebug() << m_tokenManager->supportedSchemes();

    // 拼接url
    m_url = new QUrl;
    m_url->setUrl(TOKENSTR);

    m_query = new QUrlQuery;
    m_query->addQueryItem("client_id", "hB1Sp4HeAGGBD95RKVCf9v07");
    m_query->addQueryItem("client_secret", "MpnkvHtKxP7OMyIvFRAGPyLIZOhXlRqp");
    m_query->addQueryItem("grant_type", "client_credentials");

    m_url->setQuery(*m_query);
    qDebug() << m_url->url();


    // 配置ssl
    QSslSocket::supportsSsl() ? qDebug() << "支持ssl" : qDebug() << "不支持ssl";

    m_sslConfig = QSslConfiguration::defaultConfiguration();
    m_sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    m_sslConfig.setProtocol(QSsl::TlsV1_2);

    // 组装请求
    m_networkReq = new QNetworkRequest;
    m_networkReq->setUrl(*m_url);
    m_networkReq->setSslConfiguration(m_sslConfig);

    m_tokenManager->get(*m_networkReq);
    connect(m_tokenManager, &QNetworkAccessManager::finished, this, &Widget::tokenReply);
    connect(m_imgManager, &QNetworkAccessManager::finished, this, &Widget::imgReply);

    ui->textBrowser->setText(m_access_token);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::showCameraImage(int id, QImage preview)
{
    Q_UNUSED(id);
    m_img = preview;
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
    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        return ;
    }

    // 正确应答
    QByteArray replyData = reply->readAll();
    // qDebug()<<replyData;

    // json 解析
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);
    if(jsonError.error == QJsonParseError::NoError)
    {
        // 解析成功
        QJsonObject obj = doc.object();
        if(obj.contains("access_token"))
        {
            m_access_token = obj.take("access_token").toString();
        }
        // ui->textBrowser->setText(m_access_token);
    }
    else
    {
        qDebug() << "Json error  = " << jsonError.errorString();
    }


}

void Widget::imgReply(QNetworkReply *reply)
{
    // 错误处理
    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        return ;
    }

    // 正确应答
    QByteArray replyData = reply->readAll();
    // qDebug()<<replyData;

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);

    if(jsonError.error == QJsonParseError::NoError)
    {
        QString face_info;
        // 解析成功
        // 最外层json
        QJsonObject obj = doc.object();
        if(obj.contains("result"))
        {
            //  人脸列表
            QJsonObject result_obj = obj.take("result").toObject();
            if(result_obj.contains("face_list"))
            {
                QJsonArray face_list_array = result_obj.take("face_list").toArray();

                //第一张人脸
                QJsonObject face_obj = face_list_array.at(0).toObject();

                // 年龄
                if(face_obj.contains("age"))
                {
                    double age = face_obj.take("age").toDouble();

                    face_info.append("年龄:").append(QString::number(age)).append("\r\n");
                }

                // 性别
                if(face_obj.contains("gender"))
                {
                    QJsonObject gender_obj = face_obj.take("gender").toObject();
                    if(gender_obj.contains("type"))
                    {
                        QString gender = gender_obj.take("type").toString();
                        face_info.append("性别:").append(gender).append("\r\n");
                    }
                }

                // 表情
                if(face_obj.contains("emotion"))
                {
                    QJsonObject emotion_obj = face_obj.take("emotion").toObject();
                    if(emotion_obj.contains("type"))
                    {
                        QString emotion = emotion_obj.take("type").toString();
                        face_info.append("表情:").append(emotion).append("\r\n");
                    }
                }

                // 口罩
                if(face_obj.contains("mask"))
                {
                    QJsonObject mask_obj = face_obj.take("mask").toObject();
                    if(mask_obj.contains("type"))
                    {
                        int mask = mask_obj.take("type").toInt();
                        face_info.append("口罩:").append(mask == 0 ? "没带口罩" : "带口罩").append("\r\n");
                    }
                }

                // 颜值
                if(face_obj.contains("beauty"))
                {
                    double beauty = face_obj.take("beauty").toDouble();
                    face_info.append("颜值:").append(QString::number(beauty)).append("\r\n");
                }

            }

        }
        qDebug() << face_info;
        ui->textBrowser->setText(face_info);
    }
    else
    {
        qDebug() << "Json error  = " << jsonError.errorString();
    }
}


void Widget::beginFaceDetect()
{
    //图片tobs64编码
    QByteArray ba;
    QBuffer buffer(&ba);
    //buffer.open(QIODevice::WriteOnly);
    m_img.save(&buffer, "PNG"); // writes image into ba in PNG format
    QString bs64Str = ba.toBase64();
    if(bs64Str.isEmpty())
    {
        qDebug() << "bs64Str is empty";
    }
    else
    {
        qDebug() << "bs64Str is not empty";
    }
    buffer.close();

    // 请求体body参数设置
    QJsonObject postJson;
    QJsonDocument doc;
    postJson.insert("image", bs64Str);
    postJson.insert("image_type", "BASE64");
    postJson.insert("face_field", "age,expression,face_shape,gender,glasses,eye_status,emotion,face_type,mask,beauty");

    doc.setObject(postJson);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);


    // 组装请求
    m_url->setUrl(FACEDETECTSTR);
    m_query->addQueryItem("access_token", m_access_token);
    m_url->setQuery(*m_query);
    // qDebug() << m_url->url();

    m_networkReq->setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

    m_networkReq->setUrl(*m_url);
    m_networkReq->setSslConfiguration(m_sslConfig);

    m_imgManager->post(*m_networkReq, postData);
}

