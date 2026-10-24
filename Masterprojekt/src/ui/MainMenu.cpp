#include "ui/MainMenu.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

MainMenu::MainMenu(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("VerbBot");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; margin: 20px;");
    auto* parametrizationButton = new QPushButton("Parametrization");
    auto* plausibilityButton = new QPushButton("Plausibility");
    parametrizationButton->setMinimumHeight(50);
    plausibilityButton->setMinimumHeight(50);
    layout->addStretch();
    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(parametrizationButton);
    layout->addWidget(plausibilityButton);
    layout->addStretch();
    connect(parametrizationButton, &QPushButton::clicked, this, &MainMenu::parametrizationRequested);
    connect(plausibilityButton, &QPushButton::clicked, this, &MainMenu::plausibilityRequested);
}
