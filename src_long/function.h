#pragma once
#include <complex>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using Complex = std::complex<double>;

namespace function {
    bool readFile_for_wave(const fs::path& input_path, std::vector<double>& waveForm, char* header);
    bool writeFile_for_wave(const fs::path& out_path, const std::vector<double>& new_wave, uint32_t sample_rate, uint16_t channels, uint16_t audioFormat, uint16_t bitsPerSample);
    int find_loop_length(const std::vector<double>& wave, uint32_t sample_rate, uint16_t channels);
    std::vector<int> find_loop_start(const std::vector<double>& wave, int loop_length, uint32_t sample_rate, uint16_t channels);
    std::vector<double> create_extended_bgm(const std::vector<double>& wave, const std::vector<int>& loop_begins, int loop_length, int goal_time, uint32_t sample_rate, uint16_t channels);
    void check_fade(std::vector<double>& new_wave, uint32_t sample_rate, uint16_t channels);
}