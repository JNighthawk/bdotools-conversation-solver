
template <typename T>
inline void GeneratePermutations(std::vector<std::vector<T>>& vResults, const std::vector<T>& vStartingCombination)
{	
	std::vector<T> vCurrentResult(vStartingCombination);
	do
	{
		vResults.push_back(vCurrentResult);
	} while (std::next_permutation(vCurrentResult.begin(), vCurrentResult.end()));
}

template <typename T>
inline void GenerateCombinationsAndPermutationsHelper(std::vector<std::vector<T>>& vResults, const std::vector<T>& vCurrentResult, const std::vector<T>& vObjects, int nResultSlots, int nStartSlot)
{
	if (nStartSlot >= static_cast<int>(vObjects.size()))
		return;

	for (int nSlot = nStartSlot ; nSlot < static_cast<int>(vObjects.size()) ; ++nSlot)
	{
		std::vector<T> vMyResult = vCurrentResult;
		vMyResult.push_back(vObjects[nSlot]);
		if (static_cast<int>(vMyResult.size()) >= nResultSlots)
		{
			GeneratePermutations(vResults);
		}
		else
		{
			GenerateCombinationsAndPermutationsHelper(vResults, vMyResult, vObjects, nResultSlots, nSlot + 1);
		}
	}
}

template <typename T>
inline std::vector<std::vector<T>> GenerateCombinationsAndPermutations(const std::vector<T>& vObjects, int nResultSlots)
{
	std::vector<std::vector<T>> vResults;
	if (vObjects.empty() || nResultSlots <= 0)
		return vResults;

	std::vector<T> vCurrentResult;
	GenerateCombinationsAndPermutationsHelper(vResults, vCurrentResult, vObjects, nResultSlots, 0);
	return vResults;
}

template <typename T>
inline void GeneratePermutationsStaticMemory(std::vector<T*>& vResults, T* pStartingCombination, int nResultSlots)
{
	std::vector<T> vCurrentResult(nResultSlots);
	for (int nSlot = 0 ; nSlot < nResultSlots ; ++nSlot)
	{
		vCurrentResult[nSlot] = pStartingCombination[nSlot];
	}

	do
	{
		T* pNewResult = CMemory<T>::Alloc(nResultSlots);
		memcpy(pNewResult, vCurrentResult.data(), nResultSlots * sizeof(T));
		vResults.push_back(pNewResult);
	} while (std::next_permutation(vCurrentResult.begin(), vCurrentResult.end()));
}

template <typename T, bool t_bPermutations>
inline void GenerateCombinationsAndPermutationsStaticMemoryHelper(std::vector<T*>& vResults, T* pCurrentResult, int nCurrentResultSize, const std::vector<T>& vObjects, int nResultSlots, int nStartSlot)
{
	if (nStartSlot >= static_cast<int>(vObjects.size()))
		return;

	for (int nSlot = nStartSlot ; nSlot < static_cast<int>(vObjects.size()) ; ++nSlot)
	{
		const int nSlotsLeft = static_cast<int>(vObjects.size()) - nSlot;
		if (nSlotsLeft + nCurrentResultSize < nResultSlots)
		{
			return;
		}
		
		pCurrentResult[nCurrentResultSize] = vObjects[nSlot];
		if (nCurrentResultSize + 1 >= nResultSlots)
		{
			if (t_bPermutations)
			{
				GeneratePermutationsStaticMemory(vResults, pCurrentResult, nResultSlots);
			}
			else
			{
				T* pNewResult = CMemory<T>::Alloc(nResultSlots);
				memcpy(pNewResult, pCurrentResult, nResultSlots * sizeof(T));
				vResults.push_back(pNewResult);
			}
		}
		else
		{
			GenerateCombinationsAndPermutationsStaticMemoryHelper<T, t_bPermutations>(vResults, pCurrentResult, nCurrentResultSize + 1, vObjects, nResultSlots, nSlot + 1);
		}
	}
}

template <typename T, bool t_bPermutations>
inline std::vector<T*> GenerateCombinationsAndPermutationsStaticMemory(const std::vector<T>& vObjects, int nResultSlots)
{
	std::vector<T*> vResults;
	if (vObjects.empty() || nResultSlots <= 0)
		return vResults;

	std::vector<T> vSortedObjects = vObjects;
	std::sort(vSortedObjects.begin(), vSortedObjects.end());

	std::vector<T> vCurrentResult(nResultSlots);
	GenerateCombinationsAndPermutationsStaticMemoryHelper<T, t_bPermutations>(vResults, vCurrentResult.data(), 0, vSortedObjects, nResultSlots, 0);
	return vResults;
}
