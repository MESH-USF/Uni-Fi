#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <helpers/AdvertDataHelpers.h>

TEST(AdvertData, SimulatedLocationUsesStandardChatAdvertisement) {
  uint8_t data[MAX_ADVERT_DATA_SIZE] = {};
  AdvertDataBuilder builder(ADV_TYPE_CHAT, "Uni-Fi SIM", 28.0587, -82.4139);
  const uint8_t length = builder.encodeTo(data);

  EXPECT_EQ(0x91, data[0]);
  EXPECT_EQ(19, length);
  const uint8_t coordinates[] = {
      0x4c, 0x24, 0xac, 0x01,  // 28058700 microdegrees, little-endian
      0xb4, 0x76, 0x16, 0xfb,  // -82413900 microdegrees, little-endian
  };
  EXPECT_EQ(0, std::memcmp(data + 1, coordinates, sizeof(coordinates)));
  EXPECT_EQ(0, std::memcmp(data + 9, "Uni-Fi SIM", 10));

  AdvertDataParser parsed(data, length);
  EXPECT_TRUE(parsed.isValid());
  EXPECT_EQ(ADV_TYPE_CHAT, parsed.getType());
  EXPECT_TRUE(parsed.hasName());
  EXPECT_STREQ("Uni-Fi SIM", parsed.getName());
  EXPECT_TRUE(parsed.hasLatLon());
  EXPECT_EQ(28058700, parsed.getIntLat());
  EXPECT_EQ(-82413900, parsed.getIntLon());
  EXPECT_DOUBLE_EQ(28.0587, parsed.getLat());
  EXPECT_DOUBLE_EQ(-82.4139, parsed.getLon());
}

TEST(AdvertData, OrdinaryAdvertisementDoesNotInventCoordinates) {
  uint8_t data[MAX_ADVERT_DATA_SIZE] = {};
  AdvertDataBuilder builder(ADV_TYPE_CHAT, "Uni-Fi");
  const uint8_t length = builder.encodeTo(data);

  EXPECT_EQ(0x81, data[0]);
  EXPECT_EQ(7, length);
  EXPECT_EQ(0, std::memcmp(data + 1, "Uni-Fi", 6));
  AdvertDataParser parsed(data, length);
  EXPECT_TRUE(parsed.isValid());
  EXPECT_STREQ("Uni-Fi", parsed.getName());
  EXPECT_FALSE(parsed.hasLatLon());
  EXPECT_EQ(0, parsed.getIntLat());
  EXPECT_EQ(0, parsed.getIntLon());
}

TEST(AdvertData, LocationAdvertisementTruncatesNameAtWholeUtf8CodePoint) {
  // Location occupies nine bytes, leaving 23 bytes for the name. The emoji
  // crossing that boundary must be omitted, not split into malformed UTF-8.
  const std::string prefix(21, 'A');
  const std::string name = prefix + "\xF0\x9F\x93\xA1";
  uint8_t data[MAX_ADVERT_DATA_SIZE] = {};
  AdvertDataBuilder builder(ADV_TYPE_CHAT, name.c_str(), 28.0587, -82.4139);
  const uint8_t length = builder.encodeTo(data);

  EXPECT_EQ(30, length);
  EXPECT_EQ(0x91, data[0]);
  AdvertDataParser parsed(data, length);
  EXPECT_TRUE(parsed.isValid());
  EXPECT_STREQ(prefix.c_str(), parsed.getName());
  EXPECT_EQ(28058700, parsed.getIntLat());
  EXPECT_EQ(-82413900, parsed.getIntLon());
}

TEST(AdvertData, LocationAdvertisementKeepsUtf8CodePointThatFitsExactly) {
  const std::string name = std::string(19, 'A') + "\xF0\x9F\x93\xA1";
  uint8_t data[MAX_ADVERT_DATA_SIZE] = {};
  AdvertDataBuilder builder(ADV_TYPE_CHAT, name.c_str(), 28.0587, -82.4139);
  const uint8_t length = builder.encodeTo(data);

  EXPECT_EQ(MAX_ADVERT_DATA_SIZE, length);
  AdvertDataParser parsed(data, length);
  EXPECT_TRUE(parsed.isValid());
  EXPECT_STREQ(name.c_str(), parsed.getName());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
