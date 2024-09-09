#include "common.hpp"

void write_out_map(string s, unordered_map<string, uint64_t>& um, ofstream* ofs) {
    ofs->seekp(0, std::ios::end);
    *ofs << s << endl;
    for (auto& u: um) {
        ofs->seekp(0, std::ios::end);
        *ofs << u.first << " - " << u.second << endl;
    }
    ofs->seekp(0, std::ios::end);
    *ofs << endl;
}

void write_out_vector(string s, vector<string>& vec, ofstream* ofs) {
    ofs->seekp(0, std::ios::end);
    *ofs << s << endl;
    for (auto& u: vec) {
        ofs->seekp(0, std::ios::end);
        *ofs << u << endl;
    }
    ofs->seekp(0, std::ios::end);
    *ofs << endl;
}