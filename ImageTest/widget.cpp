#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //设置摄像头功能
    m_camera=new QCamera;
    m_viewfinder=new QCameraViewfinder;
    m_imageCapture = new QCameraImageCapture(m_camera);


    m_viewfinder->show();
    m_camera->setViewfinder(m_viewfinder);


    m_camera->setCaptureMode(QCamera::CaptureStillImage);
    m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
    m_camera->start();

    connect(m_imageCapture,&QCameraImageCapture::imageCaptured,this,&Widget::showCameraImage);
    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::takePicture);

    // 布局
    this->setMaximumSize(MAINWIN_H,MAINWIN_W);
    this->setMinimumSize(MAINWIN_H,MAINWIN_W);

    m_hLayout=new QHBoxLayout(this);

    m_vLayout_l=new QVBoxLayout;
    m_vLayout_l->addWidget(ui->label);
    m_vLayout_l->addWidget(ui->pushButton);

    m_vLayout_r=new QVBoxLayout;
    m_vLayout_r->addWidget(m_viewfinder);
    m_vLayout_r->addWidget(ui->textBrowser);

    m_hLayout->addLayout(m_vLayout_l);
    m_hLayout->addLayout(m_vLayout_r);
    this->setLayout(m_hLayout);

    ui->label->setScaledContents(true); //缩放
    ui->label->setMinimumSize(LABEL_H,LABEL_W);
    ui->label->setMaximumSize(LABEL_H,LABEL_W);

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
    m_imageCapture->capture();
}

