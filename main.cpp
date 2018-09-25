
#if defined(_WIN32)
#define _WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#undef min
#undef max
#endif // defined(_WIN32)

#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <ctime>

#include <libpq-fe.h>

#include "Types.h"
#include "Utils.h"
#include "Simulation.h"

SEnvironment g_Env;

int ReadInt(PGresult* pResult, int nRow, int nColumn)
{
	int nReturn = 0;

	const char* pData = PQgetvalue(pResult, nRow, nColumn);
	if (pData == nullptr)
	{
		printf("Failed to get value\n");
		return nReturn;
	}

	nReturn = atoi(pData);

	return nReturn;
}

void CreateKnowledges(PGconn* pDatabaseConnection)
{	
	PGresult* pResult = PQexec(pDatabaseConnection, "SELECT id, name, favor_min, favor_max, interest, combo_delay, combo_length, combo_interest, combo_favor, category_id FROM knowledge;");
	if (pResult == nullptr)
	{
		printf("Failed to get result\n");
		return;
	}

	const auto eResult = PQresultStatus(pResult);
	if (eResult == PGRES_TUPLES_OK)
	{
		const int nFields = PQnfields(pResult);
		if (nFields == 10)
		{
			const int nColumnID = PQfnumber(pResult, "id");
			const int nColumnName = PQfnumber(pResult, "name");
			const int nColumnFavorMin = PQfnumber(pResult, "favor_min");
			const int nColumnFavorMax = PQfnumber(pResult, "favor_max");
			const int nColumnInterest = PQfnumber(pResult, "interest");
			const int nColumnComboDelay = PQfnumber(pResult, "combo_delay");
			const int nColumnComboLength = PQfnumber(pResult, "combo_length");
			const int nColumnComboInterest = PQfnumber(pResult, "combo_interest");
			const int nColumnComboFavor = PQfnumber(pResult, "combo_favor");
			const int nColumnCategoryID = PQfnumber(pResult, "category_id");

			const int nRows = PQntuples(pResult);
			for (int nRow = 0 ; nRow < nRows ; ++nRow)
			{
				const int nKnowledgeID = ReadInt(pResult, nRow, nColumnID);
				SKnowledge& knowledge = g_Env.mapKnowledges[nKnowledgeID];

				knowledge.nID = nKnowledgeID;
				knowledge.sName = PQgetvalue(pResult, nRow, nColumnName);
				knowledge.nFavorMin = ReadInt(pResult, nRow, nColumnFavorMin);
				knowledge.nFavorMax = ReadInt(pResult, nRow, nColumnFavorMax);
				knowledge.fInterest = static_cast<double>(ReadInt(pResult, nRow, nColumnInterest));
				knowledge.comboEffect.nDelay = ReadInt(pResult, nRow, nColumnComboDelay);
				knowledge.comboEffect.nLength = ReadInt(pResult, nRow, nColumnComboLength);
				knowledge.comboEffect.nInterest = ReadInt(pResult, nRow, nColumnComboInterest);
				knowledge.comboEffect.nFavor = ReadInt(pResult, nRow, nColumnComboFavor);
				knowledge.Finalize();

				g_Env.mapKnowledgeIDs[GetLowerString(knowledge.sName)] = nKnowledgeID;

				const int nCategoryID = ReadInt(pResult, nRow, nColumnCategoryID);
				auto itCategory = g_Env.mapKnowledgeCategories.find(nCategoryID);
				if (itCategory == g_Env.mapKnowledgeCategories.end())
				{
					printf("Failed to lookup knowledge category ID: %d\n", nCategoryID);
					continue;
				}
				SKnowledgeCategory& category = itCategory->second;
				category.vKnowledge.push_back(nKnowledgeID);
			}
		}
		else
		{
			printf("Didn't get 10 fields back from the DB\n");
		}
	}
	else if (eResult == PGRES_COMMAND_OK)
	{
		printf("No data returned from query\n");
	}

	PQclear(pResult);
}

void CreateConstellations(PGconn* pDatabaseConnection)
{
	PGresult* pResult = PQexec(pDatabaseConnection, "SELECT id, slots, slot_order FROM constellations;");
	if (pResult == nullptr)
	{
		printf("Failed to get result\n");
		return;
	}

	const auto eResult = PQresultStatus(pResult);
	if (eResult == PGRES_TUPLES_OK)
	{
		const int nFields = PQnfields(pResult);
		if (nFields == 3)
		{
			const int nColumnID = PQfnumber(pResult, "id");
			const int nColumnSlots = PQfnumber(pResult, "slots");
			const int nColumnSlotOrder = PQfnumber(pResult, "slot_order");

			const int nRows = PQntuples(pResult);
			for (int nRow = 0 ; nRow < nRows ; ++nRow)
			{
				const int nConstellationID = ReadInt(pResult, nRow, nColumnID);
				SConstellation& constellation = g_Env.mapConstellations[nConstellationID];

				constellation.nID = nConstellationID;
				constellation.nNumSlots = ReadInt(pResult, nRow, nColumnSlots);

				const char* szSlotOrderBase = PQgetvalue(pResult, nRow, nColumnSlotOrder);
				int nSlotOrderBaseLen = static_cast<int>(strlen(szSlotOrderBase));
				if (szSlotOrderBase[0] != '{' || szSlotOrderBase[nSlotOrderBaseLen - 1] != '}')
				{
					printf("Bad format for slot order array\n");
					return;
				}

				char* szSlotOrder = new char[nSlotOrderBaseLen - 1];
				strncpy(szSlotOrder, szSlotOrderBase + 1, nSlotOrderBaseLen - 2);
				szSlotOrder[nSlotOrderBaseLen - 2] = '\0';

				char* pToken = strtok(szSlotOrder, ",");
				while (pToken != nullptr)
				{
					int nSlot = atoi(pToken);
					constellation.vSlotOrder.push_back(nSlot);
					pToken = strtok(nullptr, ",");
				}
			}
		}
		else
		{
			printf("Didn't get 3 fields back from the DB\n");
		}
	}
	else if (eResult == PGRES_COMMAND_OK)
	{
		printf("No data returned from query\n");
	}

	PQclear(pResult);
}

void CreateTargets(PGconn* pDatabaseConnection)
{
	PGresult* pResult = PQexec(pDatabaseConnection, "SELECT id, name, constellation_id, category_id, interest_min, interest_max, favor_min, favor_max FROM targets;");
	if (pResult == nullptr)
	{
		printf("Failed to get result\n");
		return;
	}

	const auto eResult = PQresultStatus(pResult);
	if (eResult == PGRES_TUPLES_OK)
	{
		const int nFields = PQnfields(pResult);
		if (nFields == 8)
		{
			const int nColumnID = PQfnumber(pResult, "id");
			const int nColumnName = PQfnumber(pResult, "name");
			const int nColumnConstellationID = PQfnumber(pResult, "constellation_id");
			const int nColumnCategoryID = PQfnumber(pResult, "category_id");
			const int nColumnFavorMin = PQfnumber(pResult, "favor_min");
			const int nColumnFavorMax = PQfnumber(pResult, "favor_max");
			const int nColumnInterestMin = PQfnumber(pResult, "interest_min");
			const int nColumnInterestMax = PQfnumber(pResult, "interest_max");

			const int nRows = PQntuples(pResult);
			for (int nRow = 0 ; nRow < nRows ; ++nRow)
			{
				const int nTargetID = ReadInt(pResult, nRow, nColumnID);
				STarget& target = g_Env.mapTargets[nTargetID];

				target.nID = ReadInt(pResult, nRow, nColumnID);
				target.sName = PQgetvalue(pResult, nRow, nColumnName);
				target.nConstellationID = ReadInt(pResult, nRow, nColumnConstellationID);
				target.nKnowledgeCategoryID = ReadInt(pResult, nRow, nColumnCategoryID);
				target.nInterestMin = ReadInt(pResult, nRow, nColumnInterestMin);
				target.nInterestMax = ReadInt(pResult, nRow, nColumnInterestMax);
				target.nFavorMin = ReadInt(pResult, nRow, nColumnFavorMin);
				target.nFavorMax = ReadInt(pResult, nRow, nColumnFavorMax);

				g_Env.mapTargetIDs[GetLowerString(target.sName)] = nTargetID;
			}
		}
		else
		{
			printf("Didn't get 8 fields back from the DB\n");
		}
	}
	else if (eResult == PGRES_COMMAND_OK)
	{
		printf("No data returned from query\n");
	}

	PQclear(pResult);
}

void CreateKnowledgeCategoryIDs(PGconn* pDatabaseConnection)
{
	PGresult* pResult = PQexec(pDatabaseConnection, "SELECT id, name FROM categories;");
	if (pResult == nullptr)
	{
		printf("Failed to get result\n");
		return;
	}

	const auto eResult = PQresultStatus(pResult);
	if (eResult == PGRES_TUPLES_OK)
	{
		const int nFields = PQnfields(pResult);
		if (nFields == 2)
		{
			const int nColumnID = PQfnumber(pResult, "id");
			const int nColumnName = PQfnumber(pResult, "name");

			const int nRows = PQntuples(pResult);
			for (int nRow = 0 ; nRow < nRows ; ++nRow)
			{
				const int nCategoryID = ReadInt(pResult, nRow, nColumnID);
				SKnowledgeCategory& category = g_Env.mapKnowledgeCategories[nCategoryID];
				category.nID = nCategoryID;
				category.sName = PQgetvalue(pResult, nRow, nColumnName);

				g_Env.mapKnowledgeCategoryIDs[GetLowerString(category.sName)] = category.nID;
			}
		}
		else
		{
			printf("Didn't get 3 fields back from the DB\n");
		}
	}
	else if (eResult == PGRES_COMMAND_OK)
	{
		printf("No data returned from query\n");
	}

	PQclear(pResult);
}

void StoreResults(PGconn* pDatabaseConnection, const STarget& target, const STargetSolve& targetSolve)
{
	// Clear out the results table
	{
		printf("Clearing out previous results from database\n");

		char szDeleteStatement[2048];
		snprintf(szDeleteStatement, sizeof(szDeleteStatement), "DELETE FROM results WHERE target_id=%d AND target_interest=%d AND target_favor=%d;", target.nID, targetSolve.nInterestLevel, targetSolve.nFavor);

		PGresult* pResult = PQexec(pDatabaseConnection, szDeleteStatement);
		const auto eResult = PQresultStatus(pResult);
		if (eResult != PGRES_COMMAND_OK)
		{
			printf("Delete failed: %s\n", PQresultErrorMessage(pResult));
			PQclear(pResult);
			return;
		}
		PQclear(pResult);
	}
	
	{
		printf("Storing best results in database\n");

		{
			char szBeginTransaction[2048];
			snprintf(szBeginTransaction, sizeof(szBeginTransaction), "BEGIN;");

			PGresult* pResult = PQexec(pDatabaseConnection, szBeginTransaction);
			const auto eResult = PQresultStatus(pResult);
			if (eResult != PGRES_COMMAND_OK)
			{
				printf("Transaction failed: %s, query: %s\n", PQresultErrorMessage(pResult), szBeginTransaction);
				PQclear(pResult);
				return;
			}
			PQclear(pResult);
		}

		std::string sBulkStoreStatement = "INSERT INTO results (target_id, target_interest, target_favor, goal, goal_param, knowledge_ids, success_percentage, strict_afl_ev, version) VALUES ";

		for (int nGoal = 0 ; nGoal < NUM_GOALS ; ++nGoal)
		{
			const auto& vBest = targetSolve.bestCombinations.aBestCombinations[nGoal];
			const int nNumGoalParams = static_cast<int>(vBest.size());
			for (int nGoalParam = 0 ; nGoalParam < nNumGoalParams ; ++nGoalParam)
			{
				const auto& best = vBest[nGoalParam];
				if (best.vKnowledge.empty())
				{
					continue;
				}
			
				char szKnowledgeIDs[2048] = "";
				for (int nKnowledge = 0 ; nKnowledge < best.vKnowledge.size() ; ++nKnowledge)
				{
					char szID[16];
					if (nKnowledge == 0)
					{
						snprintf(szID, sizeof(szID), "{%d", best.vKnowledge[nKnowledge]);
					}
					else
					{
						snprintf(szID, sizeof(szID), ",%d", best.vKnowledge[nKnowledge]);
					}
					strcat_s(szKnowledgeIDs, szID);
				}
				strcat(szKnowledgeIDs, "}");

				// New, bulk insert
				char szBulkInsert[2048];
				snprintf(szBulkInsert, sizeof(szBulkInsert), "(%d, %d, %d, %d, %d, '%s', %.4f, %.2f, %d),", 
					target.nID, targetSolve.nInterestLevel, targetSolve.nFavor, nGoal, nGoalParam, szKnowledgeIDs, best.fSuccessPercentage, best.fStrictEV, targetSolve.bFastSolve ? g_Env.nResultsVersion - 1 : g_Env.nResultsVersion);
				sBulkStoreStatement += szBulkInsert;

				// Old, single insert
				/*
				char szInsert[2048];
				snprintf(szInsert, sizeof(szInsert), "INSERT INTO results (target_id, target_interest, target_favor, goal, goal_param, knowledge_ids, success_percentage, strict_afl_ev, version) VALUES "
					"(%d, %d, %d, %d, %d, '%s', %.4f, %.2f, %d);", 
					target.nID, targetSolve.nInterestLevel, targetSolve.nFavor, nGoal, nGoalParam, szKnowledgeIDs, best.fSuccessPercentage, best.fStrictEV, g_Env.nResultsVersion);
					
				PGresult* pResult = PQexec(pDatabaseConnection, szInsert);
				const auto eResult = PQresultStatus(pResult);
				if (eResult != PGRES_COMMAND_OK)
				{
					printf("Insert failed: %s\n", PQresultErrorMessage(pResult));
					PQclear(pResult);
					return;
				}

				PQclear(pResult);
				*/
			}
		}

		sBulkStoreStatement[sBulkStoreStatement.length() - 1] = ';';
		{
			PGresult* pResult = PQexec(pDatabaseConnection, sBulkStoreStatement.c_str());
			const auto eResult = PQresultStatus(pResult);
			if (eResult != PGRES_COMMAND_OK)
			{
				printf("Insert failed: %s, query: %s\n", PQresultErrorMessage(pResult), sBulkStoreStatement.c_str());
				PQclear(pResult);
				return;
			}

			PQclear(pResult);
		}

		{
			char szCommitTransaction[2048];
			snprintf(szCommitTransaction, sizeof(szCommitTransaction), "COMMIT;");

			PGresult* pResult = PQexec(pDatabaseConnection, szCommitTransaction);
			const auto eResult = PQresultStatus(pResult);
			if (eResult != PGRES_COMMAND_OK)
			{
				printf("Transaction failed: %s, query: %s\n", PQresultErrorMessage(pResult), szCommitTransaction);
				PQclear(pResult);
				return;
			}
			PQclear(pResult);
		}

		{
			char szUpdateQuery[2048];
			snprintf(szUpdateQuery, sizeof(szUpdateQuery), "UPDATE targets SET has_results=true WHERE id=%d;", target.nID);

			PGresult* pResult = PQexec(pDatabaseConnection, szUpdateQuery);
			const auto eResult = PQresultStatus(pResult);
			if (eResult != PGRES_COMMAND_OK)
			{
				printf("Transaction failed: %s, query: %s\n", PQresultErrorMessage(pResult), szUpdateQuery);
				PQclear(pResult);
				return;
			}
			PQclear(pResult);
		}

		printf("Finished storing results\n");
	}
}

bool FetchResults(PGconn* pDatabaseConnection, STargetSolve& targetSolve, EGoal eGoal, int nGoalParam)
{
	char szSelectStatement[2048];
	snprintf(szSelectStatement, sizeof(szSelectStatement), "SELECT knowledge_ids, success_percentage, strict_afl_ev, version FROM results WHERE target_id=%d AND target_interest=%d AND target_favor=%d AND goal=%d AND goal_param=%d;", 
		targetSolve.nTargetID, targetSolve.nInterestLevel, targetSolve.nFavor, static_cast<int>(eGoal), nGoalParam);

	PGresult* pResult = PQexec(pDatabaseConnection, szSelectStatement);
	const auto eResult = PQresultStatus(pResult);
	if (eResult == PGRES_TUPLES_OK)
	{
		const int nFields = PQnfields(pResult);
		if (nFields == 4)
		{
			const int nRows = PQntuples(pResult);
			if (nRows == 1)
			{
				const int nColumnKnowledgeIDs = PQfnumber(pResult, "knowledge_ids");
				const int nColumnSuccessPercentage = PQfnumber(pResult, "success_percentage");
				const int nColumnStrictAFLEV = PQfnumber(pResult, "strict_afl_ev");
				const int nColumnVersion = PQfnumber(pResult, "version");

				for (int nRow = 0 ; nRow < nRows ; ++nRow)
				{
					const int nVersion = ReadInt(pResult, nRow, nColumnVersion);
					if (nVersion < (targetSolve.bFastSolve ? g_Env.nResultsVersion - 1 : g_Env.nResultsVersion))
					{
						return false;
					}
					SBestCombinations::SBestCombination& bestCombination = targetSolve.bestCombinations.aBestCombinations[eGoal][nGoalParam];
					bestCombination.fStrictEV = static_cast<double>(atof(PQgetvalue(pResult, nRow, nColumnStrictAFLEV)));
					bestCombination.fSuccessPercentage = static_cast<double>(atof(PQgetvalue(pResult, nRow, nColumnSuccessPercentage)));
					bestCombination.vKnowledge.clear();
					
					const char* szKnowledgeIDsBase = PQgetvalue(pResult, nRow, nColumnKnowledgeIDs);
					int nKnowledgeIDsBaseLen = static_cast<int>(strlen(szKnowledgeIDsBase));
					if (szKnowledgeIDsBase[0] != '{' || szKnowledgeIDsBase[nKnowledgeIDsBaseLen - 1] != '}')
					{
						printf("Bad format for knowledge IDs array\n");
						return false;
					}

					char* szKnowledgeIDs = new char[nKnowledgeIDsBaseLen - 1];
					strncpy(szKnowledgeIDs, szKnowledgeIDsBase + 1, nKnowledgeIDsBaseLen - 2);
					szKnowledgeIDs[nKnowledgeIDsBaseLen - 2] = '\0';

					char* pToken = strtok(szKnowledgeIDs, ",");
					while (pToken != nullptr)
					{
						int nKnowledgeID = atoi(pToken);
						bestCombination.vKnowledge.push_back(nKnowledgeID);
						pToken = strtok(nullptr, ",");
					}
					return true;
				}
			}
			else if (nRows > 1)
			{
				printf("Received more than 1 result row, don't know how to handle this: %s\n", szSelectStatement);
			}
		}
		else
		{
			printf("Didn't get 4 fields back from the DB: %s\n", szSelectStatement);
		}
	}
	else if (eResult == PGRES_COMMAND_OK)
	{
		printf("No data returned from query: %s\n", szSelectStatement);
	}

	PQclear(pResult);

	return false;
}

bool MarkSolveInProgress(PGconn* pDatabaseConnection, STargetSolve& targetSolve)
{
	char szInsertStatement[2048];
	snprintf(szInsertStatement, sizeof(szInsertStatement), "INSERT INTO solve_in_progress (target_id, target_interest, target_favor) VALUES (%d, %d, %d);",
		targetSolve.nTargetID, targetSolve.nInterestLevel, targetSolve.nFavor);

	PGresult* pResult = PQexec(pDatabaseConnection, szInsertStatement);
	const auto eResult = PQresultStatus(pResult);
	PQclear(pResult);

	return (eResult == PGRES_COMMAND_OK);
}

void MarkSolveComplete(PGconn* pDatabaseConnection, STargetSolve& targetSolve)
{
	char szDeleteStatement[2048];
	snprintf(szDeleteStatement, sizeof(szDeleteStatement), "DELETE FROM solve_in_progress WHERE target_id=%d AND target_interest=%d AND target_favor=%d;",
		targetSolve.nTargetID, targetSolve.nInterestLevel, targetSolve.nFavor);

	PGresult* pResult = PQexec(pDatabaseConnection, szDeleteStatement);
	const auto eResult = PQresultStatus(pResult);
	if (eResult != PGRES_COMMAND_OK)
	{
		printf("Failed to delete from solve_in_progress table, query: %s\n", szDeleteStatement);
	}

	PQclear(pResult);
}

void main(int nArgC, const char* aArgV[])
{
	const char* aDatabaseConnectionKeywords[] =
	{
		"hostaddr",
		"user",
		"password",
		"dbname",
		nullptr,
	};
	const char* aDatabaseConnectionValues[] =
	{
		"", // Scrubbed for open sourcing
		"", // Scrubbed for open sourcing
		"", // Scrubbed for open sourcing
		"", // Scrubbed for open sourcing
		nullptr,
	};

	// DB connection to read data
	{
		printf("Reading initial data from database\n");

		PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
		CreateConstellations(pDatabaseConnection);
		CreateKnowledgeCategoryIDs(pDatabaseConnection);
		CreateTargets(pDatabaseConnection);
		CreateKnowledges(pDatabaseConnection);
		PQfinish(pDatabaseConnection);

		printf("Finished reading initial data from database\n");
	}

	if (nArgC < 2 || _stricmp(aArgV[1], "Solve") == 0 || _stricmp(aArgV[1], "SolveFast") == 0)
	{	
		STargetSolve targetSolve;
		targetSolve.bFastSolve = (nArgC >= 2 && _stricmp(aArgV[1], "SolveFast") == 0);
		std::string sTargetName;
		EGoal eGoal = NUM_GOALS;
		int nGoalParam = 0;

		if (nArgC < 3)
		{
			printf("What's the name of the target? ");
			char szTarget[256];
			std::cin.getline(szTarget, sizeof(szTarget) * sizeof(char));
			sTargetName = szTarget;
		}
		else
		{
			sTargetName = aArgV[2];
		}

		if (nArgC < 4)
		{
			printf("What's the interest level of the target? ");
			std::cin >> targetSolve.nInterestLevel;
		}
		else
		{
			targetSolve.nInterestLevel = atoi(aArgV[3]);
		}

		if (nArgC < 5)
		{
			printf("What's the favor level of the target? ");
			std::cin >> targetSolve.nFavor;
		}
		else
		{
			targetSolve.nFavor = atoi(aArgV[4]);
		}

		if (nArgC < 6)
		{
			printf("What goal does this conversation have?\n");
			for (int i = 0 ; i < NUM_GOALS ; ++i)
			{
				printf("- %d: %s\n", i, aGoalNames[i].c_str());
			}

			int nGoal = 0;
			std::cin >> nGoal;
			if (nGoal < 0 || nGoal >= NUM_GOALS)
			{
				printf("Invalid goal: %d\n", nGoal);
				return;
			}
			eGoal = static_cast<EGoal>(nGoal);
		}
		else
		{
			const int nGoal = atoi(aArgV[5]);
			if (nGoal < 0 || nGoal >= NUM_GOALS)
			{
				printf("Invalid goal: %d\n", nGoal);
				return;
			}
			eGoal = static_cast<EGoal>(nGoal);
		}

		if (eGoal != GOAL_FREE_TALK)
		{
			if (nArgC < 7)
			{
				printf("What's the goal parameter? ");
				std::cin >> nGoalParam;
			}
			else
			{
				nGoalParam = atoi(aArgV[6]);
			}
		}
	
		auto itTargetID = g_Env.mapTargetIDs.find(GetLowerString(sTargetName));
		if (itTargetID == g_Env.mapTargetIDs.end())
		{
			printf("Failed to find target: %s\n", sTargetName.c_str());
			return;
		}
		targetSolve.nTargetID = itTargetID->second;

		auto itTarget = g_Env.mapTargets.find(targetSolve.nTargetID);
		if (itTarget == g_Env.mapTargets.end())
		{
			printf("Failed to find target ID: %d\n", targetSolve.nTargetID);
			return;
		}
		const STarget& target = itTarget->second;

		// Look up stored results if we've got them
		bool bFetchedResults = false;
		{
			PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
			bFetchedResults = FetchResults(pDatabaseConnection, targetSolve, eGoal, nGoalParam);
			PQfinish(pDatabaseConnection);
		}

		if (!bFetchedResults)
		{
			printf("Failed to lookup stored results\n");

			bool bSolvingAllowed = false;
			{
				PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
				bSolvingAllowed = MarkSolveInProgress(pDatabaseConnection, targetSolve);
				PQfinish(pDatabaseConnection);
			}

			if (!bSolvingAllowed)
			{
				printf("Another process is currently solving for this target, refusing to solve\n");
				return;
			}

			printf("Solving for this target\n");

#if defined(_WIN32)
			char szConsoleTitle[512];
			sprintf_s(szConsoleTitle, "Target: %s (%d) - Interest Level: %d - Favor: %d", sTargetName.c_str(), target.nID, targetSolve.nInterestLevel, targetSolve.nFavor);
			SetConsoleTitle(szConsoleTitle);
#endif // defined(_WIN32)

			bool bSimSuccess = false;
			if (targetSolve.bFastSolve)
			{
				bSimSuccess = SimulateCombinationsFast(target, targetSolve);
			}
			else
			{
				bSimSuccess = SimulateCombinations(target, targetSolve);
			}

			// Store results
			if (bSimSuccess)
			{
				PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
				StoreResults(pDatabaseConnection, target, targetSolve);
				PQfinish(pDatabaseConnection);
			}
			
			{
				PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
				MarkSolveComplete(pDatabaseConnection, targetSolve);
				PQfinish(pDatabaseConnection);
			}

			if (!bSimSuccess)
			{
				printf("Failed to simulate\n");
				return;
			}
		}
		
		const SBestCombinations::SBestCombination& bestCombination = targetSolve.bestCombinations.aBestCombinations[eGoal][nGoalParam];
		printf("Best combination - Success: %.2f%% - Strict AFL EV: %.2f\n", bestCombination.fSuccessPercentage * 100.0f, bestCombination.fStrictEV);
		for (int nKnowledgeID : bestCombination.vKnowledge)
		{
			const SKnowledge& knowledge = g_Env.mapKnowledges[nKnowledgeID];
			printf("- %s\n", knowledge.sName.c_str());
		}
	}
	else if (_stricmp(aArgV[1], "SolveAll") == 0 || _stricmp(aArgV[1], "SolveAllFast") == 0 || _stricmp(aArgV[1], "SolveAllMin") == 0  || _stricmp(aArgV[1], "SolveAllMinFast") == 0 || 
		_stricmp(aArgV[1], "SolveAllTarget") == 0 || _stricmp(aArgV[1], "SolveAllTargetFast") == 0)
	{
		const bool bSolveMinimum = (_stricmp(aArgV[1], "SolveAllMin") == 0 || _stricmp(aArgV[1], "SolveAllMinFast") == 0);
		const bool bFastSolve = (_stricmp(aArgV[1], "SolveAllTargetFast") == 0 || _stricmp(aArgV[1], "SolveAllFast") == 0 || _stricmp(aArgV[1], "SolveAllMinFast") == 0);

		int nNumProcesses = 4;
		if (nArgC < 3)
		{
			printf("How many processes do you want to spawn? ");
			std::cin >> nNumProcesses;
		}
		else
		{
			nNumProcesses = atoi(aArgV[2]);
		}

		std::string sSolveAllTargetName;
		if (_stricmp(aArgV[1], "SolveAllTarget") == 0 || _stricmp(aArgV[1], "SolveAllTargetFast") == 0)
		{
			if (nArgC < 4)
			{
				printf("What's the name of the target? ");
				char szTarget[256];

				std::ws(std::cin);
				std::cin.getline(szTarget, sizeof(szTarget));
				sSolveAllTargetName = szTarget;
			}
			else
			{
				sSolveAllTargetName = aArgV[3];
			}
		}

#if defined(_WIN32)
		struct SSolveAllTarget
		{
			const STarget& target;
			int nInterest;
			int nFavor;
		};
		
		PGconn* pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
		std::vector<SSolveAllTarget> vTargets;
		for (auto&& itTarget : g_Env.mapTargets)
		{
			const STarget& target = itTarget.second;
			if (!sSolveAllTargetName.empty() && _stricmp(target.sName.c_str(), sSolveAllTargetName.c_str()) != 0)
			{
				continue;
			}

			printf("Generating unsolved results for target: %s\n", target.sName.c_str());

			auto itCategory = g_Env.mapKnowledgeCategories.find(target.nKnowledgeCategoryID);
			if (itCategory == g_Env.mapKnowledgeCategories.end())
			{
				printf("Failed to find knowledge category: %d\n", target.nKnowledgeCategoryID);
				continue;
			}
			const auto& category = itCategory->second;

			if (category.vKnowledge.empty())
			{
				printf("Target's knowledge category is empty, results will not be generated: %s\n", category.sName.c_str());
				continue;
			}

			char szSelect[2048];
			snprintf(szSelect, sizeof(szSelect), "select target_interest,target_favor from results where target_id=%d and goal=6;", target.nID);
			auto pResult = PQexec(pDatabaseConnection, szSelect);
			const auto eResult = PQresultStatus(pResult);
			if (eResult == PGRES_TUPLES_OK)
			{
				const int nFields = PQnfields(pResult);
				if (nFields == 2)
				{
					const int nColumnTargetInterest = PQfnumber(pResult, "target_interest");
					const int nColumnTargetFavor = PQfnumber(pResult, "target_favor");
					const int nRows = PQntuples(pResult);					

					for (int nInterest = target.nInterestMin ; nInterest <= (bSolveMinimum ? target.nInterestMin : target.nInterestMax) ; ++nInterest)
					{
						for (int nFavor = target.nFavorMin ; nFavor <= (bSolveMinimum ? target.nFavorMin : target.nFavorMax) ; ++nFavor)
						{
							bool bFound = false;
					
							for (int nRow = 0 ; nRow < nRows ; ++nRow)
							{
								const int nReadInterest = ReadInt(pResult, nRow, nColumnTargetInterest);
								const int nReadFavor = ReadInt(pResult, nRow, nColumnTargetFavor);
								if (nReadInterest == nInterest && nReadFavor == nFavor)
								{
									bFound = true;
									break;
								}
							}

							if (!bFound)
							{
								vTargets.emplace_back(SSolveAllTarget{target, nInterest, nFavor});
							}
						}
					}
				}
			}

			PQclear(pResult);
		}
		printf("Generated target list of %d targets, beginning to solve using %d processes\n", static_cast<int>(vTargets.size()), nNumProcesses);
		
		clock_t nSolveStartTime = clock();
		std::vector<HANDLE> vProcesses;
		for (int nTarget = 0 ; nTarget < vTargets.size() ; ++nTarget)
		{
			auto&& target = vTargets[nTarget];
			int nCurrentTime = clock();
			
			STargetSolve targetSolve;
			targetSolve.nTargetID = target.target.nID;
			targetSolve.nInterestLevel = target.nInterest;
			targetSolve.nFavor = target.nFavor;
			targetSolve.bFastSolve = bFastSolve;

			const bool bFetchedResults = FetchResults(pDatabaseConnection, targetSolve, GOAL_FREE_TALK, 0);
			if (bFetchedResults)
			{
				printf("Skipping %d, already solved - %s %d/%d (%.2f%% - %.3fs elapsed)\n", nTarget, target.target.sName.c_str(), target.nInterest, target.nFavor, 
					static_cast<float>(nTarget + 1) / static_cast<float>(vTargets.size()) * 100.0f, static_cast<double>(nCurrentTime - nSolveStartTime) / CLOCKS_PER_SEC);
				continue;
			}
			
			if (vProcesses.size() >= nNumProcesses)
			{
				printf("Waiting for a process to finish before spawning a new one\n");
				PQfinish(pDatabaseConnection);
				pDatabaseConnection = nullptr;

				DWORD dwWaitID = WaitForMultipleObjects(static_cast<DWORD>(vProcesses.size()), vProcesses.data(), FALSE, INFINITE);
				if (dwWaitID >= WAIT_OBJECT_0 && dwWaitID < WAIT_OBJECT_0 + vProcesses.size())
				{
					const int nProcessIndex = dwWaitID - WAIT_OBJECT_0;
					vProcesses.erase(vProcesses.begin() + nProcessIndex);
				}
				else
				{
					printf("WaitForMultipleObjects() failed, no idea what to do - return value %d\n", static_cast<int>(dwWaitID));
					return;
				}
				
				pDatabaseConnection = PQconnectdbParams(aDatabaseConnectionKeywords, aDatabaseConnectionValues, 0);
			}
			
			printf("Spawning process to solve for target %d/%d - %s %d/%d (%.2f%% - %.3fs elapsed)\n", nTarget, static_cast<int>(vTargets.size()), target.target.sName.c_str(), target.nInterest, target.nFavor, 
				static_cast<float>(nTarget + 1) / static_cast<float>(vTargets.size()) * 100.0f, static_cast<double>(nCurrentTime - nSolveStartTime) / CLOCKS_PER_SEC);

			STARTUPINFO si;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow |= SW_SHOWNOACTIVATE;
			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(pi));

			char szCommandLine[2048];
			sprintf_s(szCommandLine, "%s %s \"%s\" %d %d 6", aArgV[0], bFastSolve ? "SolveFast" : "Solve", target.target.sName.c_str(), target.nInterest, target.nFavor);

			const BOOL bCreatedProcess = CreateProcess(nullptr, szCommandLine, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
			if (bCreatedProcess == FALSE)
			{
				printf("CreateProcess() failed - error code %d\n", GetLastError());
				//return;
				continue;
			}

			vProcesses.push_back(pi.hProcess);
		}
		
		int nCurrentTime = clock();
		printf("Finished solving for everything - %.3fs elapsed\n", static_cast<double>(nCurrentTime - nSolveStartTime) / CLOCKS_PER_SEC);
#endif // defined(_WIN32)
	}
}
