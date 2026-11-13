#include "LanguageSelection.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

LanguageSelection::LanguageSelection(QWidget* parent)
    : QWidget(parent),
    englishButton(new QPushButton("English", this)),
    germanButton(new QPushButton("Deutsch", this)),
    backButton(new QPushButton("Back", this))
{
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("Choose your language", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 26px; font-weight: bold; margin: 20px;"
    );

    auto* instruction = new QLabel(
        "Please select the language first!",
        this
    );
    instruction->setAlignment(Qt::AlignCenter);

    englishButton->setMinimumHeight(55);
    germanButton->setMinimumHeight(55);
    backButton->setMinimumHeight(40);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(instruction);
    layout->addSpacing(25);
    layout->addWidget(englishButton);
    layout->addWidget(germanButton);
    layout->addSpacing(20);
    layout->addWidget(backButton);
    layout->addStretch();

    connect(englishButton, &QPushButton::clicked,
        this, [this]()
        {
            emit languageSelected("English");
        });

    connect(germanButton, &QPushButton::clicked,
        this, [this]()
        {
            emit languageSelected("German");
        });

    connect(backButton, &QPushButton::clicked,
        this, &LanguageSelection::backRequested);
}
