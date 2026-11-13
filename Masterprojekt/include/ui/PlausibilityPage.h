#pragma once

#include <QWidget>
#include <QString>
#include "model/Program.h"
#include "model/Parameter.h"

#include <map>
#include <vector>

class QComboBox;
class QGridLayout;
class QLabel;
class QLayout;
class QVBoxLayout;
class QWidget;
class QPushButton;

struct PlausibilityVariant
{
    std::vector<Parameter> objects;
    QString context;

    //overrides for variant C and D
    std::vector<Parameter> programParameterOverrides;

    //needed for variant C and D; adds another parameter with the minimum cost
    std::vector<int> anotherSlots;

    //needed foe one specific task 
    std::vector<int> hiddenInstructions;
};

class PlausibilityPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlausibilityPage(QWidget* parent = nullptr);

    void setParticipantId(const QString& participantId);
    void setStudyTask(int programId, const QString& variant);

    bool saveUserChoice(bool showMessage = true);
    void setStudyLanguage(const QString& language);
signals:
    void backToMenuRequested();

private:
    QComboBox* domainCombo;
    QComboBox* programCombo;
    QComboBox* variantCombo;

    QWidget* programPanel;
    QVBoxLayout* programLayout;
    QWidget* objectGridContainer;
    QGridLayout* objectGridLayout;
    QLabel* contextLabel;
    QLabel* statusLabel;
    QString participantId;


    QString studyLanguage = "English";

    QLabel* domainLabel;
    QLabel* taskLabel;
    QLabel* variantLabel;
    QLabel* programTitleLabel;
    QLabel* objectsTitleLabel;

    QPushButton* backButton;
    QPushButton* resetAllButton;
    QPushButton* saveButton;

    std::vector<Program> programs;

    // program ID to variant name to stimulus objects and optional context
    std::map<int, std::map<QString, PlausibilityVariant>> variantsByProgram;

    std::map<int, int> objectToSlot;
    std::vector<Parameter> currentObjects;
    Program currentDisplayProgram;
    std::vector<int> currentAnotherSlots;
    std::vector<int> currentHiddenInstructions;

    int nextObjectId = 1;

    void loadProgramsAndStimuli();
    void updateProgramList();
    void updateVariantList();
    void loadSelectedStimulus();
   

    void renderProgram(
        const Program& program,
        const std::vector<int>& anotherSlots = {},
        const std::vector<int>& hiddenInstructions = {}
    );

    void renderObjects();
    void refreshSlotLabels(const Program& program);
    void clearLayout(QLayout* layout);

};