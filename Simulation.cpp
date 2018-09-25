#include "Simulation.h"

#include <ctime>

#include "Utils.h"

// Goal type as a template parameter allows the compiler to avoid a runtime branch on the type
template <EGoal eGoal>
void AccumulateEV(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam) {}

template <>
void AccumulateEV<GOAL_CONSECUTIVE_SPARK>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nMaxConsecutiveSpark >= nGoalParam)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_CONSECUTIVE_SPARK_FAILURE>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nMaxConsecutiveSparkFailure >= nGoalParam)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_SPARK>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nSpark >= nGoalParam)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_SPARK_FAILURE>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nSparkFailure >= nGoalParam)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_ACCUMULATED_FAVOR>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nAccumulatedFavor >= nGoalParam)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_MAX_FAVOR>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (status.nMaxFavor >= nGoalParam)
{	
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

template <>
void AccumulateEV<GOAL_FREE_TALK>(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, int nGoalParam)
{
	if (true)
	{
		fSuccess += status.fChance;
		fStrictEV += status.fModifiedAccumulatedFavor;
	}
}

static void AccumulateEV(double& fStrictEV, double& fSuccess, const SSimulationStatusFinal& status, EGoal eGoal, int nGoalParam)
{
	switch (eGoal)
	{
	case GOAL_CONSECUTIVE_SPARK:
		AccumulateEV<GOAL_CONSECUTIVE_SPARK>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_CONSECUTIVE_SPARK_FAILURE:
		AccumulateEV<GOAL_CONSECUTIVE_SPARK_FAILURE>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_SPARK:
		AccumulateEV<GOAL_SPARK>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_SPARK_FAILURE:
		AccumulateEV<GOAL_SPARK_FAILURE>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_ACCUMULATED_FAVOR:
		AccumulateEV<GOAL_ACCUMULATED_FAVOR>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_MAX_FAVOR:
		AccumulateEV<GOAL_MAX_FAVOR>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	case GOAL_FREE_TALK:
		AccumulateEV<GOAL_FREE_TALK>(fStrictEV, fSuccess, status, nGoalParam);
		break;
	}
}

static void SimulateHelper(SCombinationResult& result, SSimulationStatus& status, const SConstellation& constellation, const std::vector<SKnowledge>& vSlots, int nSlot)
{
	if (nSlot >= constellation.vSlotOrder.size())
	{
		// Less branches, faster
#define THING(eGoal) \
		{ \
			auto& aBest = result.bestCombinationStats.aBestCombinations[eGoal]; \
			const int nNumGoalParams = static_cast<int>(aBest.size()); \
			for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam) \
			{ \
				auto& best = aBest[nGoalParam]; \
				AccumulateEV<eGoal>(best.fStrictEV, best.fSuccessPercentage, status, nGoalParam); \
			} \
		}
			
		THING(GOAL_SPARK);
		THING(GOAL_SPARK_FAILURE);
		THING(GOAL_CONSECUTIVE_SPARK);
		THING(GOAL_CONSECUTIVE_SPARK_FAILURE);
		THING(GOAL_ACCUMULATED_FAVOR);
		THING(GOAL_MAX_FAVOR);
		THING(GOAL_FREE_TALK);
#undef THING
			
		// More branches, slower
		/*for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
		{
			const EGoal eGoal = static_cast<EGoal>(nGoal);

			auto& vBest = result.bestCombinationStats.aBestCombinations[nGoal];
			const int nNumGoalParams = static_cast<int>(vBest.size());
			for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
			{
				auto& best = vBest[nGoalParam];

				AccumulateEV(best.fStrictEV, best.fSuccessPercentage, status, eGoal, nGoalParam);
			}
		}*/
		return;
	}

	const SKnowledge& knowledge = vSlots[constellation.vSlotOrder[nSlot]];

	// Deal with current combo effects
	for (SComboEffect& comboEffect : status.vComboEffects)
	{
		if (comboEffect.nDelay > 0)
		{
			--comboEffect.nDelay;
			if (comboEffect.nDelay <= 0)
			{
				status.fTargetInterestLevel -= comboEffect.nInterest;
				status.nTargetFavor -= comboEffect.nFavor;
			}
		}
		else if (comboEffect.nLength > 0)
		{
			--comboEffect.nLength;
			if (comboEffect.nLength <= 0)
			{
				status.fTargetInterestLevel += comboEffect.nInterest;
				status.nTargetFavor += comboEffect.nFavor;
			}
		}
	}

	// Apply new combo effects
	if (knowledge.comboEffect.nLength > 0)
	{
		status.vComboEffects.push_back(knowledge.comboEffect);
	}

	const double fSparkChance = std::min(knowledge.fInterest / std::max(status.fTargetInterestLevel, DBL_EPSILON), 1.0);

	// Spark
	{
		SSimulationStatus newStatus = status;

		const int nInterestGain = std::max(1, knowledge.nAverageFavor - newStatus.nTargetFavor);

		newStatus.nCurrentMaxFavor += nInterestGain;
		newStatus.nAccumulatedFavor += newStatus.nCurrentMaxFavor;
		++newStatus.nSpark;
		if (newStatus.nCurrentMaxFavor > newStatus.nMaxMaxFavor)
		{
			newStatus.nMaxMaxFavor = newStatus.nCurrentMaxFavor;
		}

		++newStatus.nCurrentConsecutiveSpark;
		newStatus.nCurrentConsecutiveSparkFailure = 0;
		if (newStatus.nCurrentConsecutiveSpark > newStatus.nMaxConsecutiveSpark)
		{
			newStatus.nMaxConsecutiveSpark = newStatus.nCurrentConsecutiveSpark;
		}

		newStatus.fChance *= fSparkChance;
		SimulateHelper(result, newStatus, constellation, vSlots, nSlot + 1);
	}

	if (fSparkChance >= 1.0f)
	{
		return;
	}

	// Spark failure
	{
		SSimulationStatus newStatus = status;

		newStatus.nCurrentMaxFavor = 0;
		++newStatus.nSparkFailure;

		++newStatus.nCurrentConsecutiveSparkFailure;
		newStatus.nCurrentConsecutiveSpark = 0;
		if (newStatus.nCurrentConsecutiveSparkFailure > newStatus.nMaxConsecutiveSparkFailure)
		{
			newStatus.nMaxConsecutiveSparkFailure = newStatus.nCurrentConsecutiveSparkFailure;
		}

		newStatus.fChance *= (1.0f - fSparkChance);
		SimulateHelper(result, newStatus, constellation, vSlots, nSlot + 1);
	}
}

static void Simulate(SCombinationResult& result, int nTargetInterestLevel, int nTargetFavor, const SConstellation& constellation, const std::vector<SKnowledge>& vSlots)
{
	SSimulationStatus status;
	status.fTargetInterestLevel = static_cast<double>(nTargetInterestLevel);
	status.nTargetFavor = nTargetFavor;

	if (g_Env.bSafetyChecks)
	{
		if (vSlots.size() != constellation.nNumSlots)
		{
			printf("Slot count doesn't match constellations: %d vs. %d\n", static_cast<int>(vSlots.size()), constellation.nNumSlots);
			return;
		}
	}

	SimulateHelper(result, status, constellation, vSlots, 0);
}

bool SimulateCombinations(const STarget& target, STargetSolve& targetSolve)
{
	auto itConstellation = g_Env.mapConstellations.find(target.nConstellationID);
	if (itConstellation == g_Env.mapConstellations.end())
	{
		printf("Failed to find constellation: %d\n", target.nConstellationID);
		return false;
	}
	const SConstellation& constellation = itConstellation->second;

	auto itCategory = g_Env.mapKnowledgeCategories.find(target.nKnowledgeCategoryID);
	if (itCategory == g_Env.mapKnowledgeCategories.end())
	{
		printf("Failed to find knowledge category: %d\n", target.nKnowledgeCategoryID);
		return false;
	}
	const auto& category = itCategory->second;

	if (category.vKnowledge.empty())
	{
		printf("Refusing to simulate, category has no knowledge\n");
		return false;
	}

	printf("Generating knowledge combinations for:\n- Target: %s\n- Knowledge category: %d\n- Constellation: %d\n", target.sName.c_str(), target.nKnowledgeCategoryID, target.nConstellationID);
	{
		const int nNumCombinations = static_cast<int>(std::tgamma<size_t>(category.vKnowledge.size() + 1) / std::tgamma<size_t>((category.vKnowledge.size() - constellation.nNumSlots) + 1)) + 1;
		printf("Reserving space for %d combinations (%d total knowledge IDs)\n", nNumCombinations, nNumCombinations * constellation.nNumSlots);
		CMemory<TKnowledgeID>::Init(nNumCombinations * constellation.nNumSlots);
	}
	auto vKnowledgeCombinations = GenerateCombinationsAndPermutationsStaticMemory<TKnowledgeID, true>(category.vKnowledge, constellation.nNumSlots);
	printf("Generated %d combinations, beginning simulations\n", static_cast<int>(vKnowledgeCombinations.size()));
	
	SCombinationResult result;

	clock_t nSimStartTime = clock();
	clock_t nLastPrintTime = 0;
	for (int nKnowledgeCombination = 0 ; nKnowledgeCombination < static_cast<int>(vKnowledgeCombinations.size()) ; ++nKnowledgeCombination)
	{
		int nCurrentTime = clock();
		if (nLastPrintTime == 0 || static_cast<double>(nCurrentTime - nLastPrintTime) / CLOCKS_PER_SEC >= g_Env.fSolvePrintTime)
		{
			printf("Beginning simulation %d (%.2f%%) - %.3fs elapsed\n", nKnowledgeCombination, static_cast<double>(nKnowledgeCombination) / vKnowledgeCombinations.size() * 100.0f,
				static_cast<double>(nCurrentTime - nSimStartTime) / CLOCKS_PER_SEC);
			nLastPrintTime = nCurrentTime;
		}

		const auto& pKnowledgeCombination = vKnowledgeCombinations[nKnowledgeCombination];
		result.Clear();

		std::vector<SKnowledge> vKnowledgeSlots(constellation.nNumSlots);
		for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
		{
			const int nKnowledgeID = pKnowledgeCombination[nKnowledgeIDIndex];
			auto itKnowledge = g_Env.mapKnowledges.find(nKnowledgeID);
			if (itKnowledge == g_Env.mapKnowledges.end())
			{
				printf("Failed to find knowledge: %d\n", nKnowledgeID);
				return false;
			}
			SKnowledge& knowledge = itKnowledge->second;
			vKnowledgeSlots[nKnowledgeIDIndex] = knowledge;
		}

		Simulate(result, targetSolve.nInterestLevel, targetSolve.nFavor, constellation, vKnowledgeSlots);
		
		for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
		{
			const EGoal eGoal = static_cast<EGoal>(nGoal);

			auto& vTargetBest = targetSolve.bestCombinations.aBestCombinations[nGoal];
			auto& vResultBest = result.bestCombinationStats.aBestCombinations[nGoal];

			const int nNumGoalParams = static_cast<int>(vTargetBest.size());
			for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
			{
				auto& targetBest = vTargetBest[nGoalParam];
				auto& resultBest = vResultBest[nGoalParam];

				resultBest.fStrictEV *= resultBest.fSuccessPercentage;
				bool bReplaceBest = (targetBest.vKnowledge.empty());
				if (!bReplaceBest)
				{
					const double fSuccessDeltaEV = (targetBest.fSuccessPercentage - resultBest.fSuccessPercentage) * g_Env.fDeltaSuccessEVMultiplier;
					if (resultBest.fStrictEV > (targetBest.fStrictEV + fSuccessDeltaEV))
					{
						bReplaceBest = true;
					}
				}

				if (bReplaceBest)
				{
					targetBest.fStrictEV = resultBest.fStrictEV;
					targetBest.fSuccessPercentage = resultBest.fSuccessPercentage;
					targetBest.vKnowledge.resize(vKnowledgeSlots.size());
					for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
					{
						const int nKnowledgeID = vKnowledgeSlots[nKnowledgeIDIndex].nID;
						targetBest.vKnowledge[nKnowledgeIDIndex] = nKnowledgeID;
					}
				}
			}
		}
	}

	return true;
}

static void SimulateCombinationsFastGoal(EGoal eGoal, SCombinationResult& result, STargetSolve& targetSolve, const SConstellation& constellation, std::vector<SKnowledge> vKnowledgeSlots)
{
	result.Clear();

	// Sort knowledge to approximate a best permutation for the given goal - these are heuristics that I thought should work, but there may be better solutions
	switch (eGoal)
	{
	case GOAL_SPARK:
	case GOAL_SPARK_FAILURE:
	case GOAL_ACCUMULATED_FAVOR:
	case GOAL_FREE_TALK:
		std::sort(vKnowledgeSlots.begin(), vKnowledgeSlots.end(),
			[&targetSolve](const SKnowledge& lhs, const SKnowledge& rhs)
			{	
				const double fSparkChanceLHS = std::min(lhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);
				const double fSparkChanceRHS = std::min(rhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);

				return (lhs.nAverageFavor * fSparkChanceLHS) > (rhs.nAverageFavor * fSparkChanceRHS);
			}
		);
		break;

	case GOAL_MAX_FAVOR:
		std::sort(vKnowledgeSlots.begin(), vKnowledgeSlots.end(),
			[&targetSolve](const SKnowledge& lhs, const SKnowledge& rhs)
			{	
				const double fSparkChanceLHS = std::min(lhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);
				const double fSparkChanceRHS = std::min(rhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);

				return fSparkChanceLHS > fSparkChanceRHS;
			}
		);
		break;

	case GOAL_CONSECUTIVE_SPARK:
	{
		// Middle out
		auto vSorted = vKnowledgeSlots;
		std::sort(vSorted.begin(), vSorted.end(),
			[&targetSolve](const SKnowledge& lhs, const SKnowledge& rhs)
			{	
				const double fSparkChanceLHS = std::min(lhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);
				const double fSparkChanceRHS = std::min(rhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);

				return fSparkChanceLHS > fSparkChanceRHS;
			}
		);

		for (int nSortedIndex = 0 ; nSortedIndex < vSorted.size() ; ++nSortedIndex)
		{
			int nResultIndex = static_cast<int>(vKnowledgeSlots.size() / 2);
			if ((nSortedIndex % 2) == 1)
			{
				nResultIndex -= (nSortedIndex + 1) / 2;
			}
			else
			{
				nResultIndex += (nSortedIndex + 1) / 2;
			}
			vKnowledgeSlots[nResultIndex] = vSorted[nSortedIndex];
		}
	}
		break;

	case GOAL_CONSECUTIVE_SPARK_FAILURE:
		// Middle out
		auto vSorted = vKnowledgeSlots;
		std::sort(vSorted.begin(), vSorted.end(),
			[&targetSolve](const SKnowledge& lhs, const SKnowledge& rhs)
			{	
				const double fSparkChanceLHS = std::min(lhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);
				const double fSparkChanceRHS = std::min(rhs.fInterest / std::max(static_cast<double>(targetSolve.nInterestLevel), DBL_EPSILON), 1.0);

				return fSparkChanceLHS < fSparkChanceRHS;
			}
		);

		for (int nSortedIndex = 0 ; nSortedIndex < vSorted.size() ; ++nSortedIndex)
		{
			int nResultIndex = static_cast<int>(vKnowledgeSlots.size() / 2);
			if ((nSortedIndex % 2) == 1)
			{
				nResultIndex -= (nSortedIndex + 1) / 2;
			}
			else
			{
				nResultIndex += (nSortedIndex + 1) / 2;
			}
			vKnowledgeSlots[nResultIndex] = vSorted[nSortedIndex];
		}
		break;
	}

	Simulate(result, targetSolve.nInterestLevel, targetSolve.nFavor, constellation, vKnowledgeSlots);
		
	auto& vTargetBest = targetSolve.bestCombinations.aBestCombinations[eGoal];
	auto& vResultBest = result.bestCombinationStats.aBestCombinations[eGoal];

	const int nNumGoalParams = static_cast<int>(vTargetBest.size());
	for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
	{
		auto& targetBest = vTargetBest[nGoalParam];
		auto& resultBest = vResultBest[nGoalParam];

		resultBest.fStrictEV *= resultBest.fSuccessPercentage;
		bool bReplaceBest = (targetBest.vKnowledge.empty());
		if (!bReplaceBest)
		{
			const double fSuccessDeltaEV = (targetBest.fSuccessPercentage - resultBest.fSuccessPercentage) * g_Env.fDeltaSuccessEVMultiplier;
			if (resultBest.fStrictEV > (targetBest.fStrictEV + fSuccessDeltaEV))
			{
				bReplaceBest = true;
			}
		}

		if (bReplaceBest)
		{
			targetBest.fStrictEV = resultBest.fStrictEV;
			targetBest.fSuccessPercentage = resultBest.fSuccessPercentage;
			targetBest.vKnowledge.resize(vKnowledgeSlots.size());
			for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
			{
				const int nKnowledgeID = vKnowledgeSlots[nKnowledgeIDIndex].nID;
				targetBest.vKnowledge[nKnowledgeIDIndex] = nKnowledgeID;
			}
		}
	}
}

bool SimulateCombinationsFast(const STarget& target, STargetSolve& targetSolve)
{
	auto itConstellation = g_Env.mapConstellations.find(target.nConstellationID);
	if (itConstellation == g_Env.mapConstellations.end())
	{
		printf("Failed to find constellation: %d\n", target.nConstellationID);
		return false;
	}
	const SConstellation& constellation = itConstellation->second;

	auto itCategory = g_Env.mapKnowledgeCategories.find(target.nKnowledgeCategoryID);
	if (itCategory == g_Env.mapKnowledgeCategories.end())
	{
		printf("Failed to find knowledge category: %d\n", target.nKnowledgeCategoryID);
		return false;
	}
	const auto& category = itCategory->second;

	if (category.vKnowledge.empty())
	{
		printf("Refusing to simulate, category has no knowledge\n");
		return false;
	}

	printf("Generating knowledge combinations for:\n- Target: %s\n- Knowledge category: %d\n- Constellation: %d\n", target.sName.c_str(), target.nKnowledgeCategoryID, target.nConstellationID);
	{
		const int nNumCombinations = static_cast<int>(std::tgamma<size_t>(category.vKnowledge.size() + 1) / (std::tgamma<size_t>(constellation.nNumSlots + 1) * std::tgamma<size_t>((category.vKnowledge.size() - constellation.nNumSlots) + 1))) + 1;
		printf("Reserving space for %d combinations (%d total knowledge IDs)\n", nNumCombinations, nNumCombinations * constellation.nNumSlots);
		CMemory<TKnowledgeID>::Init(nNumCombinations * constellation.nNumSlots);
	}
	auto vKnowledgeCombinations = GenerateCombinationsAndPermutationsStaticMemory<TKnowledgeID, false>(category.vKnowledge, constellation.nNumSlots);
	printf("Generated %d combinations, beginning simulations\n", static_cast<int>(vKnowledgeCombinations.size()));

	clock_t nSimStartTime = clock();
	clock_t nLastPrintTime = 0;
	
	// Find the best combinations
	SCombinationResult result;
	for (int nKnowledgeCombination = 0 ; nKnowledgeCombination < static_cast<int>(vKnowledgeCombinations.size()) ; ++nKnowledgeCombination)
	{
		int nCurrentTime = clock();
		if (nLastPrintTime == 0 || static_cast<double>(nCurrentTime - nLastPrintTime) / CLOCKS_PER_SEC >= g_Env.fSolvePrintTime)
		{
			printf("Beginning simulation %d (%.2f%%) - %.3fs elapsed\n", nKnowledgeCombination, static_cast<double>(nKnowledgeCombination) / vKnowledgeCombinations.size() * 100.0f,
				static_cast<double>(nCurrentTime - nSimStartTime) / CLOCKS_PER_SEC);
			nLastPrintTime = nCurrentTime;
		}

		const auto& pKnowledgeCombination = vKnowledgeCombinations[nKnowledgeCombination];
		std::vector<SKnowledge> vKnowledgeSlots(constellation.nNumSlots);
		for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
		{
			const int nKnowledgeID = pKnowledgeCombination[nKnowledgeIDIndex];
			auto itKnowledge = g_Env.mapKnowledges.find(nKnowledgeID);
			if (itKnowledge == g_Env.mapKnowledges.end())
			{
				printf("Failed to find knowledge: %d\n", nKnowledgeID);
				return false;
			}
			SKnowledge& knowledge = itKnowledge->second;
			vKnowledgeSlots[nKnowledgeIDIndex] = knowledge;
		}

		for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
		{
			const EGoal eGoal = static_cast<EGoal>(nGoal);
			SimulateCombinationsFastGoal(eGoal, result, targetSolve, constellation, vKnowledgeSlots);
		}
	}

	/*struct SPermutationResult
	{
		std::vector<TKnowledgeID> vKnowledge;
		SCombinationResult result;
	};

	std::map<std::vector<TKnowledgeID>, std::vector<SPermutationResult>> mapPermutationResults;
	size_t zPermutations = 0;

	// Go through each goal result and solve for permutations
	for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
	{
		const EGoal eGoal = static_cast<EGoal>(nGoal);

		auto& vTargetBest = targetSolve.bestCombinations.aBestCombinations[eGoal];
		const int nNumGoalParams = static_cast<int>(vTargetBest.size());
		for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
		{
			auto& targetBest = vTargetBest[nGoalParam];
			
			int nCurrentTime = clock();
			if (nLastPrintTime == 0 || static_cast<double>(nCurrentTime - nLastPrintTime) / CLOCKS_PER_SEC >= g_Env.fSolvePrintTime)
			{
				printf("Solving permutations for goal %d, goal param %d - %.3fs elapsed\n", static_cast<int>(eGoal), nGoalParam,
					static_cast<double>(nCurrentTime - nSimStartTime) / CLOCKS_PER_SEC);
				nLastPrintTime = nCurrentTime;
			}

			auto& vPermutationResults = mapPermutationResults[targetBest.vKnowledge];
			if (vPermutationResults.empty())
			{
				std::vector<std::vector<TKnowledgeID>> vPermutations;
				GeneratePermutations(vPermutations, targetBest.vKnowledge);

				for (int nPermutationIndex = 0 ; nPermutationIndex < vPermutations.size() ; ++nPermutationIndex)
				{
					++zPermutations;

					const auto& vPermutation = vPermutations[nPermutationIndex];
					vPermutationResults.emplace_back();
					SPermutationResult& permutationResult = vPermutationResults.back();
					permutationResult.vKnowledge = vPermutation;

					std::vector<SKnowledge> vKnowledgeSlots(constellation.nNumSlots);
					for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
					{
						const TKnowledgeID nKnowledgeID = vPermutation[nKnowledgeIDIndex];
						auto itKnowledge = g_Env.mapKnowledges.find(nKnowledgeID);
						if (itKnowledge == g_Env.mapKnowledges.end())
						{
							printf("Failed to find knowledge: %d\n", nKnowledgeID);
							return false;
						}
						SKnowledge& knowledge = itKnowledge->second;
						vKnowledgeSlots[nKnowledgeIDIndex] = knowledge;
					}

					Simulate(permutationResult.result, targetSolve.nInterestLevel, targetSolve.nFavor, constellation, vKnowledgeSlots);
				}
			}

			for (SPermutationResult& permutationResult : vPermutationResults)
			{				
				auto& resultBest = permutationResult.result.bestCombinationStats.aBestCombinations[eGoal][nGoalParam];
				resultBest.fStrictEV *= resultBest.fSuccessPercentage;
				bool bReplaceBest = (targetBest.vKnowledge.empty());
				if (!bReplaceBest)
				{
					const double fSuccessDeltaEV = (targetBest.fSuccessPercentage - resultBest.fSuccessPercentage) * g_Env.fDeltaSuccessEVMultiplier;
					if (resultBest.fStrictEV > (targetBest.fStrictEV + fSuccessDeltaEV))
					{
						bReplaceBest = true;
					}
				}

				if (bReplaceBest)
				{
					targetBest.fStrictEV = resultBest.fStrictEV;
					targetBest.fSuccessPercentage = resultBest.fSuccessPercentage;
					targetBest.vKnowledge.resize(permutationResult.vKnowledge.size());
					for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < permutationResult.vKnowledge.size() ; ++nKnowledgeIDIndex)
					{
						const int nKnowledgeID = permutationResult.vKnowledge[nKnowledgeIDIndex];
						targetBest.vKnowledge[nKnowledgeIDIndex] = nKnowledgeID;
					}
				}
			}
		}
	}*/	

	struct SThing
	{
		EGoal eGoal;
		int nGoalParam;
	};
	std::map<std::vector<TKnowledgeID>, std::vector<SThing>> mapUniqueCombinations;
	for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
	{
		const EGoal eGoal = static_cast<EGoal>(nGoal);

		auto& vTargetBest = targetSolve.bestCombinations.aBestCombinations[eGoal];
		const int nNumGoalParams = static_cast<int>(vTargetBest.size());
		for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
		{
			auto& targetBest = vTargetBest[nGoalParam];
			mapUniqueCombinations[targetBest.vKnowledge].push_back({eGoal, nGoalParam});
		}
	}
	
	size_t zPermutations = 0;
	for (const auto& itCombination : mapUniqueCombinations)
	{
		auto& vKnowledge = itCombination.first;
		auto& vThings = itCombination.second;

		int nCurrentTime = clock();
		if (nLastPrintTime == 0 || static_cast<double>(nCurrentTime - nLastPrintTime) / CLOCKS_PER_SEC >= g_Env.fSolvePrintTime)
		{
			printf("Doing stuff - %.3fs elapsed\n", 
				static_cast<double>(nCurrentTime - nSimStartTime) / CLOCKS_PER_SEC);
			nLastPrintTime = nCurrentTime;
		}

		std::vector<std::vector<TKnowledgeID>> vPermutations;
		GeneratePermutations(vPermutations, vKnowledge);

		for (int nPermutationIndex = 0 ; nPermutationIndex < vPermutations.size() ; ++nPermutationIndex)
		{
			auto& vPermutation = vPermutations[nPermutationIndex];
			++zPermutations;

			std::vector<SKnowledge> vKnowledgeSlots(constellation.nNumSlots);
			for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
			{
				const TKnowledgeID nKnowledgeID = vPermutation[nKnowledgeIDIndex];
				auto itKnowledge = g_Env.mapKnowledges.find(nKnowledgeID);
				if (itKnowledge == g_Env.mapKnowledges.end())
				{
					printf("Failed to find knowledge: %d\n", nKnowledgeID);
					return false;
				}
				SKnowledge& knowledge = itKnowledge->second;
				vKnowledgeSlots[nKnowledgeIDIndex] = knowledge;
			}
			
			result.Clear();
			Simulate(result, targetSolve.nInterestLevel, targetSolve.nFavor, constellation, vKnowledgeSlots);

			for (const SThing& thing : vThings)
			{
				auto& resultBest = result.bestCombinationStats.aBestCombinations[thing.eGoal][thing.nGoalParam];
				auto& targetBest = targetSolve.bestCombinations.aBestCombinations[thing.eGoal][thing.nGoalParam];
				resultBest.fStrictEV *= resultBest.fSuccessPercentage;
				bool bReplaceBest = (targetBest.vKnowledge.empty());
				if (!bReplaceBest)
				{
					const double fSuccessDeltaEV = (targetBest.fSuccessPercentage - resultBest.fSuccessPercentage) * g_Env.fDeltaSuccessEVMultiplier;
					if (resultBest.fStrictEV > (targetBest.fStrictEV + fSuccessDeltaEV))
					{
						bReplaceBest = true;
					}
				}

				if (bReplaceBest)
				{
					targetBest.fStrictEV = resultBest.fStrictEV;
					targetBest.fSuccessPercentage = resultBest.fSuccessPercentage;
					targetBest.vKnowledge.resize(vKnowledgeSlots.size());
					for (int nKnowledgeIDIndex = 0 ; nKnowledgeIDIndex < vKnowledgeSlots.size() ; ++nKnowledgeIDIndex)
					{
						const int nKnowledgeID = vKnowledgeSlots[nKnowledgeIDIndex].nID;
						targetBest.vKnowledge[nKnowledgeIDIndex] = nKnowledgeID;
					}
				}
			}
		}
	}
	printf("Total number of permutations generated: %zd\n", zPermutations);

	return true;
}
