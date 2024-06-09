#include "utils_impl.h"
#include <filesystem>
#include <regex>

//-------------------------------------------------------------------------
// Utility function to return a file's last modification timestamp
bool get_file_modification_time(
    const char *filename,
    qtime64_t *mtime)
{
    qstatbuf stat_buf;
    if (qstat(filename, &stat_buf) != 0)
        return false;

    if (mtime != nullptr)
        *mtime = stat_buf.qst_mtime;
    return true;
}

bool get_file_modification_time(
    const qstring &filename,
    qtime64_t *mtime)
{
    return get_file_modification_time(filename.c_str(), mtime);
}

//-------------------------------------------------------------------------
void normalize_path_sep(qstring &path)
{
#ifdef __FAT__
    path.replace("/", SDIRCHAR);
#else
    path.replace("\\", SDIRCHAR);
#endif
}

//-------------------------------------------------------------------------
void make_abs_path(qstring& path, const char* base_dir, bool normalize)
{
    if (qisabspath(path.c_str()))
        return;

    auto old_cwd = std::filesystem::current_path();
    if (base_dir == nullptr)
    {
        path = old_cwd.string().c_str();
    }
    else
    {
        std::filesystem::current_path(base_dir);
        auto abs = std::filesystem::absolute(path.c_str());
        path = abs.string().c_str();
        std::filesystem::current_path(old_cwd);
    }
    if (normalize)
        normalize_path_sep(path);
}

//-------------------------------------------------------------------------
bool get_basename_and_ext(
    const char *path, 
    char **basename,
    char **ext,
    qstring &wrk_buf)
{
    wrk_buf = path;
    qsplitfile(wrk_buf.begin(), basename, ext);
    if ((*basename = qstrrchr(wrk_buf.begin(), DIRCHAR)) != nullptr)
        return ++(*basename), true;
    else
        return false;
}

//-------------------------------------------------------------------------
inline void get_current_directory(qstring &dir)
{
    dir = std::filesystem::current_path().string().c_str();
}

//-------------------------------------------------------------------------
// Utility function similar to Python's re.sub().
// Based on https://stackoverflow.com/a/37516316
namespace std
{
    template<class BidirIt, class Traits, class CharT, class UnaryFunction>
    std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
        const std::basic_regex<CharT, Traits> &re, UnaryFunction f)
    {
        std::basic_string<CharT> s;

        typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
        auto endOfLastMatch = first;

        auto callback = [&](const std::match_results<BidirIt> &match)
        {
            auto positionOfThisMatch = match.position(0);
            auto diff = positionOfThisMatch - positionOfLastMatch;

            auto startOfThisMatch = endOfLastMatch;
            std::advance(startOfThisMatch, diff);

            s.append(endOfLastMatch, startOfThisMatch);
            s.append(f(match));

            auto lengthOfMatch = match.length(0);

            positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

            endOfLastMatch = startOfThisMatch;
            std::advance(endOfLastMatch, lengthOfMatch);
        };

        std::regex_iterator<BidirIt> begin(first, last, re), end;
        std::for_each(begin, end, callback);

        s.append(endOfLastMatch, last);

        return s;
    }

    template<class Traits, class CharT, class UnaryFunction>
    std::string regex_replace(const std::string &s,
        const std::basic_regex<CharT, Traits> &re, UnaryFunction f)
    {
        return regex_replace(s.cbegin(), s.cend(), re, f);
    }
}
