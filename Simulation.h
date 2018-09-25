#if !defined(SIMULATION_H)
#define SIMULATION_H

#include "Types.h"

bool SimulateCombinations(const STarget& target, STargetSolve& targetSolve);
bool SimulateCombinationsFast(const STarget& target, STargetSolve& targetSolve);

#endif // !defined(SIMULATION_H)
