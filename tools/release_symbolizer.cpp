#include <crashtrace/crashtrace.hpp>

#include <cstdio>
#include <cstring>

namespace {

void print_usage(const char* program_name) {
    std::fprintf(stderr,
                 "Usage: %s --symbols <debug-symbol-file> [--frames <crash-log>]\n",
                 program_name);
}

}  // namespace

int main(int argc, char** argv) {
    const char* symbols_path = nullptr;
    const char* frames_path = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--symbols") == 0 && i + 1 < argc) {
            symbols_path = argv[++i];
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames_path = argv[++i];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    if (symbols_path == nullptr) {
        print_usage(argv[0]);
        return 2;
    }

    std::FILE* input = stdin;
    if (frames_path != nullptr) {
        input = std::fopen(frames_path, "r");
        if (input == nullptr) {
            std::fprintf(stderr, "failed to open crash log: %s\n", frames_path);
            return 1;
        }
    }

    const int result = crashtrace::symbolize_release_stack(stdout, symbols_path, input);
    if (input != stdin) {
        std::fclose(input);
    }

    if (result < 0) {
        std::fprintf(stderr, "failed to symbolize release stack\n");
        return 1;
    }

    return 0;
}
