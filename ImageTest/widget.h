#ifndef WIDGET_H
#define WIDGET_H

#define MAINWIN_H 1200
#define MAINWIN_W 700
#define LABEL_H 900
#define LABEL_W 500

#include <QWidget>
#include <QDebug>

#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>


#include<QHBoxLayout>
#include<QVBoxLayout>


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
private:
    Ui::Widget *ui;

    QCamera *m_camera;
    QCameraViewfinder* m_viewfinder;
    QCameraImageCapture* m_imageCapture;

    QHBoxLayout* m_hLayout;
    QVBoxLayout* m_vLayout_l,*m_vLayout_r;
};
#endif // WIDGET_H
