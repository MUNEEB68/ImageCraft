#include "photeditor.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    photeditor w;
    w.show();
    return a.exec();
}
