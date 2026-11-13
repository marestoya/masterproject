#include "ui/MainMenu.h"

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QIntValidator>

MainMenu::MainMenu(QWidget* parent)
    : QWidget(parent),
    participantIdInput(new QLineEdit(this)),
    parametrizationButton(new QPushButton("Parametrization", this)),
    plausibilityButton(new QPushButton("Plausibility", this)),
    studyButton(new QPushButton("Study", this))
{
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("Study", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 28px; font-weight: bold; margin: 20px;"
    );

    auto* participantLabel = new QLabel("Participant ID:", this);

    participantIdInput->setPlaceholderText("Enter participant ID");
    participantIdInput->setValidator(new QIntValidator(1, 999999, this));

    parametrizationButton->setMinimumHeight(50);
    plausibilityButton->setMinimumHeight(50);
    studyButton->setMinimumHeight(50);

    layout->addStretch();
    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(participantLabel);
    layout->addWidget(participantIdInput);
    layout->addSpacing(20);
    layout->addWidget(parametrizationButton);
    layout->addWidget(plausibilityButton);
    layout->addWidget(studyButton);
    layout->addStretch();

    connect(parametrizationButton, &QPushButton::clicked,
        this, [this]()
        {
            if (participantId().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Participant ID required",
                    "Please enter a participant ID before starting a task"
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
                    "Please enter a participant ID before starting a task"
                );
                participantIdInput->setFocus();
                return;
            }

            emit plausibilityRequested();
        });

    connect(studyButton, &QPushButton::clicked, this, [this]()
        {
            if (participantId().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Participant ID required",
                    "Please enter a positive participant ID"
                );

                participantIdInput->setFocus();
                return;
            }

            emit studyRequested();
        });
}

QString MainMenu::participantId() const
{
    return participantIdInput->text().trimmed();
}
