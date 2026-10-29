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

struct PlausibilityVariant
{
    std::vector<Parameter> objects;
    QString context;

    // Optional variant-specific program parameter changes (id is the program slot ID).
    std::vector<Parameter> programParameterOverrides;

    // Slot IDs whose first mention should say "another ..." in the program text.
    std::vector<int> anotherSlots;

    // 1-based instruction numbers hidden for this variant.
    std::vector<int> hiddenInstructions;
};

class PlausibilityPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlausibilityPage(QWidget* parent = nullptr);

    void setParticipantId(const QString& participantId);
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
    void saveUserChoice();

};