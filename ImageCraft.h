// ImageCraft.h
#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ImageCraft.h"
#include <opencv2/core.hpp> // Include OpenCV core header
#include <QMouseEvent>
#include <QPainter>

class ImageCraft : public QMainWindow
{
    Q_OBJECT

public:
    ImageCraft(QWidget* parent = nullptr);
    ~ImageCraft();

private:
    Ui::ImageCraftClass ui;


private slots:
    void on_Import_Image_clicked();
    void on_Export_Image_clicked();


    void on_Resize_Button_clicked();
    void on_Resize_Slider_valueChanged(int value);

    void on_Crop_Button_clicked();

    void on_Rotate_Button_clicked();
    void on_rotatecw_clicked();
    void on_rotateacw_clicked();

    void on_Flip_Button_clicked();
    void on_vertflip_clicked();
    void on_horiflip_clicked();

    void on_AddText_Button_clicked();


    void on_Brightness_Button_clicked();
    void on_Brightness_Slider_valueChanged(int value);

    void on_Contrast_Button_clicked();
    void on_Contrast_Slider_valueChanged(int value);

    void on_Blur_Button_clicked();
    void on_Blur_Slider_valueChanged(int value);


    void on_Filter_ComboBox_currentIndexChanged(int index);
    void on_Color_ComboBox_currentIndexChanged(int index);


    void on_Reset_Button_clicked();


    void hideSliders();

    QImage MatToQImage(const cv::Mat& mat);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
};
