#include <memory>
#include <algorithm>
#include <vector>
#include <cmath>
#include <functional>

//forward-declaration
template <typename K, typename V>
class MultidimensionalNeighbourMap;

template <typename K, typename V>
class MultidimensionalNeighbourMapEntry
{
friend class MultidimensionalNeighbourMap<K, V>;
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

  const std::vector<MultidimensionalNeighbourMapEntry<K, V>> &getNext() const
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
  std::vector<std::vector<V>> getNeighbours(const std::vector<K> & coordinates);
  std::vector<std::vector<V>> getNeighbours(const std::vector<K> & coordinates, std::vector<std::vector<K>> & labels);
  V processIntoRoot(const std::vector<K> & coordinates, std::function<V(const V&, const V&, K, K, K, unsigned int)> processor, std::function<V(const V&)> singleProcessor);
  void insert(const V &value, const std::vector<K> &coordinates);
  unsigned int getDimensions() const { return this->dimensions; }
  void clear();

private:
  /* returns the closest neighbours of the coordinate in the array
      the lower neighbour is pair.first, the upper is pair.second
      if the coordinate is out of range, only one entry is returned, upper or lower, in the correct position
      if the coordinate is exact, only one entry is returned, on the lower (first) placement*/
  static std::pair<const MultidimensionalNeighbourMapEntry<K, V> *, const MultidimensionalNeighbourMapEntry<K, V> *> findNeighboursOnOneDimension(const std::vector<MultidimensionalNeighbourMapEntry<K, V>> &array, const K &coordinate);
  static void getNeighboursRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array, const std::vector<K> & coordinates, unsigned int level, std::vector<std::vector<V>> & result);
  static void getNeighboursRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array, const std::vector<K> & coordinates, unsigned int level, std::vector<std::vector<V>> & result, std::vector<std::vector<K>> & labels);
  static V processIntoRootRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array, const std::vector<K> & coordinates, unsigned int level, std::function<V(const V&, const V&, K, K, K, unsigned int)> processor, std::function<V(const V&)> singleProcessor);

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
std::pair<const MultidimensionalNeighbourMapEntry<K, V> *, const MultidimensionalNeighbourMapEntry<K, V> *>
MultidimensionalNeighbourMap<K, V>::findNeighboursOnOneDimension(const std::vector<MultidimensionalNeighbourMapEntry<K, V>> &array, const K &coordinate)
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
      array = &(maybeLower->next);
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
        array = &((std::lower_bound(array->begin() + maybeLowerIndex, array->end(), coordinate))->next);
      }
      /* inicialize dimension count*/
      if(this->dimensions==0){ this->dimensions = coordinates.size(); }
    }
  }
}

template <typename K, typename V>
std::vector<std::vector<V>> MultidimensionalNeighbourMap<K, V>::getNeighbours(const std::vector<K> & coordinates)
{
  /*dimension check*/
  if(this->dimensions != coordinates.size()) {/*TODO: exception?*/ return {}; }
  std::vector<std::vector<V>> res;
  res.reserve(pow(2, coordinates.size()-1));
  getNeighboursRecursiveCall(data, coordinates, 0, res);
  return res;
}

/*labels is the per-level structure of the search tree, without the distinction of unary or binary nodes*/
template <typename K, typename V>
std::vector<std::vector<V>> MultidimensionalNeighbourMap<K, V>::getNeighbours(const std::vector<K> & coordinates, std::vector<std::vector<K>> & labels)
{
  /*dimension check*/
  if(this->dimensions != coordinates.size()) {/*TODO: exception?*/ return {}; }
  std::vector<std::vector<V>> res;
  res.reserve(pow(2, coordinates.size()-1));
  //prepare labels
  labels = std::vector<std::vector<K>>(coordinates.size());
  for(int i = 0; i<labels.size(); i++){
    labels[0].reserve(pow(2, i+1));
  }
  getNeighboursRecursiveCall(data, coordinates, 0, res, labels);
  return res;
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::getNeighboursRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array,
                                                                    const std::vector<K> & coordinates,
                                                                    unsigned int level,
                                                                    std::vector<std::vector<V>> & result)
{
  auto neighbours = findNeighboursOnOneDimension(array, coordinates[level]);
  if((neighbours.first && neighbours.first->isValue()) || (neighbours.second && neighbours.second->isValue())){
    //next node(s) is(are) value(s)
    if(neighbours.first && neighbours.second) {result.push_back({neighbours.first->getValue(), neighbours.second->getValue()}); return;}
    if(neighbours.first) {result.push_back({neighbours.first->getValue()}); return;}
    if(neighbours.second) {result.push_back({neighbours.second->getValue()}); return;}
  }else{
    //continue search
    if(neighbours.first){ getNeighboursRecursiveCall(neighbours.first->getNext(), coordinates, level+1, result); }
    if(neighbours.second){ getNeighboursRecursiveCall(neighbours.second->getNext(), coordinates, level+1, result); }
  }
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::getNeighboursRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array,
                                                                    const std::vector<K> & coordinates,
                                                                    unsigned int level,
                                                                    std::vector<std::vector<V>> & result,
                                                                    std::vector<std::vector<K>> & labels)
{
  auto neighbours = findNeighboursOnOneDimension(array, coordinates[level]);
  if((neighbours.first && neighbours.first->isValue()) || (neighbours.second && neighbours.second->isValue())){
    //next node(s) is(are) value(s)
    if(neighbours.first && neighbours.second) {
      labels[level].push_back(neighbours.first->key);
      labels[level].push_back(neighbours.second->key);
      result.push_back({neighbours.first->getValue(), neighbours.second->getValue()});
      return;
    }
    if(neighbours.first) {
      labels[level].push_back(neighbours.first->key);
      result.push_back({neighbours.first->getValue()});
       return;
    }
    if(neighbours.second) {
      labels[level].push_back(neighbours.second->key);
      result.push_back({neighbours.second->getValue()});
      return;
    }
  }else{
    //continue search
    if(neighbours.first){
      labels[level].push_back(neighbours.first->key);
      getNeighboursRecursiveCall(neighbours.first->getNext(), coordinates, level+1, result, labels);
    }
    if(neighbours.second){
      labels[level].push_back(neighbours.second->key);
      getNeighboursRecursiveCall(neighbours.second->getNext(), coordinates, level+1, result, labels);
    }
  }
}

template <typename K, typename V>
void MultidimensionalNeighbourMap<K, V>::clear()
{
  this->data.clear();
  this->dimensions = 0;
}

template <typename K, typename V>
V MultidimensionalNeighbourMap<K, V>::processIntoRoot(const std::vector<K> & coordinates, std::function<V(const V&, const V&, K, K, K, unsigned int)> processor, std::function<V(const V&)> singleProcessor)
{
  //starting from root
  if(coordinates.size() < dimensions)
  {
    //TODO: exception handling
    //TMP
    return V();
  }
  return processIntoRootRecursiveCall(data, coordinates, 0, processor, singleProcessor);
}

template <typename K, typename V>
V MultidimensionalNeighbourMap<K, V>::processIntoRootRecursiveCall(const std::vector<MultidimensionalNeighbourMapEntry<K,V>> & array, const std::vector<K> & coordinates, unsigned int level, std::function<V(const V&, const V&, K, K, K, unsigned int)> processor, std::function<V(const V&)> singleProcessor)
{
  //TODO: deeper testing, complexity
  auto neighbours = findNeighboursOnOneDimension(array, coordinates[level]);
  if((neighbours.first && neighbours.first->isValue()) || (neighbours.second && neighbours.second->isValue()))
  {
    if(neighbours.first && neighbours.second)
    {
      return processor(neighbours.first->getValue(), neighbours.second->getValue(), coordinates[level], neighbours.first->key, neighbours.second->key, level);
    }
    if(neighbours.first) return singleProcessor(neighbours.first->getValue());
    if(neighbours.second) return singleProcessor(neighbours.second->getValue());
  }
  if(neighbours.first && neighbours.second)
  {
    return processor(
      processIntoRootRecursiveCall(neighbours.first->getNext(), coordinates, level+1, processor, singleProcessor),
      processIntoRootRecursiveCall(neighbours.second->getNext(), coordinates, level+1, processor, singleProcessor),
      coordinates[level], neighbours.first->key, neighbours.second->key, level
    );
  }
  if(neighbours.first) return processIntoRootRecursiveCall(neighbours.first->getNext(), coordinates, level+1, processor, singleProcessor);
  //TODO: FIXME: potential nullptr if no neighbours are found (should only happen on empty maps)
  /*if(neighbours.second)*/ return processIntoRootRecursiveCall(neighbours.second->getNext(), coordinates, level+1, processor, singleProcessor);
}
