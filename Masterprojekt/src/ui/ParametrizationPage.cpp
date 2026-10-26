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
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

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

    updateAvailableObjectTypes();
    clearInputObjects();
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


void ParametrizationPage::updateAvailableObjectTypes()
{
    const QString domain = domainCombo->currentText();

    objectTypeCombo->clear();

    if (domain == "Laboratory")
    {
        objectTypeCombo->addItem("Beaker", "beaker");
        objectTypeCombo->addItem("Glass", "glass");
        objectTypeCombo->addItem("Test tube", "test_tube");
        objectTypeCombo->addItem("Sample tube", "sample_tube");
        objectTypeCombo->addItem("Tube rack", "test_tube_rack");
        objectTypeCombo->addItem("Sample rack", "sample_rack");
        objectTypeCombo->addItem("Sample", "sample");
        objectTypeCombo->addItem("Tray", "tray");
        objectTypeCombo->addItem("Table", "table");
        objectTypeCombo->addItem("Spoon", "spoon");
        objectTypeCombo->addItem("Stirrer", "stirrer");
        objectTypeCombo->addItem("Thermometer", "thermometer");
        objectTypeCombo->addItem("Barometer", "barometer");
        objectTypeCombo->addItem("Cotton pad", "cotton_pad");
        objectTypeCombo->addItem("Glove", "glove");
        objectTypeCombo->addItem("Liquid", "liquid");
    }
    else if (domain == "Catering")
    {
        objectTypeCombo->addItem("Plate", "plate");
        objectTypeCombo->addItem("Plate stack", "plate_stack");
        objectTypeCombo->addItem("Tray", "tray");
        objectTypeCombo->addItem("Serving tray", "serving_tray");
        objectTypeCombo->addItem("Cup", "cup");
        objectTypeCombo->addItem("Glass", "glass");
        objectTypeCombo->addItem("Spoon", "spoon");
        objectTypeCombo->addItem("Kettle", "kettle");
        objectTypeCombo->addItem("Spoon container", "spoon_container");
        objectTypeCombo->addItem("Fork container", "fork_container");
        objectTypeCombo->addItem("Napkin", "napkin");
        objectTypeCombo->addItem("Sale sticker", "sale_sticker");
        objectTypeCombo->addItem("Price tag", "price_tag");
        objectTypeCombo->addItem("Drink", "drink");
        objectTypeCombo->addItem("Champagne", "champagne");
        objectTypeCombo->addItem("Tea", "tea");
        objectTypeCombo->addItem("Cake", "cake");
        objectTypeCombo->addItem("Muffin", "muffin");
        objectTypeCombo->addItem("Cake stand", "cake_stand");
        objectTypeCombo->addItem("Chocolate fountain", "chocolate_fountain");
        objectTypeCombo->addItem("Table", "table");
        objectTypeCombo->addItem("Small table", "small_table");
        objectTypeCombo->addItem("Counter", "counter");
    }
    else if (domain == "Workshop")
    {
        objectTypeCombo->addItem("Toolbox", "toolbox");
        objectTypeCombo->addItem("Wrench", "wrench");
        objectTypeCombo->addItem("Hammer", "hammer");
        objectTypeCombo->addItem("Screwdriver", "screwdriver");
        objectTypeCombo->addItem("Gear", "gear");
        objectTypeCombo->addItem("Vault", "vault");
        objectTypeCombo->addItem("Container", "container");
        objectTypeCombo->addItem("Box", "box");
        objectTypeCombo->addItem("Package", "package");
        objectTypeCombo->addItem("Tray", "tray");
        objectTypeCombo->addItem("Shelf", "shelf");
        objectTypeCombo->addItem("Table", "table");
        objectTypeCombo->addItem("Table plate", "table_plate");
        objectTypeCombo->addItem("Table leg", "table_leg");
        objectTypeCombo->addItem("Long table leg", "long_table_leg");
        objectTypeCombo->addItem("Short table leg", "short_table_leg");
        objectTypeCombo->addItem("Screw", "screw");
    }
    else if (domain == "Electronics")
    {
        objectTypeCombo->addItem("Light", "light");
        objectTypeCombo->addItem("Resistor", "resistor");
        objectTypeCombo->addItem("Connector", "connector");
        objectTypeCombo->addItem("Power connector", "power_connector");
        objectTypeCombo->addItem("Wire", "wire");
        objectTypeCombo->addItem("Battery", "battery");
        objectTypeCombo->addItem("Button", "button");
        objectTypeCombo->addItem("Switch", "switch");
        objectTypeCombo->addItem("Module", "module");
        objectTypeCombo->addItem("Sensor", "sensor");
        objectTypeCombo->addItem("Temperature sensor", "temperature_sensor");
        objectTypeCombo->addItem("ID card", "id_card");
        objectTypeCombo->addItem("Branch", "branch");
        objectTypeCombo->addItem("Circuit", "circuit");
        objectTypeCombo->addItem("Panel", "panel");
        objectTypeCombo->addItem("Container", "container");
    }
}
//run the parametrization depending on the mode (implicit or explicit)


void ParametrizationPage::runParametrization()
{
    if (participantId.trimmed().isEmpty())
    {
        QMessageBox::warning(
            this,
            "Participant ID required",
            "Please return to the main menu and enter a participant ID."
        );
        return;
    }

    if (programCombo->currentIndex() < 0)
    {
        QMessageBox::warning(
            this,
            "No program selected",
            "Select a program before running parametrization."
        );
        return;
    }

    const int selectedId = programCombo->currentData().toInt();

    const auto programIt = std::find_if(
        programs.begin(),
        programs.end(),
        [selectedId](const Program& program)
        {
            return program.id == selectedId;
        }
    );

    if (programIt == programs.end())
    {
        QMessageBox::critical(
            this,
            "Error",
            "The selected program could not be found!"
        );
        return;
    }

    const Program& program = *programIt;
    const QString mode = modeCombo->currentText();

    try
    {
        AssignmentResult result;

        if (mode == "Implicit")
        {
            const std::vector<Parameter> programParameters =
                getFirstParameters(program);

            const std::string ontologyPath =
                "ontologies/" + program.domain + ".json";

            Ontology ontology(ontologyPath);
            CostModel costModel(ontology);

            CostMatrix costMatrix(
                programParameters,
                inputParameters,
                costModel
            );

            GlobalImplicit method;

            result = method.parametrize(
                costMatrix,
                programParameters,
                inputParameters
            );
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
                userAssignments
            );
        }

        ProgramInstantiator instantiator;
        const Program instantiatedProgram = instantiator.instantiate(
            program,
            inputParameters,
            result
        );

        //save the mapping in a Json file
        QJsonObject response;

        response["participant_id"] = participantId.trimmed();
        response["program_id"] = program.id;
        response["program_name"] = QString::fromStdString(program.name);
        response["domain"] = QString::fromStdString(program.domain);
        response["mode"] = mode;
        response["total_cost"] = result.totalCost;
        response["timestamp"] =
            QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonArray objectsArray;

        for (const Parameter& parameter : inputParameters)
        {
            QJsonObject object;

            object["object_id"] = parameter.id;
            object["type"] = QString::fromStdString(parameter.type);
            object["color"] = QString::fromStdString(parameter.color);

            QJsonValue assignedSlot = QJsonValue::Null;

            for (const Assignment& assignment : result.assignments)
            {
                if (assignment.inputParameterId == parameter.id)
                {
                    assignedSlot = assignment.programParameterId;
                    break;
                }
            }

            object["program_slot_id"] = assignedSlot;
            objectsArray.append(object);
        }

        response["objects"] = objectsArray;

        QJsonArray assignmentsArray;

        for (const Assignment& assignment : result.assignments)
        {
            QJsonObject assignmentObject;

            assignmentObject["input_parameter_id"] =
                assignment.inputParameterId;
            assignmentObject["program_parameter_id"] =
                assignment.programParameterId;
            assignmentObject["cost"] = assignment.cost;

            assignmentsArray.append(assignmentObject);
        }

        response["assignments"] = assignmentsArray;
        const QString outputPath =
            "C:/Users/marsp/source/repos/Masterprojekt/"
            "Masterprojekt/programs/parametrization_responses.json";

        QFile outputFile(outputPath);
        QJsonArray responsesArray;

        if (outputFile.exists())
        {
            if (!outputFile.open(QIODevice::ReadOnly))
            {
                QMessageBox::warning(
                    this,
                    "Save error",
                    "Could not read the existing parametrization responses file"
                );
                return;
            }

            const QByteArray existingData = outputFile.readAll();
            outputFile.close();

            if (!existingData.trimmed().isEmpty())
            {
                QJsonParseError parseError;

                const QJsonDocument existingDocument =
                    QJsonDocument::fromJson(existingData, &parseError);

                if (parseError.error != QJsonParseError::NoError ||
                    !existingDocument.isObject())
                {
                    QMessageBox::warning(
                        this,
                        "Save error",
                        "The existing parametrization responses JSON is invalid "
                        "No response was saved"
                    );
                    return;
                }

                const QJsonValue existingResponses =
                    existingDocument.object().value("responses");

                if (!existingResponses.isArray())
                {
                    QMessageBox::warning(
                        this,
                        "Save error",
                        "The existing JSON does not contain a valid responses array"
                    );
                    return;
                }

                responsesArray = existingResponses.toArray();
            }
        }

        responsesArray.append(response);

        QJsonObject root;
        root["responses"] = responsesArray;

        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QMessageBox::warning(
                this,
                "Save error",
                "Could not open the parametrization responses file for writing"
            );
            return;
        }

        const QByteArray jsonData =
            QJsonDocument(root).toJson(QJsonDocument::Indented);

        const qint64 bytesWritten = outputFile.write(jsonData);
        outputFile.close();

        if (bytesWritten != jsonData.size())
        {
            QMessageBox::warning(
                this,
                "Save error",
                "The response could not be completely written to the file"
            );
            return;
        }

        renderProgram(instantiatedProgram);

        QMessageBox::information(
            this,
            "Parametrization successful",
            QString(
                "Parametrization completed successfully.\n\n"
                "Participant: %1\n"
                "Mode: %2\n"
                "Assignments: %3\n"
                "Total cost: %4\n\n"
                "Response saved."
            )
            .arg(participantId.trimmed())
            .arg(mode)
            .arg(static_cast<qulonglong>(result.assignments.size()))
            .arg(result.totalCost)
        );
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            "Parametrization failed",
            QString::fromStdString(e.what())
        );
    }
}


void ParametrizationPage::setParticipantId(const QString& id)
{
    participantId = id.trimmed();
}

