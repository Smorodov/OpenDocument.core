#include <common/TablePosition.h>
#include <gtest/gtest.h>

namespace odr {

TEST(TablePosition, default) {
  TablePosition tp;
  EXPECT_EQ(tp.getRow(), 0);
  EXPECT_EQ(tp.getCol(), 0);
}

TEST(TablePosition, direct) {
  TablePosition tp(1, 2);
  EXPECT_EQ(tp.getRow(), 1);
  EXPECT_EQ(tp.getCol(), 2);
}

TEST(TablePosition, string1) {
  TablePosition tp("A1");
  EXPECT_EQ(tp.getRow(), 0);
  EXPECT_EQ(tp.getCol(), 0);
}

TEST(TablePosition, string2) {
  const std::string input = "AA11";
  TablePosition tp(input);
  EXPECT_EQ(tp.getRow(), 10);
  EXPECT_EQ(tp.getCol(), 26);
  EXPECT_EQ(tp.toString(), input);
}

TEST(TablePosition, string3) {
  const std::string input = "ZZ1";
  TablePosition tp(input);
  EXPECT_EQ(tp.getRow(), 0);
  EXPECT_EQ(tp.getCol(), 701);
  EXPECT_EQ(tp.toString(), input);
}

TEST(TablePosition, string4) {
  const std::string input = "AAA1";
  TablePosition tp(input);
  EXPECT_EQ(tp.getRow(), 0);
  EXPECT_EQ(tp.getCol(), 702);
  EXPECT_EQ(tp.toString(), input);
}

} // namespace odr