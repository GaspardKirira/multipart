/**
 * @file multipart.hpp
 * @brief Multipart/form-data parser for file uploads and mixed fields.
 *
 * `multipart` provides a small, deterministic parser for HTTP
 * multipart/form-data bodies:
 * - parse parts using a boundary string
 * - expose per-part headers (case-insensitive lookup)
 * - expose per-part name, optional filename, and raw body bytes
 * - handle mixed fields and file uploads
 *
 * This parser is designed for server-side request handling and assumes the
 * full body is already available in memory.
 *
 * Requirements: C++17+
 * Header-only. Zero external dependencies.
 */

#ifndef MULTIPART_MULTIPART_HPP
#define MULTIPART_MULTIPART_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace multipart
{
  /**
   * @brief Multipart parsing options.
   */
  struct options
  {
    // If true, supports the final boundary without trailing CRLF.
    bool allow_missing_final_crlf = true;

    // If true, trims trailing CRLF from each part body when present.
    bool trim_part_trailing_crlf = true;

    // Maximum allowed parts (0 = unlimited).
    std::size_t max_parts = 0;

    // Maximum allowed bytes per part (0 = unlimited).
    std::size_t max_part_bytes = 0;
  };

  /**
   * @brief A case-insensitive map for header storage (ASCII rules).
   *
   * Keys are stored as lowercase.
   */
  using headers_map = std::unordered_map<std::string, std::string>;

  /**
   * @brief Single multipart part.
   */
  struct part
  {
    headers_map headers{};
    std::string name{};     // from Content-Disposition: form-data; name="..."
    std::string filename{}; // optional; from filename="..."
    std::vector<std::uint8_t> body{};
  };

  /**
   * @brief Parsed multipart result.
   */
  struct result
  {
    std::vector<part> parts{};

    /**
     * @brief Find first part by name. Returns nullptr if missing.
     */
    const part *find(std::string_view n) const
    {
      for (const auto &p : parts)
      {
        if (p.name == n)
          return &p;
      }
      return nullptr;
    }

    /**
     * @brief Return all parts by name.
     */
    std::vector<const part *> find_all(std::string_view n) const
    {
      std::vector<const part *> out;
      for (const auto &p : parts)
      {
        if (p.name == n)
          out.push_back(&p);
      }
      return out;
    }
  };

  namespace detail
  {
    inline char ascii_lower(char c) noexcept
    {
      if (c >= 'A' && c <= 'Z')
        return static_cast<char>(c - 'A' + 'a');
      return c;
    }

    inline std::string lower_ascii(std::string_view s)
    {
      std::string out;
      out.reserve(s.size());
      for (char c : s)
        out.push_back(ascii_lower(c));
      return out;
    }

    inline bool is_space(char c) noexcept
    {
      return c == ' ' || c == '\t';
    }

    inline void ltrim_inplace(std::string &s)
    {
      std::size_t i = 0;
      while (i < s.size() && is_space(s[i]))
        ++i;
      if (i > 0)
        s.erase(0, i);
    }

    inline void rtrim_inplace(std::string &s)
    {
      while (!s.empty() && is_space(s.back()))
        s.pop_back();
    }

    inline std::string trim_copy(std::string s)
    {
      ltrim_inplace(s);
      rtrim_inplace(s);
      return s;
    }

    inline bool starts_with(std::string_view s, std::string_view pfx) noexcept
    {
      return s.size() >= pfx.size() && s.substr(0, pfx.size()) == pfx;
    }

    inline bool ends_with(std::string_view s, std::string_view suf) noexcept
    {
      return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
    }

    inline std::size_t find_sub(std::string_view hay, std::string_view needle, std::size_t from = 0)
    {
      const std::size_t pos = hay.find(needle, from);
      return pos;
    }

    inline void put_header(headers_map &h, std::string_view name, std::string_view value)
    {
      std::string k = lower_ascii(trim_copy(std::string(name)));
      std::string v = trim_copy(std::string(value));
      h[std::move(k)] = std::move(v);
    }

    inline std::string unquote(std::string s)
    {
      if (s.size() >= 2)
      {
        const char a = s.front();
        const char b = s.back();
        if ((a == '"' && b == '"') || (a == '\'' && b == '\''))
          return s.substr(1, s.size() - 2);
      }
      return s;
    }

    inline std::string extract_param(std::string_view s, std::string_view key)
    {
      const std::string wanted = lower_ascii(key);

      auto token_lower_eq = [&](std::string_view token) -> std::string
      {
        std::string t = trim_copy(std::string(token));
        if (t.empty())
          return "";

        const std::size_t eq = t.find('=');
        if (eq == std::string::npos)
          return "";

        std::string k = trim_copy(t.substr(0, eq));
        std::string v = trim_copy(t.substr(eq + 1));

        if (lower_ascii(k) != wanted)
          return "";

        return unquote(std::move(v));
      };

      std::size_t start = 0;
      bool in_quote = false;
      char q = '\0';

      for (std::size_t i = 0; i <= s.size(); ++i)
      {
        const bool end = (i == s.size());
        const char ch = end ? ';' : s[i];

        if (!end)
        {
          if (!in_quote && (ch == '"' || ch == '\''))
          {
            in_quote = true;
            q = ch;
          }
          else if (in_quote && ch == q)
          {
            in_quote = false;
          }
        }

        if (end || (!in_quote && ch == ';'))
        {
          const std::string_view token = s.substr(start, i - start);
          if (auto val = token_lower_eq(token); !val.empty())
            return val;

          start = i + 1;
        }
      }

      return "";
    }

    inline headers_map parse_headers_block(std::string_view block)
    {
      headers_map out;

      std::size_t i = 0;
      while (i < block.size())
      {
        const std::size_t nl = block.find('\n', i);
        std::size_t end = (nl == std::string_view::npos) ? block.size() : nl;

        // strip optional '\r'
        if (end > i && block[end - 1] == '\r')
          --end;

        const std::string_view line = block.substr(i, end - i);
        i = (nl == std::string_view::npos) ? block.size() : nl + 1;

        if (line.empty())
          break;

        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos)
          throw std::runtime_error("multipart: invalid header line (missing ':')");

        const std::string_view name = line.substr(0, colon);
        const std::string_view value = line.substr(colon + 1);

        put_header(out, name, value);
      }

      return out;
    }

    inline std::size_t find_headers_end(std::string_view body, std::size_t from)
    {
      // prefer CRLFCRLF, fallback to LFLF
      const std::size_t crlf = body.find("\r\n\r\n", from);
      if (crlf != std::string_view::npos)
        return crlf;

      const std::size_t lf = body.find("\n\n", from);
      return lf;
    }

    inline void trim_trailing_crlf(std::vector<std::uint8_t> &buf)
    {
      if (buf.size() >= 2 && buf[buf.size() - 2] == '\r' && buf[buf.size() - 1] == '\n')
      {
        buf.pop_back();
        buf.pop_back();
      }
      else if (!buf.empty() && buf.back() == '\n')
      {
        buf.pop_back();
      }
    }
  } // namespace detail

  /**
   * @brief Parse a multipart/form-data body.
   *
   * @param body The full HTTP request body.
   * @param boundary The multipart boundary without the leading "--".
   * @param opts Parsing options.
   * @throws std::runtime_error on malformed input.
   */
  inline result parse(std::string_view body, std::string_view boundary, const options &opts = {})
  {
    if (boundary.empty())
      throw std::runtime_error("multipart: empty boundary");

    const std::string delim = std::string("--") + std::string(boundary);
    const std::string delim_final = delim + "--";

    result out;

    // Find first boundary (must appear at start or after a leading CRLF/LF).
    std::size_t pos = body.find(delim);
    if (pos == std::string_view::npos)
      return out;

    pos += delim.size();

    // After first boundary: expect CRLF/LF, or "--" for empty final.
    if (pos + 2 <= body.size() && body.substr(pos, 2) == "--")
    {
      return out;
    }

    // consume line ending
    if (pos + 2 <= body.size() && body.substr(pos, 2) == "\r\n")
      pos += 2;
    else if (pos + 1 <= body.size() && body[pos] == '\n')
      pos += 1;
    else
      throw std::runtime_error("multipart: invalid boundary line ending");

    while (pos < body.size())
    {
      if (opts.max_parts != 0 && out.parts.size() >= opts.max_parts)
        throw std::runtime_error("multipart: max_parts exceeded");

      const std::size_t headers_end = detail::find_headers_end(body, pos);
      if (headers_end == std::string_view::npos)
        throw std::runtime_error("multipart: headers terminator not found");

      const std::string_view headers_block = body.substr(pos, headers_end - pos);

      std::size_t content_start = headers_end;
      if (body.substr(headers_end, 4) == "\r\n\r\n")
        content_start += 4;
      else
        content_start += 2; // "\n\n"

      // Find next boundary. It should be preceded by a newline.
      // We search for "\n--boundary" from content_start.
      const std::string needle = std::string("\n") + delim;
      std::size_t next = body.find(needle, content_start);

      // Also allow boundary at exact start (no preceding \n) for rare inputs.
      if (next == std::string_view::npos)
      {
        const std::size_t direct = body.find(delim, content_start);
        if (direct != std::string_view::npos)
          next = direct;
      }

      if (next == std::string_view::npos)
      {
        // No more boundaries. For valid multipart, the final boundary should exist.
        throw std::runtime_error("multipart: next boundary not found");
      }

      std::size_t boundary_pos = next;
      if (boundary_pos > 0 && body[boundary_pos] == '\n')
      {
        // boundary starts after '\n'
        boundary_pos += 1;
      }

      // part body ends before the newline that starts the boundary
      std::size_t content_end = next;
      // If next points to "\n--boundary", content_end is at '\n'. Trim optional '\r' before it.
      if (content_end > content_start && body[content_end - 1] == '\r')
        content_end -= 1;

      // Build part
      part p;
      p.headers = detail::parse_headers_block(headers_block);

      // Extract Content-Disposition params
      const auto it = p.headers.find("content-disposition");
      if (it != p.headers.end())
      {
        const std::string &cd = it->second;
        p.name = detail::extract_param(cd, "name");
        p.filename = detail::extract_param(cd, "filename");
      }

      // Copy body bytes
      if (content_end < content_start)
        content_end = content_start;

      const std::size_t len = content_end - content_start;
      if (opts.max_part_bytes != 0 && len > opts.max_part_bytes)
        throw std::runtime_error("multipart: max_part_bytes exceeded");

      p.body.reserve(len);
      for (std::size_t k = content_start; k < content_end; ++k)
      {
        p.body.push_back(static_cast<std::uint8_t>(body[k]));
      }

      if (opts.trim_part_trailing_crlf)
      {
        detail::trim_trailing_crlf(p.body);
      }

      out.parts.push_back(std::move(p));

      // Move pos to after boundary line
      pos = boundary_pos + delim.size();

      // Final boundary?
      if (pos + 2 <= body.size() && body.substr(pos, 2) == "--")
      {
        pos += 2;

        // Optional trailing CRLF
        if (pos + 2 <= body.size() && body.substr(pos, 2) == "\r\n")
          pos += 2;
        else if (pos < body.size() && body[pos] == '\n')
          pos += 1;
        else if (!opts.allow_missing_final_crlf && pos != body.size())
          throw std::runtime_error("multipart: invalid final boundary termination");

        break;
      }

      // Consume boundary line ending
      if (pos + 2 <= body.size() && body.substr(pos, 2) == "\r\n")
        pos += 2;
      else if (pos < body.size() && body[pos] == '\n')
        pos += 1;
      else
        throw std::runtime_error("multipart: invalid boundary line ending");
    }

    return out;
  }

} // namespace multipart

#endif // MULTIPART_MULTIPART_HPP
