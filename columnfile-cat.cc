#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/sendfile.h>
#include <sysexits.h>
#include <unistd.h>

#include <columnfile.h>
#include <kj/debug.h>

namespace {

int print_version = 0;
int print_help = 0;
std::string format;
std::string output_format;
std::vector<std::pair<uint32_t, std::string>> filters;

enum Option {
  kOptionFilter = 'F',
  kOptionFormat = 'f',
  kOptionOutputFormat = 'o'
};

struct option kLongOptions[] = {
    {"filter", required_argument, nullptr, kOptionFilter},
    {"format", required_argument, nullptr, kOptionFormat},
    {"output-format", required_argument, nullptr, kOptionOutputFormat},
    {"version", no_argument, &print_version, 1},
    {"help", no_argument, &print_help, 1},
    {nullptr, 0, nullptr, 0}};

}  // namespace

int main(int argc, char** argv) {
  int i;
  while ((i = getopt_long(argc, argv, "f:", kLongOptions, 0)) != -1) {
    if (!i) continue;
    if (i == '?')
      errx(EX_USAGE, "Try '%s --help' for more information.", argv[0]);

    switch (static_cast<Option>(i)) {
      case kOptionFilter: {
        auto delimiter = strchr(optarg, ':');
        KJ_REQUIRE(delimiter != nullptr, optarg);
        *delimiter = 0;
        filters.emplace_back(std::stoull(optarg), delimiter + 1);
      } break;

      case kOptionFormat:
        format = optarg;
        break;

      case kOptionOutputFormat:
        output_format = optarg;
        break;
    }
  }

  if (print_help) {
    printf(
        "Usage: %s [OPTION]... [FILE]...\n"
        "\n"
        "      --format=FORMAT        column formats\n"
        "      --filter=COL:PATTERN   only show rows whose COLUMN matches "
        "PATTERN\n"
        "      --help     display this help and exit\n"
        "      --version  display version information and exit\n"
        "\n"
        "With no FILE, or when FILE is -, read standard input.\n"
        "\n"
        "Report bugs to <morten.hustveit@gmail.com>\n",
        argv[0]);

    return EXIT_SUCCESS;
  }

  if (print_version) {
    puts(PACKAGE_STRING);

    return EXIT_SUCCESS;
  }

  std::vector<kj::AutoCloseFd> inputs;

  if (optind == argc) {
    inputs.emplace_back(STDIN_FILENO);
  } else {
    for (auto i = optind; i < argc; ++i) {
      if (!strcmp(argv[i], "-"))
        inputs.emplace_back(STDIN_FILENO);
      else
        inputs.emplace_back(kj::AutoCloseFd{open(argv[i], O_RDONLY)});
    }
  }

  std::unordered_set<uint32_t> selected_fields;

  if (!format.empty()) {
    for (uint32_t i = 0; i < format.size(); ++i) {
      if (format[i] != '_') selected_fields.emplace(i);
    }
  }

  for (const auto& filter : filters) selected_fields.emplace(filter.first);

  std::sort(filters.begin(), filters.end());

  if (output_format.empty() || output_format == "text") {
    for (auto& input : inputs) {
      cantera::ColumnFileReader reader(std::move(input));

      if (!selected_fields.empty())
        reader.SetColumnFilter(selected_fields.begin(), selected_fields.end());

      while (!reader.End()) {
        const auto& row = reader.GetRow();
        size_t column = 0;

        if (!filters.empty()) {
          auto filter = filters.begin();
          auto field = row.begin();

          while (filter != filters.end() && field != row.end()) {
            if (field->first < filter->first) {
              ++field;
              continue;
            }

            if (filter->first < field->first) break;

            if (std::string::npos == field->second.value().find(filter->second))
              break;

            ++field;
            ++filter;
          }

          if (filter != filters.end()) continue;
        }

        bool need_tab = false;

        for (const auto& field : row) {
          auto fmt = 's';

          if (!format.empty()) {
            if (field.first >= format.size()) break;

            fmt = format[field.first];
          }

          while (column < field.first) {
            if (format[column++] != '_') std::cout << '\t';
          }

          if (fmt == '_') continue;

          if (need_tab) std::cout << '\t';

          switch (fmt) {
#define FMT(ch, type) \
            case ch: { \
              KJ_REQUIRE(field.second.value().size() >= sizeof(type), \
                         field.second.value().size()); \
              type f; \
              memcpy(&f, field.second.value().data(), sizeof(f)); \
              std::cout << f; \
              need_tab = true; \
            } break;

            // Based on Python's "struct" module format characters.
            FMT('H', uint16_t)
            FMT('I', uint32_t)
            FMT('Q', uint64_t)
            FMT('d', double)
            FMT('f', float)
            FMT('h', int16_t)
            FMT('i', int32_t)
            FMT('q', int64_t)

            case 's':
              std::cout << field.second.value();
              need_tab = true;
              break;
          }

          ++column;
        }

        std::cout << '\n';
      }
    }
  } else if (output_format == "columnfile") {
    cantera::ColumnFileWriter output(kj::AutoCloseFd(STDOUT_FILENO));
    size_t next_flush = 10000;

    for (auto& input : inputs) {
      cantera::ColumnFileReader reader(std::move(input));

      while (!reader.End()) {
        output.PutRow(reader.GetRow());

        if (!--next_flush) {
          output.Flush();
          next_flush = 10000;
        }
      }
    }
  } else {
    KJ_FAIL_REQUIRE("Unknown output format", output_format);
  }
}
