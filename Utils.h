#if !defined(UTILS_H)
#define UTILS_H

#include <string>
#include <vector>

std::string GetLowerString(const std::string& sString);
void LowerString(std::string& sString);

// Block allocator using static memory
template <typename T>
class CMemory
{
public:
	static std::vector<T> s_vBlocks;
	static int s_nBlockIndex;
	static void Init(int nNumBlocks)
	{
		s_vBlocks.resize(nNumBlocks);
		s_nBlockIndex = 0;
	}


	static T* Alloc(int nNum)
	{
		T* pReturn = &s_vBlocks[s_nBlockIndex];
		s_nBlockIndex += nNum;
		if (s_nBlockIndex > s_vBlocks.size())
		{
			return nullptr;
		}
		return pReturn;
	}
};

template <typename T>
std::vector<T> CMemory<T>::s_vBlocks;
template <typename T>
int CMemory<T>::s_nBlockIndex = 0;

template <typename T>
inline void GeneratePermutations(std::vector<std::vector<T>>& vResults, const std::vector<T>& vStartingCombination);
template <typename T>
inline std::vector<std::vector<T>> GenerateCombinationsAndPermutations(const std::vector<T>& vObjects, int nResultSlots);

template <typename T>
inline void GeneratePermutationsStaticMemory(std::vector<T*>& vResults, T* pStartingCombination, int nResultSlots);
template <typename T, bool t_bPermutations>
inline std::vector<T*> GenerateCombinationsAndPermutationsStaticMemory(const std::vector<T>& vObjects, int nResultSlots);

#include "Utils.inl"

#endif // !defined(UTILS_H)
