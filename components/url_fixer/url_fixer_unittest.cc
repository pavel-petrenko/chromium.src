// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include "base/base_paths.h"
#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/url_fixer/url_fixer.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_parse.h"

namespace url {

std::ostream& operator<<(std::ostream& os, const Component& part) {
  return os << "(begin=" << part.begin << ", len=" << part.len << ")";
}

}  // namespace url

struct SegmentCase {
  const std::string input;
  const std::string result;
  const url::Component scheme;
  const url::Component username;
  const url::Component password;
  const url::Component host;
  const url::Component port;
  const url::Component path;
  const url::Component query;
  const url::Component ref;
};

static const SegmentCase segment_cases[] = {
  { "http://www.google.com/", "http",
    url::Component(0, 4), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(7, 14), // host
    url::Component(), // port
    url::Component(21, 1), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "aBoUt:vErSiOn", "about",
    url::Component(0, 5), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(6, 7), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "about:host/path?query#ref", "about",
    url::Component(0, 5), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(6, 4), // host
    url::Component(), // port
    url::Component(10, 5), // path
    url::Component(16, 5), // query
    url::Component(22, 3), // ref
  },
  { "about://host/path?query#ref", "about",
    url::Component(0, 5), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(8, 4), // host
    url::Component(), // port
    url::Component(12, 5), // path
    url::Component(18, 5), // query
    url::Component(24, 3), // ref
  },
  { "chrome:host/path?query#ref", "chrome",
    url::Component(0, 6), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(7, 4), // host
    url::Component(), // port
    url::Component(11, 5), // path
    url::Component(17, 5), // query
    url::Component(23, 3), // ref
  },
  { "chrome://host/path?query#ref", "chrome",
    url::Component(0, 6), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(9, 4), // host
    url::Component(), // port
    url::Component(13, 5), // path
    url::Component(19, 5), // query
    url::Component(25, 3), // ref
  },
  { "    www.google.com:124?foo#", "http",
    url::Component(), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(4, 14), // host
    url::Component(19, 3), // port
    url::Component(), // path
    url::Component(23, 3), // query
    url::Component(27, 0), // ref
  },
  { "user@www.google.com", "http",
    url::Component(), // scheme
    url::Component(0, 4), // username
    url::Component(), // password
    url::Component(5, 14), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "ftp:/user:P:a$$Wd@..ftp.google.com...::23///pub?foo#bar", "ftp",
    url::Component(0, 3), // scheme
    url::Component(5, 4), // username
    url::Component(10, 7), // password
    url::Component(18, 20), // host
    url::Component(39, 2), // port
    url::Component(41, 6), // path
    url::Component(48, 3), // query
    url::Component(52, 3), // ref
  },
  { "[2001:db8::1]/path", "http",
    url::Component(), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(0, 13), // host
    url::Component(), // port
    url::Component(13, 5), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "[::1]", "http",
    url::Component(), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(0, 5), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
  // Incomplete IPv6 addresses (will not canonicalize).
  { "[2001:4860:", "http",
    url::Component(), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(0, 11), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "[2001:4860:/foo", "http",
    url::Component(), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(0, 11), // host
    url::Component(), // port
    url::Component(11, 4), // path
    url::Component(), // query
    url::Component(), // ref
  },
  { "http://:b005::68]", "http",
    url::Component(0, 4), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(7, 10), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
  // Can't do anything useful with this.
  { ":b005::68]", "",
    url::Component(0, 0), // scheme
    url::Component(), // username
    url::Component(), // password
    url::Component(), // host
    url::Component(), // port
    url::Component(), // path
    url::Component(), // query
    url::Component(), // ref
  },
};

typedef testing::Test URLFixerTest;

TEST(URLFixerTest, SegmentURL) {
  std::string result;
  url::Parsed parts;

  for (size_t i = 0; i < arraysize(segment_cases); ++i) {
    SegmentCase value = segment_cases[i];
    result = url_fixer::SegmentURL(value.input, &parts);
    EXPECT_EQ(value.result, result);
    EXPECT_EQ(value.scheme, parts.scheme);
    EXPECT_EQ(value.username, parts.username);
    EXPECT_EQ(value.password, parts.password);
    EXPECT_EQ(value.host, parts.host);
    EXPECT_EQ(value.port, parts.port);
    EXPECT_EQ(value.path, parts.path);
    EXPECT_EQ(value.query, parts.query);
    EXPECT_EQ(value.ref, parts.ref);
  }
}

// Creates a file and returns its full name as well as the decomposed
// version. Example:
//    full_path = "c:\foo\bar.txt"
//    dir = "c:\foo"
//    file_name = "bar.txt"
static bool MakeTempFile(const base::FilePath& dir,
                         const base::FilePath& file_name,
                         base::FilePath* full_path) {
  *full_path = dir.Append(file_name);
  return base::WriteFile(*full_path, "", 0) == 0;
}

// Returns true if the given URL is a file: URL that matches the given file
static bool IsMatchingFileURL(const std::string& url,
                              const base::FilePath& full_file_path) {
  if (url.length() <= 8)
    return false;
  if (std::string("file:///") != url.substr(0, 8))
    return false; // no file:/// prefix
  if (url.find('\\') != std::string::npos)
    return false; // contains backslashes

  base::FilePath derived_path;
  net::FileURLToFilePath(GURL(url), &derived_path);

  return base::FilePath::CompareEqualIgnoreCase(derived_path.value(),
                                          full_file_path.value());
}

struct FixupCase {
  const std::string input;
  const std::string output;
} fixup_cases[] = {
  {"www.google.com", "http://www.google.com/"},
  {" www.google.com     ", "http://www.google.com/"},
  {" foo.com/asdf  bar", "http://foo.com/asdf%20%20bar"},
  {"..www.google.com..", "http://www.google.com./"},
  {"http://......", "http://....../"},
  {"http://host.com:ninety-two/", "http://host.com:ninety-two/"},
  {"http://host.com:ninety-two?foo", "http://host.com:ninety-two/?foo"},
  {"google.com:123", "http://google.com:123/"},
  {"about:", "chrome://version/"},
  {"about:foo", "chrome://foo/"},
  {"about:version", "chrome://version/"},
  {"about:blank", "about:blank"},
  {"about:usr:pwd@hst/pth?qry#ref", "chrome://usr:pwd@hst/pth?qry#ref"},
  {"about://usr:pwd@hst/pth?qry#ref", "chrome://usr:pwd@hst/pth?qry#ref"},
  {"chrome:usr:pwd@hst/pth?qry#ref", "chrome://usr:pwd@hst/pth?qry#ref"},
  {"chrome://usr:pwd@hst/pth?qry#ref", "chrome://usr:pwd@hst/pth?qry#ref"},
  {"www:123", "http://www:123/"},
  {"   www:123", "http://www:123/"},
  {"www.google.com?foo", "http://www.google.com/?foo"},
  {"www.google.com#foo", "http://www.google.com/#foo"},
  {"www.google.com?", "http://www.google.com/?"},
  {"www.google.com#", "http://www.google.com/#"},
  {"www.google.com:123?foo#bar", "http://www.google.com:123/?foo#bar"},
  {"user@www.google.com", "http://user@www.google.com/"},
  {"\xE6\xB0\xB4.com", "http://xn--1rw.com/"},
  // It would be better if this next case got treated as http, but I don't see
  // a clean way to guess this isn't the new-and-exciting "user" scheme.
  {"user:passwd@www.google.com:8080/", "user:passwd@www.google.com:8080/"},
  // {"file:///c:/foo/bar%20baz.txt", "file:///C:/foo/bar%20baz.txt"},
  {"ftp.google.com", "ftp://ftp.google.com/"},
  {"    ftp.google.com", "ftp://ftp.google.com/"},
  {"FTP.GooGle.com", "ftp://ftp.google.com/"},
  {"ftpblah.google.com", "http://ftpblah.google.com/"},
  {"ftp", "http://ftp/"},
  {"google.ftp.com", "http://google.ftp.com/"},
  // URLs which end with 0x85 (NEL in ISO-8859).
  {"http://foo.com/s?q=\xd0\x85", "http://foo.com/s?q=%D0%85"},
  {"http://foo.com/s?q=\xec\x97\x85", "http://foo.com/s?q=%EC%97%85"},
  {"http://foo.com/s?q=\xf0\x90\x80\x85", "http://foo.com/s?q=%F0%90%80%85"},
  // URLs which end with 0xA0 (non-break space in ISO-8859).
  {"http://foo.com/s?q=\xd0\xa0", "http://foo.com/s?q=%D0%A0"},
  {"http://foo.com/s?q=\xec\x97\xa0", "http://foo.com/s?q=%EC%97%A0"},
  {"http://foo.com/s?q=\xf0\x90\x80\xa0", "http://foo.com/s?q=%F0%90%80%A0"},
  // URLs containing IPv6 literals.
  {"[2001:db8::2]", "http://[2001:db8::2]/"},
  {"[::]:80", "http://[::]/"},
  {"[::]:80/path", "http://[::]/path"},
  {"[::]:180/path", "http://[::]:180/path"},
  // TODO(pmarks): Maybe we should parse bare IPv6 literals someday.
  {"::1", "::1"},
  // Semicolon as scheme separator for standard schemes.
  {"http;//www.google.com/", "http://www.google.com/"},
  {"about;chrome", "chrome://chrome/"},
  // Semicolon left as-is for non-standard schemes.
  {"whatsup;//fool", "whatsup://fool"},
  // Semicolon left as-is in URL itself.
  {"http://host/port?query;moar", "http://host/port?query;moar"},
  // Fewer slashes than expected.
  {"http;www.google.com/", "http://www.google.com/"},
  {"http;/www.google.com/", "http://www.google.com/"},
  // Semicolon at start.
  {";http://www.google.com/", "http://%3Bhttp//www.google.com/"},
};

TEST(URLFixerTest, FixupURL) {
  for (size_t i = 0; i < arraysize(fixup_cases); ++i) {
    FixupCase value = fixup_cases[i];
    EXPECT_EQ(value.output,
              url_fixer::FixupURL(value.input, "").possibly_invalid_spec())
        << "input: " << value.input;
  }

  // Check the TLD-appending functionality.
  FixupCase tld_cases[] = {
    {"google", "http://www.google.com/"},
    {"google.", "http://www.google.com/"},
    {"google..", "http://www.google.com/"},
    {".google", "http://www.google.com/"},
    {"www.google", "http://www.google.com/"},
    {"google.com", "http://google.com/"},
    {"http://google", "http://www.google.com/"},
    {"..google..", "http://www.google.com/"},
    {"http://www.google", "http://www.google.com/"},
    {"9999999999999999", "http://www.9999999999999999.com/"},
    {"google/foo", "http://www.google.com/foo"},
    {"google.com/foo", "http://google.com/foo"},
    {"google/?foo=.com", "http://www.google.com/?foo=.com"},
    {"www.google/?foo=www.", "http://www.google.com/?foo=www."},
    {"google.com/?foo=.com", "http://google.com/?foo=.com"},
    {"http://www.google.com", "http://www.google.com/"},
    {"google:123", "http://www.google.com:123/"},
    {"http://google:123", "http://www.google.com:123/"},
  };
  for (size_t i = 0; i < arraysize(tld_cases); ++i) {
    FixupCase value = tld_cases[i];
    EXPECT_EQ(value.output,
              url_fixer::FixupURL(value.input, "com").possibly_invalid_spec());
  }
}

// Test different types of file inputs to URIFixerUpper::FixupURL. This
// doesn't go into the nice array of fixups above since the file input
// has to exist.
TEST(URLFixerTest, FixupFile) {
  // this "original" filename is the one we tweak to get all the variations
  base::FilePath dir;
  base::FilePath original;
  ASSERT_TRUE(PathService::Get(base::DIR_MODULE, &dir));
  ASSERT_TRUE(MakeTempFile(
      dir,
      base::FilePath(FILE_PATH_LITERAL("url fixer upper existing file.txt")),
      &original));

  // reference path
  GURL golden(net::FilePathToFileURL(original));

  // c:\foo\bar.txt -> file:///c:/foo/bar.txt (basic)
  GURL fixedup(url_fixer::FixupURL(original.AsUTF8Unsafe(), std::string()));
  EXPECT_EQ(golden, fixedup);

  // TODO(port): Make some equivalent tests for posix.
#if defined(OS_WIN)
  // c|/foo\bar.txt -> file:///c:/foo/bar.txt (pipe allowed instead of colon)
  std::string cur(base::WideToUTF8(original.value()));
  EXPECT_EQ(':', cur[1]);
  cur[1] = '|';
  EXPECT_EQ(golden, url_fixer::FixupURL(cur, std::string()));

  FixupCase cases[] = {
    {"c:\\Non-existent%20file.txt", "file:///C:/Non-existent%2520file.txt"},

    // \\foo\bar.txt -> file://foo/bar.txt
    // UNC paths, this file won't exist, but since there are no escapes, it
    // should be returned just converted to a file: URL.
    {"\\\\NonexistentHost\\foo\\bar.txt", "file://nonexistenthost/foo/bar.txt"},
    // We do this strictly, like IE8, which only accepts this form using
    // backslashes and not forward ones.  Turning "//foo" into "http" matches
    // Firefox and IE, silly though it may seem (it falls out of adding "http"
    // as the default protocol if you haven't entered one).
    {"//NonexistentHost\\foo/bar.txt", "http://nonexistenthost/foo/bar.txt"},
    {"file:///C:/foo/bar", "file:///C:/foo/bar"},

    // Much of the work here comes from GURL's canonicalization stage.
    {"file://C:/foo/bar", "file:///C:/foo/bar"},
    {"file:c:", "file:///C:/"},
    {"file:c:WINDOWS", "file:///C:/WINDOWS"},
    {"file:c|Program Files", "file:///C:/Program%20Files"},
    {"file:/file", "file://file/"},
    {"file:////////c:\\foo", "file:///C:/foo"},
    {"file://server/folder/file", "file://server/folder/file"},

    // These are fixups we don't do, but could consider:
    //   {"file:///foo:/bar", "file://foo/bar"},
    //   {"file:/\\/server\\folder/file", "file://server/folder/file"},
  };
#elif defined(OS_POSIX)

#if defined(OS_MACOSX)
#define HOME "/Users/"
#else
#define HOME "/home/"
#endif
  url_fixer::home_directory_override = "/foo";
  FixupCase cases[] = {
    // File URLs go through GURL, which tries to escape intelligently.
    {"/A%20non-existent file.txt", "file:///A%2520non-existent%20file.txt"},
    // A plain "/" refers to the root.
    {"/", "file:///"},

    // These rely on the above home_directory_override.
    {"~", "file:///foo"},
    {"~/bar", "file:///foo/bar"},

    // References to other users' homedirs.
    {"~foo", "file://" HOME "foo"},
    {"~x/blah", "file://" HOME "x/blah"},
  };
#endif

  for (size_t i = 0; i < arraysize(cases); i++) {
    EXPECT_EQ(cases[i].output,
              url_fixer::FixupURL(cases[i].input, "").possibly_invalid_spec());
  }

  EXPECT_TRUE(base::DeleteFile(original, false));
}

TEST(URLFixerTest, FixupRelativeFile) {
  base::FilePath full_path, dir;
  base::FilePath file_part(
      FILE_PATH_LITERAL("url_fixer_upper_existing_file.txt"));
  ASSERT_TRUE(PathService::Get(base::DIR_MODULE, &dir));
  ASSERT_TRUE(MakeTempFile(dir, file_part, &full_path));
  full_path = base::MakeAbsoluteFilePath(full_path);
  ASSERT_FALSE(full_path.empty());

  // make sure we pass through good URLs
  for (size_t i = 0; i < arraysize(fixup_cases); ++i) {
    FixupCase value = fixup_cases[i];
    base::FilePath input = base::FilePath::FromUTF8Unsafe(value.input);
    EXPECT_EQ(value.output,
              url_fixer::FixupRelativeFile(dir, input).possibly_invalid_spec());
  }

  // make sure the existing file got fixed-up to a file URL, and that there
  // are no backslashes
  EXPECT_TRUE(IsMatchingFileURL(
      url_fixer::FixupRelativeFile(dir, file_part).possibly_invalid_spec(),
      full_path));
  EXPECT_TRUE(base::DeleteFile(full_path, false));

  // create a filename we know doesn't exist and make sure it doesn't get
  // fixed up to a file URL
  base::FilePath nonexistent_file(
      FILE_PATH_LITERAL("url_fixer_upper_nonexistent_file.txt"));
  std::string fixedup(url_fixer::FixupRelativeFile(dir, nonexistent_file)
                          .possibly_invalid_spec());
  EXPECT_NE(std::string("file:///"), fixedup.substr(0, 8));
  EXPECT_FALSE(IsMatchingFileURL(fixedup, nonexistent_file));

  // make a subdir to make sure relative paths with directories work, also
  // test spaces:
  // "app_dir\url fixer-upper dir\url fixer-upper existing file.txt"
  base::FilePath sub_dir(FILE_PATH_LITERAL("url fixer-upper dir"));
  base::FilePath sub_file(
      FILE_PATH_LITERAL("url fixer-upper existing file.txt"));
  base::FilePath new_dir = dir.Append(sub_dir);
  base::CreateDirectory(new_dir);
  ASSERT_TRUE(MakeTempFile(new_dir, sub_file, &full_path));
  full_path = base::MakeAbsoluteFilePath(full_path);
  ASSERT_FALSE(full_path.empty());

  // test file in the subdir
  base::FilePath relative_file = sub_dir.Append(sub_file);
  EXPECT_TRUE(IsMatchingFileURL(
      url_fixer::FixupRelativeFile(dir, relative_file).possibly_invalid_spec(),
      full_path));

  // test file in the subdir with different slashes and escaping.
  base::FilePath::StringType relative_file_str = sub_dir.value() +
      FILE_PATH_LITERAL("/") + sub_file.value();
  ReplaceSubstringsAfterOffset(&relative_file_str, 0,
      FILE_PATH_LITERAL(" "), FILE_PATH_LITERAL("%20"));
  EXPECT_TRUE(IsMatchingFileURL(
      url_fixer::FixupRelativeFile(dir, base::FilePath(relative_file_str))
          .possibly_invalid_spec(),
      full_path));

  // test relative directories and duplicate slashes
  // (should resolve to the same file as above)
  relative_file_str = sub_dir.value() + FILE_PATH_LITERAL("/../") +
      sub_dir.value() + FILE_PATH_LITERAL("///./") + sub_file.value();
  EXPECT_TRUE(IsMatchingFileURL(
      url_fixer::FixupRelativeFile(dir, base::FilePath(relative_file_str))
          .possibly_invalid_spec(),
      full_path));

  // done with the subdir
  EXPECT_TRUE(base::DeleteFile(full_path, false));
  EXPECT_TRUE(base::DeleteFile(new_dir, true));

  // Test that an obvious HTTP URL isn't accidentally treated as an absolute
  // file path (on account of system-specific craziness).
  base::FilePath empty_path;
  base::FilePath http_url_path(FILE_PATH_LITERAL("http://../"));
  EXPECT_TRUE(
      url_fixer::FixupRelativeFile(empty_path, http_url_path).SchemeIs("http"));
}
