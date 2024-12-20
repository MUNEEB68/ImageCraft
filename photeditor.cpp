// photeditor.cpp
#include "photeditor.h"
#include "ui_photeditor.h"
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QScrollBar>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

using namespace std;
using namespace cv;
Mat universal_image_for_resize;

// Image Class Definition
class Image {
private:
    Mat img; // Holds the image data

public:
    // Load an image from the specified path
    void loadImage(const string& path) {
        img = imread(path);
        if (img.empty()) {
            cout << "Error: Could not load the image from " << path << endl;
        }
        else {
            cout << "Image loaded successfully from " << path << endl;
        }
    }

    // Retrieve the loaded image data
    Mat getImageData() const { return img; }

    // Check if an image is loaded
    bool isImageLoaded() const { return !img.empty(); }

    friend class Image_operations;
};

class Image_operations {
public:
    Mat resizeImage(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot resize.");
        }
        // Calculate the scaling factor
        double scale = value / 100.0;
        Mat resizedImg;
        resize(img, resizedImg, Size(), scale, scale);
        return resizedImg;
    }
};

// Global Image Object
Image Imag1;
//Mat universal_imag2;
Image_operations imageOps;
Mat universal_image;
//int count_resize;
// Constructor
photeditor::photeditor(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);
    ui.VERTICALSCROLLBAR->setVisible(false);
    connect(ui.VERTICALSCROLLBAR, &QScrollBar::valueChanged, this,
        &photeditor::on_cropScrollBar_valueChanged);
}

// Destructor
photeditor::~photeditor() {}

void photeditor::on_cropScrollBar_valueChanged(int value) {
    // Get the current value of the vertical scrollbar, representing the crop
    // value
    int crop_value = ui.VERTICALSCROLLBAR->value();
 //   Mat  universal_imag2;
   // universal_imag2 = universal_image;
   
    // Resize the image using the crop value as the scaling factor (percentage)
    // This calls the resizeImage function defined in Image_operations
    Mat resizedImg = imageOps.resizeImage(universal_image_for_resize, crop_value);

    // Open a file named "Text.txt" in append mode to log the crop value and
    // resized image dimensions
    ofstream outFile("Text.txt", ios::app);
    if (outFile.is_open()) {
        // Write the crop value and resized image dimensions to the file
        outFile << "Crop value: " << crop_value << endl;
        outFile << "Resized image dimensions: " << resizedImg.rows << "x"
            << resizedImg.cols << endl;

        // Close the file after writing
        outFile.close();
    }
    else {
        // If the file could not be opened, print an error message to the console
        cout << "Error: Could not open the file." << endl;
    }

    // Convert the crop value into a floating-point scaling factor
    float val = crop_value / 100.0;

    // Convert the resized image to QImage format for display in the GUI
    QImage img_edited((const uchar*)resizedImg.data, resizedImg.cols,
        resizedImg.rows, resizedImg.step, QImage::Format_BGR888);

    // Calculate the new dimensions of the QLabel based on the scaling factor
    int labelw = ui.uploaded_pic->width() * val;
    int labelh = ui.uploaded_pic->height() * val;

    // Set the QLabel (uploaded_pic) to display the resized image, scaling it to
    // fit the calculated dimensions
    ui.uploaded_pic->setPixmap(
        QPixmap::fromImage(img_edited).scaled(labelw, labelh));

    // Ensure the universal_image object is updated for further use if necessary
    // (placeholder, as the assignment is missing)
   universal_image = resizedImg;
	//universal_imag2 = resizedImg;
}

void photeditor::on_upload_clicked() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Image"), ".",
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    if (!path.isEmpty()) {
        Imag1.loadImage(path.toStdString());

        if (Imag1.isImageLoaded()) {
            // Show a message box to the user
            QMessageBox::information(this, tr("Success"),
                tr("Image uploaded successfully!"));

            // Optionally update the status bar
            statusBar()->showMessage(tr("Image uploaded successfully!"), 3000);

            // Access the loaded image data
            Mat imageData = Imag1.getImageData();
            universal_image = imageData;
            cout << "Image dimensions: " << imageData.rows << "x" << imageData.cols
                << endl;

            // Convert cv::Mat to QImage
            QImage qImage(imageData.data, imageData.cols, imageData.rows,
                imageData.step, QImage  ::Format_BGR888);

            // Display the image in the QLabel
            QPixmap pixmap = QPixmap::fromImage(qImage);

            int width = ui.uploaded_pic->width();
            int height = ui.uploaded_pic->height();
            ui.uploaded_pic->setPixmap(
                pixmap.scaled(width, height, Qt::KeepAspectRatio));
        }
        else {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to load the selected image."));
        }
    }
    else {
        cout << "No file selected." << endl;
    }
}
void photeditor::on_resize_clicked() {
    if (Imag1.isImageLoaded()) {
        ui.VERTICALSCROLLBAR->setVisible(true);
		universal_image_for_resize = universal_image;
	

        

    }
    else {
        QMessageBox::warning(this, tr("Error"),
            tr("No image loaded. Please upload an image first."));
    }
}
