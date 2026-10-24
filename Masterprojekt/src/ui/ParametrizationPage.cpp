#include "ui/ParametrizationPage.h"

#include "ui/ProgramLoader.h"
#include "cost/CostMatrix.h"
#include "cost/CostModel.h"
#include "model/ProgramInstantiator.h"
#include "parametrization/GlobalExplicit.h"
#include "parametrization/GlobalImplicit.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>

#include <algorithm>
#include <exception>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr const char* ParameterMimeType =
        "application/x-verbbot-parameter";

    QString parameterName(const Parameter& parameter)
    {
        return QString("%1 %2")
            .arg(QString::fromStdString(parameter.color),
                QString::fromStdString(parameter.type));
    }

    QColor parameterColor(const QString& colorName)
    {
        QColor color(colorName);

        if (!color.isValid() ||
            colorName.compare("glass", Qt::CaseInsensitive) == 0)
        {
            return QColor("#b8c4cc");
        }

        if (colorName.compare("silver", Qt::CaseInsensitive) == 0)
            return QColor("#b8bec5");

        return color;
    }

    QString slotIdentifier(int slotId)
    {
        if (slotId >= 1 && slotId <= 26)
            return QString(QChar(QLatin1Char('A').unicode() + slotId - 1));

        return QString::number(slotId);
    }

    QColor slotBackgroundColor(int slotId)
    {
        static const QColor colors[] = {
            QColor("#F3E5F5"),
            QColor("#E3F2FD"),
            QColor("#E8F5E9"),
            QColor("#FFF3E0"),
            QColor("#FCE4EC"),
            QColor("#E0F7FA"),
            QColor("#FFFDE7"),
            QColor("#EDE7F6")
        };

        constexpr int count = sizeof(colors) / sizeof(colors[0]);
        return colors[(slotId - 1) % count];
    }

    QString defaultSlotText(const Program& program, int slotId)
    {
        const auto it = std::find_if(
            program.parameters.begin(),
            program.parameters.end(),
            [slotId](const ParameterSlot& slot)
            {
                return slot.id == slotId;
            });

        if (it == program.parameters.end() || it->parameters.empty())
            return QString("[Unknown parameter %1]").arg(slotId);

        return parameterName(it->parameters.front());
    }
}

//draggable for input param for the explicit parametrization
class DraggableObject : public QLabel
{
public:
    DraggableObject(
        const Parameter& parameter,
        QObject* modeOwner,
        QWidget* parent = nullptr)
        : QLabel(parameterName(parameter), parent),
        parameterId(parameter.id),
        colorName(QString::fromStdString(parameter.color)),
        modeOwner(modeOwner)
    {
        setProperty("parameterId", parameterId);
        setFrameShape(QFrame::StyledPanel);
        setAlignment(Qt::AlignCenter);
        setFixedSize(150, 64);
        setWordWrap(true);
        setCursor(Qt::OpenHandCursor);

        setStyleSheet(
            "QLabel { background: #e8f0fe;"
            "border: 1px solid #8aa9e6;"
            "border-radius: 6px; padding: 6px;"
            "color: #202124; }");

        setToolTip(
            QString("%1 (ID: %2)")
            .arg(parameterName(parameter))
            .arg(parameter.id));
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
            dragStart = event->pos();

        QLabel::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!modeOwner ||
            !modeOwner->property("explicitMode").toBool())
        {
            return;
        }

        if (!(event->buttons() & Qt::LeftButton))
            return;

        if ((event->pos() - dragStart).manhattanLength()
            < QApplication::startDragDistance())
        {
            return;
        }

        QDrag drag(this);
        auto* mimeData = new QMimeData;

        mimeData->setData(
            ParameterMimeType,
            QByteArray::number(parameterId));

        drag.setMimeData(mimeData);

        QPixmap preview(28, 28);
        preview.fill(Qt::transparent);

        QPainter painter(&preview);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setOpacity(0.60);
        painter.setPen(QPen(QColor("#555555"), 1));
        painter.setBrush(parameterColor(colorName));
        painter.drawRoundedRect(QRect(3, 3, 22, 22), 4, 4);
        painter.end();

        drag.setPixmap(preview);
        drag.setHotSpot(QPoint(14, 14));
        drag.exec(Qt::CopyAction);
    }

private:
    QPoint dragStart;
    int parameterId;
    QString colorName;
    QObject* modeOwner;
};

//program slot in the program text
class DropSlot : public QLabel
{
public:
    using AssignmentCallback = std::function<void(int, int)>;

    DropSlot(
        int slotId,
        const QString& initialText,
        QObject* modeOwner,
        AssignmentCallback callback,
        QWidget* parent = nullptr)
        : QLabel(initialText, parent),
        slotId(slotId),
        modeOwner(modeOwner),
        callback(std::move(callback))
    {
        setProperty("slotId", slotId);
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setContentsMargins(5, 13, 24, 5);
        setMinimumSize(135, 42);
        setMaximumWidth(220);
        setWordWrap(true);
        setFrameShape(QFrame::StyledPanel);

        setToolTip(QString("Program slot %1").arg(slotId));

        const QColor bgColor = slotBackgroundColor(slotId);

        setStyleSheet(QString(
            "QLabel {"
            " background-color: %1;"
            " color: black;"
            " border: 2px solid %2;"
            " border-radius: 6px;"
            " padding: 5px;"
            "}"
        ).arg(bgColor.name(), bgColor.darker(125).name()));
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);

        // Draw a small corner badge so repeated references to the same slot
        // are easy to identify throughout the program instructions.
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QString identifier = slotIdentifier(slotId);
        const QRect badgeRect(width() - 25, 4, 19, 17);
        const QColor badgeColor = slotBackgroundColor(slotId).darker(118);

        painter.setPen(QPen(badgeColor.darker(145), 1));
        painter.setBrush(badgeColor);
        painter.drawRoundedRect(badgeRect, 4, 4);

        painter.setPen(Qt::black);
        QFont badgeFont = painter.font();
        badgeFont.setBold(true);
        badgeFont.setPointSize(8);
        painter.setFont(badgeFont);
        painter.drawText(badgeRect, Qt::AlignCenter, identifier);
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (modeOwner &&
            modeOwner->property("explicitMode").toBool() &&
            event->mimeData()->hasFormat(ParameterMimeType))
        {
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
    }

    void dropEvent(QDropEvent* event) override
    {
        if (!modeOwner ||
            !modeOwner->property("explicitMode").toBool() ||
            !event->mimeData()->hasFormat(ParameterMimeType))
        {
            event->ignore();
            return;
        }

        bool ok = false;

        const int inputId = QString::fromUtf8(
            event->mimeData()->data(ParameterMimeType)).toInt(&ok);

        if (!ok)
        {
            event->ignore();
            return;
        }

        if (callback)
            callback(inputId, slotId);

        event->acceptProposedAction();
    }

private:
    int slotId;
    QObject* modeOwner;
    AssignmentCallback callback;
};


//main window

ParametrizationPage::ParametrizationPage(QWidget* parent)
    : QWidget(parent),
    domainCombo(new QComboBox(this)),
    programCombo(new QComboBox(this)),
    modeCombo(new QComboBox(this)),
    objectTypeCombo(new QComboBox(this)),
    objectColorCombo(new QComboBox(this)),
    inputGridContainer(new QWidget(this)),
    inputGridLayout(new QGridLayout(inputGridContainer)),
    programPanel(new QWidget(this)),
    inputPanel(new QWidget(this)),
    programLayout(new QVBoxLayout),
    inputLayout(new QVBoxLayout)
{
    setProperty("explicitMode", false);

    try
    {
        const QString programPath =
            "C:/Users/marsp/source/repos/Masterprojekt/"
            "Masterprojekt/programs/parametrization_programs.json";

        programs = loadPrograms(programPath.toStdString());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
            this,
            "Program loading error",
            QString::fromStdString(e.what()));
    }

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(central);
    auto* backButton = new QPushButton("Back");
    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
    connect(backButton, &QPushButton::clicked, this, &ParametrizationPage::backToMenuRequested);

    auto* topLayout = new QHBoxLayout;

    topLayout->addWidget(new QLabel("Domain:"));
    topLayout->addWidget(domainCombo);

    topLayout->addWidget(new QLabel("Program:"));
    topLayout->addWidget(programCombo);

    topLayout->addWidget(new QLabel("Mode:"));
    modeCombo->addItems({ "Implicit", "Explicit" });
    topLayout->addWidget(modeCombo);

    mainLayout->addLayout(topLayout);

    auto* contentLayout = new QHBoxLayout;
    auto* leftPanel = new QVBoxLayout;
    auto* rightPanel = new QVBoxLayout;

    auto* programTitle = new QLabel("Program");
    programTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold;");
    leftPanel->addWidget(programTitle);

    programPanel->setLayout(programLayout);

    auto* programScroll = new QScrollArea;
    programScroll->setWidgetResizable(true);
    programScroll->setWidget(programPanel);
    leftPanel->addWidget(programScroll);

    auto* inputTitle = new QLabel("Input objects");
    inputTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold;");
    rightPanel->addWidget(inputTitle);


    objectTypeCombo->addItem("Beaker", "beaker");
    objectTypeCombo->addItem("Test tube", "testTube");
    objectTypeCombo->addItem("Thermometer", "thermometer");
    objectTypeCombo->addItem("Stirrer", "stirrer");
    objectTypeCombo->addItem("Tray", "tray");
    objectTypeCombo->addItem("Test tube rack", "testTubeRack");
    objectTypeCombo->addItem("Sample tube", "sampleTube");
    objectTypeCombo->addItem("Storage box", "storageBox");
    objectTypeCombo->addItem("Powder", "powder");
    objectTypeCombo->addItem("Liquid", "liquid");
    objectTypeCombo->addItem("Container", "container");
    objectTypeCombo->addItem("Water bottle", "waterBottle");
    objectTypeCombo->addItem("Water", "water");

    objectColorCombo->addItem("Red", "red");
    objectColorCombo->addItem("Green", "green");
    objectColorCombo->addItem("Blue", "blue");
    objectColorCombo->addItem("Yellow", "yellow");
    objectColorCombo->addItem("White", "white");
    objectColorCombo->addItem("Silver", "silver");
    objectColorCombo->addItem("Glass", "glass");

    auto* objectControls = new QHBoxLayout;
    objectControls->addWidget(objectTypeCombo);
    objectControls->addWidget(objectColorCombo);

    auto* addButton = new QPushButton("+ Add object");
    objectControls->addWidget(addButton);
    rightPanel->addLayout(objectControls);

    auto* clearButton = new QPushButton("Clear all input objects");
    rightPanel->addWidget(clearButton);

    inputGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    inputGridLayout->setSpacing(10);

    auto* inputScroll = new QScrollArea;
    inputScroll->setWidgetResizable(true);
    inputScroll->setWidget(inputGridContainer);
    inputScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    inputScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    rightPanel->addWidget(inputScroll);

    auto* leftContainer = new QWidget;
    leftContainer->setLayout(leftPanel);

    auto* rightContainer = new QWidget;
    rightContainer->setLayout(rightPanel);

    contentLayout->addWidget(leftContainer, 1);
    contentLayout->addWidget(rightContainer, 1);

    mainLayout->addLayout(contentLayout, 1);

    auto* actionLayout = new QHBoxLayout;
    auto* runButton = new QPushButton("Run parametrization");

    actionLayout->addStretch();
    actionLayout->addWidget(runButton);
    mainLayout->addLayout(actionLayout);





    connect(addButton, &QPushButton::clicked,
        this, [this]() { addInputObject(); });

    connect(clearButton, &QPushButton::clicked,
        this, [this]() { clearInputObjects(); });

    connect(runButton, &QPushButton::clicked,
        this, [this]() { runParametrization(); });

    connect(domainCombo, &QComboBox::currentIndexChanged,
        this, [this](int) { updateProgramList(); });

    connect(programCombo, &QComboBox::currentIndexChanged,
        this, [this](int) { updateProgram(); });

    connect(modeCombo, &QComboBox::currentTextChanged,
        this, [this](const QString& mode)
        {
            const bool explicitMode = (mode == "Explicit");
            setProperty("explicitMode", explicitMode);

            setWindowTitle(
                explicitMode
                ? "VerbBot - Explicit Parametrization"
                : "VerbBot - Implicit Parametrization");
        });

    std::set<std::string> domains;

    for (const Program& program : programs)
        domains.insert(program.domain);

    for (const std::string& domain : domains)
        domainCombo->addItem(QString::fromStdString(domain));

    updateProgramList();
}

//progtam dropdown
void ParametrizationPage::updateProgramList()
{
    const QString domain = domainCombo->currentText();

    programCombo->blockSignals(true);
    programCombo->clear();

    for (const Program& program : programs)
    {
        if (QString::fromStdString(program.domain) == domain)
        {
            programCombo->addItem(
                QString::fromStdString(program.name),
                program.id);
        }
    }

    programCombo->blockSignals(false);

    explicitAssignments.clear();
    updateProgram();
}

//show selected program
void ParametrizationPage::updateProgram()
{
    explicitAssignments.clear();

    if (programCombo->currentIndex() < 0)
    {
        while (QLayoutItem* item = programLayout->takeAt(0))
        {
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }

        programLayout->addWidget(
            new QLabel("No programs available for this domain"));
        programLayout->addStretch();
        return;
    }

    const int selectedId = programCombo->currentData().toInt();

    const auto it = std::find_if(
        programs.begin(), programs.end(),
        [selectedId](const Program& program)
        {
            return program.id == selectedId;
        });

    if (it == programs.end())
        return;

    renderProgram(*it);
}

void ParametrizationPage::renderProgram(const Program& program)
{
    while (QLayoutItem* item = programLayout->takeAt(0))
    {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    auto* description = new QLabel(
        QString::fromStdString(program.name));

    description->setWordWrap(true);
    description->setStyleSheet(
        "font-size: 16px; font-weight: bold; padding: 8px;");

    programLayout->addWidget(description);

    auto* assignmentHint = new QLabel(
        modeCombo->currentText() == "Explicit"
        ? "Drag each input object onto its intended slot."
        : "Input objects will be matched automatically.");

    assignmentHint->setWordWrap(true);
    programLayout->addWidget(assignmentHint);

    const auto updateAssignment =
        [this, program](int inputId, int slotId)
        {
            explicitAssignments[inputId] = slotId;

            const auto labels =
                programPanel->findChildren<QLabel*>();

            for (QLabel* label : labels)
            {
                const QVariant slotProperty =
                    label->property("slotId");

                if (!slotProperty.isValid())
                    continue;

                const int currentSlotId = slotProperty.toInt();

                QStringList assignedNames;

                for (const auto& assignment : explicitAssignments)
                {
                    if (assignment.second != currentSlotId)
                        continue;

                    const auto input = std::find_if(
                        inputParameters.begin(),
                        inputParameters.end(),
                        [&assignment](const Parameter& parameter)
                        {
                            return parameter.id == assignment.first;
                        });

                    if (input != inputParameters.end())
                        assignedNames.append(parameterName(*input));
                }

                if (assignedNames.isEmpty())
                {
                    label->setText(
                        defaultSlotText(program, currentSlotId));
                }
                else
                {
                    label->setText(assignedNames.join(" + "));
                }
            }
        };

    int stepNumber = 1;

    for (const auto& instruction : program.instructions)
    {
        auto* stepFrame = new QFrame;
        stepFrame->setFrameShape(QFrame::StyledPanel);

        auto* row = new QHBoxLayout(stepFrame);
        row->setSpacing(4);

        row->addWidget(new QLabel(
            QString("%1.").arg(stepNumber++)), 0, Qt::AlignTop);

        for (const InstructionSegment& segment : instruction)
        {
            if (segment.isSlot)
            {
                auto* slot = new DropSlot(
                    segment.slotId,
                    defaultSlotText(program, segment.slotId),
                    this,
                    updateAssignment,
                    stepFrame);

                row->addWidget(slot, 0, Qt::AlignVCenter);
            }
            else
            {
                auto* text = new QLabel(
                    QString::fromStdString(segment.text));

                text->setWordWrap(true);
                row->addWidget(text, 0, Qt::AlignVCenter);
            }
        }

        row->addStretch();
        programLayout->addWidget(stepFrame);
    }

    programLayout->addStretch();
}

//add param
void ParametrizationPage::addInputObject()
{
    Parameter parameter;
    parameter.id = nextInputParameterId++;
    parameter.type = objectTypeCombo->currentData().toString().toStdString();
    parameter.color = objectColorCombo->currentData().toString().toStdString();

    inputParameters.push_back(parameter);

    auto* cell = new QWidget(inputGridContainer);
    auto* cellLayout = new QVBoxLayout(cell);

    cellLayout->setContentsMargins(0, 0, 0, 0);
    cellLayout->setSpacing(4);

    auto* object = new DraggableObject(parameter, this, cell);
    auto* deleteButton = new QPushButton("Remove", cell);
    deleteButton->setFixedWidth(150);

    cellLayout->addWidget(object, 0, Qt::AlignCenter);
    cellLayout->addWidget(deleteButton);

    const int index = inputGridLayout->count();

    inputGridLayout->addWidget(
        cell, index / 2, index % 2,
        Qt::AlignTop | Qt::AlignLeft);

    connect(deleteButton, &QPushButton::clicked,
        this, [this, cell, parameter]()
        {
            inputParameters.erase(
                std::remove_if(
                    inputParameters.begin(),
                    inputParameters.end(),
                    [&parameter](const Parameter& candidate)
                    {
                        return candidate.id == parameter.id;
                    }),
                inputParameters.end());

            explicitAssignments.erase(parameter.id);

            inputGridLayout->removeWidget(cell);
            cell->deleteLater();

            while (QLayoutItem* item = inputGridLayout->takeAt(0))
                delete item;

            const auto cells =
                inputGridContainer->findChildren<QWidget*>(
                    QString(), Qt::FindDirectChildrenOnly);

            int index = 0;

            for (QWidget* widget : cells)
            {
                if (widget->isHidden())
                    continue;

                inputGridLayout->addWidget(
                    widget, index / 2, index % 2,
                    Qt::AlignTop | Qt::AlignLeft);

                ++index;
            }

            updateProgram();
        });
}

//clear input parameters (all)
void ParametrizationPage::clearInputObjects()
{
    inputParameters.clear();
    explicitAssignments.clear();

    while (QLayoutItem* item = inputGridLayout->takeAt(0))
    {
        if (item->widget())
            item->widget()->deleteLater();

        delete item;
    }

    updateProgram();
}

//run the parametrization depending on the mode (implicit or explicit)

void ParametrizationPage::runParametrization()
{
    if (programCombo->currentIndex() < 0)
    {
        QMessageBox::warning(
            this, "No program selected",
            "Select a program before running parametrization");
        return;
    }

    const int selectedId = programCombo->currentData().toInt();

    const auto programIt = std::find_if(
        programs.begin(), programs.end(),
        [selectedId](const Program& program)
        {
            return program.id == selectedId;
        });

    if (programIt == programs.end())
    {
        QMessageBox::critical(
            this, "Error", "The selected program could not be found!");
        return;
    }

    const Program& program = *programIt;

    try
    {
        AssignmentResult result;

        if (modeCombo->currentText() == "Implicit")
        {
            const std::vector<Parameter> programParameters =
                getFirstParameters(program);

            CostModel costModel;
            CostMatrix costMatrix(
                programParameters,
                inputParameters,
                costModel);

            GlobalImplicit method;

            result = method.parametrize(
                costMatrix,
                programParameters,
                inputParameters);
        }
        else
        {
            std::vector<Assignment> userAssignments;
            userAssignments.reserve(explicitAssignments.size());

            for (const auto& entry : explicitAssignments)
            {
                userAssignments.push_back({
                    entry.first,
                    entry.second,
                    0.0
                    });
            }

            GlobalExplicit method;

            result = method.applyUserAssignments(
                program,
                inputParameters,
                userAssignments);
        }

        ProgramInstantiator instantiator;

        const Program instantiatedProgram = instantiator.instantiate(
            program,
            inputParameters,
            result);

        renderProgram(instantiatedProgram);

        QMessageBox::information(
            this,
            "Parametrization successful",
            QString("Parametrization completed successfully.\n\n"
                "Assignments: %1\n"
                "Total cost: %2")
            .arg(static_cast<qulonglong>(result.assignments.size()))
            .arg(result.totalCost));
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            "Parametrization failed",
            QString::fromStdString(e.what()));
    }
}

