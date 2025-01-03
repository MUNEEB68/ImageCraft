// ImageCraft.cpp
#include "ImageCraft.h"
#include "ui_ImageCraft.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QFontDialog>
#include <QColorDialog>
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
Mat universal_image_for_brightness;
Mat universal_image_for_contrast;
Mat universal_image_for_blur;
Mat universal_image_for_crop;

Mat original_image;		// Holds the original image data

QPoint startPoint;     // Starting point of the mouse drag
QPoint endPoint;       // Ending point of the mouse drag
bool isDragging = false; // Flag to track if cropping is in progress
QRect selectionRect;	// Rectangle to display the selection area

int current_image_width = 0;
int current_image_height = 0;


class Image {
private:
    Mat img;    // holds image data

public:
    void loadImage(const string& path) {    // load an image from the specified path
        img = imread(path);
        if (img.empty()) {
            cout << "Error: Could not load the image from " << path << endl;
        }
        else {
            cout << "Image loaded successfully from " << path << endl;
        }
    }
    Mat getImageData() const { return img; } // retrieve image data
    bool isImageLoaded() const { return !img.empty(); }  // check if an image is loaded
};
class Image_Filters {       // handles filter and enhancements
public:
    Mat brightness_adjustment(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust brightness.");
        }
        else {
            Mat return_image;
            img.convertTo(return_image, -1, 1, value);      // data type, scaling factor contrast, brightness offset
            return return_image;
        }
    }
    Mat contrast_adjustment(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust contrast.");
        }
        else {
            Mat return_image;
            double alpha = 1 + (value / 100.0);     // maps slider value (-100 to 100) to a scaling factor (0-2)
            img.convertTo(return_image, -1, alpha, 0); // alpha is the contrast scaling factor, 0 is the brightness offset
            return return_image;
        }
    }
    Mat blur_adjustment(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust sharpness.");
        }
        else {
            Mat blur_image;
            if (value > 0) {        // blur
                int kernel_size = value * 2 + 1;        // size must be odd
                GaussianBlur(img, blur_image, Size(kernel_size, kernel_size), 0); //input, output, dimension, standard deviation
            }
            else if (value < 0) {       // sharpen
                float k = abs(value) / 50;              // scaling factor for intensity
                Mat kernel = (Mat_<float>(3, 3) <<
                    0, -k, 0,
                    -k, 1 + 4 * k, -k, // central pixel and neighbouring pixels to enhance edge
                    0, -k, 0);
                filter2D(img, blur_image, -1, kernel);      // applies kernel to output image
            }
            return blur_image;
        }
    }
    Mat gray_filter(Mat& img) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust sharpness.");
        }
        else {
            Mat gray_image;
            cvtColor(img, gray_image, COLOR_BGR2GRAY);      // converts RGB to grayscale
            return gray_image;
        }
    }
    Mat sepia_filter(Mat& img) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot adjust sharpness.");
        }
        else {
            Mat sepia_image;
            Mat kernel = (cv::Mat_<float>(3, 3) <<
                0.272, 0.534, 0.131,
                0.349, 0.686, 0.168,                // transformation matrix for sepia effect
                0.393, 0.769, 0.189);
            transform(img, sepia_image, kernel);        // applies transformation to input image
            return sepia_image;
        }
    }
    Mat color_inversion(Mat& img) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot invert colors.");
        }
        Mat inverted_img;
        if (img.channels() == 1 || img.channels() == 3 || img.channels() == 4) {    // throw error if channels not allowed
            bitwise_not(img, inverted_img);         // converts each pixel to its inverse i.e, 255 to 0, 0 to 255
        }
        else {
            throw runtime_error("Invalid number of channels in the image.");
        }

        if (inverted_img.empty()) {
            throw std::runtime_error("Inverted image is empty.");
        }
        return inverted_img;
    }

    Mat color_isolation(Mat& img, int color) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot isolate color.");
        }
        Mat hsv;
        cvtColor(img, hsv, COLOR_BGR2HSV);      // hsv format more suitable for color based operations

        Mat mask;

        if (color == 0) {
            // Red can wrap around the hue values in HSV, so we need to consider two ranges
            Mat lower_red_mask, upper_red_mask;
            inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), lower_red_mask);
            inRange(hsv, Scalar(170, 100, 100), Scalar(180, 255, 255), upper_red_mask);
            mask = lower_red_mask | upper_red_mask;
        }
        else if (color == 1) { // Green
            inRange(hsv, Scalar(35, 50, 50), Scalar(85, 255, 255), mask);
        }
        else if (color == 2) { // Blue
            inRange(hsv, Scalar(100, 150, 80), Scalar(140, 255, 255), mask);
        }
        else if (color == 3) { // Yellow
            inRange(hsv, Scalar(20, 150, 150), Scalar(30, 255, 255), mask);
        }
        else {
            throw std::invalid_argument("Invalid color value. Use 0 for Red, 1 for Green, or 2 for Blue.");
        }

        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);        // convert image to gray
        Mat gray_bgr;
        cvtColor(gray, gray_bgr, COLOR_GRAY2BGR);   // convert the grayscale image back to BGR to fix channel issue

        Mat result = gray_bgr.clone(); // Create the final image by combining color and grayscale images using the mask
        img.copyTo(result, mask);        // Copy the color 

        return result;
    }
};
class Image_Operations {    // handles general operations
public:
    Mat resizeImage(Mat& img, int value) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot resize.");
        }
        double scale = value / 100.0;       // Calculate the scaling factor as a number from 0 to 1
        Mat resizedImg;
        resize(img, resizedImg, Size(), scale, scale);
        return resizedImg;
    }
    Mat rotateimage(Mat& img, int state) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot rotate.");
        }
        Mat rotatedImg;
        if (state == 1) {   // clockwise state
            rotate(img, rotatedImg, ROTATE_90_CLOCKWISE);
        }
        else if (state == -1) {     // anticlockwise state
            rotate(img, rotatedImg, ROTATE_90_COUNTERCLOCKWISE);
        }
        return rotatedImg;
    }
    Mat flipimage(Mat& img, int state) {
        if (img.empty()) {
            throw std::runtime_error("Image is empty, cannot flip.");
        }
        Mat flippedImg;
        if (state == -1) {
            flip(img, flippedImg, 0);       // flip vertically
        }
        else if (state == 1) {
            flip(img, flippedImg, 1);       // flip horizontally
        }
        return flippedImg;
    }
};

Image Imag1;        // universal object of the image class
Mat universal_image;        // universal image

void ImageCraft::hideSliders() {    // hide sliders and buttons when not needed
    ui.Brightness_Slider->setVisible(false);
    ui.Resize_Slider->setVisible(false);
    ui.Contrast_Slider->setVisible(false);
    ui.Blur_Slider->setVisible(false);
    ui.rotatecw->setVisible(false);
    ui.rotateacw->setVisible(false);
    ui.vertflip->setVisible(false);
    ui.horiflip->setVisible(false);
}

ImageCraft::ImageCraft(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);       // setup ui from .ui file
    hideSliders();

    // set icons for visual appeal
    ui.rotatecw->setIcon(QIcon("cw.png"));
    ui.rotateacw->setIcon(QIcon("anticw.png"));
    ui.vertflip->setIcon(QIcon("vertflip.png"));
    ui.horiflip->setIcon(QIcon("horiflip.png"));

    // connect sliders with their respective handlers
    connect(ui.Contrast_Slider, &QScrollBar::valueChanged, this, &ImageCraft::on_Contrast_Slider_valueChanged);
    connect(ui.Filter_ComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageCraft::on_Filter_ComboBox_currentIndexChanged);
    connect(ui.Color_ComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageCraft::on_Color_ComboBox_currentIndexChanged);
    connect(ui.Resize_Slider, &QScrollBar::valueChanged, this, &ImageCraft::on_Resize_Slider_valueChanged);
    connect(ui.Brightness_Slider, &QScrollBar::valueChanged, this, &ImageCraft::on_Brightness_Slider_valueChanged);
    connect(ui.AddText_Button, &QPushButton::clicked, this, &ImageCraft::on_AddText_Button_clicked);
}
ImageCraft::~ImageCraft() {}

void ImageCraft::on_Import_Image_clicked() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open Image"), ".", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));     // open file dialog to select image file

    if (!path.isEmpty()) {
        Imag1.loadImage(path.toStdString());    // load image using Imag1 object

        if (Imag1.isImageLoaded()) {
            QMessageBox::information(this, tr("Success"), tr("Image uploaded successfully!")); // Show a message box to the user

            Mat imageData = Imag1.getImageData();   // Access the loaded image data
            universal_image = imageData;            // store as global variable for further operations
            original_image = imageData.clone();
            cout << "Image dimensions: " << imageData.rows << "x" << imageData.cols << endl;

            // Convert Mat to QImage
            QImage qImage(imageData.data, imageData.cols, imageData.rows, imageData.step, QImage::Format_BGR888);

            // create a Qpixmap from QImage for displaying in the label
            QPixmap pixmap = QPixmap::fromImage(qImage);

            // resize pixmap to fit the qlabel while maintaining the aspect ratio
            int width = ui.uploaded_pic->width();
            int height = ui.uploaded_pic->height();
            ui.uploaded_pic->setPixmap(pixmap.scaled(width, height, Qt::KeepAspectRatio));

            // update dimensions for later use
            current_image_width = width;
            current_image_height = height;
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load the selected image."));
        }
    }
    else {
        cout << "No file selected." << endl;
    }
}
void ImageCraft::on_Export_Image_clicked() {
    if (Imag1.isImageLoaded()) {
        QString path = QFileDialog::getSaveFileName(this, tr("Save Image"), ".", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));     // open file dialog to select a save location and file name
        if (!path.isEmpty()) {
            try {
                imwrite(path.toStdString(), universal_image);   // save the current processed image to specified path
                QMessageBox::information(this, tr("Success"), tr("Image exported successfully!"));
            }
            catch (const std::exception& e) {
                QMessageBox::warning(this, tr("Error"), tr(e.what()));
            }
        }
        else {
            cout << "No file selected." << endl;
        }
    }
    else {
        QMessageBox::warning(this, tr("Error"),
            tr("No image loaded. Please upload an image first."));
    }
}


void ImageCraft::on_Resize_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        hideSliders();
        ui.Resize_Slider->setVisible(true);
        universal_image_for_resize = universal_image;
    }
    else {
        QMessageBox::warning(this, tr("Error"),
            tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_Resize_Slider_valueChanged(int value) {
    Image_Operations imageOps;
    Mat resizedImg = imageOps.resizeImage(universal_image_for_resize, value);   // resize universal image based on slider value

    cout << "Error: Could not open the file." << endl;
    float val = value / 100.0;  // scale factor based on slider value

    QImage img_edited((const uchar*)resizedImg.data, resizedImg.cols, resizedImg.rows, resizedImg.step, QImage::Format_BGR888);
    // calculate new dimensions for QLabel based on scale factor
    int labelw = ui.uploaded_pic->width() * val;
    int labelh = ui.uploaded_pic->height() * val;

    current_image_width = labelw;
    current_image_height = labelh;

    // display resized image in qlabel, scaling to fit updated dimension
    ui.uploaded_pic->setPixmap(QPixmap::fromImage(img_edited).scaled(labelw, labelh));
    universal_image = resizedImg;
}

void ImageCraft::on_Rotate_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        cout << "Brightness button clicked. Showing brightness slider." << endl;
        hideSliders();
        ui.rotatecw->setVisible(true);
        ui.rotateacw->setVisible(true);
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_rotatecw_clicked() {
    try {
        Image_Operations imageops;
        Mat rotated_image;
        rotated_image = imageops.rotateimage(universal_image, 1);       //clockwise rotation
        QImage rotatedQImage((const uchar*)rotated_image.data, rotated_image.cols, rotated_image.rows, rotated_image.step, QImage::Format_BGR888);
        universal_image = rotated_image;
        ui.uploaded_pic->setPixmap(QPixmap::fromImage(rotatedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
}
void ImageCraft::on_rotateacw_clicked() {
    try {
        Image_Operations imageops;
        Mat rotated_image;
        rotated_image = imageops.rotateimage(universal_image, -1);      // anticlockwise rotation
        QImage rotatedQImage((const uchar*)rotated_image.data, rotated_image.cols, rotated_image.rows, rotated_image.step, QImage::Format_BGR888);
        universal_image = rotated_image;
        ui.uploaded_pic->setPixmap(QPixmap::fromImage(rotatedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
}

void ImageCraft::on_Flip_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        cout << "Brightness button clicked. Showing brightness slider." << endl;
        hideSliders();
        ui.vertflip->setVisible(true);
        ui.horiflip->setVisible(true);
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_vertflip_clicked() {
    try {
        Image_Operations imageops;
        Mat flipped_image;
        flipped_image = imageops.flipimage(universal_image, 1);
        QImage flippedQImage((const uchar*)flipped_image.data, flipped_image.cols, flipped_image.rows, flipped_image.step, QImage::Format_BGR888);
        universal_image = flipped_image;
        ui.uploaded_pic->setPixmap(QPixmap::fromImage(flippedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
}
void ImageCraft::on_horiflip_clicked() {
    try {
        Image_Operations imageops;
        Mat flipped_image;
        flipped_image = imageops.flipimage(universal_image, -1);
        QImage flippedQImage((const uchar*)flipped_image.data, flipped_image.cols, flipped_image.rows, flipped_image.step, QImage::Format_BGR888);
        universal_image = flipped_image;
        ui.uploaded_pic->setPixmap(
            QPixmap::fromImage(flippedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
}

void ImageCraft::on_Crop_Button_clicked() {
    universal_image_for_crop = universal_image;

    if (selectionRect.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("No crop area selected."));
        return;
    }

    // map selectionRect to the actual image coordinates
    double xScale = static_cast<double>(universal_image.cols) / ui.uploaded_pic->width();
    double yScale = static_cast<double>(universal_image.rows) / ui.uploaded_pic->height();

    int x = static_cast<int>(selectionRect.x() * xScale);
    int y = static_cast<int>(selectionRect.y() * yScale);
    int width = static_cast<int>(selectionRect.width() * xScale);
    int height = static_cast<int>(selectionRect.height() * yScale);

    // Ensure valid crop area
    if (x < 0 || y < 0 || x + width > universal_image.cols || y + height > universal_image.rows) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid crop area. Please try again."));
        return;
    }

    Mat croppedImage = universal_image_for_crop(Rect(x, y, width, height)); // Crop the image 

    QImage croppedQImage((const uchar*)croppedImage.data, croppedImage.cols, croppedImage.rows, croppedImage.step, QImage::Format_BGR888);
    ui.uploaded_pic->setPixmap(QPixmap::fromImage(croppedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    universal_image = croppedImage;
    QMessageBox::information(this, tr("Success"), tr("Image cropped successfully!"));
    //current_image_width = croppedImage.cols;
    //current_image_height = croppedImage.rows;
    current_image_height = ui.uploaded_pic->height();
    current_image_width = ui.uploaded_pic->width();

    selectionRect = QRect(); // Reset the selection rectangle
    update();
}

void ImageCraft::on_AddText_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Add Text"), tr("Enter the text:"), QLineEdit::Normal, "", &ok);
        if (!ok || text.isEmpty()) {
            return;
        }
        QFont font = QFontDialog::getFont(&ok, this);
        if (!ok) {
            return;
        }
        QColor color = QColorDialog::getColor(Qt::white, this, tr("Select Font Color"));
        if (!color.isValid()) {
            return;
        }
        QStringList items;
        items << tr("Top-Left") << tr("Top-Right") << tr("Bottom-Left") << tr("Bottom-Right") << tr("Center");
        QString position = QInputDialog::getItem(this, tr("Text Position"), tr("Select the position:"), items, 0, false, &ok);
        if (!ok || position.isEmpty()) {
            return;
        }

        int fontFace = FONT_HERSHEY_SIMPLEX;    // Convert QFont to cv::HersheyFonts equivalent
        int thickness = 4;
        int baseline = 0;
        int fontSize = font.pointSize();
        cv::Size textSize = cv::getTextSize(text.toStdString(), fontFace, fontSize / 10.0, thickness, &baseline);

        cv::Point textOrg;
        if (position == "Top-Left") {
            textOrg = cv::Point(10, textSize.height + 10);
        }
        else if (position == "Top-Right") {
            textOrg = cv::Point(universal_image.cols - textSize.width - 10, textSize.height + 10);
        }
        else if (position == "Bottom-Left") {
            textOrg = cv::Point(10, universal_image.rows - 10);
        }
        else if (position == "Bottom-Right") {
            textOrg = cv::Point(universal_image.cols - textSize.width - 10, universal_image.rows - 10);
        }
        else if (position == "Center") {
            textOrg = cv::Point((universal_image.cols - textSize.width) / 2, (universal_image.rows + textSize.height) / 2);
        }
        Scalar fontColor(color.blue(), color.green(), color.red()); // Convert QColor to cv::Scalar
        putText(universal_image, text.toStdString(), textOrg, fontFace, fontSize / 10.0, fontColor, thickness); // Add text to the original image
        QImage updatedQImage = MatToQImage(universal_image);
        ui.uploaded_pic->setPixmap(QPixmap::fromImage(updatedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}


void ImageCraft::on_Brightness_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        cout << "Brightness button clicked. Showing brightness slider." << endl;
        hideSliders();
        ui.Brightness_Slider->setVisible(true);
        universal_image_for_brightness = universal_image;
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_Brightness_Slider_valueChanged(int value) {
    try {
        Image_Filters obj1;
        Mat bright_image;
        bright_image = obj1.brightness_adjustment(universal_image_for_brightness, value); // Adjust brightness using the slider value

        QImage brightenedQImage((const uchar*)bright_image.data, bright_image.cols,
            bright_image.rows, bright_image.step, QImage::Format_BGR888);

        universal_image = bright_image;
        ui.uploaded_pic->setPixmap(
            QPixmap::fromImage(brightenedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }

}

void ImageCraft::on_Contrast_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        hideSliders();
        ui.Contrast_Slider->setVisible(true);
        universal_image_for_contrast = universal_image;
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_Contrast_Slider_valueChanged(int value) {
    try {
        Image_Filters obj2;
        Mat contrast_image;

        contrast_image = obj2.contrast_adjustment(universal_image_for_contrast, value); // adjust contrast using slider value
        QImage contrastedQImage((const uchar*)contrast_image.data, contrast_image.cols, contrast_image.rows, contrast_image.step, QImage::Format_BGR888);

        universal_image = contrast_image;
        ui.uploaded_pic->setPixmap(QPixmap::fromImage(contrastedQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }
}

void ImageCraft::on_Blur_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        cout << "Brightness button clicked. Showing brightness slider." << endl;
        hideSliders();
        ui.Blur_Slider->setVisible(true);
        universal_image_for_blur = universal_image;
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_Blur_Slider_valueChanged(int value) {
    try {
        Image_Filters obj1;
        Mat blur_image;
        blur_image = obj1.blur_adjustment(universal_image_for_blur, value); // adjust blurness using slider value

        QImage blurredQImage((const uchar*)blur_image.data, blur_image.cols, blur_image.rows, blur_image.step, QImage::Format_BGR888);

        universal_image = blur_image;
        ui.uploaded_pic->setPixmap(
            QPixmap::fromImage(blurredQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Error"), tr(e.what()));
    }

}


void ImageCraft::on_Filter_ComboBox_currentIndexChanged(int index) {
    if (Imag1.isImageLoaded()) {
        hideSliders();
        Image_Filters filters;
        Mat filtered_image = original_image.clone();

        try {
            if (index == 0) {
                filtered_image = original_image.clone();
            }
            else if (index == 1) { // Grayscale
                filtered_image = filters.gray_filter(filtered_image);
                cvtColor(filtered_image, filtered_image, COLOR_GRAY2BGR);
            }
            else if (index == 2) { // Sepia
                filtered_image = filters.sepia_filter(filtered_image);
            }
            else if (index == 3) { // Inversion
                filtered_image = filters.color_inversion(filtered_image);
            }

            if (filtered_image.empty()) {
                throw std::runtime_error("Filtered image is empty.");
            }
            universal_image = filtered_image.clone();      // Update universal_image and display the filtered image

            QImage filteredQImage = MatToQImage(filtered_image);
            ui.uploaded_pic->setPixmap(QPixmap::fromImage(filteredQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
        }
        catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Error"), tr(e.what()));
        } 
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}
void ImageCraft::on_Color_ComboBox_currentIndexChanged(int index) {
    if (Imag1.isImageLoaded()) {
        hideSliders();
        Image_Filters filters;
        Mat filtered_image = universal_image.clone();

        try {
            if (index == 0) {
                filtered_image = original_image.clone(); // Reset to original image
            }
            else if (index == 1) { // Red
                filtered_image = filters.color_isolation(filtered_image, 0);
            }
            else if (index == 2) { // Green
                filtered_image = filters.color_isolation(filtered_image, 1);
            }
            else if (index == 3) { // Blue
                filtered_image = filters.color_isolation(filtered_image, 2);
            }
            else if (index == 4) {
                filtered_image = filters.color_isolation(filtered_image, 3);
            }
            universal_image = filtered_image;   // Update universal_image and display the filtered image
            QImage filteredQImage = MatToQImage(filtered_image);
            ui.uploaded_pic->setPixmap(QPixmap::fromImage(filteredQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
        }
        catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Error"), tr(e.what()));
        }
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}


void ImageCraft::on_Reset_Button_clicked() {
    if (Imag1.isImageLoaded()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("Reset Image"), tr("This will clear all presets. Do you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            universal_image = original_image.clone();
            QImage originalQImage = MatToQImage(original_image);
            ui.uploaded_pic->setPixmap(QPixmap::fromImage(originalQImage).scaled(current_image_width, current_image_height, Qt::KeepAspectRatio));
            hideSliders();
        }
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("No image loaded. Please upload an image first."));
    }
}


void ImageCraft::mousePressEvent(QMouseEvent* event) {
    // Start dragging if mouse click is within the QLabel area
    if (ui.uploaded_pic->geometry().contains(event->pos())) {
        isDragging = true;
        startPoint = event->pos() - ui.uploaded_pic->geometry().topLeft();
        endPoint = startPoint;
    }
}
void ImageCraft::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging) {
        // Update the endPoint as the mouse moves
        endPoint = event->pos() - ui.uploaded_pic->geometry().topLeft();
        selectionRect = QRect(startPoint, endPoint).normalized(); // Ensure valid rectangle
        update(); // Trigger repaint
    }
}
void ImageCraft::mouseReleaseEvent(QMouseEvent* event) {
    if (isDragging) {
        isDragging = false;
        endPoint = event->pos() - ui.uploaded_pic->geometry().topLeft();
        selectionRect = QRect(startPoint, endPoint).normalized();
        update(); // Final repaint
    }
}
void ImageCraft::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (isDragging || !selectionRect.isNull()) {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(QBrush(QColor(255, 0, 0, 50))); // Transparent red
        painter.drawRect(selectionRect.translated(ui.uploaded_pic->geometry().topLeft()));
    }
}

QImage ImageCraft::MatToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        throw std::runtime_error("Empty image provided.");
    }
    if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }
    else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    else if (mat.type() == CV_8UC4) {
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        return QImage(rgba.data, rgba.cols, rgba.rows, rgba.step, QImage::Format_RGBA8888).copy();
    }
    else {
        throw std::runtime_error("Unsupported image format.");
    }
}
