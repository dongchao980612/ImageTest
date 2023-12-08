#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    m_cameraList = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, m_cameraList)
    {
        //qDebug() << cameraInfo.description() << cameraInfo.deviceName();
        ui->comboBox->addItem(cameraInfo.description());
    }

    // void (QComboBox::*p_currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
    // connect(ui->comboBox, p_currentIndexChanged, this, &Widget::pickCamera);

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Widget::pickCamera);

    // 设置摄像头功能
    m_camera = new QCamera;
    m_viewfinder = new QCameraViewfinder;
    m_imageCapture = new QCameraImageCapture(m_camera);

    m_viewfinder->show();
    m_camera->setViewfinder(m_viewfinder);




    m_camera->setCaptureMode(QCamera::CaptureStillImage); // 静态图片
    m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    m_camera->start();

    connect(m_imageCapture, &QCameraImageCapture::imageCaptured, this, &Widget::showCameraImage);
    connect(ui->pushButton, &QPushButton::clicked, this, &Widget::preparePostData);

    // 布局
    this->setMaximumSize(MAINWIN_H, MAINWIN_W);
    this->setMinimumSize(MAINWIN_H, MAINWIN_W);

    m_hLayout = new QHBoxLayout(this);

    m_vLayout_l = new QVBoxLayout;

    m_vLayout_l->addWidget(ui->label);
    m_vLayout_l->addWidget(ui->pushButton);

    m_vLayout_r = new QVBoxLayout;
    m_vLayout_r->addWidget(ui->comboBox);
    m_vLayout_r->addWidget(m_viewfinder);
    m_vLayout_r->addWidget(ui->textBrowser);

    m_hLayout->addLayout(m_vLayout_l);
    m_hLayout->addLayout(m_vLayout_r);
    this->setLayout(m_hLayout);

    ui->label->setScaledContents(true); // 缩放
    ui->label->setMinimumSize(LABEL_H, LABEL_W);
    ui->label->setMaximumSize(LABEL_H, LABEL_W);

    // 利用定时器，不断拍照
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
    // qDebug() << m_url->url();


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
    // 绘制人脸

    QPainter painter(&m_img);
    painter.setPen(Qt::red);
    painter.drawRect(m_left, m_top, m_width, m_height);

    QFont font;
    font.setPixelSize(30);
    painter.setFont(font);

    painter.drawText(m_left + m_width + TEXTOFFSET, m_top + m_top, QString("年龄：").append(QString::number(m_age)));
    painter.drawText(m_left + m_width + TEXTOFFSET, m_top + m_top + 40, QString("性别：").append(m_gender.compare("male") ? "女" : "男"));
    painter.drawText(m_left + m_width + TEXTOFFSET, m_top + m_top + 80, QString("口罩：").append(m_mask == 0 ? "没带口罩" : "带口罩"));
    painter.drawText(m_left + m_width + TEXTOFFSET, m_top + m_top + 120, QString("颜值：").append(QString::number(m_beauty)));

    ui->label->setPixmap(QPixmap::fromImage(m_img));
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

    reply->deleteLater();

    preparePostData();
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
    // qDebug() << replyData;

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);

    if(jsonError.error == QJsonParseError::NoError)
    {
        QString face_info = "";
        // 解析成功
        // 最外层json
        QJsonObject obj = doc.object();
        if(obj.contains("result"))
        {
            // 人脸列表
            QJsonObject result_obj = obj.take("result").toObject();
            if(result_obj.contains("face_list"))
            {
                QJsonArray face_list_array = result_obj.take("face_list").toArray();

                // 第一张人脸
                QJsonObject face_obj = face_list_array.at(0).toObject();


                // 取出人脸信息
                if(face_obj.contains("location"))
                {
                    QJsonObject location_obj = face_obj.take("location").toObject();
                    if(location_obj.contains("left"))
                    {
                        m_left = location_obj.take("left").toDouble();
                    }

                    if(location_obj.contains("top"))
                    {
                        m_top = location_obj.take("top").toDouble();
                    }
                    if(location_obj.contains("width"))
                    {
                        m_width = location_obj.take("width").toDouble();
                    }
                    if(location_obj.contains("height"))
                    {
                        m_height = location_obj.take("height").toDouble();
                    }
                    // qDebug() << m_left << m_top << m_width << m_height;
                }
                // 年龄
                if(face_obj.contains("age"))
                {
                    m_age = face_obj.take("age").toDouble();

                    face_info.append("年龄:").append(QString::number(m_age)).append("\r\n");
                }

                // 性别
                if(face_obj.contains("gender"))
                {
                    QJsonObject gender_obj = face_obj.take("gender").toObject();
                    if(gender_obj.contains("type"))
                    {
                        m_gender = gender_obj.take("type").toString();
                        face_info.append("性别:").append(m_gender.compare("male") ? "女" : "男").append("\r\n");
                    }
                }

                // 表情
                if(face_obj.contains("emotion"))
                {
                    QJsonObject emotion_obj = face_obj.take("emotion").toObject();
                    if(emotion_obj.contains("type"))
                    {
                        QString emotion = emotion_obj.take("type").toString();
                        face_info.append("表情:").append(emo_map[emotion]).append("\r\n");
                    }
                }

                // 口罩
                if(face_obj.contains("mask"))
                {
                    QJsonObject mask_obj = face_obj.take("mask").toObject();
                    if(mask_obj.contains("type"))
                    {
                        m_mask = mask_obj.take("type").toInt();
                        face_info.append("口罩:").append(m_mask == 0 ? "没带口罩" : "带口罩").append("\r\n");
                    }
                }

                // 颜值
                if(face_obj.contains("beauty"))
                {
                    m_beauty = face_obj.take("beauty").toDouble();
                    face_info.append("颜值:").append(QString::number(m_beauty)).append("\r\n");
                }

            }

        }
        // qDebug() << face_info;
        ui->textBrowser->setText(face_info);
    }
    else
    {
        qDebug() << "Json error  = " << jsonError.errorString();
    }

    reply->deleteLater();
    preparePostData();
}


void Widget::beginFaceDetect(QByteArray postData)
{

    // 另一个槽函数
    // 关闭子线程
    m_childThread->exit();
    m_childThread->wait();
    // m_childThread->isFinished() ? qDebug() << "childThread finished" : qDebug() << "childThread not finished";

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

void Widget::preparePostData()
{
    // 创建线程
    m_childThread = new QThread(this);

    // 创建工人
    Worker *worker = new Worker;

    // 把工人送入子线程
    worker->moveToThread(m_childThread);

    connect(m_childThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Widget::beginWork, worker, &Worker::doWork);
    connect(worker, &Worker::resultReady, this, &Widget::beginFaceDetect);

    // 启动子线程
    m_childThread->start();

    // 给工人发通知干活
    emit beginWork(m_img);
}

void Widget::pickCamera(int index)
{
    qDebug() << m_cameraList.at(index).description();
    m_refreshTimer->stop();
    m_camera->stop();

    m_camera = new QCamera(m_cameraList.at(index));
    m_imageCapture = new QCameraImageCapture(m_camera);

    connect(m_imageCapture, &QCameraImageCapture::imageCaptured, this, &Widget::showCameraImage);


    m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    m_camera->setCaptureMode(QCamera::CaptureStillImage); // 静态图片
    m_camera->setViewfinder(m_viewfinder);
    m_camera->start();

    // m_viewfinder->show();
    m_refreshTimer->start(TIME_OUT);


}

