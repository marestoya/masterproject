
#include "ui/MainWindow.h"

#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Parametrization Tester");
    resize(1000, 650);

    auto* label = new QLabel("UI is working!", this);
    label->setAlignment(Qt::AlignCenter);

    setCentralWidget(label);
}
