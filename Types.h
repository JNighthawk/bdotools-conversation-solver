#if !defined(TYPES_H)
#define TYPES_H

#include <array>
#include <vector>
#include <random>
#include <map>

enum EGoal
{
	GOAL_SPARK,
	GOAL_SPARK_FAILURE,
	GOAL_CONSECUTIVE_SPARK,
	GOAL_CONSECUTIVE_SPARK_FAILURE,
	GOAL_ACCUMULATED_FAVOR,
	GOAL_MAX_FAVOR,
	GOAL_FREE_TALK,

	NUM_GOALS,
};
extern const std::array<std::string, NUM_GOALS> aGoalNames;

typedef unsigned short TKnowledgeID;

struct STarget
{
	int nID = -1;
	std::string sName;
	int nKnowledgeCategoryID = -1;
	int nConstellationID = -1;
	int nInterestMin = 0;
	int nInterestMax = 0;
	int nFavorMin = 0;
	int nFavorMax = 0;
};

struct SComboEffect
{
	int nDelay = 0;
	int nLength = 0;
	int nInterest = 0;
	int nFavor = 0;
};

struct SKnowledge
{
	SKnowledge() {}
	void Finalize()
	{
		nAverageFavor = (nFavorMin + nFavorMax) / 2;
	}

	bool operator ==(TKnowledgeID rhs) const { return nID == rhs; }

	TKnowledgeID nID = -1;
	std::string sName;
	double fInterest = 0.0f;
	int nFavorMin = -1;
	int nFavorMax = -1;
	int nAverageFavor = -1;

	SComboEffect comboEffect;
};

struct SKnowledgeCategory
{
	int nID = -1;
	std::string sName;
	std::vector<TKnowledgeID> vKnowledge;
};

struct SConstellation
{
	int nID = -1;
	int nNumSlots = 0;
	std::vector<int> vSlotOrder;
};

struct SSimulationStatus
{
	double fTargetInterestLevel = 0;
	int nTargetFavor = 0;
	double fChance = 1.0f;

	int nSpark = 0;
	int nSparkFailure = 0;
	int nAccumulatedFavor = 0;
	int nCurrentMaxFavor = 0;
	int nMaxMaxFavor = 0;
	int nCurrentConsecutiveSpark = 0;
	int nMaxConsecutiveSpark = 0;
	int nCurrentConsecutiveSparkFailure = 0;
	int nMaxConsecutiveSparkFailure = 0;

	std::vector<SComboEffect> vComboEffects;
};

struct SSimulationStatusFinal
{
	SSimulationStatusFinal(const SSimulationStatus& status)
	: nSpark(status.nSpark)
	, nSparkFailure(status.nSparkFailure)
	, nAccumulatedFavor(status.nAccumulatedFavor)
	, nMaxFavor(status.nMaxMaxFavor)
	, nMaxConsecutiveSpark(status.nMaxConsecutiveSpark)
	, nMaxConsecutiveSparkFailure(status.nMaxConsecutiveSparkFailure)
	, fChance(status.fChance)
	{
		fModifiedAccumulatedFavor = fChance * status.nAccumulatedFavor;
	}

	double fChance = 1.0;
	double fModifiedAccumulatedFavor = 0.0;
	
	int nAccumulatedFavor = 0;
	int nMaxFavor = 0;
	int nSpark = 0;
	int nSparkFailure = 0;
	int nMaxConsecutiveSpark = 0;
	int nMaxConsecutiveSparkFailure = 0;
};

struct SBestCombinations
{
	SBestCombinations()
	{
		// These numbers are the max that I've seen in game. It will crash if it attempts to solve outside of these ranges.
		aBestCombinations[GOAL_SPARK].resize(8);
		aBestCombinations[GOAL_SPARK_FAILURE].resize(8);
		aBestCombinations[GOAL_CONSECUTIVE_SPARK].resize(8);
		aBestCombinations[GOAL_CONSECUTIVE_SPARK_FAILURE].resize(8);
		aBestCombinations[GOAL_ACCUMULATED_FAVOR].resize(250);
		aBestCombinations[GOAL_MAX_FAVOR].resize(150);
		aBestCombinations[GOAL_FREE_TALK].resize(1);
	}

	void Clear()
	{
		for (auto& vGoalCombinations : aBestCombinations)
		{
			for (SBestCombination& combination : vGoalCombinations)
			{
				combination.vKnowledge.clear();
				combination.fStrictEV = 0.0f;
				combination.fSuccessPercentage = 0.0f;
			}
		}
	}

	struct SBestCombination
	{
		std::vector<TKnowledgeID> vKnowledge;
		double fStrictEV = 0.0f;
		double fSuccessPercentage = 0.0f;
	};
	std::array<std::vector<SBestCombination>, NUM_GOALS> aBestCombinations;
};

struct SCombinationResult
{
	void Clear()
	{
		bestCombinationStats.Clear();
	}

	SBestCombinations bestCombinationStats;
};

struct STargetSolve
{
	int nTargetID = -1;
	int nInterestLevel = 0;
	int nFavor = 0;
	bool bFastSolve = false;
	SBestCombinations bestCombinations;
};

struct SEnvironment
{
	std::map<int, SKnowledgeCategory> mapKnowledgeCategories;
	std::map<std::string, int> mapKnowledgeCategoryIDs;

	std::map<TKnowledgeID, SKnowledge> mapKnowledges;
	std::map<std::string, TKnowledgeID> mapKnowledgeIDs;

	std::map<int, SConstellation> mapConstellations;

	std::map<int, STarget> mapTargets;
	std::map<std::string, int> mapTargetIDs;

	std::random_device randomDevice;
	std::mt19937 rng = std::mt19937(randomDevice());

	const int nResultsVersion = 3;
	const bool bSafetyChecks = false;
	const double fSolvePrintTime = 0.5f;
	const double fStorePrintTime = 0.5f;
	const float fDeltaSuccessEVMultiplier = 15.0f * 100.0f; // Guesswork
};
extern SEnvironment g_Env;

#endif // TYPES_H