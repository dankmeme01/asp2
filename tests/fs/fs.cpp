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
                auto entry = result.unwrap();
                EXPECT_EQ(entry.type, FileType::RegularFile);
                return entry.path;
            } else {
                return std::nullopt;
            }
        })
        .collect();

    std::sort(actualPaths.begin(), actualPaths.end());
    EXPECT_EQ(actualPaths, expectedPaths);

    asp::fs::removeDirAll(tempfile).unwrap();
}

TEST_F(FsTest, IterDirRecursive) {
    ASSERT_TRUE(asp::fs::createDirAll(tempfile / "1" / "2").isOk());

    std::vector<Path> filePaths {
        tempfile / "1.txt",
        tempfile / "2.txt",
        tempfile / "1" / "1.txt",
        tempfile / "1" / "2" / "1.txt",
    };
    for (auto& p : filePaths) {
        asp::fs::write(p, "Hello").unwrap();
    }

    std::vector<Path> expectedPaths {
        tempfile / "1.txt",
        tempfile / "2.txt",
        tempfile / "1",
        tempfile / "1" / "1.txt",
        tempfile / "1" / "2",
        tempfile / "1" / "2" / "1.txt",
    };
    for (auto& p : expectedPaths) {
        p = p.canonicalize().unwrap();
    }
    std::sort(expectedPaths.begin(), expectedPaths.end());

    // retrieve paths via iterDirRecursive

    auto actualPaths = asp::fs::iterDirRecursive(tempfile).unwrap()
        .filterMap([](auto result) -> std::optional<Path> {
            if (result.isOk()) {
                auto entry = result.unwrap();
                if (entry.path.extension() == "txt") {
                    EXPECT_EQ(entry.type, FileType::RegularFile);
                } else {
                    EXPECT_EQ(entry.type, FileType::Directory);
                }

                return entry.path;
            } else {
                return std::nullopt;
            }
        })
        .collect();

    for (auto& p : actualPaths) {
        p = p.canonicalize().unwrap();
    }
    std::sort(actualPaths.begin(), actualPaths.end());

    // sorted canonical paths must be the same as the ones we expect above: 2 directories and 4 files
    EXPECT_EQ(actualPaths.size(), expectedPaths.size());
    for (size_t i = 0; i < actualPaths.size(); ++i) {
        EXPECT_EQ(actualPaths[i].string(), expectedPaths[i].string());
    }

    asp::fs::removeDirAll(tempfile).unwrap();
}

TEST_F(FsTest, CreateFileSymlink) {
    asp::fs::write(tempfile, "Hello").unwrap();

    Path symlinkPath = tempfile.parent().value() / fmt::format("{}_symlink", tempfile.filename().value());
    ASSERT_TRUE(asp::fs::createSymlink(tempfile, symlinkPath).isOk());
    EXPECT_TRUE(symlinkPath.exists());

    auto fileMetadata = tempfile.metadata().unwrap();
    auto symTargetMetadata = symlinkPath.metadata().unwrap();

    // fileMetadata by all means should be equal to symTargetMetadata, minus a few things like accessedAt
    EXPECT_EQ(fileMetadata.type, symTargetMetadata.type);
    EXPECT_EQ(fileMetadata.createdAt, symTargetMetadata.createdAt);
    EXPECT_EQ(fileMetadata.modifiedAt, symTargetMetadata.modifiedAt);
    EXPECT_EQ(fileMetadata.size, symTargetMetadata.size);

    // and the paths should be considered equivalent, when following symlinks
    EXPECT_TRUE(tempfile.equivalent(symlinkPath));
    // but not if using symlinkEquivalent
    EXPECT_FALSE(tempfile.symlinkEquivalent(symlinkPath));


    auto symlinkMetadata = symlinkPath.symlinkMetadata().unwrap();
    EXPECT_TRUE(symlinkMetadata.isSymlink());
    EXPECT_FALSE(symlinkMetadata.isFile());
    EXPECT_FALSE(symlinkMetadata.isDir());

    auto computedTarget = asp::fs::readSymlink(symlinkPath).unwrap();
    EXPECT_TRUE(computedTarget.equivalent(tempfile));

    EXPECT_TRUE(asp::fs::remove(symlinkPath).isOk());
}

TEST_F(FsTest, CreateDirSymlink) {
    asp::fs::createDir(tempfile).unwrap();

    auto testPath = tempfile / "test.txt";
    asp::fs::write(testPath, "Hello").unwrap();

    // create link to 'tempfile' directory

    Path symlinkDir = tempfile.parent().value() / fmt::format("{}_symlink", tempfile.filename().value());
    ASSERT_TRUE(asp::fs::createDirSymlink(tempfile, symlinkDir).isOk());
    EXPECT_TRUE(symlinkDir.exists());

    auto fileMetadata = tempfile.metadata().unwrap();
    auto symTargetMetadata = symlinkDir.metadata().unwrap();

    // fileMetadata by all means should be equal to symTargetMetadata, minus a few things like accessedAt
    EXPECT_EQ(fileMetadata.type, symTargetMetadata.type);
    EXPECT_EQ(fileMetadata.createdAt, symTargetMetadata.createdAt);
    EXPECT_EQ(fileMetadata.modifiedAt, symTargetMetadata.modifiedAt);
    EXPECT_EQ(fileMetadata.size, symTargetMetadata.size);

    // and the paths should be considered equivalent, when following symlinks
    EXPECT_TRUE(tempfile.equivalent(symlinkDir));
    // but not if using symlinkEquivalent
    EXPECT_FALSE(tempfile.symlinkEquivalent(symlinkDir));


    auto symlinkMetadata = symlinkDir.symlinkMetadata().unwrap();
    EXPECT_TRUE(symlinkMetadata.isSymlink());
    EXPECT_FALSE(symlinkMetadata.isFile());
    EXPECT_FALSE(symlinkMetadata.isDir());

    auto computedTarget = asp::fs::readSymlink(symlinkDir).unwrap();
    EXPECT_TRUE(computedTarget.equivalent(tempfile));

    // expect to be able to read the file via symlink
    auto symlinkedFile = symlinkDir / "test.txt";
    EXPECT_TRUE(symlinkedFile.exists());
    EXPECT_EQ(asp::fs::readToString(symlinkedFile).unwrap(), "Hello");

    asp::fs::remove(symlinkDir).unwrap();
    asp::fs::removeDirAll(tempfile).unwrap();
}

TEST_F(FsTest, CopyDir) {
    ASSERT_TRUE(asp::fs::createDirAll(tempfile / "1" / "2").isOk());

    std::vector<Path> filePaths {
        tempfile / "1.txt",
        tempfile / "2.txt",
        tempfile / "1" / "1.txt",
        tempfile / "1" / "2" / "1.txt",
    };
    for (auto& p : filePaths) {
        asp::fs::write(p, "Hello").unwrap();
    }

    std::vector<Path> expectedSuffixes {
        Path{"1.txt"},
        Path{"2.txt"},
        Path{"1"},
        Path{"1"} / "1.txt",
        Path{"1"} / "2",
        Path{"1"} / "2" / "1.txt",
    };
    std::sort(expectedSuffixes.begin(), expectedSuffixes.end());

    auto destDir = tempfile.parent().value() / fmt::format("{}_copy", tempfile.filename().value());
    asp::fs::copy(tempfile, destDir).unwrap();

    // expect all suffixes to appear
    auto actualSuffixes = asp::fs::iterDirRecursive(destDir).unwrap()
        .map([&](auto result) {
            DirectoryEntry entry = result.unwrap();
            return entry.path.lexicallyRelative(destDir).value();
        })
        .collect();
    std::sort(actualSuffixes.begin(), actualSuffixes.end());

    // sorted canonical paths must be the same as the ones we expect above: 2 directories and 4 files
    EXPECT_EQ(actualSuffixes.size(), expectedSuffixes.size());
    for (size_t i = 0; i < actualSuffixes.size(); ++i) {
        EXPECT_EQ(actualSuffixes[i].string(), expectedSuffixes[i].string());
    }

    asp::fs::removeDirAll(tempfile).unwrap();
    asp::fs::removeDirAll(destDir).unwrap();
}
