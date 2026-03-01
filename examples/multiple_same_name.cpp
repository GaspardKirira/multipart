#include <multipart/multipart.hpp>
#include <iostream>
#include <string>

static std::string to_string(const std::vector<std::uint8_t> &b)
{
  return std::string(reinterpret_cast<const char *>(b.data()), b.size());
}

int main()
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

  auto tags = res.find_all("tag");
  for (const auto *p : tags)
  {
    std::cout << "Tag: " << to_string(p->body) << "\n";
  }

  return 0;
}
