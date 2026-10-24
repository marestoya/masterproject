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

#include <algorithm>
#include <fstream>
#include <functional>
#include <utility>
#include <set>
#include <stdexcept>
#include <string>

//TODO make this page and the other page into a common class
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
        explicit PlausibilityDraggableObject(const Parameter& parameter, QWidget* parent = nullptr)
            : QLabel(parameterName(parameter), parent), parameterId(parameter.id), colorName(QString::fromStdString(parameter.color))
        {
            setProperty("parameterId", parameterId);
            setFrameShape(QFrame::StyledPanel);
            setAlignment(Qt::AlignCenter);
            setFixedSize(150, 64);
            setWordWrap(true);
            setCursor(Qt::OpenHandCursor);
            setStyleSheet(
                "QLabel { background: #e8f0fe; border: 1px solid #8aa9e6;"
                " border-radius: 6px; padding: 6px; color: #202124; }");
            setToolTip(QString("%1 (ID: %2)").arg(parameterName(parameter)).arg(parameter.id));
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
            if (!(event->buttons() & Qt::LeftButton) ||
                (event->pos() - dragStart).manhattanLength() < QApplication::startDragDistance())
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
    };

    //program slot
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

    auto* backButton = new QPushButton("← Back to Main Menu", this);
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
            throw std::runtime_error("plausibility_stimuli.json must contain a stimuli array");

        for (const auto& entry : root["stimuli"])
        {
            const int programId = entry.at("programId").get<int>();
            const QString variantName = QString::fromStdString(entry.at("variant").get<std::string>()).trimmed();
            if (variantName.isEmpty())
                continue;

            PlausibilityVariant variant;
            if (entry.contains("context") && entry["context"].is_string())
                variant.context = QString::fromStdString(entry["context"].get<std::string>());

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
        statusLabel->setText("Stimuli could not be loaded, check plausibility_stimuli.json");
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

    renderProgram(*programIt);

    const QString selectedVariant = variantCombo->currentText();
    const auto variantsIt = variantsByProgram.find(programId);
    if (variantsIt == variantsByProgram.end() || !variantsIt->second.count(selectedVariant))
    {
        contextLabel->hide();
        statusLabel->setText("No variants found for this task. Add its A/B/C/D entries to plausibility_stimuli.json.");
        renderObjects();
        return;
    }

    const PlausibilityVariant& stimulus = variantsIt->second.at(selectedVariant);
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

    statusLabel->setText("Drag each object onto the corresponding slot in the program.");
    renderObjects();
    refreshSlotLabels(*programIt);
}

void PlausibilityPage::renderProgram(const Program& program)
{
    clearLayout(programLayout);
    auto* title = new QLabel(QString::fromStdString(program.name));
    title->setWordWrap(true);
    title->setStyleSheet("font-size: 16px; font-weight: bold; padding: 8px;");
    programLayout->addWidget(title);

    int stepNumber = 1;
    for (const auto& instruction : program.instructions)
    {
        auto* stepFrame = new QFrame(programPanel);
        stepFrame->setFrameShape(QFrame::StyledPanel);
        auto* row = new QHBoxLayout(stepFrame);
        row->setSpacing(4);
        row->addWidget(new QLabel(QString("%1.").arg(stepNumber++), stepFrame), 0, Qt::AlignTop);

        for (const InstructionSegment& segment : instruction)
        {
            if (segment.isSlot)
            {
                auto* slot = new PlausibilityDropSlot(
                    segment.slotId,
                    defaultSlotText(program, segment.slotId),
                    [this, program](int objectId, int slotId)
                    {
                        objectToSlot[objectId] = slotId;
                        refreshSlotLabels(program);
                    }, stepFrame);
                row->addWidget(slot, 0, Qt::AlignVCenter);
            }
            else
            {
                auto* text = new QLabel(QString::fromStdString(segment.text), stepFrame);
                text->setWordWrap(true);
                row->addWidget(text, 0, Qt::AlignVCenter);
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
