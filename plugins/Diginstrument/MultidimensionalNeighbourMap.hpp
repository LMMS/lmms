#include <memory>
#include <algorithm>
#include <vector>
#include <cmath>

template <typename K, typename V>
class MultidimensionalNeighbourMapEntry
{
public:
  K key;

  bool operator<(const K key)
  {
    return this->key < key;
  }

  static bool CompareWithKey(const K left, const MultidimensionalNeighbourMapEntry<K, V> &right)
  {
    return left < right.key;
  }

  bool operator<(const MultidimensionalNeighbourMapEntry &other) const
  {
    return key < other.key;
  }

  bool operator==(const MultidimensionalNeighbourMapEntry &other) const
  {
    return key == other.key;
  }

  MultidimensionalNeighbourMapEntry<K, V> &operator=(const MultidimensionalNeighbourMapEntry &other);

  bool isValue() const
  {
    return !!value;
  }

  const V &getValue() const
  {
    return *value;
  }

  void setValue(const V &value)
  {
    this->value = std::make_unique<V>(value);
  }

  std::vector<MultidimensionalNeighbourMapEntry<K, V>> &getNext()
  {
    return next;
  }

  /*value constructor*/
  MultidimensionalNeighbourMapEntry(const K key, const V &value) : key(key), next(0)
  {
    this->value = std::make_unique<V>(value);
  };
  /*empty node constructor*/
  MultidimensionalNeighbourMapEntry(const K key) : key(key), next(0) {}
  /*copy constructor*/
  MultidimensionalNeighbourMapEntry(const MultidimensionalNeighbourMapEntry &other);
  MultidimensionalNeighbourMapEntry() : key(0), next(0) {}

private:
  std::unique_ptr<V> value;
  std::vector<MultidimensionalNeighbourMapEntry<K, V>> next;
};

/*TODO: complexity analisys*/
template <typename K, typename V>
class MultidimensionalNeighbourMap
{
public:
  std::vector<V> getNeighbours(const std::vector<K> & coordinates);
  void insert(const V &value, const std::vector<K> &coordinates);
  unsigned int getDimensions() const { return this->dimensions; }
  void clear();

private:
  /* returns the closest neighbours of the coordinate in the array
      the lower neighbour is pair.first, the upper is pair.second
      if the coordinate is out of range, only one entry is returned, upper or lower, in the correct position
      if the coordinate is exact, only one entry is returned, on the lower (first) placement*/
  static std::pair<MultidimensionalNeighbourMapEntry<K, V> *, MultidimensionalNeighbourMapEntry<K, V> *> findNeighboursOnOneDimension(std::vector<MultidimensionalNeighbourMapEntry<K, V>> &array, const K &coordinate);
  static void getNeighboursRecursiveCall(std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array, const std::vector<K> & coordinates, unsigned int level, std::vector<V> & result);

  std::vector<MultidimensionalNeighbourMapEntry<K, V>> data;
  unsigned int dimensions = 0;
};

template <typename K, typename V>
MultidimensionalNeighbourMapEntry<K, V> &MultidimensionalNeighbourMapEntry<K, V>::operator=(const MultidimensionalNeighbourMapEntry &other)
{
  key = other.key;
  if (other.value)
    this->value = std::make_unique<V>(*other.value);
  this->next = other.next;
  return *this;
}

template <typename K, typename V>
MultidimensionalNeighbourMapEntry<K, V>::MultidimensionalNeighbourMapEntry(const MultidimensionalNeighbourMapEntry<K, V> &other) : key(other.key)
{
  if (other.value)
    this->value = std::make_unique<V>(*other.value);
  this->next = other.next;
}

template <typename K, typename V>
std::pair<MultidimensionalNeighbourMapEntry<K, V> *, MultidimensionalNeighbourMapEntry<K, V> *>
MultidimensionalNeighbourMap<K, V>::findNeighboursOnOneDimension(std::vector<MultidimensionalNeighbourMapEntry<K, V>> &array, const K &coordinate)
{
  auto maybeLower = std::lower_bound(array.begin(), array.end(), coordinate);
  if (coordinate == maybeLower->key)
  {
    /*exact match*/
    return std::make_pair(&(*maybeLower), nullptr);
  }
  if (maybeLower == array.begin())
  {
    /*out of range: low*/
    return std::make_pair(&(array.front()), nullptr);
  }
  auto maybeUpper = std::upper_bound(array.begin(), array.end(), coordinate, MultidimensionalNeighbourMapEntry<K, V>::CompareWithKey);
  if (maybeUpper == array.end())
  {
    /*out of range: high*/
    return std::make_pair(nullptr, &(array.back()));
  }
  return std::make_pair(&(*(maybeLower - 1)), &(*maybeUpper));
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::insert(const V &value, const std::vector<K> &coordinates)
{
  if(this->dimensions != coordinates.size() && this->dimensions != 0) { /*TODO:exception?*/ return; }
  std::vector<MultidimensionalNeighbourMapEntry<K, V>> *array = &data;
  int coordinateCounter = 0;
  for (const K &coordinate : coordinates)
  {
    coordinateCounter++;
    auto maybeLower = std::lower_bound(array->begin(), array->end(), coordinate);
    int maybeLowerIndex = std::distance(array->begin(), maybeLower);

    if (maybeLower != array->end() && maybeLower->key == coordinate)
    {
      if (maybeLower->isValue())
      {
        /*found and is value*/
        maybeLower->setValue(value);
        std::sort(array->begin() + maybeLowerIndex, array->end());
        return;
      }
      /*found, but not value - continue*/
      array = &(maybeLower->getNext());
    }

    else
    {
      /*not found - insert*/
      if (coordinateCounter == coordinates.size())
      {
        /*last coordinate - value*/
        array->push_back(MultidimensionalNeighbourMapEntry<K, V>(coordinate, value));
        std::sort(array->begin() + maybeLowerIndex, array->end());
      }else{
        /* not last coordinate - insert node and go to next*/
        array->push_back(MultidimensionalNeighbourMapEntry<K, V>(coordinate));
        std::sort(array->begin() + maybeLowerIndex, array->end());
        /*find its new position to get the next dimension*/
        array = &(std::lower_bound(array->begin() + maybeLowerIndex, array->end(), coordinate))->getNext();
      }
      /* inicialize dimension count*/
      if(this->dimensions==0){ this->dimensions = coordinates.size(); }
    }
  }
}

template <typename K, typename V>
std::vector<V> MultidimensionalNeighbourMap<K, V>::getNeighbours(const std::vector<K> & coordinates)
{
  std::vector<V> res;
  /*dimension check*/
  if(this->dimensions != coordinates.size()) {/*TODO: exception?*/ return res; }
  res.reserve(pow(2,coordinates.size()));
  getNeighboursRecursiveCall(data, coordinates, 0, res);
  return res;
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::getNeighboursRecursiveCall(std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array,
                                                                    const std::vector<K> & coordinates,
                                                                    unsigned int level,
                                                                    std::vector<V> & result)
{
  auto neighbours = findNeighboursOnOneDimension(array, coordinates[level]);
  if(neighbours.first){
    if(neighbours.first->isValue()){
     result.push_back(neighbours.first->getValue());
    }else{
      getNeighboursRecursiveCall(neighbours.first->getNext(), coordinates, level+1, result);
    }
  }
  if(neighbours.second){
    if(neighbours.second->isValue()){
     result.push_back(neighbours.second->getValue());
    }else{
      getNeighboursRecursiveCall(neighbours.second->getNext(), coordinates, level+1, result);
    }
  }
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::clear()
{
  this->data.clear();
  this->dimensions = 0;
}