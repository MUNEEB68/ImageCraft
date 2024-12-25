// photeditor.cpp
#include "photeditor.h"
#include "ui_photeditor.h"
//#include "photeditor.ui"
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
//gloa
Mat universal_image_for_resize;
Mat universal_image_for_brightness;
Mat universal_image_for_contrast;
 int current_image_width = 0;
 int current_image_height = 0;
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
class Image_Filters {
public:
    Mat brightness_adjustment(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust brightness.");
        }
        else {
            Mat return_image;
            img.convertTo(return_image, -1, 1, value);
            return return_image;
        }
    }
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
Mat universal_image_for_filter;
//int count_resize;
// Constructor
photeditor::photeditor(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);
    ui.VERTICALSCROLLBAR->setVisible(false);
	ui.Brightness_button->setVisible(false);
	ui.Brightness_Slider->setVisible(false);
    connect(ui.VERTICALSCROLLBAR, &QScrollBar::valueChanged, this,
        &photeditor::on_cropScrollBar_valueChanged);
    connect(ui.Brightness_Slider, &QScrollBar::valueChanged, this, &photeditor::on_brightnessSlider_valueChanged);
    
}


// Destructor
photeditor::~photeditor() {}

void photeditor::on_brightnessSlider_valueChanged(int value) {
    try {
        Image_Filters obj1;
        Mat bright_image;

        // Adjust brightness using the slider value
        bright_image = obj1.brightness_adjustment(universal_image_for_brightness, value);

        // Convert the brightened cv::Mat to QImage
        QImage brightenedQImage((const uchar*)bright_image.data, bright_image.cols,
            bright_image.rows, bright_image.step, QImage::Format_BGR888);

        // Display the brightened image in the QLabel
        universal_image = bright_image;
        ui.uploaded_pic->setPixmap(
            QPixmap::fromImage(brightenedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
		
        // Optional: Log brightness value to a file
        ofstream outFile("brightness.txt", ios::app);
        if (outFile.is_open()) {
            outFile << "Brightness value: " << value << endl;
            outFile.close();
        }
        else {
            cout << "Error: Could not open the brightness log file." << endl;
        }
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
    
}



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
    ofstream outFile("check.txt", ios::app);
    if (outFile.is_open()) {
        // Write the crop vae and resized image dimensions to the file
        outFile << "universal_image_dimensions at " << crop_value << endl;

        outFile << "universal_image_dimensions " << universal_image.rows << "x"
            << universal_image.cols << endl;
        outFile << "universal_image_for_resize_dimensions" << universal_image_for_resize.rows << "x" << universal_image_for_resize.cols << endl;

        outFile << "Resized_image dimensions" << resizedImg.rows << "x" << resizedImg.cols
            << endl;

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
	current_image_width = labelw;
	current_image_height = labelh;
   

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
                imageData.step, QImage::Format_BGR888);

            // Display the image in the QLabel
            QPixmap pixmap = QPixmap::fromImage(qImage);

            int width = ui.uploaded_pic->width();
            int height = ui.uploaded_pic->height();
            ui.uploaded_pic->setPixmap(
                pixmap.scaled(width, height, Qt::KeepAspectRatio));
			current_image_width = width;
			current_image_height = height;
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
        ui.Brightness_Slider->setVisible(false);
       
        universal_image_for_resize = universal_image;




    }
    else {
        QMessageBox::warning(this, tr("Error"),
            tr("No image loaded. Please upload an image first."));
    }
}
void photeditor::on_Filter_button_clicked() {
    // Ensure the vertical scrollbar is hidden when the filter button is clicked
    cout << "Filter button clicked. Hiding VERTICALSCROLLBAR." << endl;

    ui.VERTICALSCROLLBAR->setVisible(false);
    ui.Brightness_button->setVisible(true);
   // QMessageBox::warning(this, tr("Error"), tr("button clicked"));

    if (Imag1.isImageLoaded()) {
        cout << "Image is loaded. Preparing for filtering." << endl;

        // Update the universal image for filter operations
        universal_image_for_filter = universal_image;
    }
    else {
        cout << "No image loaded. Showing warning to the user." << endl;

        QMessageBox::warning(this, tr("Error"),
            tr("No image loaded. Please upload an image first."));
    }
}
void photeditor::on_Brightness_button_clicked() {
	if (Imag1.isImageLoaded()) {
		cout << "Brightness button clicked. Showing brightness slider." << endl;
        ui.Brightness_Slider->setVisible(true);
        ui.VERTICALSCROLLBAR->setVisible(false);
		universal_image_for_brightness = universal_image;

	}
	else {
		QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
	}
   
}
