#include "ui/MainMenu.h"

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>

MainMenu::MainMenu(QWidget* parent)
    : QWidget(parent),
    participantIdInput(new QLineEdit(this)),
    parametrizationButton(new QPushButton("Parametrization", this)),
    plausibilityButton(new QPushButton("Plausibility", this))
{
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("VerbBot", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 28px; font-weight: bold; margin: 20px;"
    );

    auto* participantLabel = new QLabel("Participant ID:", this);

    participantIdInput->setPlaceholderText("Enter participant ID (e.g. P001)");
    participantIdInput->setMaxLength(100);

    parametrizationButton->setMinimumHeight(50);
    plausibilityButton->setMinimumHeight(50);

    layout->addStretch();
    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(participantLabel);
    layout->addWidget(participantIdInput);
    layout->addSpacing(20);
    layout->addWidget(parametrizationButton);
    layout->addWidget(plausibilityButton);
    layout->addStretch();

    connect(parametrizationButton, &QPushButton::clicked,
        this, [this]()
        {
            if (participantId().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Participant ID required",
                    "Please enter a participant ID before starting a task."
                );
                participantIdInput->setFocus();
                return;
            }

            emit parametrizationRequested();
        });

    connect(plausibilityButton, &QPushButton::clicked,
        this, [this]()
        {
            if (participantId().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Participant ID required",
                    "Please enter a participant ID before starting a task."
                );
                participantIdInput->setFocus();
                return;
            }

            emit plausibilityRequested();
        });
}

QString MainMenu::participantId() const
{
    return participantIdInput->text().trimmed();
}
