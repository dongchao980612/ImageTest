#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    m_game_count = 3;

    m_cameraList = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, m_cameraList)
    {
        //qDebug() << cameraInfo.description() << cameraInfo.deviceName();
        ui->comboBox->addItem(cameraInfo.description());
    }

    // void (QComboBox::*p_currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
    // connect(ui->comboBox, p_currentIndexChanged, this, &Widget::pickCamera);

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Widget::pickCamera);

    ui->label_game->setText("");

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
    // connect(ui->pushButton, &QPushButton::clicked, this, &Widget::preparePostData);
    connect(ui->pushButton, &QPushButton::clicked, this, &Widget::beginGame);

    // 布局
    this->setMaximumSize(MAINWIN_H, MAINWIN_W);
    this->setMinimumSize(MAINWIN_H, MAINWIN_W);

    m_hLayout = new QHBoxLayout(this);

    m_vLayout_l = new QVBoxLayout;

    m_vLayout_l->addWidget(ui->label_game);
    m_vLayout_l->addWidget(ui->pushButton_again);
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

    // 定时器不断进行人脸识别请求
    m_netTimer = new QTimer;
    connect(m_netTimer, &QTimer::timeout, this, &Widget::preparePostData);

    m_gameTimer = new QTimer;
    connect(m_gameTimer, &QTimer::timeout, this, &Widget::analysGame);
    m_gameTimer->start(GAME_TIME_OUT);

    connect(ui->pushButton_again, &QPushButton::clicked, this, &Widget::beginGame);

    m_tokenManager = new QNetworkAccessManager(this);
    m_imgManager = new QNetworkAccessManager(this);
    qDebug() << m_tokenManager->supportedSchemes();

    // 拼接url
    m_url = new QUrl;
    m_url->setUrl(TOKENSTR);

    m_query = new QUrlQuery;
    m_query->addQueryItem("client_id", BODY_CLIENT_ID);
    m_query->addQueryItem("client_secret",  BODY_CLIENT_SECRET);
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
    // qDebug() << "reply token...";

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
    // m_netTimer->start(NET_TIME_OUT);

    // preparePostData();
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
    qDebug() << replyData;


    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);

    if(jsonError.error == QJsonParseError::NoError)
    {
        QString face_info = "";
        // 解析成功
        // 最外层json
        QJsonObject obj = doc.object();
        if(obj.contains("result_num"))
        {

            m_u_hand = -1;
            int result_num = obj.take("result_num").toInt();
            if(obj.contains("result"))
            {
                QJsonArray result_arr = obj.take("result").toArray();
                for (int i = 0; i < result_num; i++)
                {
                    QJsonObject hand_obj = result_arr.at(i).toObject();
                    if(hand_obj.contains("classname"))
                    {
                        QString hand_class = hand_obj.take("classname").toString();
                        face_info.append(QString::number(i + 1)).append(".").append(hand_class).append(("\r\n"));
                        if(hand_class == "Two")
                        {
                            m_u_hand = 2;
                            break;
                        }
                        else if(hand_class == "Fist")
                        {
                            m_u_hand = 0;
                            break;
                        }
                        else if(hand_class == "Five")
                        {
                            m_u_hand = 5;
                            break;
                        }

                    }
                }
                if(m_u_hand == -1)
                {
                    m_game_str = "识别失败，请重试";
                    ui->label_game->setText(m_game_str);
                }
                else
                {
                    int res = (m_u_hand + 3 - m_com_hand) % 3;
                    m_game_str.clear();
                    m_game_str.append("电脑出拳:")
                              .append(m_com_hand == 0 ? "石头" : (m_com_hand == 1 ? "剪刀" : "布"))
                              .append("\t你的出拳:").append(m_u_hand == 0 ? "石头" : (m_u_hand == 1 ? "剪刀" : "布"))
                              .append("\t最终结果").append(res == 0 ? "平局" : (res == 1 ? "输了" : "赢了"));
                    ui->label_game->setText(m_game_str);
                }
                ui->textBrowser->setText(face_info);
            }
        }
    }
    else
    {
        qDebug() << "Json error  = " << jsonError.errorString();
    }
    reply->deleteLater();

// preparePostData();
}


void Widget::beginFaceDetect(QByteArray postData, QThread *childThread)
{

    // 另一个槽函数
    // 关闭子线程
    childThread->exit();
    childThread->wait();
    // childThread->isFinished() ? qDebug() << "childThread finished" : qDebug() << "childThread not finished";

    // 组装请求
    m_url->setUrl(BODY_DETECT_URL);
    m_query->addQueryItem("access_token", m_access_token);
    m_url->setQuery(*m_query);
    // qDebug() << m_url->url();

    m_networkReq->setHeader(QNetworkRequest::ContentTypeHeader, QVariant(BODY_CONTENT_TYPE_HEADER));

    m_networkReq->setUrl(*m_url);
    m_networkReq->setSslConfiguration(m_sslConfig);
    // qDebug() << "post img data...";
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
    emit beginWork(m_img, m_childThread);
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

void Widget::beginGame()
{

    m_game_str = "准备出拳";
    ui->label_game->setText(m_game_str);
    ui->pushButton_again->setEnabled(false);
    m_gameTimer->start(GAME_TIME_OUT);


}

void Widget::analysGame()
{

    if(--m_game_count > 0)
    {
        m_game_str = "准备出拳" + QString::number(m_game_count);
        ui->label_game->setText(m_game_str);
    }
    else
    {
        m_com_hand = QRandomGenerator::global()->generate() % 3;
        m_gameTimer->stop();
        ui->pushButton_again->setEnabled(true);
        m_game_count = 3;
        m_game_str.clear();
        m_game_str.append("电脑出拳:").append(m_com_hand == 0 ? "石头" : (m_com_hand == 1 ? "剪刀" : "布")).append("\t你的出拳结果，分析中...");
        ui->label_game->setText(m_game_str);
        preparePostData();
    }

}

