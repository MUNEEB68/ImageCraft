// photeditor.h
#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_photeditor.h"
#include <opencv2/core.hpp> // Include OpenCV core header

class photeditor : public QMainWindow
{
    Q_OBJECT

public:
    photeditor(QWidget* parent = nullptr);
    ~photeditor();

private:
    Ui::photeditorClass ui;

private slots:
    void on_upload_clicked();
    void on_resize_clicked();
    void on_cropScrollBar_valueChanged(int value); // Ensure this declaration matches the definition
    // void updateDisplayedImage(const cv::Mat& image);
	void on_Filter_button_clicked();
	void on_Brightness_button_clicked();
	void on_brightnessSlider_valueChanged(int value);
};

