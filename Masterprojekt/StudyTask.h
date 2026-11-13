
#pragma once

#include <QString>

enum class StudyPageType
{
    Plausibility,
    Parametrization
};

struct StudyTask
{
    StudyPageType pageType;
    QString domain;
    int programId;
    int taskNumber;
    QString variant;
};
