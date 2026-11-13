#pragma once

#include <QWidget>
#include "model/Program.h"
#include "model/Parameter.h"
#include <map>
#include <vector>

class QComboBox;
class QGridLayout;
class QVBoxLayout;
class QWidget;

class ParametrizationPage : public QWidget
{
    Q_OBJECT
public:
    explicit ParametrizationPage(QWidget* parent = nullptr);
    void setParticipantId(const QString& id);
    void setStudyTask(
        int programId,
        const QString& mode,
        const QString& variant);

    bool runParametrization(bool showMessage = true);
    void setStudyLanguage(const QString& language);

signals:
    void backToMenuRequested();

private:
    QComboBox* domainCombo;
    QComboBox* programCombo;
    QComboBox* modeCombo;
    QComboBox* objectTypeCombo;
    QComboBox* objectColorCombo;
    QWidget* inputGridContainer;
    QGridLayout* inputGridLayout;
    QWidget* programPanel;
    QWidget* inputPanel;
    QVBoxLayout* programLayout;
    QVBoxLayout* inputLayout;
    QString participantId;
    QString studyVariant;

    std::vector<Program> programs;
    std::vector<Parameter> inputParameters;
    std::map<int, int> explicitAssignments;
    int nextInputParameterId = 1;

    void updateProgramList();
    void updateProgram();
    void renderProgram(const Program& program);
    void addInputObject();
    void clearInputObjects();
    void updateAvailableObjectTypes();
};
