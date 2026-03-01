#include <multipart/multipart.hpp>
#include <iostream>
#include <string>

static std::string to_string(const std::vector<std::uint8_t> &b)
{
  return std::string(reinterpret_cast<const char *>(b.data()), b.size());
}

int main()
{
  const std::string boundary = "----BOUNDARY";

  const std::string body =
      "--" + boundary + "\r\n"
                        "Content-Disposition: form-data; name=\"username\"\r\n"
                        "\r\n"
                        "gaspard\r\n"
                        "--" +
      boundary + "\r\n"
                 "Content-Disposition: form-data; name=\"email\"\r\n"
                 "\r\n"
                 "user@example.com\r\n"
                 "--" +
      boundary + "--\r\n";

  auto res = multipart::parse(body, boundary);

  for (const auto &p : res.parts)
  {
    std::cout << "Field: " << p.name
              << " = " << to_string(p.body) << "\n";
  }

  return 0;
}
