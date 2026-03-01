#include <multipart/multipart.hpp>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

static std::string to_string(const std::vector<std::uint8_t> &b)
{
  return std::string(reinterpret_cast<const char *>(b.data()), b.size());
}

static void test_parse_fields_and_file()
{
  const std::string boundary = "----BOUNDARY";

  const std::string body =
      "--" + boundary + "\r\n"
                        "Content-Disposition: form-data; name=\"title\"\r\n"
                        "\r\n"
                        "hello\r\n"
                        "--" +
      boundary + "\r\n"
                 "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "ABCDEF\r\n"
                 "--" +
      boundary + "--\r\n";

  auto res = multipart::parse(body, boundary);

  assert(res.parts.size() == 2);

  const auto *p0 = res.find("title");
  assert(p0 != nullptr);
  assert(p0->filename.empty());
  assert(to_string(p0->body) == "hello");

  const auto *p1 = res.find("file");
  assert(p1 != nullptr);
  assert(p1->filename == "a.txt");
  assert(to_string(p1->body) == "ABCDEF");

  // header lookup stored lowercased
  assert(p1->headers.find("content-type") != p1->headers.end());
}

static void test_multiple_same_name()
{
  const std::string boundary = "X";

  const std::string body =
      "--" + boundary + "\n"
                        "Content-Disposition: form-data; name=\"tag\"\n"
                        "\n"
                        "a\n"
                        "--" +
      boundary + "\n"
                 "Content-Disposition: form-data; name=\"tag\"\n"
                 "\n"
                 "b\n"
                 "--" +
      boundary + "--\n";

  auto res = multipart::parse(body, boundary);

  auto all = res.find_all("tag");
  assert(all.size() == 2);
  assert(to_string(all[0]->body) == "a");
  assert(to_string(all[1]->body) == "b");
}

static void test_limits()
{
  const std::string boundary = "B";

  const std::string body =
      "--" + boundary + "\r\n"
                        "Content-Disposition: form-data; name=\"a\"\r\n"
                        "\r\n"
                        "1\r\n"
                        "--" +
      boundary + "--\r\n";

  multipart::options opts;
  opts.max_parts = 1;
  opts.max_part_bytes = 1;

  auto res = multipart::parse(body, boundary, opts);
  assert(res.parts.size() == 1);
  assert(to_string(res.parts[0].body) == "1");
}

int main()
{
  test_parse_fields_and_file();
  test_multiple_same_name();
  test_limits();
  return 0;
}
