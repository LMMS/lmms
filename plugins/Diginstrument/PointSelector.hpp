#pragma once

#include <vector>
#include <algorithm>

class PointSelector
{
public:
  /*Return an array of indices between beginIndex and endIndex. Begins and ends with the given parameter indices.
    If leftSide is false, the indicies are denser at endIndex, otherwise they are denser at beginIndex.
    The parameter a is the base of the exponential distance function.
    a=1 reults in all indices being selected.*/
  static std::vector<unsigned int> selectIndices(unsigned int beginIndex, unsigned int endIndex, double a, bool leftSide = false)
  {
    std::vector<unsigned int> res;
    res.emplace_back(beginIndex);
    int i = 1;
    int j = 0;
    while (i < endIndex - beginIndex)
    {
      if (leftSide)
      {
        res.emplace_back(endIndex - i);
      }
      else
      {
        res.emplace_back(beginIndex + i);
      }
      i += pow(a, j);
      j++;
    }
    res.emplace_back(endIndex);
    //sort the indices
    std::sort(res.begin() + 1, res.end() - 1);
    return res;
  }

  /*Return an array of indices between beginIndex and endIndex. Begins and ends with the given parameter indices.
    The selected points are denser around the focus point.
    The parameter a is the base of the exponential distance function.
    a=1 reults in all indices being selected.*/
  static std::vector<unsigned int> selectIndices(unsigned int beginIndex, unsigned int endIndex, double a, unsigned int focusPoint)
  {
    std::vector<unsigned int> leftHalf = selectIndices(beginIndex, focusPoint, a, true);
    std::vector<unsigned int> rightHalf = selectIndices(focusPoint, endIndex, a);

    leftHalf.reserve(leftHalf.size() + rightHalf.size());
    for (int i = 1; i < rightHalf.size(); i++)
    {
      leftHalf.emplace_back(rightHalf[i]);
    }
    return leftHalf;
  }
};
