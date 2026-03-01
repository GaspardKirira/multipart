#include <multipart/multipart.hpp>
#include <iostream>
#include <string>

int main()
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
  opts.max_parts = 5;
  opts.max_part_bytes = 1024;

  auto res = multipart::parse(body, boundary, opts);

  std::cout << "Parsed parts: " << res.parts.size() << "\n";

  return 0;
}
