#include <string>
#include <iostream>
#include <assert.h>
#if __has_include(<libgit2/git2.h>)
#include <libgit2/git2.h>
#else
#include "git2.h"
#endif
#include <cstring>

using namespace std;

// A helper function to throw away prefix logic if needed
static std::string stripPrefix(const std::string &full, const std::string &prefix) {
    // If 'full' starts with 'prefix', remove it from the front
    if (full.rfind(prefix, 0) == 0) { // rfind(prefix, 0) == 0 => prefix is at start
        return full.substr(prefix.size());
    }
    return full;
}

string get_version() {
    // Initialize libgit2
    git_libgit2_init();

    // Attempt to open the current repository
    git_repository *repo = NULL;
    if (git_repository_open_ext(&repo, ".", 0, NULL) < 0 || !repo) {
        std::cerr << "Failed to open a git repository.\n";
        return 1;
    }

    // 1) Get current branch name
    string branch = "UNKNOWN";
    {
        git_reference *headRef = NULL;
        int error = 0;
        error = git_repository_head(&headRef, repo);
        if (error == 0 && headRef) {
            // If HEAD is a branch, get the branch name
            // branch = git_reference_shorthand(headRef);
            if (!error) {
                branch = git_reference_shorthand(headRef);
            } else
                branch = "";
            git_reference_free(headRef);
        }
    }

    // 2) Get a "long" description (like "v1.2.3-5-gabcdef")
    std::string describeLong = "UNKNOWN";
    {
        git_describe_options desc_opts;
        // git_describe_options desc_opts = GIT_DESCRIBE_OPTIONS_INIT;
        git_describe_options_init(&desc_opts, GIT_DESCRIBE_OPTIONS_VERSION);
        desc_opts.describe_strategy = GIT_DESCRIBE_TAGS;
        desc_opts.max_candidates_tags = 10;
        git_describe_result *descResult = NULL;
        // "describe" the HEAD commit
        if (git_describe_workdir(&descResult, repo, &desc_opts) == 0 && descResult) {
            // Format the describe result into a git_buf
            // git_describe_format_options fmt_opts = GIT_DESCRIBE_FORMAT_OPTIONS_INIT;
            git_describe_format_options fmt_opts;
            git_describe_format_options_init(&fmt_opts, GIT_DESCRIBE_FORMAT_OPTIONS_VERSION);
            fmt_opts.always_use_long_format = 1;
            git_buf descBuf = GIT_BUF_INIT_CONST(nullptr, 0);

            if (git_describe_format(&descBuf, descResult, &fmt_opts) == 0) {
                describeLong = (descBuf.ptr ? descBuf.ptr : "");
            }

            git_buf_dispose(&descBuf);
            git_describe_result_free(descResult);
        }
    }

    // 3) Get only the nearest tag (similar to "git describe --tags --abbrev=0")
    std::string tagShort = "UNKNOWN";
    {
        git_describe_options desc_opts;
        // git_describe_options desc_opts = GIT_DESCRIBE_OPTIONS_INIT;
        git_describe_options_init(&desc_opts, GIT_DESCRIBE_OPTIONS_VERSION);
        desc_opts.describe_strategy = GIT_DESCRIBE_TAGS;
        desc_opts.max_candidates_tags = 10;
        git_describe_result *descResult = NULL;
        // "describe" the HEAD commit
        if (git_describe_workdir(&descResult, repo, &desc_opts) == 0 && descResult) {
            // Format the describe result into a git_buf
            // git_describe_format_options fmt_opts = GIT_DESCRIBE_FORMAT_OPTIONS_INIT;
            git_describe_format_options fmt_opts;
            git_describe_format_options_init(&fmt_opts, GIT_DESCRIBE_FORMAT_OPTIONS_VERSION);
            fmt_opts.abbreviated_size = 0;
            git_buf descBuf = GIT_BUF_INIT_CONST(nullptr, 0);

            if (git_describe_format(&descBuf, descResult, &fmt_opts) == 0) {
                tagShort = (descBuf.ptr ? descBuf.ptr : "");
            }

            git_buf_dispose(&descBuf);
            git_describe_result_free(descResult);
        }
    }

    // 4) Strip the short tag from the front of describeLong
    //    e.g., "v1.2.3-5-gabcdef" minus "v1.2.3-" => "5-gabcdef"
    std::string revision = "UNKNOWN";
    if (describeLong != "UNKNOWN" && tagShort != "UNKNOWN") {
        revision = stripPrefix(describeLong, tagShort + "-");
    }

    // 5) Combine: "<TAG>-<BRANCH>-rev<REV>"
    std::string versionString = tagShort + "-" + branch + "-rev" + revision;

    // Print the line your Bash script used to produce:
    // std::cout << "const char* g_version = \"" << versionString << "\";\n";

    // Cleanup
    git_repository_free(repo);
    git_libgit2_shutdown();

    return versionString;
}

const char *g_version = "v35-HEAD-rev13-gea329df";
