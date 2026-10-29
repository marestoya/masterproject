#include "ui/PlausibilityPage.h"

#include "ui/ProgramLoader.h"

#include <nlohmann/json.hpp>

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QFont>
#include <QPixmap>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QStringList>
#include <QVariant>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QLineEdit>

#include <algorithm>
#include <fstream>
#include <functional>
#include <utility>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
    constexpr const char* ParameterMimeType = "application/x-verbbot-plausibility-parameter";

    QString parameterName(const Parameter& parameter)
    {
        const QString color = QString::fromStdString(parameter.color).trimmed();
        const QString type = QString::fromStdString(parameter.type).trimmed();
        if (color.isEmpty())
            return type;
        return QString("%1 %2").arg(color, type);
    }

    QColor parameterColor(const QString& colorName)
    {
        QColor color(colorName);
        if (!color.isValid() || colorName.compare("glass", Qt::CaseInsensitive) == 0)
            return QColor("#b8c4cc");
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
            QColor("#F3E5F5"), QColor("#E3F2FD"), QColor("#E8F5E9"),
            QColor("#FFF3E0"), QColor("#FCE4EC"), QColor("#E0F7FA"),
            QColor("#FFFDE7"), QColor("#EDE7F6")
        };
        constexpr int count = sizeof(colors) / sizeof(colors[0]);
        return colors[(slotId - 1) % count];
    }

    QString defaultSlotText(const Program& program, int slotId)
    {
        const auto it = std::find_if(program.parameters.begin(), program.parameters.end(),
            [slotId](const ParameterSlot& slot) { return slot.id == slotId; });
        if (it == program.parameters.end() || it->parameters.empty())
            return QString("[Unknown parameter %1]").arg(slotId);
        return parameterName(it->parameters.front());
    }
    //draggable parameter
    class PlausibilityDraggableObject : public QLabel
    {
    public:
        explicit PlausibilityDraggableObject(
            const Parameter& parameter,
            bool assigned,
            QWidget* parent = nullptr)
            : QLabel(parameterName(parameter), parent),
            parameterId(parameter.id),
            colorName(QString::fromStdString(parameter.color)),
            isAssigned(assigned)
        {
            setProperty("parameterId", parameterId);
            setFrameShape(QFrame::StyledPanel);
            setAlignment(Qt::AlignCenter);
            setFixedSize(150, 64);
            setWordWrap(true);

            if (isAssigned)
            {
                setCursor(Qt::ForbiddenCursor);
                setStyleSheet(
                    "QLabel { background: #e0e0e0; border: 1px solid #999999;"
                    " border-radius: 6px; padding: 6px; color: #777777; }");
                setToolTip("Already assigned. Reset its slot to use this object again.");
            }
            else
            {
                setCursor(Qt::OpenHandCursor);
                setStyleSheet(
                    "QLabel { background: #e8f0fe; border: 1px solid #8aa9e6;"
                    " border-radius: 6px; padding: 6px; color: #202124; }");
                setToolTip(QString("%1 (ID: %2)")
                    .arg(parameterName(parameter))
                    .arg(parameter.id));
            }
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
            if (isAssigned)
                return;

            if (!(event->buttons() & Qt::LeftButton) ||
                (event->pos() - dragStart).manhattanLength()
                < QApplication::startDragDistance())
                return;

            QDrag drag(this);
            auto* mimeData = new QMimeData;
            mimeData->setData(ParameterMimeType, QByteArray::number(parameterId));
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
        bool isAssigned;
    };
    //param slot
    class PlausibilityDropSlot : public QLabel
    {
    public:
        using AssignmentCallback = std::function<void(int, int)>;

        PlausibilityDropSlot(int slotId, const QString& initialText,
            AssignmentCallback callback, QWidget* parent = nullptr)
            : QLabel(initialText, parent), slotId(slotId), callback(std::move(callback))
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
                "QLabel { background-color: %1; color: black; border: 2px solid %2;"
                " border-radius: 6px; padding: 5px; }")
                .arg(bgColor.name(), bgColor.darker(125).name()));
        }

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            QLabel::paintEvent(event);
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            const QRect badgeRect(width() - 25, 4, 19, 17);
            const QColor badgeColor = slotBackgroundColor(slotId).darker(118);
            painter.setPen(QPen(badgeColor.darker(145), 1));
            painter.setBrush(badgeColor);
            painter.drawRoundedRect(badgeRect, 4, 4);
            painter.setPen(Qt::black);
            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(8);
            painter.setFont(font);
            painter.drawText(badgeRect, Qt::AlignCenter, slotIdentifier(slotId));
        }

        void dragEnterEvent(QDragEnterEvent* event) override
        {
            if (event->mimeData()->hasFormat(ParameterMimeType))
                event->acceptProposedAction();
            else
                event->ignore();
        }

        void dropEvent(QDropEvent* event) override
        {
            if (!event->mimeData()->hasFormat(ParameterMimeType))
            {
                event->ignore();
                return;
            }
            bool ok = false;
            const int objectId = QString::fromUtf8(event->mimeData()->data(ParameterMimeType)).toInt(&ok);
            if (!ok)
            {
                event->ignore();
                return;
            }
            if (callback)
                callback(objectId, slotId);
            event->acceptProposedAction();
        }

    private:
        int slotId;
        AssignmentCallback callback;
    };
}

PlausibilityPage::PlausibilityPage(QWidget* parent)
    : QWidget(parent),
    domainCombo(new QComboBox(this)),
    programCombo(new QComboBox(this)),
    variantCombo(new QComboBox(this)),
    programPanel(new QWidget(this)),
    programLayout(new QVBoxLayout),
    objectGridContainer(new QWidget(this)),
    objectGridLayout(new QGridLayout(objectGridContainer)),
    contextLabel(new QLabel(this)),
    statusLabel(new QLabel(this))
{
    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(8, 8, 8, 8);

    auto* backButton = new QPushButton("Back to Main Menu", this);
    pageLayout->addWidget(backButton, 0, Qt::AlignLeft);
    connect(backButton, &QPushButton::clicked, this, &PlausibilityPage::backToMenuRequested);

    auto* selectors = new QHBoxLayout;
    selectors->addWidget(new QLabel("Domain:"));
    selectors->addWidget(domainCombo, 1);
    selectors->addWidget(new QLabel("Task:"));
    selectors->addWidget(programCombo, 2);
    selectors->addWidget(new QLabel("Variant:"));
    selectors->addWidget(variantCombo);
    pageLayout->addLayout(selectors);

    auto* contentLayout = new QHBoxLayout;
    auto* leftPanel = new QVBoxLayout;
    auto* rightPanel = new QVBoxLayout;

    auto* programTitle = new QLabel("Program", this);
    programTitle->setStyleSheet("font-size: 18px; font-weight: bold;");
    leftPanel->addWidget(programTitle);
    programPanel->setLayout(programLayout);
    auto* programScroll = new QScrollArea(this);
    programScroll->setWidgetResizable(true);
    programScroll->setWidget(programPanel);
    leftPanel->addWidget(programScroll, 1);

    auto* objectsTitle = new QLabel("Available objects", this);
    objectsTitle->setStyleSheet("font-size: 18px; font-weight: bold;");
    rightPanel->addWidget(objectsTitle);

    contextLabel->setWordWrap(true);
    contextLabel->setTextFormat(Qt::PlainText);
    contextLabel->setStyleSheet(
        "QLabel { background: #fff8e1; color: #000000; border: 1px solid #e0c878;"
        " border-radius: 6px; padding: 8px; }");
    contextLabel->hide();
    rightPanel->addWidget(contextLabel);

    objectGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    objectGridLayout->setSpacing(10);
    auto* objectScroll = new QScrollArea(this);
    objectScroll->setWidgetResizable(true);
    objectScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    objectScroll->setWidget(objectGridContainer);
    rightPanel->addWidget(objectScroll, 1);

    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("color: #666666; padding: 4px;");
    rightPanel->addWidget(statusLabel);

    auto* leftContainer = new QWidget(this);
    leftContainer->setLayout(leftPanel);
    auto* rightContainer = new QWidget(this);
    rightContainer->setLayout(rightPanel);
    contentLayout->addWidget(leftContainer, 1);
    contentLayout->addWidget(rightContainer, 1);
    pageLayout->addLayout(contentLayout, 1);
    //reset button
    auto* resetAllButton = new QPushButton("Reset all assignments", this);
    resetAllButton->setMinimumHeight(40);
    resetAllButton->setStyleSheet(
        "QPushButton { background-color: #c62828; color: white;"
        " font-weight: bold; padding: 8px 16px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #e53935; }"
    );

    pageLayout->addWidget(resetAllButton, 0, Qt::AlignRight);

    connect(resetAllButton, &QPushButton::clicked, this, [this]()
        {
            if (objectToSlot.empty())
            {
                statusLabel->setText("There are no assignments to reset");
                return;
            }

            objectToSlot.clear();

            const int programId = programCombo->currentData().toInt();
            const auto programIt = std::find_if(
                programs.begin(), programs.end(),
                [programId](const Program& program)
                {
                    return program.id == programId;
                }
            );

            if (programIt != programs.end())
                refreshSlotLabels(*programIt);

            renderObjects();

            statusLabel->setText("All assignments for this task have been reset");
        });

    //save the current mapping
    auto* saveButton = new QPushButton("Save user's choice", this);
    saveButton->setMinimumHeight(40);
    saveButton->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white;"
        " font-weight: bold; padding: 8px 16px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #388e3c; }"
    );

    pageLayout->addWidget(saveButton, 0, Qt::AlignRight);

    connect(saveButton, &QPushButton::clicked,
        this, &PlausibilityPage::saveUserChoice);
    connect(domainCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateProgramList(); });
    connect(programCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateVariantList(); });
    connect(variantCombo, &QComboBox::currentIndexChanged, this, [this](int) { loadSelectedStimulus(); });

    loadProgramsAndStimuli();
}

void PlausibilityPage::loadProgramsAndStimuli()
{
    const QString basePath = "C:/Users/marsp/source/repos/Masterprojekt/Masterprojekt/programs/";
    try
    {
        programs = loadPrograms((basePath + "plausibility_programs.json").toStdString());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Program loading error", QString::fromStdString(e.what()));
        statusLabel->setText("Could not load plausibility_programs.json.");
        return;
    }

    try
    {
        std::ifstream input((basePath + "plausibility_stimuli.json").toStdString());
        if (!input)
            throw std::runtime_error("Could not open plausibility_stimuli.json");
        nlohmann::json root;
        input >> root;

        if (!root.contains("stimuli") || !root["stimuli"].is_array())
            throw std::runtime_error("plausibility_stimuli.json must contain a 'stimuli' array.");

        for (const auto& entry : root["stimuli"])
        {
            const int programId = entry.at("programId").get<int>();
            const QString variantName = QString::fromStdString(entry.at("variant").get<std::string>()).trimmed();
            if (variantName.isEmpty())
                continue;

            PlausibilityVariant variant;
            if (entry.contains("context") && entry["context"].is_string())
                variant.context = QString::fromStdString(entry["context"].get<std::string>());

            if (entry.contains("programParameterOverrides") && entry["programParameterOverrides"].is_array())
            {
                for (const auto& parameterJson : entry["programParameterOverrides"])
                {
                    Parameter parameter{};
                    parameter.id = parameterJson.at("id").get<int>();
                    parameter.type = parameterJson.at("type").get<std::string>();
                    parameter.color = parameterJson.value("color", std::string{});
                    if (parameterJson.contains("number") && !parameterJson["number"].is_null())
                        parameter.number = parameterJson["number"].get<int>();
                    variant.programParameterOverrides.push_back(parameter);
                }
            }

            if (entry.contains("anotherSlots") && entry["anotherSlots"].is_array())
            {
                for (const auto& slotJson : entry["anotherSlots"])
                    variant.anotherSlots.push_back(slotJson.get<int>());
            }

            if (entry.contains("hiddenInstructions") &&
                entry["hiddenInstructions"].is_array())
            {
                for (const auto& instruction : entry["hiddenInstructions"])
                {
                    if (instruction.is_number_integer())
                        variant.hiddenInstructions.push_back(instruction.get<int>());
                }
            }

            int fallbackId = 1;
            if (entry.contains("objects") && entry["objects"].is_array())
            {
                for (const auto& objectJson : entry["objects"])
                {
                    Parameter parameter{};
                    parameter.id = objectJson.value("id", fallbackId++);
                    parameter.type = objectJson.at("type").get<std::string>();
                    parameter.color = objectJson.value("color", std::string{});
                    if (objectJson.contains("number") && !objectJson["number"].is_null())
                        parameter.number = objectJson["number"].get<int>();
                    variant.objects.push_back(parameter);
                }
            }
            variantsByProgram[programId][variantName] = variant;
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "Plausibility stimuli", QString::fromStdString(e.what()));
        statusLabel->setText("Stimuli could not be loaded. Check plausibility_stimuli.json.");
    }

    std::set<std::string> domains;
    for (const Program& program : programs)
    {
        if (variantsByProgram.find(program.id) != variantsByProgram.end())
            domains.insert(program.domain);
    }
    for (const std::string& domain : domains)
        domainCombo->addItem(QString::fromStdString(domain));

    updateProgramList();
}

void PlausibilityPage::updateProgramList()
{
    const QString selectedDomain = domainCombo->currentText();
    programCombo->blockSignals(true);
    programCombo->clear();
    for (const Program& program : programs)
    {
        if (QString::fromStdString(program.domain) == selectedDomain &&
            variantsByProgram.find(program.id) != variantsByProgram.end())
        {
            programCombo->addItem(QString::fromStdString(program.name), program.id);
        }
    }
    programCombo->blockSignals(false);
    updateVariantList();
}

void PlausibilityPage::updateVariantList()
{
    variantCombo->blockSignals(true);
    variantCombo->clear();
    const int programId = programCombo->currentData().toInt();
    const auto programIt = variantsByProgram.find(programId);
    if (programIt != variantsByProgram.end())
    {
        for (const auto& variant : programIt->second)
            variantCombo->addItem(variant.first);
    }
    variantCombo->blockSignals(false);
    loadSelectedStimulus();
}

void PlausibilityPage::loadSelectedStimulus()
{
    objectToSlot.clear();
    currentObjects.clear();
    currentAnotherSlots.clear();
    nextObjectId = 1;

    const int programId = programCombo->currentData().toInt();
    const auto programIt = std::find_if(programs.begin(), programs.end(),
        [programId](const Program& program) { return program.id == programId; });

    if (programIt == programs.end())
    {
        clearLayout(programLayout);
        programLayout->addWidget(new QLabel("No task selected."));
        renderObjects();
        contextLabel->hide();
        statusLabel->setText({});
        return;
    }

    const QString selectedVariant = variantCombo->currentText();
    const auto variantsIt = variantsByProgram.find(programId);
    if (variantsIt == variantsByProgram.end() || !variantsIt->second.count(selectedVariant))
    {
        currentDisplayProgram = *programIt;
        renderProgram(currentDisplayProgram);
        contextLabel->hide();
        statusLabel->setText("No variants found for this task. Add its A/B/C/D entries to plausibility_stimuli.json.");
        renderObjects();
        return;
    }

    const PlausibilityVariant& stimulus = variantsIt->second.at(selectedVariant);

    //here the override for variants C and D
    currentDisplayProgram = *programIt;
    for (const Parameter& overrideParameter : stimulus.programParameterOverrides)
    {
        const auto slotIt = std::find_if(
            currentDisplayProgram.parameters.begin(), currentDisplayProgram.parameters.end(),
            [&overrideParameter](const ParameterSlot& slot)
            {
                return slot.id == overrideParameter.id;
            });

        if (slotIt == currentDisplayProgram.parameters.end())
            continue;

        if (slotIt->parameters.empty())
            slotIt->parameters.push_back(overrideParameter);
        else
        {
            slotIt->parameters.front().type = overrideParameter.type;
            slotIt->parameters.front().color = overrideParameter.color;
            slotIt->parameters.front().descriptor = overrideParameter.descriptor;
            slotIt->parameters.front().number = overrideParameter.number;
        }
    }

    currentAnotherSlots = stimulus.anotherSlots;
    currentHiddenInstructions = stimulus.hiddenInstructions;

    renderProgram(
        currentDisplayProgram,
        currentAnotherSlots,
        currentHiddenInstructions
    );

    currentObjects = stimulus.objects;
    for (Parameter& object : currentObjects)
    {
        if (object.id <= 0)
            object.id = nextObjectId;
        nextObjectId = std::max(nextObjectId, object.id + 1);
    }

    if (stimulus.context.trimmed().isEmpty())
    {
        contextLabel->clear();
        contextLabel->hide();
    }
    else
    {
        contextLabel->setText(stimulus.context);
        contextLabel->show();
    }

    statusLabel->setText("Drag each object onto the corresponding slot in the program");
    renderObjects();
    refreshSlotLabels(currentDisplayProgram);
}


void PlausibilityPage::renderProgram(
    const Program& program,
    const std::vector<int>& anotherSlots,
    const std::vector<int>& hiddenInstructions)
{
    clearLayout(programLayout);

    auto* title = new QLabel(QString::fromStdString(program.name));
    title->setWordWrap(true);
    title->setStyleSheet("font-size: 16px; font-weight: bold; padding: 8px;");
    programLayout->addWidget(title);

    std::set<int> anotherAlreadyMentioned;
    int stepNumber = 1;

    for (std::size_t instructionIndex = 0;
        instructionIndex < program.instructions.size();
        ++instructionIndex)
    {
        const int instructionNumber =
            static_cast<int>(instructionIndex + 1);

        if (std::find(
            hiddenInstructions.begin(),
            hiddenInstructions.end(),
            instructionNumber) != hiddenInstructions.end())
        {
            continue;
        }

        const auto& instruction = program.instructions[instructionIndex];

        auto* stepFrame = new QFrame(programPanel);
        stepFrame->setFrameShape(QFrame::StyledPanel);

        auto* row = new QHBoxLayout(stepFrame);
        row->setSpacing(4);
        row->addWidget(
            new QLabel(QString("%1.").arg(stepNumber++), stepFrame),
            0,
            Qt::AlignTop
        );

        for (std::size_t segmentIndex = 0;
            segmentIndex < instruction.size();
            ++segmentIndex)
        {
            const InstructionSegment& segment = instruction[segmentIndex];

            if (segment.isSlot)
            {
                auto* slot = new PlausibilityDropSlot(
                    segment.slotId,
                    defaultSlotText(program, segment.slotId),
                    [this, program](int objectId, int slotId)
                    {
                        objectToSlot[objectId] = slotId;
                        refreshSlotLabels(program);
                    },
                    stepFrame
                );

                row->addWidget(slot, 0, Qt::AlignVCenter);
            }
            else
            {
                QString text = QString::fromStdString(segment.text);

                //another should be plkaced at the first mention
                if (segmentIndex + 1 < instruction.size() &&
                    instruction[segmentIndex + 1].isSlot)
                {
                    const int nextSlotId =
                        instruction[segmentIndex + 1].slotId;

                    const bool isAnotherSlot =
                        std::find(
                            anotherSlots.begin(),
                            anotherSlots.end(),
                            nextSlotId
                        ) != anotherSlots.end();

                    if (isAnotherSlot &&
                        !anotherAlreadyMentioned.count(nextSlotId))
                    {
                        const QString slotName =
                            defaultSlotText(program, nextSlotId);

                        const bool pluralLabel =
                            slotName.compare(
                                "price tags",
                                Qt::CaseInsensitive
                            ) == 0;

                        const QString replacement =
                            pluralLabel ? "another set of " : "another ";

                        if (text.endsWith("the ", Qt::CaseInsensitive))
                            text.chop(4);
                        else if (text.endsWith("a ", Qt::CaseInsensitive))
                            text.chop(2);
                        else if (text.endsWith("an ", Qt::CaseInsensitive))
                            text.chop(3);

                        text += replacement;
                        anotherAlreadyMentioned.insert(nextSlotId);
                    }
                }

                auto* label = new QLabel(text, stepFrame);
                label->setWordWrap(true);
                row->addWidget(label, 0, Qt::AlignVCenter);
            }
        }

        row->addStretch();
        programLayout->addWidget(stepFrame);
    }

    programLayout->addStretch();
}


void PlausibilityPage::renderObjects()
{
    clearLayout(objectGridLayout);
    for (std::size_t i = 0; i < currentObjects.size(); ++i)
    {
        auto* object = new PlausibilityDraggableObject(currentObjects[i], objectGridContainer);
        objectGridLayout->addWidget(object, static_cast<int>(i / 2), static_cast<int>(i % 2), Qt::AlignTop | Qt::AlignLeft);
    }
    objectGridLayout->setRowStretch(static_cast<int>((currentObjects.size() + 1) / 2), 1);
}

void PlausibilityPage::refreshSlotLabels(const Program& program)
{
    const auto labels = programPanel->findChildren<QLabel*>();
    for (QLabel* label : labels)
    {
        const QVariant slotProperty = label->property("slotId");
        if (!slotProperty.isValid())
            continue;

        const int slotId = slotProperty.toInt();
        QStringList assignedNames;
        for (const auto& assignment : objectToSlot)
        {
            if (assignment.second != slotId)
                continue;
            const auto objectIt = std::find_if(currentObjects.begin(), currentObjects.end(),
                [&assignment](const Parameter& parameter) { return parameter.id == assignment.first; });
            if (objectIt != currentObjects.end())
                assignedNames.append(parameterName(*objectIt));
        }

        label->setText(assignedNames.isEmpty() ? defaultSlotText(program, slotId) : assignedNames.join(" + "));
    }
}


void PlausibilityPage::clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
            widget->deleteLater();

        if (QLayout* childLayout = item->layout())
            clearLayout(childLayout);

        delete item;
    }
}

void PlausibilityPage::saveUserChoice()
{
    QString responseParticipantId = participantId.trimmed();

    if (responseParticipantId.isEmpty())
    {
        bool ok = false;

        responseParticipantId = QInputDialog::getText(
            this,
            "Participant identification",
            "Enter participant ID or name:",
            QLineEdit::Normal,
            QString(),
            &ok
        ).trimmed();

        if (!ok)
            return;

        if (responseParticipantId.isEmpty())
        {
            QMessageBox::warning(
                this,
                "Missing participant ID",
                "Please enter a participant ID or name."
            );
            return;
        }

        participantId = responseParticipantId;
    }
    const int programId = programCombo->currentData().toInt();

    const auto programIt = std::find_if(
        programs.begin(),
        programs.end(),
        [programId](const Program& program)
        {
            return program.id == programId;
        }
    );

    if (programIt == programs.end())
    {
        QMessageBox::warning(
            this,
            "No task selected",
            "Please select a task before saving"
        );
        return;
    }

    const QString variantName = variantCombo->currentText();
    const auto variantsIt = variantsByProgram.find(programId);

    if (variantsIt == variantsByProgram.end() ||
        variantsIt->second.find(variantName) == variantsIt->second.end())
    {
        QMessageBox::warning(
            this,
            "No stimulus selected",
            "Please select a valid stimulus variant before saving"
        );
        return;
    }

    const PlausibilityVariant& stimulus =
        variantsIt->second.at(variantName);

    nlohmann::json response;
    response["participant_id"] = responseParticipantId.toStdString();
    response["program_id"] = programIt->id;
    response["domain"] = programIt->domain;
    response["task"] = programIt->name;
    response["variant"] = variantName.toStdString();
    response["context"] = stimulus.context.toStdString();
    response["mapping"] = nlohmann::json::array();

    for (const Parameter& object : currentObjects)
    {
        nlohmann::json mappingEntry;

        mappingEntry["input_object"] = {
            {"id", object.id},
            {"type", object.type},
            {"color", object.color}
        };

        const auto assignmentIt = objectToSlot.find(object.id);

        if (assignmentIt != objectToSlot.end())
            mappingEntry["program_slot_id"] = assignmentIt->second;
        else
            mappingEntry["program_slot_id"] = nullptr;

        response["mapping"].push_back(mappingEntry);
    }

    const QString outputPath =
        "C:/Users/marsp/source/repos/Masterprojekt/"
        "Masterprojekt/programs/plausibility_responses.json";

    nlohmann::json root;
    {
        std::ifstream input(outputPath.toStdString());

        if (input.good())
        {
            try
            {
                input >> root;
            }
            catch (const std::exception& e)
            {
                QMessageBox::critical(
                    this,
                    "Could not read responses",
                    QString(
                        "The existing response file contains invalid JSON:\n%1"
                    ).arg(QString::fromStdString(e.what()))
                );
                return;
            }
        }
    }

    if (root.is_null())
        root = nlohmann::json::object();

    if (!root.is_object())
    {
        QMessageBox::critical(
            this,
            "Invalid response file",
            "The response file must contain a JSON object"
        );
        return;
    }

    if (!root.contains("responses"))
        root["responses"] = nlohmann::json::array();

    if (!root["responses"].is_array())
    {
        QMessageBox::critical(
            this,
            "Invalid response file",
            "The 'responses' field must be a JSON array"
        );
        return;
    }

    root["responses"].push_back(response);

    {
        std::ofstream output(outputPath.toStdString());

        if (!output.is_open())
        {
            QMessageBox::critical(
                this,
                "Could not save response",
                "Could not open the response file for writing:\n" + outputPath
            );
            return;
        }

        output << root.dump(4) << '\n';

        if (!output.good())
        {
            QMessageBox::critical(
                this,
                "Could not save response",
                "An error occurred while writing the response file"
            );
            return;
        }
    }

    statusLabel->setText(
        QString("Response saved for participant %1.")
        .arg(responseParticipantId)
    );

    QMessageBox::information(
        this,
        "Response saved",
        QString("The choice for participant '%1' was saved successfully")
        .arg(responseParticipantId)
    );
}

void PlausibilityPage::setParticipantId(const QString& participantId)
{
    this->participantId = participantId;
}


void PlausibilityPage::setParticipantId(const QString& participantId)
{
    this->participantId = participantId;
}
