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
};

class PlausibilityPage : public QWidget
{
    Q_OBJECT
public:
    explicit PlausibilityPage(QWidget* parent = nullptr);
    void setParticipantId(const QString& id);

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
    int nextObjectId = 1;

    void loadProgramsAndStimuli();
    void updateProgramList();
    void updateVariantList();
    void loadSelectedStimulus();
    void renderProgram(const Program& program);
    void renderObjects();
    void refreshSlotLabels(const Program& program);
    void clearLayout(QLayout* layout);
    void saveUserChoice();
    
};
