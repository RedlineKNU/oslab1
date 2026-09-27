#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

static const char kErr[] = "An error has occurred\n";
static void die() { write(STDERR_FILENO, kErr, sizeof(kErr) - 1); }

static std::vector<std::string> g_path = {"/bin"};

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

static std::string resolve(const std::string& name) {
    for (const auto& dir : g_path) {
        std::string full = dir;
        if (!full.empty() && full.back() != '/') full += '/';
        full += name;
        if (access(full.c_str(), X_OK) == 0) return full;
    }
    return "";
}

static void spawn(const std::vector<std::string>& args) {
    std::string full = resolve(args[0]);
    if (full.empty()) { die(); return; }
    pid_t pid = fork();
    if (pid < 0) { die(); return; }
    if (pid == 0) {
        std::vector<char*> argv;
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execv(full.c_str(), argv.data());
        die();
        _exit(1);
    }
    waitpid(pid, nullptr, 0);
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
        if (toks[0] == "cd") {
            if (toks.size() != 2 || chdir(toks[1].c_str()) != 0) die();
            continue;
        }
        if (toks[0] == "path") {
            g_path.assign(toks.begin() + 1, toks.end());
            continue;
        }
        spawn(toks);
    }
    free(line);
    return 0;
}
