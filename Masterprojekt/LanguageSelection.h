#pragma once

#include <QWidget>

class QPushButton;

class LanguageSelection : public QWidget
{
    Q_OBJECT

public:
    explicit LanguageSelection(QWidget* parent = nullptr);

signals:
    void languageSelected(const QString& language);
    void backRequested();

private:
    QPushButton* englishButton;
    QPushButton* germanButton;
    QPushButton* backButton;
};
