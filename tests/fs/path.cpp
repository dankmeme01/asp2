#include <asp/fs.hpp>
#include <gtest/gtest.h>

using namespace asp::fs;

static Path createTmpFile() {
    auto tmp = Path::tmp().unwrap();
    auto file = tmp / "__asp_fs_test.txt";

    asp::fs::write(file, "hi").unwrap();
    return file;
}

TEST(Path, BlankPath) {
    Path p;

    EXPECT_TRUE(p.empty());
    EXPECT_EQ(p.string(), "");
    EXPECT_EQ(p.view().view(), "");
    EXPECT_EQ(p.size(), 0);
    EXPECT_EQ(p.filename(), std::nullopt);
    EXPECT_EQ(p.extension(), std::nullopt);
    EXPECT_EQ(p.rootName(), std::nullopt);
    EXPECT_EQ(p.parent(), std::nullopt);
}

TEST(Path, CopyMove) {
    Path p1("foo/bar");
    Path p2(p1);
    Path p3(std::move(p1));

    EXPECT_EQ(p1.string(), "");
    EXPECT_EQ(p2.string(), "foo/bar");
    EXPECT_EQ(p3.string(), "foo/bar");
}

TEST(Path, CWD) {
    auto res = Path::cwd();
    EXPECT_TRUE(res.isOk());

    auto path = res.unwrap();
    EXPECT_TRUE(path.isAbsolute());
}

TEST(Path, Home) {
    auto res = Path::home();
    EXPECT_TRUE(res.isOk());

    auto path = res.unwrap();
    EXPECT_TRUE(path.isAbsolute());
}

TEST(Path, TempDir) {
    auto res = Path::tmp();
    EXPECT_TRUE(res.isOk());

    auto path = res.unwrap();
    EXPECT_TRUE(path.isAbsolute());
}

TEST(Path, SlashEqual) {
    Path p1("foo");
    Path p2("bar");

    p1 /= p2;
    EXPECT_EQ(p1.string(), "foo/bar");

    p1 /= p1;
    EXPECT_EQ(p1.string(), "foo/bar/foo/bar");

    p1 /= "";
    EXPECT_EQ(p1.string(), "foo/bar/foo/bar");
}

TEST(Path, Append) {
    Path p1("foo");
    Path p2("bar");

    p1.append(p2);
    EXPECT_EQ(p1.string(), "foo/bar");

    p1.append(p1);
    EXPECT_EQ(p1.string(), "foo/bar/foo/bar");

    p1.append("");
    EXPECT_EQ(p1.string(), "foo/bar/foo/bar");
}

TEST(Path, PlusEqual) {
    Path p1("foo");
    Path p2("bar");

    p1 += p2;
    EXPECT_EQ(p1.string(), "foobar");

    p1 += p1;
    EXPECT_EQ(p1.string(), "foobarfoobar");

    p1 += "";
    EXPECT_EQ(p1.string(), "foobarfoobar");
}

TEST(Path, Concat) {
    Path p1("foo");
    Path p2("bar");

    p1.concat(p2);
    EXPECT_EQ(p1.string(), "foobar");

    p1.concat(p1);
    EXPECT_EQ(p1.string(), "foobarfoobar");

    p1.concat("");
    EXPECT_EQ(p1.string(), "foobarfoobar");
}

TEST(Path, Clear) {
    Path p("foo/bar");
    EXPECT_FALSE(p.empty());

    p.clear();
    EXPECT_TRUE(p.empty());
    EXPECT_EQ(p.string(), "");
}

TEST(Path, HashDiffers) {
    Path p1("foo/bar");
    Path p2("foo/baz");

    EXPECT_NE(p1.hash(), p2.hash());
}

TEST(Path, Join) {
    Path p1("foo");
    Path p2("bar");

    auto p3 = p1.join(p2);
    EXPECT_EQ(p3.string(), "foo/bar");

    auto p4 = p3.join("baz");
    EXPECT_EQ(p4.string(), "foo/bar/baz");
}

TEST(Path, Slash) {
    Path p1("foo");
    Path p2("bar");

    auto p3 = p1 / p2;
    EXPECT_EQ(p3.string(), "foo/bar");

    auto p4 = p3 / "baz";
    EXPECT_EQ(p4.string(), "foo/bar/baz");
}

TEST(Path, JoinAbsolute) {
    Path p1("foo");
    Path p2("/bar");

    EXPECT_TRUE(p1.isRelative());
    EXPECT_TRUE(p2.isAbsolute());

    auto p3 = p1.join(p2);
    EXPECT_EQ(p3.string(), "/bar");

    auto p4 = p3.join("baz");
    EXPECT_EQ(p4.string(), "/bar/baz");
}

TEST(Path, MakePreferred) {
    Path p1("foo/bar");
    Path p2("foo\\bar");

    auto pr1 = p1.makePreferred();
    auto pr2 = p2.makePreferred();

#ifdef _WIN32
    EXPECT_TRUE(pr1.string().contains(PreferredSep));
    EXPECT_TRUE(pr2.string().contains(PreferredSep));
#else
    // unix expects no difference
    EXPECT_EQ(pr1.string(), "foo/bar");
    EXPECT_EQ(pr2.string(), "foo\\bar");
#endif
}

TEST(Path, RemoveFilename) {
    Path p1("foo/bar.txt");
    Path p2("foo/bar");

    auto r1 = p1.removeFilename();
    auto r2 = p2.removeFilename();

    EXPECT_EQ(r1.string(), "foo/");
    EXPECT_EQ(r2.string(), "foo/");
}

TEST(Path, ReplaceFilename) {
    Path p1("foo/bar.txt");
    Path p2("foo/bar");

    auto r1 = p1.replaceFilename("baz.txt");
    auto r2 = p2.replaceFilename("baz");

    EXPECT_EQ(r1.string(), "foo/baz.txt");
    EXPECT_EQ(r2.string(), "foo/baz");
}

TEST(Path, ReplaceExtension) {
    Path p1("file.txt");
    Path p2("file.txt");
    Path p3("file.txt");
    Path p4("file");
    Path p5("file");
    Path p6("file.");
    Path p7("file..");

    EXPECT_EQ(p1.replaceExtension("md").string(), "file.md");
    EXPECT_EQ(p2.replaceExtension(".md").string(), "file.md");
    EXPECT_EQ(p3.replaceExtension("..md").string(), "file..md");
    EXPECT_EQ(p4.replaceExtension("md").string(), "file.md");
    EXPECT_EQ(p5.replaceExtension("..md").string(), "file..md");
    EXPECT_EQ(p6.replaceExtension("md").string(), "file.md");
    EXPECT_EQ(p7.replaceExtension("md").string(), "file..md");
}

TEST(Path, Formattable) {
    Path p1("foo/bar.txt");
    auto f = fmt::format("{}", p1);

    EXPECT_EQ(f, "foo/bar.txt");
}

TEST(Path, Parent) {
    Path p1("foo/bar/baz.txt");
    Path p2("foo/bar/.");
    Path p3("foo");
    Path p4("/");
    Path p5("/foo");

    EXPECT_EQ(p1.parent()->string(), "foo/bar");
    EXPECT_EQ(p2.parent()->string(), "foo/bar");
    EXPECT_EQ(p3.parent(), std::nullopt);
    EXPECT_EQ(p4.parent(), std::nullopt);
    EXPECT_EQ(p5.parent()->string(), "/");
}

TEST(Path, Filename) {
    Path p1("foo/bar/baz.txt");
    Path p2("foo/bar/.");
    Path p3("foo");
    Path p4("/");

    EXPECT_EQ(p1.filename(), "baz.txt");
    EXPECT_EQ(p2.filename(), ".");
    EXPECT_EQ(p3.filename(), "foo");
    EXPECT_EQ(p4.filename(), std::nullopt);
}

TEST(Path, Extension) {
    Path p1("foo/bar/baz.txt");
    Path p2("foo/bar/baz.");
    Path p3("foo/bar/baz");
    Path p4("foo/bar/.baz");
    Path p5("foo/bar/..baz");

    EXPECT_EQ(p1.extension(), "txt");
    EXPECT_EQ(p2.extension(), "");
    EXPECT_EQ(p3.extension(), std::nullopt);
    EXPECT_EQ(p4.extension(), std::nullopt);
    EXPECT_EQ(p5.extension(), "baz");
}

TEST(Path, Canonicalize) {
    auto dest = createTmpFile();
    auto tmp = dest.parent().value();
    auto alt = tmp / ".." / tmp.filename().value() / "__asp_fs_test.txt";

    asp::fs::write(dest, "hi").unwrap();

    auto altCanon = alt.canonicalize();
    EXPECT_TRUE(altCanon.isOk());

    EXPECT_NE(dest.string(), alt.string());
    EXPECT_EQ(altCanon.unwrap().string(), dest.string());

    asp::fs::remove(dest).unwrap();
}

TEST(Path, Equal) {
    Path p1("foo/bar");
    Path p2("foo/bar");
    PathView p3("foo/baz");

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 != p2);
    EXPECT_FALSE(p1 == p3);
    EXPECT_TRUE(p1 != p3);
}

TEST(Path, Equivalent) {
    auto p1 = createTmpFile();
    auto tmp = p1.parent().value();
    auto p2 = tmp / ".." / tmp.filename().value() / "__asp_fs_test.txt";

    asp::fs::write(p1, "hi").unwrap();
    EXPECT_TRUE(p1.equivalent(p2));
    asp::fs::remove(p1).unwrap();
}

TEST(Path, Exists) {
    auto p1 = createTmpFile();
    auto tmp = p1.parent().value();
    auto p2 = tmp / ".." / tmp.filename().value() / "__asp_fs_test.txt";
    auto p3 = tmp / "nonexistent.txt";

    EXPECT_TRUE(p1.exists());
    EXPECT_TRUE(p2.exists());
    EXPECT_FALSE(p3.exists());

    asp::fs::remove(p1).unwrap();
}

TEST(Path, Metadata) {
    auto p1 = createTmpFile();
    auto tmp = p1.parent().value();
    auto p2 = tmp / ".." / tmp.filename().value() / "__asp_fs_test.txt";
    auto p3 = tmp / "nonexistent.txt";

    auto m1 = p1.metadata();
    auto m2 = p2.metadata();
    auto m3 = p3.metadata();

    EXPECT_TRUE(m1.isOk());
    EXPECT_TRUE(m2.isOk());
    EXPECT_FALSE(m3.isOk());

    EXPECT_TRUE(p1.isFile());
    EXPECT_TRUE(p2.isFile());

    asp::fs::remove(p1).unwrap();

    auto meta = m1.unwrap();
    EXPECT_TRUE(meta.isFile());
    if (meta.createdAt) {
        EXPECT_TRUE(meta.createdAt->elapsed() < asp::Duration::fromMinutes(1));
    }
}

TEST(Path, Components) {
    Path p1("foo/bar/baz.txt");
    Path p2("/foo/bar/baz.txt");
    Path p3("C:/foo/bar/baz.txt");
    Path p4("C:foo/bar/baz.txt");

    std::vector<std::string_view> c1 = {"foo", "bar", "baz.txt"};
    std::vector<std::string_view> c2 = {"/", "foo", "bar", "baz.txt"};

#ifdef _WIN32
    std::vector<std::string_view> c3 = {"C:/", "foo", "bar", "baz.txt"};
    std::vector<std::string_view> c4 = {"C:", "foo", "bar", "baz.txt"};
#else
    std::vector<std::string_view> c3 = {"C:", "foo", "bar", "baz.txt"};
    std::vector<std::string_view> c4 = {"C:foo", "bar", "baz.txt"};
#endif

    EXPECT_EQ(p1.components().collect(), c1);
    EXPECT_EQ(p2.components().collect(), c2);
    EXPECT_EQ(p3.components().collect(), c3);
    EXPECT_EQ(p4.components().collect(), c4);
}

TEST(Path, StartsWith) {
    Path p1{"foo/bar"};
    EXPECT_TRUE(p1.startsWith("foo"));
}

TEST(Path, EndsWith) {
    Path p1{"foo/bar"};
    EXPECT_TRUE(p1.endsWith("bar"));
}

TEST(Path, Conversions) {
    std::string str = "test.txt";
    std::string_view sv = "test.txt";
    auto ptr = "test.txt";

    Path p1{str};
    Path p2{sv};
    Path p3{ptr};

    PathView pv1{str};
    PathView pv2{sv};
    PathView pv3{ptr};

    PathView p1v{p1};

}

TEST(PathView, ToPath) {
    PathView pv("foo/bar.txt");
    Path p = pv.toPath();

    EXPECT_EQ(p.string(), "foo/bar.txt");
}