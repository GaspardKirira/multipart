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
                        "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "Hello World\r\n"
                        "--" +
      boundary + "--\r\n";

  auto res = multipart::parse(body, boundary);

  const auto *file = res.find("file");
  if (file)
  {
    std::cout << "Filename: " << file->filename << "\n";
    std::cout << "Content-Type: "
              << file->headers.at("content-type") << "\n";
    std::cout << "Size: " << file->body.size() << " bytes\n";
    std::cout << "Data: " << to_string(file->body) << "\n";
  }

  return 0;
}
