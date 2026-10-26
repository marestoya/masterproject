#pragma once

#include <QMainWindow>

class QStackedWidget;
class MainMenu;
class ParametrizationPage;
class PlausibilityPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QStackedWidget* stackedWidget;
    MainMenu* mainMenu;
    ParametrizationPage* parametrizationPage;
    PlausibilityPage* plausibilityPage;

    void showMainMenu();
};
