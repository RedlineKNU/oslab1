#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

static const char kErr[] = "An error has occurred\n";
static void die() { write(STDERR_FILENO, kErr, sizeof(kErr) - 1); }

static std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] { if (!cur.empty()) { out.push_back(cur); cur.clear(); } };
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n') flush();
        else cur.push_back(c);
    }
    flush();
    return out;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char* line = nullptr;
    size_t cap = 0;
    while (true) {
        fputs("wish> ", stdout);
        fflush(stdout);
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) break;
        auto toks = tokenize(std::string(line, static_cast<size_t>(n)));
        if (toks.empty()) continue;
        if (toks[0] == "exit") {
            if (toks.size() != 1) { die(); continue; }
            free(line);
            return 0;
        }
        die();
    }
    free(line);
    return 0;
}
