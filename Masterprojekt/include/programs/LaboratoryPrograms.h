#pragma once

#include "model/Program.h"

Program getSetUpExperiment();
Program getStoreSamples();
Program getSynthesize();
Program getPrepareSolution();

void executeSetUpExperiment(const std::vector<Parameter>& parameters);
void executeStoreSamples(const std::vector<Parameter>& parameters);
void executeSynthesize(const std::vector<Parameter>& parameters);
void executePrepareSolution(const std::vector<Parameter>& parameters);