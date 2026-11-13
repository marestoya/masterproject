#pragma once

#include <QWidget>
#include <QString>

class QPushButton;
class QLineEdit;

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget* parent = nullptr);

    QString participantId() const;

signals:
    void parametrizationRequested();
    void plausibilityRequested();
    void studyRequested();

private:
    QLineEdit* participantIdInput;
    QPushButton* parametrizationButton;
    QPushButton* plausibilityButton;
    QPushButton* studyButton;
};
