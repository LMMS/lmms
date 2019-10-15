#include <gtest/gtest.h>

#include <MultidimensionalNeighbourMap.hpp>

namespace
{
class MultidimensionalNeighbourMapTest : public ::testing::Test
{
protected:
  virtual void SetUp()
  {
    map.insert("100; 0.5", {100, 0.5f});
    map.insert("200; 0.5", {200, 0.5f});
    map.insert("400; 0.5", {400, 0.5f});
    map.insert("999; 0.5", {999, 0.5f});
    map.insert("45; 0.5", {45, 0.5f});
    map.insert("1931; 0.5", {1931, 0.5f});
    map.insert("442; 0.5", {442, 0.5f});
    map.insert("200; 1", {200, 1});
    map.insert("400; 0.8", {400, 0.8f});
  }
  virtual void TearDown() {}

public:
  MultidimensionalNeighbourMap<float, std::string> map;
  MultidimensionalNeighbourMapTest() {}
};

TEST_F(MultidimensionalNeighbourMapTest, Constructor)
{
  MultidimensionalNeighbourMap<float, std::string> emptyMap;
  EXPECT_EQ(0, emptyMap.getDimensions());
}

TEST_F(MultidimensionalNeighbourMapTest, Dimensions)
{
  EXPECT_EQ(2, map.getDimensions());
}

TEST_F(MultidimensionalNeighbourMapTest, Clear)
{
  EXPECT_EQ(2, map.getDimensions());
  map.clear();
  EXPECT_EQ(0, map.getDimensions());
}

TEST_F(MultidimensionalNeighbourMapTest, ExactMatch)
{
  std::vector<std::string> neighbours = map.getNeighbours({200, 0.5f});
  EXPECT_EQ(1, neighbours.size());
  EXPECT_EQ("200; 0.5", neighbours.front());
}

TEST_F(MultidimensionalNeighbourMapTest, OutOfBoundsLow)
{
  std::vector<std::string> neighbours = map.getNeighbours({10, 0.5f});
  EXPECT_EQ(1, neighbours.size());
  EXPECT_EQ("45; 0.5", neighbours.front());
}

TEST_F(MultidimensionalNeighbourMapTest, OutOfBoundsHigh)
{
  std::vector<std::string> neighbours = map.getNeighbours({99999, 0.5f});
  EXPECT_EQ(1, neighbours.size());
  EXPECT_EQ("1931; 0.5", neighbours.front());
}

TEST_F(MultidimensionalNeighbourMapTest, ExactOnSecondDimension)
{
  std::vector<std::string> neighbours = map.getNeighbours({300, 0.5f});
  EXPECT_EQ(2, neighbours.size());
  EXPECT_EQ("200; 0.5", neighbours.front());
  EXPECT_EQ("400; 0.5", neighbours.back());
}

TEST_F(MultidimensionalNeighbourMapTest, ExactOnFirstDimension)
{
  std::vector<std::string> neighbours = map.getNeighbours({200, 0.8f});
  EXPECT_EQ(2, neighbours.size());
  EXPECT_EQ("200; 0.5", neighbours.front());
  EXPECT_EQ("200; 1", neighbours.back());
}

TEST_F(MultidimensionalNeighbourMapTest, FullNeighbours)
{
  std::vector<std::string> neighbours = map.getNeighbours({300, 0.666f});
  EXPECT_EQ(4, neighbours.size());
  EXPECT_EQ("200; 0.5", neighbours.front());
  EXPECT_EQ("200; 1", neighbours[1]);
  EXPECT_EQ("400; 0.5", neighbours[2]);
  EXPECT_EQ("400; 0.8", neighbours.back());
}

TEST_F(MultidimensionalNeighbourMapTest, Overwrite)
{
  map.insert("OWERWRITTEN", {200, 0.5f});
  std::vector<std::string> neighbours = map.getNeighbours({200, 0.5f});
  EXPECT_EQ(1, neighbours.size());
  EXPECT_EQ("OWERWRITTEN", neighbours.front());
}

}; // namespace