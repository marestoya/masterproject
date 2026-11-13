
#include "ui/MainWindow.h"
#include "ui/MainMenu.h"
#include "ui/ParametrizationPage.h"
#include "ui/PlausibilityPage.h"
#include "LanguageSelection.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QStringList>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    stackedWidget(new QStackedWidget(this)),
    mainMenu(new MainMenu(this)),
    parametrizationPage(new ParametrizationPage(this)),
    plausibilityPage(new PlausibilityPage(this)),
    nextStudyButton(new QPushButton("Next", this)),
    languageSelection(new LanguageSelection(this))
{
    setWindowTitle("Study");
    resize(1100, 720);

    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);

    centralLayout->setContentsMargins(0, 0, 0, 0);

    centralLayout->addWidget(stackedWidget);
    centralLayout->addWidget(nextStudyButton, 0, Qt::AlignRight);

    setCentralWidget(centralWidget);

    nextStudyButton->setMinimumHeight(45);
    nextStudyButton->hide();

    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(parametrizationPage);
    stackedWidget->addWidget(plausibilityPage);
    stackedWidget->addWidget(languageSelection);
    stackedWidget->setCurrentWidget(mainMenu);


    connect(mainMenu, &MainMenu::parametrizationRequested,
        this, [this]()
        {
            parametrizationPage->setParticipantId(
                mainMenu->participantId());

            stackedWidget->setCurrentWidget(parametrizationPage);
            setWindowTitle("Parametrization");
        });

    connect(mainMenu, &MainMenu::plausibilityRequested,
        this, [this]()
        {
            plausibilityPage->setParticipantId(
                mainMenu->participantId());

            stackedWidget->setCurrentWidget(plausibilityPage);
            setWindowTitle("Plausibility");
        });

    connect(mainMenu, &MainMenu::studyRequested,
        this, &MainWindow::showLanguageSelection);

    connect(languageSelection,
        &LanguageSelection::languageSelected,
        this, &MainWindow::startStudy);

    connect(languageSelection,
        &LanguageSelection::backRequested,
        this, &MainWindow::showMainMenu);

    connect(nextStudyButton, &QPushButton::clicked,
        this, &MainWindow::advanceStudy);

    connect(parametrizationPage,
        &ParametrizationPage::backToMenuRequested,
        this, &MainWindow::showMainMenu);

    connect(plausibilityPage,
        &PlausibilityPage::backToMenuRequested,
        this, &MainWindow::showMainMenu);
}

void MainWindow::showMainMenu()
{
    studyMode = false;
    nextStudyButton->hide();

    stackedWidget->setCurrentWidget(mainMenu);
    setWindowTitle("Study");
}


void MainWindow::showLanguageSelection()
{
    stackedWidget->setCurrentWidget(languageSelection);
    setWindowTitle("Study: Language Selection");
}

void MainWindow::startStudy(const QString& language)
{
    bool ok = false;

    const int participantId =
        mainMenu->participantId().toInt(&ok);

    if (!ok || participantId <= 0)
        return;

    studyLanguage = language;

    studyListNumber = (participantId - 1) % 4;
    studyTaskIndex = 0;
    studyMode = true;

    plausibilityPage->setParticipantId(
        mainMenu->participantId());

    parametrizationPage->setParticipantId(
        mainMenu->participantId());

    nextStudyButton->show();

    showStudyTask();
}


void MainWindow::showStudyTask()
{
    if (studyTaskIndex >= 20)
    {
        studyMode = false;
        nextStudyButton->hide();

        QMessageBox::information(
            this,
            "Study completed",
            "Thank you for participating!\n\n"
            "All 20 tasks have been completed and save"
        );

        showMainMenu();
        return;
    }

    static const std::vector<StudyTask> tasks =
    {
        //PLAUSIBILITY

        { StudyPageType::Plausibility, "Laboratory", 4, 1, "A" },
        { StudyPageType::Plausibility, "Laboratory", 5, 2, "B" },
        { StudyPageType::Plausibility, "Laboratory", 6, 3, "C" },

        { StudyPageType::Plausibility, "Catering", 7, 1, "D" },
        { StudyPageType::Plausibility, "Catering", 8, 2, "A" },
        { StudyPageType::Plausibility, "Catering", 9, 3, "B" },

        { StudyPageType::Plausibility, "Workshop", 1, 1, "C" },
        { StudyPageType::Plausibility, "Workshop", 2, 2, "D" },
        { StudyPageType::Plausibility, "Workshop", 3, 3, "A" },

        { StudyPageType::Plausibility, "Electronics", 10, 1, "B" },
        { StudyPageType::Plausibility, "Electronics", 11, 2, "C" },
        { StudyPageType::Plausibility, "Electronics", 12, 3, "D" },

        // PARAMETRIZATION

        { StudyPageType::Parametrization, "Laboratory", 1, 1, "E2" },
        { StudyPageType::Parametrization, "Laboratory", 2, 2, "I2" },

        { StudyPageType::Parametrization, "Catering", 3, 1, "E4" },
        { StudyPageType::Parametrization, "Catering", 4, 2, "I4" },

        { StudyPageType::Parametrization, "Workshop", 5, 1, "E2" },
        { StudyPageType::Parametrization, "Workshop", 6, 2, "I2" },

        { StudyPageType::Parametrization, "Electronics", 7, 1, "E4" },
        { StudyPageType::Parametrization, "Electronics", 8, 2, "I4" }
    };

    //selector based on the id
    StudyTask task = tasks[studyTaskIndex];

    static const QStringList plausibilityVariants[12] =
    {
        { "A", "B", "C", "D" },
        { "B", "C", "D", "A" },
        { "C", "D", "A", "B" },
        { "D", "A", "B", "C" },
        { "A", "B", "C", "D" },
        { "B", "C", "D", "A" },
        { "C", "D", "A", "B" },
        { "D", "A", "B", "C" },
        { "A", "B", "C", "D" },
        { "B", "C", "D", "A" },
        { "C", "D", "A", "B" },
        { "D", "A", "B", "C" }
    };

    static const QStringList parametrizationVariants[8] =
    {
        { "E2", "I2", "E4", "I4" },
        { "I2", "E4", "I4", "E2" },
        { "E4", "I4", "E2", "I2" },
        { "I4", "E2", "I2", "E4" },
        { "E2", "I2", "E4", "I4" },
        { "I2", "E4", "I4", "E2" },
        { "E4", "I4", "E2", "I2" },
        { "I4", "E2", "I2", "E4" }
    };

    if (task.pageType == StudyPageType::Plausibility)
    {
        task.variant =
            plausibilityVariants[studyTaskIndex][studyListNumber];

        plausibilityPage->setStudyLanguage(studyLanguage);

        plausibilityPage->setStudyTask(
            task.programId,
            task.variant
        );

        stackedWidget->setCurrentWidget(plausibilityPage);
        setWindowTitle(
            QString("Study: Plausibility %1/12")
            .arg(studyTaskIndex + 1)
        );
    }
    else
    {
        const int parametrizationIndex = studyTaskIndex - 12;

        task.variant =
            parametrizationVariants[parametrizationIndex][studyListNumber];

        const QString mode =
            task.variant.startsWith("E")
            ? "Explicit"
            : "Implicit";
        parametrizationPage->setStudyLanguage(studyLanguage);
        parametrizationPage->setStudyTask(
            task.programId,
            mode,
            task.variant
        );

        stackedWidget->setCurrentWidget(parametrizationPage);

        setWindowTitle(
            QString("Study: Parametrization %1/8")
            .arg(parametrizationIndex + 1)
        );
    }

    nextStudyButton->setText(
        studyTaskIndex == 19 ? "Finish" : "Next"
    );
}

void MainWindow::advanceStudy()
{
    if (!studyMode)
        return;

    bool success = false;

    if (studyTaskIndex < 12)
    {
        success = plausibilityPage->saveUserChoice(false);
    }
    else
    {
        success = parametrizationPage->runParametrization(false);
    }

    if (!success)
        return;

    ++studyTaskIndex;

    showStudyTask();
}