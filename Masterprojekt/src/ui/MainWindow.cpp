
#include "ui/MainWindow.h"
#include "ui/MainMenu.h"
#include "ui/ParametrizationPage.h"
#include "ui/PlausibilityPage.h"

#include <QStackedWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    stackedWidget(new QStackedWidget(this)),
    mainMenu(new MainMenu(this)),
    parametrizationPage(new ParametrizationPage(this)),
    plausibilityPage(new PlausibilityPage(this))
{
    setWindowTitle("VerbBot");
    resize(1100, 720);

    setCentralWidget(stackedWidget);

    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(parametrizationPage);
    stackedWidget->addWidget(plausibilityPage);
    stackedWidget->setCurrentWidget(mainMenu);

    connect(
        mainMenu,
        &MainMenu::parametrizationRequested,
        this,
        [this]()
        {
            parametrizationPage->setParticipantId(
                mainMenu->participantId()
            );

            stackedWidget->setCurrentWidget(parametrizationPage);
            setWindowTitle("VerbBot - Parametrization");
        }
    );

    connect(
        mainMenu,
        &MainMenu::plausibilityRequested,
        this,
        [this]()
        {
            plausibilityPage->setParticipantId(
                mainMenu->participantId()
            );

            stackedWidget->setCurrentWidget(plausibilityPage);
            setWindowTitle("VerbBot - Plausibility");
        }
    );

    connect(
        parametrizationPage,
        &ParametrizationPage::backToMenuRequested,
        this,
        &MainWindow::showMainMenu
    );

    connect(
        plausibilityPage,
        &PlausibilityPage::backToMenuRequested,
        this,
        &MainWindow::showMainMenu
    );
}

void MainWindow::showMainMenu()
{
    stackedWidget->setCurrentWidget(mainMenu);
    setWindowTitle("VerbBot");
}
