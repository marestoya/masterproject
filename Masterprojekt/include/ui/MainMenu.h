#pragma once
#include <QWidget>
class QPushButton;
class MainMenu : public QWidget
{
    Q_OBJECT
public:
    explicit MainMenu(QWidget* parent = nullptr);
signals:
    void parametrizationRequested();
    void plausibilityRequested();
};
