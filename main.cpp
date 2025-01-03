#include "ImageCraft.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ImageCraft w;
    w.show();
    return a.exec();
}


