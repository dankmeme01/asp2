#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include <gtest/gtest.h>
#include <fmt/format.h>
#include <random>
#include <algorithm>

using namespace asp::fs;

class FsTest : public ::testing::Test {
protected:
    Path tempfile;

    void SetUp() override {
        std::random_device rdev{};
        tempfile = Path::tmp().unwrap() / fmt::format("tempfile_{}", rdev());

        if (tempfile.exists()) {
            remove(tempfile).unwrap();
        }
    }

    void TearDown() override {
        if (tempfile.exists()) {
            remove(tempfile).unwrap();
        }
    }
};

TEST_F(FsTest, ReadFailsOnNonexistent) {
    auto result = asp::fs::read(tempfile);
    EXPECT_FALSE(result.isOk());
}

TEST_F(FsTest, WriteAndRead) {
    std::string content = "Hello, world!";
    asp::fs::write(tempfile, content).unwrap();

    auto result = asp::fs::readToString(tempfile);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), content);
}

TEST_F(FsTest, WriteAndCheckMetadata) {
    std::string content = "Hello, world!";
    asp::fs::write(tempfile, content).unwrap();

    auto result = tempfile.metadata();
    ASSERT_TRUE(result.isOk());

    auto metadata = result.unwrap();
    EXPECT_EQ(metadata.size, content.size());
    EXPECT_TRUE(metadata.isFile());
    EXPECT_FALSE(metadata.isDir());
    EXPECT_FALSE(metadata.isSymlink());

    if (metadata.createdAt) {
        EXPECT_TRUE(metadata.createdAt->elapsed() < asp::Duration::fromSecs(5));
    }

    if (metadata.accessedAt) {
        EXPECT_TRUE(metadata.accessedAt->elapsed() < asp::Duration::fromSecs(5));
    }
}

TEST_F(FsTest, Copy) {
    std::string content = "Hello, world!";
    asp::fs::write(tempfile, content).unwrap();

    Path destPath = tempfile.parent().value() / fmt::format("{}_copy", tempfile.filename().value());
    asp::fs::copy(tempfile, destPath).unwrap();

    auto m1 = tempfile.metadata();
    auto m2 = destPath.metadata();
    ASSERT_TRUE(m1.isOk());
    ASSERT_TRUE(m2.isOk());

    auto meta1 = m1.unwrap();
    auto meta2 = m2.unwrap();

    EXPECT_EQ(meta1.size, meta2.size);
    EXPECT_EQ(meta1.type, meta2.type);
    EXPECT_EQ(meta1.permissions, meta2.permissions);

    EXPECT_EQ(content, asp::fs::readToString(destPath).unwrap());

    asp::fs::remove(destPath).unwrap();
}

TEST_F(FsTest, CopyFailsOnNonexistentSource) {
    Path destPath = tempfile.parent().value() / fmt::format("{}_copy", tempfile.filename().value());
    auto result = asp::fs::copy(tempfile, destPath);
    EXPECT_FALSE(result.isOk());
}

TEST_F(FsTest, Rename) {
    std::string content = "Hello, world!";
    asp::fs::write(tempfile, content).unwrap();

    Path newPath = tempfile.parent().value() / fmt::format("{}_renamed", tempfile.filename().value());
    asp::fs::rename(tempfile, newPath).unwrap();

    EXPECT_FALSE(tempfile.exists());
    EXPECT_TRUE(newPath.exists());

    EXPECT_EQ(content, asp::fs::readToString(newPath).unwrap());

    asp::fs::remove(newPath).unwrap();
}

TEST_F(FsTest, FileSize) {
    std::string content = "Hello, world!";
    asp::fs::write(tempfile, content).unwrap();

    auto result = asp::fs::fileSize(tempfile);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), content.size());
}

TEST_F(FsTest, CreateDir) {
    ASSERT_TRUE(asp::fs::createDir(tempfile).isOk());

    EXPECT_TRUE(tempfile.exists());
    EXPECT_TRUE(tempfile.isDir());
    EXPECT_FALSE(tempfile.isFile());
    EXPECT_FALSE(tempfile.isSymlink());
}

TEST_F(FsTest, CreateDirAll) {
    Path nestedDir = tempfile / "nested" / "dir";
    ASSERT_TRUE(asp::fs::createDirAll(nestedDir).isOk());

    EXPECT_TRUE(nestedDir.exists());
    EXPECT_TRUE(nestedDir.isDir());
    EXPECT_TRUE(nestedDir.parent()->exists());

    asp::fs::removeDirAll(tempfile).unwrap();

    EXPECT_FALSE(tempfile.exists());
}

TEST_F(FsTest, RemoveDirAll) {
    Path nestedDir = tempfile / "nested" / "dir";
    ASSERT_TRUE(asp::fs::createDirAll(nestedDir).isOk());

    asp::fs::write(tempfile / "1.txt", "Hello").unwrap();
    asp::fs::write(tempfile / "2.txt", "Hello").unwrap();
    asp::fs::write(tempfile / "nested" / "1.txt", "Hello").unwrap();
    asp::fs::write(tempfile / "nested" / "2.txt", "Hello").unwrap();
    asp::fs::write(tempfile / "nested" / "dir" / "1.txt", "Hello").unwrap();
    asp::fs::write(tempfile / "nested" / "dir" / "2.txt", "Hello").unwrap();

    ASSERT_TRUE(nestedDir.exists());
    EXPECT_TRUE(asp::fs::removeDirAll(tempfile).isOk());
    EXPECT_FALSE(tempfile.exists());
}

TEST_F(FsTest, IterDir) {
    ASSERT_TRUE(asp::fs::createDirAll(tempfile).isOk());

    std::vector<Path> expectedPaths {
        tempfile / "1.txt",
        tempfile / "2.txt",
        tempfile / "3.txt",
    };
    std::sort(expectedPaths.begin(), expectedPaths.end());

    for (auto& path : expectedPaths) {
        asp::fs::write(path, "Hello").unwrap();
    }

    auto actualPaths = asp::fs::iterDir(tempfile).unwrap()
        .filterMap([](auto result) -> std::optional<Path> {
            if (result.isOk()) {
                return result.unwrap();
            } else {
                return std::nullopt;
            }
        })
        .collect();

    std::sort(actualPaths.begin(), actualPaths.end());
    EXPECT_EQ(actualPaths, expectedPaths);

    asp::fs::removeDirAll(tempfile).unwrap();
}

// TODO: symlink tests
