#include "ui/PlausibilityPage.h"
#include "StudyTask.h"
#include "ui/ProgramLoader.h"
#include <QApplication>
#include <QComboBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVariant>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QFileInfo>
#include <QDir>

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>
//TODO translation is ugly rn
namespace
{
    constexpr const char* ParameterMimeType = "application/x-parameter-id";

    bool isGerman(const QWidget* widget)
    {
        return widget && widget->property("studyLanguage").toString() == "German";
    }

    QString translatedText(const QString& english, bool german)
    {
        if (!german)
            return english;

        static const std::map<QString, QString> translations = {
            {"Back to Main Menu", "Zurück zum Hauptmenü"},
            {"Domain:", "Bereich:"},
            {"Task:", "Aufgabe:"},
            {"Variant:", "Variante:"},
            {"Program", "Programm"},
            {"Available objects", "Verfügbare Objekte"},
            {"Reset all assignments", "Alle Zuordnungen zurücksetzen"},
            {"Save user's choice", "Auswahl speichern"},
            {"No task selected.", "Keine Aufgabe ausgewählt."},
            {"No variants found for this task. Add its A/B/C/D entries to plausibility_stimuli.json.", "Für diese Aufgabe wurden keine Varianten gefunden. Bitte die A/B/C/D-Einträge in plausibility_stimuli.json ergänzen."},
            {"Drag each object onto the corresponding slot in the program", "Ziehe jedes Objekt auf den entsprechenden Platz im Programm."},
            {"There are no assignments to reset", "Es gibt keine Zuordnungen zum Zurücksetzen."},
            {"All assignments for this task have been reset", "Alle Zuordnungen für diese Aufgabe wurden zurückgesetzt."},
            {"Response saved for participant %1.", "Antwort für Teilnehmer %1 gespeichert."},
            {"Response saved", "Antwort gespeichert"},
            {"The choice for participant '%1' was saved successfully", "Die Auswahl für Teilnehmer '%1' wurde erfolgreich gespeichert."},
            {"Participant identification", "Teilnehmeridentifikation"},
            {"Enter participant ID or name:", "Teilnehmer-ID oder Namen eingeben:"},
            {"Missing participant ID", "Teilnehmer-ID fehlt"},
            {"Please enter a participant ID or name.", "Bitte eine Teilnehmer-ID oder einen Namen eingeben."},
            {"No task selected", "Keine Aufgabe ausgewählt"},
            {"Please select a task before saving", "Bitte vor dem Speichern eine Aufgabe auswählen."},
            {"No stimulus selected", "Keine Stimulusvariante ausgewählt"},
            {"Please select a valid stimulus variant before saving", "Bitte vor dem Speichern eine gültige Stimulusvariante auswählen."},
            {"Could not read responses", "Antworten konnten nicht gelesen werden"},
            {"Invalid response file", "Ungültige Antwortdatei"},
            {"The response file must contain a JSON object", "Die Antwortdatei muss ein JSON-Objekt enthalten."},
            {"The 'responses' field must be a JSON array", "Das Feld 'responses' muss ein JSON-Array enthalten."},
            {"Could not save response", "Antwort konnte nicht gespeichert werden"},
            {"An error occurred while writing the response file", "Beim Schreiben der Antwortdatei ist ein Fehler aufgetreten."},
            {"Program loading error", "Fehler beim Laden des Programms"},
            {"Could not load plausibility_programs.json.", "plausibility_programs.json konnte nicht geladen werden."},
            {"Plausibility stimuli", "Plausibilitätsstimuli"},
            {"Stimuli could not be loaded. Check plausibility_stimuli.json.", "Stimuli konnten nicht geladen werden. Bitte plausibility_stimuli.json überprüfen."},
            {"Already assigned. Reset its slot to use this object again.", "Bereits zugeordnet. Setze den Platz zurück, um dieses Objekt erneut zu verwenden."}
        };

        const auto it = translations.find(english);
        return it == translations.end() ? english : it->second;
    }

    QString translatedDomain(const QString& domain, bool german)
    {
        if (!german) return domain;
        if (domain == "Workshop") return "Werkstatt";
        if (domain == "Laboratory") return "Labor";
        if (domain == "Catering") return "Gastronomie";
        if (domain == "Electronics") return "Elektronik";
        return domain;
    }

    QString translatedProgramName(const std::string& name, bool german)
    {
        if (!german) return QString::fromStdString(name);
        static const std::map<std::string, QString> names = {
            {"Prepare work zone", "Arbeitsbereich vorbereiten"},
            {"Assemble table", "Tisch zusammenbauen"},
            {"Organize storage", "Lager organisieren"},
            {"Prepare sample storage", "Probenlager vorbereiten"},
            {"Assemble setup", "Aufbau zusammenstellen"},
            {"Mix a reagent", "Ein Reagenz mischen"},
            {"Set up a serving station", "Servierstation vorbereiten"},
            {"Prepare beverage service", "Getränkeservice vorbereiten"},
            {"Arrange buffet", "Buffet anrichten"},
            {"Assemble branches", "Zweige zusammenbauen"},
            {"Prepare sensor circuits", "Sensorschaltungen vorbereiten"},
            {"Organize control panels", "Bedienfelder organisieren"}
        };
        const auto it = names.find(name);
        return it == names.end() ? QString::fromStdString(name) : it->second;
    }


    QString translatedParameterType(const QString& raw, bool german)
    {
        if (!german)
            return raw;

        static const std::map<QString, QString> names = {
            {"toolbox", "Werkzeugkiste"}, {"storage 1", "Lager 1"},
            {"storage 2", "Lager 2"}, {"wrench", "Schraubenschlüssel"},
            {"table plate", "Tischplatte"}, {"working tray", "Arbeitsablage"},
            {"long table leg", "langes Tischbein"}, {"short table leg", "kurzes Tischbein"},
            {"screw", "Schraube"}, {"container", "Behälter"},
            {"shelf a", "Regal A"}, {"shelf b", "Regal B"},
            {"box", "Kiste"}, {"package", "Paket"}, {"sample rack", "Probenständer"},
            {"table", "Tisch"}, {"sample", "Probe"}, {"stirrer", "Rührstab"},
            {"beaker", "Becherglas"}, {"cotton pads", "Wattepads"},
            {"thermometer", "Thermometer"}, {"liquid", "Flüssigkeit"},
            {"serving tray", "Serviertablett"}, {"plate stack", "Tellerstapel"},
            {"fork container", "Gabelbehälter"}, {"spoon container", "Löffelbehälter"},{"napkins", "Servietten"},
            {"station a", "Station A"}, {"station b", "Station B"},
            {"drinks", "Getränke"}, {"cups", "Becher"}, {"price tags", "Preisschilder"},  {"sale stickers", "Angebotssticker"},
            {"cake stand", "Kuchenständer"}, {"muffins", "Muffins"}, {"cake", "Kuchen"},
            {"chocolate fountain", "Schokoladenbrunnen"}, {"small table", "kleiner Tisch"},
            {"plates", "Teller"}, {"light", "Lampe"}, {"branch 1", "Zweig 1"},
            {"branch 2", "Zweig 2"}, {"resistor", "Widerstand"},
            {"power connector", "Stromanschluss"}, {"temperature sensor", "Temperatursensor"},
            {"circuit a", "Schaltung A"}, {"circuit b", "Schaltung B"},
            {"connector", "Steckverbinder"}, {"id card", "Ausweis"},
            {"switch", "Schalter"}, {"panel a", "Bedienfeld A"},
            {"panel b", "Bedienfeld B"}, {"module", "Modul"},
            {"testtubeholderfoam", "Reagenzglasständer"},
            {"spoon", "Löffel"},
            {"glass", "Glas"},
            {"grey liquid", "graue Flüssigkeit"},
            {"barometer", "Barometer"},
            {"white package", "weißes Paket"},
            {"blue toolbox", "blaue Werkzeugkiste"},
            {"green wrench", "grüner Schraubenschlüssel"},
            {"counter", "Schalter"},
            {"bolt", "Bolzen"},
            {"wire", "Draht"},
            {"button", "Knopf"}

        };

        const QString key = raw.trimmed().toLower();
        const auto it = names.find(key);

        if (it != names.end())
            return it->second;

        QString result = raw.trimmed();
        result.replace("_", " ");
        result.replace(QRegularExpression("([a-z])([A-Z])"), "\\1 \\2");
        return result.toLower();
    }

    QString translatedColor(const QString& raw, bool german)
    {
        if (!german) return raw.toLower();
        static const std::map<QString, QString> colors = {
            {"red", "rot"}, {"blue", "blau"}, {"green", "grün"},
            {"white", "weiß"}, {"grey", "grau"}, {"gray", "grau"},
            {"brown", "braun"}, {"sale", "Sale"}
        };
        const QString key = raw.toLower();
        const auto it = colors.find(key);
        return it == colors.end() ? key : it->second;
    }

    QString displayParameterName(const Parameter& parameter, bool german)
    {
        QString type = translatedParameterType(
            QString::fromStdString(parameter.type),
            german
        );

        const QString color = translatedColor(
            QString::fromStdString(parameter.color),
            german
        );

        if (color.isEmpty())
            return type;

        if (german)
            return QString("%1 (%2)").arg(type, color);

        return QString("%1 %2").arg(color, type);
    }

    QString translatedInstructionText(const std::string& raw, bool german)
    {
        QString text = QString::fromStdString(raw);
        if (!german) return text;
        static const std::map<QString, QString> fragments = {
            {"Place the ", "Platziere "}, {"Put the ", "Lege "},
            {" in the ", " in "}, {" in ", " in "},
            {" on the ", " auf"}, {" on ", " auf "},
            {" next to the ", " neben "}, {" on the left of the ", " links neben "},
            {" on the right of the ", " rechts neben "}, {" and the ", " und "},
            {"Pour the ", "Gieße "}, {"Stir with the ", "Rühre mit "},
            {"Fasten the ", "Befestige "}, {"Glue the ", "Klebe "},
            {"Put another ", "Lege ein weiteres "}, {"Place a ", "Platziere "},
            {"Attach an ", "Befestige "}, {"Place ", "Platziere "},
            {" on all ", " auf allen "}, {" at ", " an "},
            {"Turn the ", "Schalte "}, {". ", ". "}, {" on.", " ein."}
        };
        const auto it = fragments.find(text);
        if (it != fragments.end()) return it->second;
        if (text == ".") return ".";
        if (text == " and ") return " und ";
        return text;
    }

    QString translatedContext(const QString& context, bool german)
    {
        if (!german || context.isEmpty()) return context;
        static const std::map<QString, QString> contexts = {
            {"The blue toolbox belongs to the green wrench.", "Die blaue Werkzeugkiste gehört zum grünen Schraubenschlüssel."},
            {"The bolt should be used to fasten the long table leg.", "Die Schraube soll zur Befestigung des langen Tischbeins verwendet werden."},
            {"The white package should be in the green container.", "Das weiße Paket soll sich im grünen Behälter befinden."},
            {"The spoon should be placed on the left of the sample rack.", "Der Löffel soll links neben dem Probenständer platziert werden."},
            {"The barometer should be inside the beaker.", "Das Barometer soll sich im Becherglas befinden."},
            {"The glass should be filled with the grey liquid.", "Das Glas soll mit der grauen Flüssigkeit gefüllt werden."},
            {"The spoon container should be at station B.", "Der Löffelbehälter soll an Station B stehen."},
            {"Sale stickers should be placed on cups.", "Die Sale-Aufkleber sollen auf Bechern platziert werden."},
            {"The red cake stand should be on the counter.", "Der rote Kuchenständer soll auf der Theke stehen."},
            {"Green light needs to be in branch 1.", "Die grüne Lampe muss sich in Zweig 1 befinden."},
            {"Blue wire needs to be in circuit A.", "Das blaue Kabel muss sich in Schaltung A befinden."},
            {"Red button needs to be switched on.", "Der rote Knopf muss eingeschaltet werden."}
        };
        const auto it = contexts.find(context);
        return it == contexts.end() ? context : it->second;
    }

    QString parameterName(const Parameter& parameter)
    {
        QString type = QString::fromStdString(parameter.type);

        if (type == "testTubeHolderFoam")
            type = "test tube holder";
        else
        {
            type.replace("_", " ");
            type.replace(
                QRegularExpression("([a-z])([A-Z])"),
                "\\1 \\2"
            );
        }

        type = type.toLower();

        QString color = QString::fromStdString(parameter.color).toLower();

        if (color.isEmpty())
            return type;

        return QString("%1 (%2)").arg(type, color);
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

    QString defaultSlotText(const Program& program, int slotId, bool german)
    {
        const auto it = std::find_if(program.parameters.begin(), program.parameters.end(),
            [slotId](const ParameterSlot& slot) { return slot.id == slotId; });
        if (it == program.parameters.end() || it->parameters.empty())
            return german ? QString("[Unbekannter Parameter %1]").arg(slotId) : QString("[Unknown parameter %1]").arg(slotId);
        return displayParameterName(it->parameters.front(), german);
    }
    //draggable parameter
    class PlausibilityDraggableObject : public QLabel
    {
    public:
        explicit PlausibilityDraggableObject(
            const Parameter& parameter,
            bool assigned,
            bool german,
            QWidget* parent = nullptr)
            : QLabel(displayParameterName(parameter, german), parent),
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
                setToolTip(german ? "Bereits zugeordnet. Setze den Platz zurück, um dieses Objekt erneut zu verwenden." : "Already assigned. Reset its slot to use this object again.");
            }
            else
            {
                setCursor(Qt::OpenHandCursor);
                setStyleSheet(
                    "QLabel { background: #e8f0fe; border: 1px solid #8aa9e6;"
                    " border-radius: 6px; padding: 6px; color: #202124; }");
                setToolTip(QString("%1 (ID: %2)")
                    .arg(displayParameterName(parameter, german))
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

        PlausibilityDropSlot(
            int slotId,
            const QString& initialText,
            bool german,
            AssignmentCallback callback,
            QWidget* parent = nullptr)
            : QLabel(initialText, parent),
            slotId(slotId),
            originalText(initialText),
            callback(std::move(callback))
        {
            setProperty("slotId", slotId);
            setAcceptDrops(true);
            setAlignment(Qt::AlignCenter);
            setContentsMargins(5, 23, 24, 5);
            setMinimumSize(135, 55);
            setMaximumWidth(220);
            setWordWrap(true);
            setFrameShape(QFrame::StyledPanel);

            setToolTip(german ? QString("Programmplatz %1").arg(slotId) : QString("Program slot %1").arg(slotId));

            updateStyle();
        }

        void setAssignment(const QString& assignedText)
        {
            isAssigned = !assignedText.isEmpty();

            setText(
                isAssigned
                ? assignedText
                : originalText
            );

            updateStyle();
            update();
        }

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            QLabel::paintEvent(event);

            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            //add the original param in the corner
            if (isAssigned)
            {
                QFont font = painter.font();
                font.setBold(true);
                font.setPointSize(8);
                painter.setFont(font);

                const QFontMetrics metrics(font);
                const int textWidth =
                    metrics.horizontalAdvance(originalText) + 12;

                const QRect nameRect(
                    4,
                    4,
                    std::min(textWidth, width() - 34),
                    17
                );

                painter.setPen(QPen(QColor("#388e3c"), 1));
                painter.setBrush(QColor("#c8e6c9"));
                painter.drawRoundedRect(nameRect, 4, 4);

                painter.setPen(QColor("#1b5e20"));
                painter.drawText(
                    nameRect,
                    Qt::AlignCenter,
                    metrics.elidedText(
                        originalText,
                        Qt::ElideRight,
                        nameRect.width() - 6
                    )
                );
            }

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

            painter.drawText(
                badgeRect,
                Qt::AlignCenter,
                slotIdentifier(slotId)
            );
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

            const int objectId =
                QString::fromUtf8(
                    event->mimeData()->data(ParameterMimeType)
                ).toInt(&ok);

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
        void updateStyle()
        {
            if (isAssigned)
            {
                setStyleSheet(
                    "QLabel { background-color: #e8f5e9; color: #1b5e20;"
                    " border: 2px solid #66bb6a; border-radius: 6px;"
                    " padding: 5px; }"
                );
            }
            else
            {
                const QColor bgColor = slotBackgroundColor(slotId);

                setStyleSheet(QString(
                    "QLabel { background-color: %1; color: black;"
                    " border: 2px solid %2; border-radius: 6px;"
                    " padding: 5px; }"
                ).arg(
                    bgColor.name(),
                    bgColor.darker(125).name()
                ));
            }
        }

        int slotId;
        QString originalText;
        bool isAssigned = false;
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
    backButton->setObjectName("backButton");
    pageLayout->addWidget(backButton, 0, Qt::AlignLeft);
    connect(backButton, &QPushButton::clicked, this, &PlausibilityPage::backToMenuRequested);

    auto* selectors = new QHBoxLayout;
    auto* domainLabel = new QLabel("Domain:", this);
    domainLabel->setObjectName("domainLabel");
    selectors->addWidget(domainLabel);
    selectors->addWidget(domainCombo, 1);
    auto* taskLabel = new QLabel("Task:", this);
    taskLabel->setObjectName("taskLabel");
    selectors->addWidget(taskLabel);
    selectors->addWidget(programCombo, 2);
    auto* variantLabel = new QLabel("Variant:", this);
    variantLabel->setObjectName("variantLabel");
    selectors->addWidget(variantLabel);
    selectors->addWidget(variantCombo);
    pageLayout->addLayout(selectors);

    auto* contentLayout = new QHBoxLayout;
    auto* leftPanel = new QVBoxLayout;
    auto* rightPanel = new QVBoxLayout;

    auto* programTitle = new QLabel("Program", this);
    programTitle->setObjectName("programTitle");
    programTitle->setStyleSheet("font-size: 18px; font-weight: bold;");
    leftPanel->addWidget(programTitle);
    programPanel->setLayout(programLayout);
    auto* programScroll = new QScrollArea(this);
    programScroll->setWidgetResizable(true);
    programScroll->setWidget(programPanel);
    leftPanel->addWidget(programScroll, 1);

    auto* objectsTitle = new QLabel("Available objects", this);
    objectsTitle->setObjectName("objectsTitle");
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
    auto* resetAllButton = new QPushButton("Reset all assignments", this);
    resetAllButton->setObjectName("resetAllButton");
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
                statusLabel->setText(translatedText("There are no assignments to reset", isGerman(this)));
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

            statusLabel->setText(translatedText("All assignments for this task have been reset", isGerman(this)));
        });

    auto* saveButton = new QPushButton("Save user's choice", this);
    saveButton->setObjectName("saveButton");
    saveButton->setMinimumHeight(40);
    saveButton->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white;"
        " font-weight: bold; padding: 8px 16px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #388e3c; }"
    );

    pageLayout->addWidget(saveButton, 0, Qt::AlignRight);

    connect(saveButton, &QPushButton::clicked,
        this, [this]()
        {
            saveUserChoice();
        });
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
        QMessageBox::critical(this, translatedText("Program loading error", isGerman(this)), QString::fromStdString(e.what()));
        statusLabel->setText(translatedText("Could not load plausibility_programs.json.", isGerman(this)));
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
        QMessageBox::warning(this, translatedText("Plausibility stimuli", isGerman(this)), QString::fromStdString(e.what()));
        statusLabel->setText(translatedText("Stimuli could not be loaded. Check plausibility_stimuli.json.", isGerman(this)));
    }

    std::set<std::string> domains;
    for (const Program& program : programs)
    {
        if (variantsByProgram.find(program.id) != variantsByProgram.end())
            domains.insert(program.domain);
    }
    for (const std::string& domain : domains)
        domainCombo->addItem(translatedDomain(QString::fromStdString(domain), isGerman(this)), QString::fromStdString(domain));

    updateProgramList();
}

void PlausibilityPage::updateProgramList()
{
    const QString selectedDomain = domainCombo->currentData().toString();
    programCombo->blockSignals(true);
    programCombo->clear();
    for (const Program& program : programs)
    {
        if (QString::fromStdString(program.domain) == selectedDomain &&
            variantsByProgram.find(program.id) != variantsByProgram.end())
        {
            programCombo->addItem(translatedProgramName(program.name, isGerman(this)), program.id);
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
        programLayout->addWidget(new QLabel(translatedText("No task selected.", isGerman(this))));
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
        statusLabel->setText(translatedText("No variants found for this task. Add its A/B/C/D entries to plausibility_stimuli.json.", isGerman(this)));
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
        contextLabel->setText(translatedContext(stimulus.context, isGerman(this)));
        contextLabel->show();
    }

    statusLabel->setText(translatedText("Drag each object onto the corresponding slot in the program", isGerman(this)));
    renderObjects();
    refreshSlotLabels(currentDisplayProgram);
}


void PlausibilityPage::renderProgram(
    const Program& program,
    const std::vector<int>& anotherSlots,
    const std::vector<int>& hiddenInstructions)
{
    clearLayout(programLayout);

    auto* title = new QLabel(translatedProgramName(program.name, isGerman(this)));
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
                    defaultSlotText(program, segment.slotId, isGerman(this)),
                    isGerman(this),
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
                QString text = translatedInstructionText(segment.text, isGerman(this));

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
                            defaultSlotText(program, nextSlotId, isGerman(this));

                        const bool pluralLabel =
                            (slotName.compare("price tags", Qt::CaseInsensitive) == 0 ||
                                slotName.compare("Preisschilder", Qt::CaseInsensitive) == 0);

                        if (isGerman(this))
                        {
                            // this is still pretty bad, maybe just roll back to always using "weiteres"
                            if (!text.contains("ein weiteres", Qt::CaseInsensitive))
                                text += pluralLabel ? "ein weiteres Set: " : "ein weiteres Objekt: ";
                        }
                        else
                        {
                            const QString replacement = pluralLabel ? "another set of " : "another ";
                            if (text.endsWith("the ", Qt::CaseInsensitive)) text.chop(4);
                            else if (text.endsWith("a ", Qt::CaseInsensitive)) text.chop(2);
                            else if (text.endsWith("an ", Qt::CaseInsensitive)) text.chop(3);
                            text += replacement;
                        }
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
        const bool assigned =
            objectToSlot.find(currentObjects[i].id) != objectToSlot.end();

        auto* object = new PlausibilityDraggableObject(
            currentObjects[i],
            assigned,
            isGerman(this),
            objectGridContainer
        );

        objectGridLayout->addWidget(
            object,
            static_cast<int>(i / 2),
            static_cast<int>(i % 2),
            Qt::AlignTop | Qt::AlignLeft
        );
    }

    objectGridLayout->setRowStretch(
        static_cast<int>((currentObjects.size() + 1) / 2), 1);
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

            const auto objectIt = std::find_if(
                currentObjects.begin(),
                currentObjects.end(),
                [&assignment](const Parameter& parameter)
                {
                    return parameter.id == assignment.first;
                }
            );

            if (objectIt != currentObjects.end())
                assignedNames.append(displayParameterName(*objectIt, isGerman(this)));
        }

        auto* slot = static_cast<PlausibilityDropSlot*>(label);

        slot->setAssignment(assignedNames.join(" + "));
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

bool PlausibilityPage::saveUserChoice(bool showMessage)
{
    QString responseParticipantId = participantId.trimmed();

    if (responseParticipantId.isEmpty())
    {
        bool ok = false;

        responseParticipantId = QInputDialog::getText(
            this,
            translatedText("Participant identification", isGerman(this)),
            translatedText("Enter participant ID or name:", isGerman(this)),
            QLineEdit::Normal,
            QString(),
            &ok
        ).trimmed();

        if (!ok)
            return false;

        if (responseParticipantId.isEmpty())
        {
            QMessageBox::warning(
                this,
                translatedText("Missing participant ID", isGerman(this)),
                translatedText("Please enter a participant ID or name.", isGerman(this))
            );
            return false;
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
            translatedText("No task selected", isGerman(this)),
            translatedText("Please select a task before saving", isGerman(this))
        );
        return false;
    }

    const QString variantName = variantCombo->currentText();
    const auto variantsIt = variantsByProgram.find(programId);

    if (variantsIt == variantsByProgram.end() ||
        variantsIt->second.find(variantName) == variantsIt->second.end())
    {
        QMessageBox::warning(
            this,
            translatedText("No stimulus selected", isGerman(this)),
            translatedText("Please select a valid stimulus variant before saving", isGerman(this))
        );
        return false;
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
                    translatedText("Could not read responses", isGerman(this)),
                    QString(isGerman(this) ? "Die vorhandene Antwortdatei enthält ungültiges JSON:\n%1" : "The existing response file contains invalid JSON:\n%1").arg(QString::fromStdString(e.what()))
                );
                return false;
            }
        }
    }

    if (root.is_null())
        root = nlohmann::json::object();

    if (!root.is_object())
    {
        QMessageBox::critical(
            this,
            translatedText("Invalid response file", isGerman(this)),
            translatedText("The response file must contain a JSON object", isGerman(this))
        );
        return false;
    }

    if (!root.contains("responses"))
        root["responses"] = nlohmann::json::array();

    if (!root["responses"].is_array())
    {
        QMessageBox::critical(
            this,
            translatedText("Invalid response file", isGerman(this)),
            translatedText("The 'responses' field must be a JSON array", isGerman(this))
        );
        return false;
    }

    root["responses"].push_back(response);

    {
        std::ofstream output(outputPath.toStdString());

        if (!output.is_open())
        {
            QMessageBox::critical(
                this,
                translatedText("Could not save response", isGerman(this)),
                (isGerman(this) ? "Die Antwortdatei konnte nicht zum Schreiben geöffnet werden:\n" : "Could not open the response file for writing:\n") + outputPath
            );
            return false;
        }

        output << root.dump(4) << '\n';

        if (!output.good())
        {
            QMessageBox::critical(
                this,
                translatedText("Could not save response", isGerman(this)),
                translatedText("An error occurred while writing the response file", isGerman(this))
            );
            return false;
        }
    }

    statusLabel->setText(
        translatedText("Response saved for participant %1.", isGerman(this))
        .arg(responseParticipantId)
    );

    if (showMessage)
    {
        QMessageBox::information(
            this,
            translatedText("Response saved", isGerman(this)),
            translatedText("The choice for participant '%1' was saved successfully", isGerman(this))
            .arg(responseParticipantId)
        );
    }

    return true;
}

void PlausibilityPage::setStudyLanguage(const QString& language)
{
    const bool german = (language.compare("German", Qt::CaseInsensitive) == 0 ||
        language.compare("Deutsch", Qt::CaseInsensitive) == 0);
    setProperty("studyLanguage", german ? "German" : "English");

    if (auto* label = findChild<QLabel*>("domainLabel"))
        label->setText(translatedText("Domain:", german));
    if (auto* label = findChild<QLabel*>("taskLabel"))
        label->setText(translatedText("Task:", german));
    if (auto* label = findChild<QLabel*>("variantLabel"))
        label->setText(translatedText("Variant:", german));
    if (auto* label = findChild<QLabel*>("programTitle"))
        label->setText(translatedText("Program", german));
    if (auto* label = findChild<QLabel*>("objectsTitle"))
        label->setText(translatedText("Available objects", german));
    if (auto* button = findChild<QPushButton*>("backButton"))
        button->setText(translatedText("Back to Main Menu", german));
    if (auto* button = findChild<QPushButton*>("resetAllButton"))
        button->setText(translatedText("Reset all assignments", german));
    if (auto* button = findChild<QPushButton*>("saveButton"))
        button->setText(translatedText("Save user's choice", german));

    domainCombo->blockSignals(true);
    for (int i = 0; i < domainCombo->count(); ++i)
    {
        const QString raw = domainCombo->itemData(i).toString();
        domainCombo->setItemText(i, translatedDomain(raw, german));
    }
    domainCombo->blockSignals(false);

    const int currentProgramId = programCombo->currentData().toInt();
    updateProgramList();
    const int index = programCombo->findData(currentProgramId);
    if (index >= 0)
        programCombo->setCurrentIndex(index);

    loadSelectedStimulus();
}

void PlausibilityPage::setParticipantId(const QString& participantId)
{
    this->participantId = participantId;
}

void PlausibilityPage::setStudyTask(
    int programId,
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

    const QString domain =
        QString::fromStdString(programIt->domain);

    domainCombo->setCurrentIndex(domainCombo->findData(domain));

    const int programIndex =
        programCombo->findData(programId);

    if (programIndex < 0)
        return;

    programCombo->setCurrentIndex(programIndex);

    const int variantIndex =
        variantCombo->findText(variant);

    if (variantIndex >= 0)
        variantCombo->setCurrentIndex(variantIndex);
}

