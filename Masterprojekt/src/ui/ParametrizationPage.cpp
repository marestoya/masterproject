#include "ui/ParametrizationPage.h"

#include "ui/ProgramLoader.h"
#include "cost/CostMatrix.h"
#include "cost/CostModel.h"
#include "model/ProgramInstantiator.h"
#include "parametrization/GlobalExplicit.h"
#include "parametrization/GlobalImplicit.h"
#include "RobotController.h"
#include "CommandExecutor.h"
#include <unity_connection.hpp>
#include <unity_simulation.hpp>
#include <programs/Programs.h>

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
#include <optional>
#include <iostream>

//TODO: Translation could be saved in json, for now thats fine i think
namespace
{
    constexpr const char* ParameterMimeType =
        "application/x-verbbot-parameter";

    QString parameterName(const Parameter& parameter, bool german)
    {
        QString type = QString::fromStdString(parameter.type);

        if (type == "testTubeHolderFoam")
        {
            type = german ? QStringLiteral("Reagenzglasständer")
                : QStringLiteral("test tube holder");
        }
        else if (german)
        {
            static const std::map<QString, QString> translations = {
                {"beaker", "Becherglas"},
                {"tray", "Tablett"},
                {"testTube", "Reagenzglas"},
                {"thermometer", "Thermometer"},
                {"box", "Box"},
                {"plate", "Teller"},
                {"cup", "Tasse"},
                {"spoon", "Löffel"},
                {"smallPlate", "Kleiner Teller"},
                {"glass", "Glas"},
                {"matchaTeaBag", "Matcha-Teebeutel"},
                {"hammer", "Hammer"},
                {"drill", "Bohrer"},
                {"gear", "Zahnrad"},
                {"vat", "Wanne"},
                {"wire", "Kabel"},
                {"battery", "Batterie"},
                {"button", "Knopf"},
                {"bowl", "Schüssel"},
                {"cake_stand", "Kuchenständer"},
                {"chamomileTea", "Kamillentee"},
                {"counter", "Theke"},
                {"cuttingBoard", "Schneidebrett"},
                {"fork", "Gabel"},
                {"fork_container", "Gabelbehälter"},
                {"juice", "Saft"},
                {"largeCup", "Große Tasse"},
                {"longSpoon", "Langer Löffel"},
                {"muffin", "Muffin"},
                {"napkin", "Serviette"},
                {"plate_stack", "Tellerstapel"},
                {"price_tag", "Preisschild"},
                {"sale_sticker", "Angebotsaufkleber"},
                {"small_table", "Kleiner Tisch"},
                {"spoon_container", "Löffelbehälter"},
                {"table", "Tisch"},
                {"tea_bag", "Teebeutel"},
                {"branch", "Zweig"},
                {"bolt", "Schraubbolzen"},
                {"circuit", "Schaltkreis"},
                {"connector", "Steckverbinder"},
                {"container", "Behälter"},
                {"id_card", "Ausweis"},
                {"light", "Leuchte"},
                {"module", "Modul"},
                {"panel", "Bedienfeld"},
                {"powerUnit", "Akku"},
                {"resistor", "Widerstand"},
                {"sensor", "Sensor"},
                {"switch", "Schalter"},
                {"temperature_sensor", "Temperatursensor"},
                {"barometer", "Barometer"},
                {"cotton_pad", "Wattepad"},
                {"liquid", "Flüssigkeit"},
                {"stirrer", "Rührstab"},
                {"testTubeRack", "Reagenzglasständer"},
                {"long_table_leg", "Langes Tischbein"},
                {"package", "Paket"},
                {"screw", "Schraube"},
                {"screwdriver", "Schraubendreher"},
                {"shelf", "Regal"},
                {"short_table_leg", "Kurzes Tischbein"},
                {"storage", "Lager"},
                {"table_plate", "Tischplatte"},
                {"toolbox", "Werkzeugkasten"},
                {"wrench", "Schraubenschlüssel"},
                {"chocolate_fountain", "Schokoladenbrunnen"}

            };

            auto it = translations.find(type);

            if (it != translations.end())
                type = it->second;
        }
        else
        {
            type.replace(QStringLiteral("_"), QStringLiteral(" "));

            type.replace(
                QRegularExpression(QStringLiteral("([a-z])([A-Z])")),
                QStringLiteral("\\1 \\2")
            );

            type = type.toLower();
        }

        QString parameterColorName =
            QString::fromStdString(parameter.color).toLower();

        if (parameterColorName.isEmpty())
            return type;

        if (german)
        {
            static const std::map<QString, QString> colorTranslations = {
                {"red", "Rot"},
                {"brown", "Braun"},
                {"green", "Grün"},
                {"blue", "Blau"},
                {"yellow", "Gelb"},
                {"white", "Weiss"},
                {"silver", "Silber"},
                {"glass", "Glas"},
                {"grey", "Grau"},
                {"black", "Schwarz"}
            };

            auto it = colorTranslations.find(parameterColorName);

            if (it != colorTranslations.end())
                parameterColorName = it->second;

            return QString("%1 (%2)").arg(type, parameterColorName);
        }

        return QString("%1 %2").arg(parameterColorName, type);
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

    QString defaultSlotText(const Program& program, int slotId, bool german)
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

        return parameterName(it->parameters.front(), german);
    }

    bool isGerman(const QObject* object)
    {
        return object &&
            object->property("studyLanguage").toString() == "German";
    }

    QString translatedDomain(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        static const std::map<QString, QString> translations = {
            {"Laboratory", "Labor"},
            {"Catering", "Gastronomie"},
            {"Workshop", "Werkstatt"},
            {"Electronics", "Elektronik"}
        };

        const auto it = translations.find(raw);
        return it != translations.end() ? it->second : raw;
    }

    QString translatedMode(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        if (raw == "Implicit")
            return "Implizit";

        if (raw == "Explicit")
            return "Explizit";

        return raw;
    }

    QString translatedColor(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        static const std::map<QString, QString> translations = {
            {"red", "Rot"},
            {"brown", "Braun"},
            {"green", "Grün"},
            {"blue", "Blau"},
            {"yellow", "Gelb"},
            {"white", "Weiss"},
            {"silver", "Silber"},
            {"glass", "Glas"},
            {"grey", "Grau"},
            {"black", "Schwarz"}
        };

        const auto it = translations.find(raw.toLower());
        return it != translations.end() ? it->second : raw;
    }

    QString translatedProgramName(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        static const std::map<QString, QString> translations = {
            {"Set up experiment", "Experiment vorbereiten"},
            {"Store samples", "Proben lagern"},
            {"Clear table", "Tisch abräumen"},
            {"Set up table", "Tisch decken"},
            {"Return tools", "Werkzeuge zurücklegen"},
            {"Galvanize a gear", "Ein Zahnrad verzinken"},
            {"Prepare sensor assembly", "Sensorbaugruppe vorbereiten"},
            {"Sort components", "Bauteile sortieren"}
        };

        auto it = translations.find(raw);
        return it != translations.end() ? it->second : raw;
    }

    QString translatedInstructionText(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        static const std::map<QString, QString> translations = {
            {"Pick up the ", "Nimm "},
            {"Place the ", "Platziere "},
            {"Put the ", "Lege "},
            {" on the ", " auf "},
            {" in the ", " in "},
            {" in the middle of the ", " in die Mitte von "},
            {" next to the ", " neben "},
            {" in front of the ", " vor "},
            {"Press the ", "Drücke "}
        };

        auto it = translations.find(raw);
        return it != translations.end() ? it->second : raw;
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
        : QLabel(parameterName(parameter, isGerman(modeOwner)), parent),
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
            .arg(parameterName(parameter, isGerman(modeOwner)))
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
        callback(std::move(callback)),
        originalText(initialText)
    {
        setProperty("slotId", slotId);
        setAcceptDrops(true);
        setAlignment(Qt::AlignCenter);
        setContentsMargins(5, 23, 24, 5);
        setMinimumSize(135, 55);
        setMaximumWidth(220);
        setWordWrap(true);
        setFrameShape(QFrame::StyledPanel);

        setToolTip(QString("Program slot %1").arg(slotId));

        updateStyle();
    }

    void setAssignment(const QString& assignedText)
    {
        isAssigned = !assignedText.isEmpty();

        setText(isAssigned ? assignedText : originalText);

        updateStyle();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (isAssigned)
        {
            const QFontMetrics metrics(painter.font());
            const QString labelText = originalText;

            const int labelWidth =
                metrics.horizontalAdvance(labelText) + 10;

            const QRect originalRect(5, 3, labelWidth, 17);

            painter.setPen(QPen(QColor("#81c784"), 1));
            painter.setBrush(QColor("#e8f5e9"));
            painter.drawRoundedRect(originalRect, 4, 4);

            painter.setPen(QColor("#2e7d32"));
            painter.drawText(
                originalRect,
                Qt::AlignCenter,
                labelText
            );
        }

        const QRect badgeRect(width() - 25, 4, 19, 17);
        const QColor badgeColor =
            slotBackgroundColor(slotId).darker(118);

        painter.setPen(QPen(badgeColor.darker(145), 1));
        painter.setBrush(badgeColor);
        painter.drawRoundedRect(badgeRect, 4, 4);

        painter.setPen(Qt::black);

        QFont badgeFont = painter.font();
        badgeFont.setBold(true);
        badgeFont.setPointSize(8);
        painter.setFont(badgeFont);

        painter.drawText(
            badgeRect,
            Qt::AlignCenter,
            slotIdentifier(slotId)
        );
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

    QString originalText;
    bool isAssigned = false;

    void updateStyle()
    {
        if (isAssigned)
        {
            setStyleSheet(
                "QLabel {"
                " background-color: #e8f5e9;"
                " color: #202124;"
                " border: 2px solid #4caf50;"
                " border-radius: 6px;"
                " padding: 5px;"
                "}"
            );
        }
        else
        {
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
    }
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
    setProperty("studyLanguage", "English");

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

    // Back button
    auto* backButton = new QPushButton("Back", this);
    backButton->setObjectName("backButton");

    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &ParametrizationPage::backToMenuRequested
    );

    // Top controls
    auto* topLayout = new QHBoxLayout;

    auto* domainLabel = new QLabel("Domain:", this);
    domainLabel->setObjectName("domainLabel");
    topLayout->addWidget(domainLabel);
    topLayout->addWidget(domainCombo);

    auto* programLabel = new QLabel("Program:", this);
    programLabel->setObjectName("programLabel");
    topLayout->addWidget(programLabel);
    topLayout->addWidget(programCombo);

    auto* modeLabel = new QLabel("Mode:", this);
    modeLabel->setObjectName("modeLabel");
    topLayout->addWidget(modeLabel);

    modeCombo->addItem("Implicit", "Implicit");
    modeCombo->addItem("Explicit", "Explicit");

    topLayout->addWidget(modeCombo);

    mainLayout->addLayout(topLayout);

    auto* contentLayout = new QHBoxLayout;
    auto* leftPanel = new QVBoxLayout;
    auto* rightPanel = new QVBoxLayout;

    auto* programTitle = new QLabel("Program", this);
    programTitle->setObjectName("programTitle");

    programTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold;");

    leftPanel->addWidget(programTitle);

    programPanel->setLayout(programLayout);

    auto* programScroll = new QScrollArea;
    programScroll->setWidgetResizable(true);
    programScroll->setWidget(programPanel);

    leftPanel->addWidget(programScroll);

    auto* inputTitle = new QLabel("Input objects", this);
    inputTitle->setObjectName("inputTitle");

    inputTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold;");

    rightPanel->addWidget(inputTitle);

    objectColorCombo->addItem("Red", "red");
    objectColorCombo->addItem("Brown", "brown");
    objectColorCombo->addItem("Green", "green");
    objectColorCombo->addItem("Blue", "blue");
    objectColorCombo->addItem("Yellow", "yellow");
    objectColorCombo->addItem("White", "white");
    objectColorCombo->addItem("Silver", "silver");
    objectColorCombo->addItem("Glass", "glass");
    objectColorCombo->addItem("Grey", "grey");
    objectColorCombo->addItem("Black", "black");
    objectColorCombo->addItem("No color", "");

    auto* objectControls = new QHBoxLayout;

    objectControls->addWidget(objectTypeCombo);
    objectControls->addWidget(objectColorCombo);

    auto* addButton = new QPushButton("+ Add object", this);
    addButton->setObjectName("addButton");

    objectControls->addWidget(addButton);

    rightPanel->addLayout(objectControls);

    auto* clearButton = new QPushButton(
        "Clear all input objects", this);

    clearButton->setObjectName("clearButton");

    rightPanel->addWidget(clearButton);

    inputGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    inputGridLayout->setSpacing(10);

    auto* inputScroll = new QScrollArea;
    inputScroll->setWidgetResizable(true);
    inputScroll->setWidget(inputGridContainer);

    inputScroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    inputScroll->setVerticalScrollBarPolicy(
        Qt::ScrollBarAsNeeded);

    rightPanel->addWidget(inputScroll);

    auto* leftContainer = new QWidget;
    leftContainer->setLayout(leftPanel);

    auto* rightContainer = new QWidget;
    rightContainer->setLayout(rightPanel);

    contentLayout->addWidget(leftContainer, 1);
    contentLayout->addWidget(rightContainer, 1);

    mainLayout->addLayout(contentLayout, 1);

    auto* actionLayout = new QHBoxLayout;

    auto* runButton = new QPushButton(
        "Run parametrization", this);

    runButton->setObjectName("runButton");

    actionLayout->addStretch();
    actionLayout->addWidget(runButton);

    mainLayout->addLayout(actionLayout);

    connect(
        addButton,
        &QPushButton::clicked,
        this,
        [this]() { addInputObject(); }
    );

    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        [this]() { clearInputObjects(); }
    );

    connect(
        runButton,
        &QPushButton::clicked,
        this,
        [this]() { runParametrization(); }
    );

    connect(
        domainCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) { updateProgramList(); }
    );

    connect(
        programCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) { updateProgram(); }
    );

    connect(
        modeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int)
        {
            const bool explicitMode =
                modeCombo->currentData().toString() == "Explicit";

            setProperty("explicitMode", explicitMode);

            setWindowTitle(
                explicitMode
                ? "VerbBot - Explicit Parametrization"
                : "VerbBot - Implicit Parametrization"
            );
        }
    );

    std::set<std::string> domains;

    for (const Program& program : programs)
        domains.insert(program.domain);

    for (const std::string& domain : domains)
    {
        const QString domainName =
            QString::fromStdString(domain);

        domainCombo->addItem(domainName, domainName);
    }

    updateProgramList();
}

//progtam dropdown

void ParametrizationPage::updateProgramList()
{
    const bool german = isGerman(this);
    const QString domain = domainCombo->currentData().toString();

    programCombo->blockSignals(true);
    programCombo->clear();

    for (const Program& program : programs)
    {
        if (QString::fromStdString(program.domain) == domain)
        {
            const QString rawName =
                QString::fromStdString(program.name);

            programCombo->addItem(
                translatedProgramName(rawName, german),
                program.id
            );
        }
    }

    programCombo->blockSignals(false);

    updateAvailableObjectTypes();
    clearInputObjects();

    updateProgram();
}


//show selected program

void ParametrizationPage::updateProgram()
{
    explicitAssignments.clear();

    const bool german = isGerman(this);

    if (programCombo->currentIndex() < 0)
    {
        while (QLayoutItem* item = programLayout->takeAt(0))
        {
            if (item->widget())
                item->widget()->deleteLater();

            delete item;
        }

        programLayout->addWidget(
            new QLabel(
                german
                ? "Keine Programme für diesen Bereich verfügbar"
                : "No programs available for this domain"
            )
        );

        programLayout->addStretch();
        return;
    }

    const int selectedId = programCombo->currentData().toInt();

    const auto it = std::find_if(
        programs.begin(),
        programs.end(),
        [selectedId](const Program& program)
        {
            return program.id == selectedId;
        }
    );

    if (it == programs.end())
        return;

    renderProgram(*it);
}

void ParametrizationPage::renderProgram(const Program& program)
{
    QVBoxLayout* layout = programLayout;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr)
    {
        if (item->widget())
            delete item->widget();

        delete item;
    }

    const bool german = isGerman(this);

    QString programName = translatedProgramName(
        QString::fromStdString(program.name),
        german
    );

    auto* titleLabel = new QLabel(programName, programPanel);
    titleLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold;"
    );
    layout->addWidget(titleLabel);
    QString assignmentHint;

    if (modeCombo->currentData().toString() == "Explicit")
    {
        assignmentHint = german
            ? "Ziehe jedes Eingabeobjekt auf den vorgesehenen Platz."
            : "Drag each input object onto its intended slot.";
    }
    else
    {
        assignmentHint = german
            ? "Die Eingabeobjekte werden automatisch zugeordnet."
            : "Input objects will be matched automatically.";
    }

    auto* hintLabel = new QLabel(assignmentHint, programPanel);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);

    auto refreshSlotDisplays = [this, german, &program]()
        {
            const auto labels = programPanel->findChildren<QLabel*>();

            for (auto* label : labels)
            {
                if (!label->property("slotId").isValid())
                    continue;

                auto* slot = dynamic_cast<DropSlot*>(label);

                if (!slot)
                    continue;

                const int slotId = label->property("slotId").toInt();

                auto it = explicitAssignments.find(slotId);

                if (it != explicitAssignments.end())
                {
                    const int inputId = it->second;

                    auto input = std::find_if(
                        inputParameters.begin(),
                        inputParameters.end(),
                        [inputId](const Parameter& parameter)
                        {
                            return parameter.id == inputId;
                        }
                    );

                    if (input != inputParameters.end())
                    {
                        slot->setAssignment(
                            parameterName(*input, german)
                        );
                        continue;
                    }
                }

                slot->setAssignment("");
            }
        };

    auto updateAssignment =
        [this, refreshSlotDisplays](int inputId, int slotId)
        {
            for (auto it = explicitAssignments.begin();
                it != explicitAssignments.end();)
            {
                if (it->second == inputId)
                    it = explicitAssignments.erase(it);
                else
                    ++it;
            }

            explicitAssignments[slotId] = inputId;

            refreshSlotDisplays();
        };

    for (const auto& instruction : program.instructions)
    {
        auto* instructionWidget = new QWidget(programPanel);

        auto* instructionLayout = new QHBoxLayout(instructionWidget);
        instructionLayout->setContentsMargins(0, 4, 0, 4);
        instructionLayout->setSpacing(4);

        for (const InstructionSegment& segment : instruction)
        {
            if (segment.isSlot)
            {
                auto* slot = new DropSlot(
                    segment.slotId,
                    defaultSlotText(
                        program,
                        segment.slotId,
                        german
                    ),
                    this,
                    updateAssignment,
                    instructionWidget
                );

                instructionLayout->addWidget(
                    slot,
                    0,
                    Qt::AlignVCenter
                );
            }
            else
            {
                QString translatedText =
                    translatedInstructionText(
                        QString::fromStdString(segment.text),
                        german
                    );

                auto* textLabel = new QLabel(
                    translatedText,
                    instructionWidget
                );

                textLabel->setWordWrap(true);

                instructionLayout->addWidget(
                    textLabel,
                    0,
                    Qt::AlignVCenter
                );
            }
        }

        instructionLayout->addStretch();
        layout->addWidget(instructionWidget);
    }

    layout->addStretch();

    refreshSlotDisplays();
}
//add param

void ParametrizationPage::addInputObject()
{
    const bool german = isGerman(this);

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

    auto* deleteButton = new QPushButton(
        german ? "Entfernen" : "Remove",
        cell
    );

    deleteButton->setFixedWidth(150);

    cellLayout->addWidget(object, 0, Qt::AlignCenter);
    cellLayout->addWidget(deleteButton);

    const int index = inputGridLayout->count();

    inputGridLayout->addWidget(
        cell, index / 2, index % 2,
        Qt::AlignTop | Qt::AlignLeft
    );

    connect(
        deleteButton,
        &QPushButton::clicked,
        this,
        [this, cell, parameter]()
        {
            inputParameters.erase(
                std::remove_if(
                    inputParameters.begin(),
                    inputParameters.end(),
                    [&parameter](const Parameter& candidate)
                    {
                        return candidate.id == parameter.id;
                    }),
                inputParameters.end()
            );

            for (auto it = explicitAssignments.begin();
                it != explicitAssignments.end();)
            {
                if (it->second == parameter.id)
                    it = explicitAssignments.erase(it);
                else
                    ++it;
            }

            inputGridLayout->removeWidget(cell);
            cell->deleteLater();

            while (QLayoutItem* item = inputGridLayout->takeAt(0))
                delete item;

            const auto cells =
                inputGridContainer->findChildren<QWidget*>(
                    QString(), Qt::FindDirectChildrenOnly
                );

            int index = 0;

            for (QWidget* widget : cells)
            {
                if (widget->isHidden())
                    continue;

                inputGridLayout->addWidget(
                    widget, index / 2, index % 2,
                    Qt::AlignTop | Qt::AlignLeft
                );

                ++index;
            }

            updateProgram();
        }
    );
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
    const bool german = isGerman(this);
    const QString domain = domainCombo->currentData().toString();

    objectTypeCombo->clear();

    auto addObjectType = [this, german](
        const QString& englishName,
        const QString& internalName)
        {
            Parameter parameter;
            parameter.type = internalName.toStdString();
            parameter.color = "";

            objectTypeCombo->addItem(
                parameterName(parameter, german),
                internalName
            );
        };

    if (domain == "Laboratory")
    {
        addObjectType("Barometer", "barometer");
        addObjectType("Beaker", "beaker");
        addObjectType("Box", "box");
        addObjectType("Cotton pad", "cotton_pad");
        addObjectType("Glass", "glass");
        addObjectType("Liquid", "liquid");
        addObjectType("Spoon", "spoon");
        addObjectType("Stirrer", "stirrer");
        addObjectType("Table", "table");
        addObjectType("Test tube", "testTube");
        addObjectType("Thermometer", "thermometer");
        addObjectType("Tray", "tray");
        addObjectType("Tube rack", "testTubeRack");
    }
    else if (domain == "Catering")
    {
        addObjectType("Bowl", "bowl");
        addObjectType("Cake stand", "cake_stand");
        addObjectType("Chamomile Tea", "chamomileTea");
        addObjectType("Chocolate fountain", "chocolate_fountain");
        addObjectType("Counter", "counter");
        addObjectType("Cutting Board", "cuttingBoard");
        addObjectType("Cup", "cup");
        addObjectType("Fork", "fork");
        addObjectType("Fork container", "fork_container");
        addObjectType("Glass", "glass");
        addObjectType("Juice", "juice");
        addObjectType("Large Cup", "largeCup");
        addObjectType("Long Spoon", "longSpoon");
        addObjectType("Muffin", "muffin");
        addObjectType("Napkin", "napkin");
        addObjectType("Plate", "plate");
        addObjectType("Plate stack", "plate_stack");
        addObjectType("Price tag", "price_tag");
        addObjectType("Sale sticker", "sale_sticker");
        addObjectType("Small Plate", "smallPlate");
        addObjectType("Small table", "small_table");
        addObjectType("Spoon", "spoon");
        addObjectType("Spoon container", "spoon_container");
        addObjectType("Table", "table");
        addObjectType("Tea bag", "tea_bag");
        addObjectType("Tray", "tray");
    }
    else if (domain == "Workshop")
    {
        addObjectType("Bolt", "bolt");
        addObjectType("Box", "box");
        addObjectType("Drill", "drill");
        addObjectType("Gear", "gear");
        addObjectType("Hammer", "hammer");
        addObjectType("Long table leg", "long_table_leg");
        addObjectType("Package", "package");
        addObjectType("Screw", "screw");
        addObjectType("Screwdriver", "screwdriver");
        addObjectType("Shelf", "shelf");
        addObjectType("Shelf", "shelf");
        addObjectType("Short table leg", "short_table_leg");
        addObjectType("Storage", "storage");
        addObjectType("Table", "table");
        addObjectType("Table plate", "table_plate");
        addObjectType("Toolbox", "toolbox");
        addObjectType("Tray", "tray");
        addObjectType("Vat", "vat");
        addObjectType("Wrench", "wrench");
    }
    else if (domain == "Electronics")
    {
        addObjectType("Battery", "battery");
        addObjectType("Branch", "branch");
        addObjectType("Bolt", "bolt");
        addObjectType("Box", "box");
        addObjectType("Button", "button");
        addObjectType("Circuit", "circuit");
        addObjectType("Connector", "connector");
        addObjectType("Container", "container");
        addObjectType("Gear", "gear");
        addObjectType("ID card", "id_card");
        addObjectType("Light", "light");
        addObjectType("Module", "module");
        addObjectType("Panel", "panel");
        addObjectType("Power unit", "powerUnit");
        addObjectType("Resistor", "resistor");
        addObjectType("Sensor", "sensor");
        addObjectType("Switch", "switch");
        addObjectType("Temperature sensor", "temperature_sensor");
        addObjectType("Tray", "tray");
        addObjectType("Wire", "wire");
    }
}

//run the parametrization depending on the mode (implicit or explicit)

bool ParametrizationPage::runParametrization(bool showMessage)
{
    const bool german = isGerman(this);

    if (participantId.trimmed().isEmpty())
    {
        QMessageBox::warning(
            this,
            german ? "Teilnehmer-ID erforderlich" : "Participant ID required",
            german
            ? "Bitte kehre zum Hauptmenü zurück und gib eine Teilnehmer-ID ein"
            : "Please return to the main menu and enter a participant ID"
        );
        return false;
    }

    if (programCombo->currentIndex() < 0)
    {
        QMessageBox::warning(
            this,
            german ? "Kein Programm ausgewählt" : "No program selected",
            german
            ? "Bitte wähle vor der Parametrisierung ein Programm aus"
            : "Select a program before running parametrization"
        );
        return false;
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
            german ? "Fehler" : "Error",
            german
            ? "Das ausgewählte Programm konnte nicht gefunden werden!"
            : "The selected program could not be found!"
        );
        return false;
    }

    const Program& program = *programIt;

    const QString mode = modeCombo->currentData().toString();

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

        Program instantiatedProgram = instantiator.instantiate(
            program,
            inputParameters,
            result
        );

        explicitAssignments.clear();

        for (const Assignment& assignment : result.assignments)
        {
            explicitAssignments[assignment.programParameterId] =
                assignment.inputParameterId;
        }

       //RENDER WITH ORIGINAL
        renderProgram(program);

       //show the assignments before unity connection
        QApplication::processEvents();

        // Robot execution
        {
            ursim::unity_simulation simulation;
            std::this_thread::sleep_for(std::chrono::seconds(2));

            RobotController robotController(simulation);

            if (!robotController.initializeScene(instantiatedProgram))
            {
                std::cerr << "Program execution aborted: scene validation failed\n";
                return false;
            }

            CommandExecutor executor(robotController);
            executor.execute(instantiatedProgram);
        }

        //No disconnection from unity needed, gets destroyed after leaving the scope!

        QJsonObject response;

        response["participant_id"] = participantId.trimmed();
        response["program_id"] = program.id;
        response["program_name"] = QString::fromStdString(program.name);
        response["domain"] = QString::fromStdString(program.domain);
        response["mode"] = mode;
        response["total_cost"] = result.totalCost;
        response["timestamp"] =
            QDateTime::currentDateTime().toString(Qt::ISODate);
        response["variant"] = studyVariant;

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
                    german ? "Speicherfehler" : "Save error",
                    german
                    ? "Die vorhandene Parametrisierungsdatei konnte nicht gelesen werden."
                    : "Could not read the existing parametrization responses file"
                );
                return false;
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
                        german ? "Speicherfehler" : "Save error",
                        german
                        ? "Die vorhandene JSON-Datei mit den Parametrisierungsergebnissen ist ungültig. Es wurde keine Antwort gespeichert"
                        : "The existing parametrization responses JSON is invalid. No response was saved"
                    );
                    return false;
                }

                const QJsonValue existingResponses =
                    existingDocument.object().value("responses");

                if (!existingResponses.isArray())
                {
                    QMessageBox::warning(
                        this,
                        german ? "Speicherfehler" : "Save error",
                        german
                        ? "Die vorhandene JSON-Datei enthält kein gültiges responses-Array"
                        : "The existing JSON does not contain a valid responses array"
                    );
                    return false;
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
                german ? "Speicherfehler" : "Save error",
                german
                ? "Die Parametrisierungsdatei konnte nicht zum Schreiben geöffnet werden"
                : "Could not open the parametrization responses file for writing"
            );
            return false;
        }

        const QByteArray jsonData =
            QJsonDocument(root).toJson(QJsonDocument::Indented);

        const qint64 bytesWritten = outputFile.write(jsonData);
        outputFile.close();

        if (bytesWritten != jsonData.size())
        {
            QMessageBox::warning(
                this,
                german ? "Speicherfehler" : "Save error",
                german
                ? "Die Antwort konnte nicht vollständig in die Datei geschrieben werden"
                : "The response could not be completely written to the file"
            );
            return false;
        }

        if (showMessage)
        {
            QMessageBox::information(
                this,
                german
                ? "Parametrisierung erfolgreich"
                : "Parametrization successful",
                german
                ? QString(
                    "Die Parametrisierung wurde erfolgreich abgeschlossen\n\n"
                    "Teilnehmer: %1\n"
                    "Zuordnung: %2\n"
                    "Zuordnungen: %3\n"
                    "Gesamtkosten: %4\n\n"
                    "Antwort gespeichert."
                )
                .arg(participantId.trimmed())
                .arg(mode == "Implicit" ? "Implizit" : "Explizit")
                .arg(static_cast<qulonglong>(result.assignments.size()))
                .arg(result.totalCost)
                : QString(
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

        return true;
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(
            this,
            german ? "Parametrisierung fehlgeschlagen" : "Parametrization failed",
            german
            ? QString("Bei der Parametrisierung ist ein Fehler aufgetreten:\n%1")
            .arg(QString::fromStdString(e.what()))
            : QString::fromStdString(e.what())
        );

        return false;
    }
}

void ParametrizationPage::setParticipantId(const QString& id)
{
    participantId = id.trimmed();
}

//used for the study mode
void ParametrizationPage::setStudyTask(
    int programId,
    const QString& mode,
    const QString& variant)
{
    const auto programIt = std::find_if(
        programs.begin(),
        programs.end(),
        [programId](const Program& program)
        {
            return program.id == programId;
        });

    if (programIt == programs.end())
        return;

    clearInputObjects();
    explicitAssignments.clear();

    const QString domain =
        QString::fromStdString(programIt->domain);

    domainCombo->setCurrentText(domain);

    const int programIndex =
        programCombo->findData(programId);

    if (programIndex < 0)
        return;

    programCombo->setCurrentIndex(programIndex);

    const int modeIndex =
        modeCombo->findText(mode);

    if (modeIndex >= 0)
        modeCombo->setCurrentIndex(modeIndex);

    studyVariant = variant;
}


void ParametrizationPage::setStudyLanguage(const QString& language)
{
    setProperty(
        "studyLanguage",
        language == "German" ? "German" : "English"
    );

    const bool german = isGerman(this);

    auto* domainLabel = findChild<QLabel*>("domainLabel");
    auto* programLabel = findChild<QLabel*>("programLabel");
    auto* modeLabel = findChild<QLabel*>("modeLabel");
    auto* programTitle = findChild<QLabel*>("programTitle");
    auto* inputTitle = findChild<QLabel*>("inputTitle");

    if (domainLabel)
        domainLabel->setText(german ? "Bereich:" : "Domain:");

    if (programLabel)
        programLabel->setText(german ? "Programm:" : "Program:");

    if (modeLabel)
        modeLabel->setText(german ? "Zuordnung:" : "Mode:");

    if (programTitle)
        programTitle->setText(german ? "Programm" : "Program");

    if (inputTitle)
        inputTitle->setText(
            german ? "Eingabeobjekte" : "Input objects"
        );

    auto* backButton = findChild<QPushButton*>("backButton");
    auto* addButton = findChild<QPushButton*>("addButton");
    auto* clearButton = findChild<QPushButton*>("clearButton");
    auto* runButton = findChild<QPushButton*>("runButton");

    if (backButton)
        backButton->setText(german ? "Zurück" : "Back");

    if (addButton)
        addButton->setText(
            german ? "+ Objekt hinzufügen" : "+ Add object"
        );

    if (clearButton)
        clearButton->setText(
            german
            ? "Alle Eingabeobjekte löschen"
            : "Clear all input objects"
        );

    if (runButton)
        runButton->setText(
            german
            ? "Parametrisierung starten"
            : "Run parametrization"
        );

    for (int i = 0; i < domainCombo->count(); ++i)
    {
        const QString rawDomain =
            domainCombo->itemData(i).toString();

        domainCombo->setItemText(
            i,
            translatedDomain(rawDomain, german)
        );
    }

    for (int i = 0; i < modeCombo->count(); ++i)
    {
        const QString rawMode =
            modeCombo->itemData(i).toString();

        modeCombo->setItemText(
            i,
            translatedMode(rawMode, german)
        );
    }

    for (int i = 0; i < objectColorCombo->count(); ++i)
    {
        const QString rawColor =
            objectColorCombo->itemData(i).toString();

        if (rawColor.isEmpty())
        {
            objectColorCombo->setItemText(
                i,
                german ? "Keine Farbe" : "No color"
            );
        }
        else
        {
            objectColorCombo->setItemText(
                i,
                translatedColor(rawColor, german)
            );
        }
    }

    const bool explicitMode =
        modeCombo->currentData().toString() == "Explicit";

    setWindowTitle(
        explicitMode
        ? (german
            ? "VerbBot - Explizite Parametrisierung"
            : "VerbBot - Explicit Parametrization")
        : (german
            ? "VerbBot - Implizite Parametrisierung"
            : "VerbBot - Implicit Parametrization")
    );

    updateProgramList();
}
