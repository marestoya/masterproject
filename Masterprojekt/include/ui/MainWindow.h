#pragma once

#include <QMainWindow>
#include <QString>
#include "StudyTask.h"
#include <vector>

class QStackedWidget;
class MainMenu;
class ParametrizationPage;
class LanguageSelection;
class PlausibilityPage;
class QPushButton;

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
    QPushButton* nextStudyButton;
    LanguageSelection* languageSelection;

    QString studyLanguage = "English";

    std::vector<StudyTask> studyTasks;

    int studyTaskIndex = 0;
    int studyListNumber = 1;
    bool studyMode = false;

    void showMainMenu();
    void showStudyTask();
    void advanceStudy();
    void showLanguageSelection();
    void startStudy(const QString& language);
};
