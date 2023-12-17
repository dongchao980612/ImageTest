# ImageTest
## 1、摄像头调用
- 用到的类**QCamera**、**QCameraViewfinder**、**QCameraImageCapture**类
- 在**.pro**文件添加**multimedia**、**multimediawidgets**
```c++
// 设置摄像头功能
m_camera = new QCamera;
m_viewfinder = new QCameraViewfinder;
m_imageCapture = new QCameraImageCapture(m_camera);

m_viewfinder->show();
m_camera->setViewfinder(m_viewfinder);

m_camera->setCaptureMode(QCamera::CaptureStillImage); // 静态图片
m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
m_camera->start();
```
- **imageCapture**添加相应的槽函数
```c++
connect(m_imageCapture, &QCameraImageCapture::imageCaptured, this, &Widget::showCameraImage);

void Widget::showCameraImage(int id, QImage preview)
{
	ui->label->setPixmap(QPixmap::fromImage(m_img));
}
```
- 利用定时器，不断拍照
```c++
m_refreshTimer = new QTimer;
connect(m_refreshTimer, &QTimer::timeout, this, &Widget::takePicture);
m_refreshTimer->start(TIME_OUT);

void Widget::takePicture()
{
    // qDebug()<<__FILE__<<"\t"<<__FUNCTION__;
    m_imageCapture->capture();
}
```
- 摄像头选择 
```c++
 m_cameraList = QCameraInfo::availableCameras();
foreach (const QCameraInfo &cameraInfo, m_cameraList)
{
    //qDebug() << cameraInfo.description() << cameraInfo.deviceName();
    ui->comboBox->addItem(cameraInfo.description());
}

connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Widget::pickCamera);

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
```
## 2、网络模块

- 用到的类**QNetworkAccessManager**、**QNetworkRequest**、**QNetworkReply**

- 在**.pro**文件添加**network**
  
- 网络请求
    - 新建网络请求模块
    
      ```c++
      Manager = new QNetworkAccessManager(this);
      ```
    
    - 拼接url
      ```c++
      m_url = new QUrl;
      m_url->setUrl(TOKENSTR);
      
      m_query = new QUrlQuery;
      m_query->addQueryItem("client_id", FACE_CLIENT_ID);
      m_query->addQueryItem("client_secret",  FACE_CLIENT_SECRET);
      m_query->addQueryItem("grant_type", "client_credentials");
      
      m_url->setQuery(*m_query);
    	```
    
    - 配置ssl
      ```c++
      QSslSocket::supportsSsl() ? qDebug() << "支持ssl" : qDebug() << "不支持ssl";

      m_sslConfig = QSslConfiguration::defaultConfiguration();
      m_sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
      m_sslConfig.setProtocol(QSsl::TlsV1_2);
      ```
    
    - 组装请求
      ```c++
	  m_networkReq = new QNetworkRequest;
      m_networkReq->setUrl(*m_url);
      m_networkReq->setSslConfiguration(m_sslConfig);
      m_tokenManager->get(*m_networkReq);
      ```
      
    - 绑定信号
      ```c++
      connect(m_tokenManager, &QNetworkAccessManager::finished, this, &Widget::tokenReply);
      connect(m_imgManager, &QNetworkAccessManager::finished, this, &Widget::imgReply);
      ```
      
## json解析

- 用到的类**QJsonParseError**、**QJsonDocument**、**QJsonArray**、**QJsonValue**、**QJsonObject**

## 多线程

- 子线程

  ```c++
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
  
  // 创建线程
  m_childThread = new QThread(this);
  
  // 创建工人
  Worker *worker = new Worker;
  
  // 把工人送入子线程
  worker->moveToThread(m_childThread);
  
   // 启动子线程
  m_childThread->start();
  
  // 给工人发通知干活
  emit beginWork(m_img, m_childThread);
  ```


