#include <iostream>
#include <cstdint>
#include <fstream>
#include "function.h"

const double PI = std::acos(-1.0);

namespace {

    void fft_ifft(std::vector<Complex>& a, bool inverse) {
        int n = a.size();

        for (int step = n; step >= 2; step /= 2) {
            int half = step / 2;

            double angle = 2.0 * PI / step * (inverse ? 1.0 : -1.0);
            Complex wn(std::cos(angle), std::sin(angle));

            for (int i = 0; i < n; i += step) {
                Complex w(1.0, 0.0);
                for (int k = 0; k < half; ++k) {
                    Complex u = a[i + k];
                    Complex v = a[i + k + half];
                    a[i + k] = u + v;
                    a[i + k + half] = (u - v) * w;
                    w *= wn;
                }
            }
        }

        int j = 0;
        for (int i = 1; i < n; ++i) {
            int bit = n >> 1;
            while (j >= bit) {
                j -= bit;
                bit >>= 1;
            }
            j += bit;

            if (i < j) {
                std::swap(a[i], a[j]);
            }
        }
    }

    void fft(std::vector<Complex>& a) {
        fft_ifft(a, false);
    }

    void ifft(std::vector<Complex>& a) {
        fft_ifft(a, true);
        int n = a.size();
        for (int i = 0; i < n; ++i) {
            a[i] /= n;  // 逆変換の最後はデータ数 N で割ってスケールを戻す
        }
    }

    int next_power_of_2(int n) {
        int p = 1;
        while(p < n) {
            p *= 2;
        }
        return p;
    }
}

//ファイル読み取り関数
bool function::readFile_for_wave(const fs::path& input_path, std::vector<double>& waveForm, char* header) {
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.read(header, 44);

    uint16_t audioFormat{*reinterpret_cast<uint16_t*>(&header[20])};
    uint16_t bitsPerSample{*reinterpret_cast<uint16_t*>(&header[34])};

    if (audioFormat == 1) {
        switch (bitsPerSample) {
            case 16: {
                int16_t sample{};
                while (file.read(reinterpret_cast<char*>(&sample), sizeof(int16_t))) {
                    waveForm.push_back(sample / 32768.0);
                }
                break;
            }
            case 24: {
                char buf[3];
                while (file.read(buf, 3)) {
                    int32_t sample = (buf[2] << 24) | (buf[1] << 16) | buf[0] << 8;
                    sample = sample >> 8;
                    waveForm.push_back(sample / 8388608.0);
                }
                break;
            }
            default:
                std::cerr << "エラー: 未対応の量子化ビット数です (" << bitsPerSample << "bit)\n";
                return false;
        }
    } 
    else if (audioFormat == 3) {
        if (bitsPerSample == 32) {
            float sample{};
            while (file.read(reinterpret_cast<char*>(&sample), sizeof(float))) {
                waveForm.push_back(static_cast<double>(sample));
            }
        }
    }
    else {
        std::cerr << "エラー: 未対応のオーディオフォーマットです\n";
        return false;
    }
    
    return true;
}

bool function::writeFile_for_wave(const fs::path& out_path, const std::vector<double>& new_wave, uint32_t sample_rate, uint16_t channels, uint16_t audioFormat, uint16_t bitsPerSample) {
    std::ofstream file(out_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    //16bit出力用ヘッダー
    char header[44] = {0};
    int bytesPerSample = bitsPerSample / 8;
    int32_t data_size = new_wave.size() * sizeof(int16_t);
    int32_t file_size = 36 + data_size;

    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    *reinterpret_cast<int32_t*>(&header[4]) = file_size;
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    *reinterpret_cast<int32_t*>(&header[16]) = 16;
    *reinterpret_cast<int16_t*>(&header[20]) = audioFormat;
    *reinterpret_cast<int16_t*>(&header[22]) = channels;
    *reinterpret_cast<int32_t*>(&header[24]) = sample_rate;
    *reinterpret_cast<int32_t*>(&header[28]) = sample_rate * channels * bytesPerSample;
    *reinterpret_cast<int16_t*>(&header[32]) = channels * bytesPerSample;
    *reinterpret_cast<int16_t*>(&header[34]) = bitsPerSample;
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    *reinterpret_cast<int32_t*>(&header[40]) = data_size;
    file.write(header, 44);

    if (audioFormat == 1) {
        switch (bitsPerSample) {
            case 16: {
                for (double val : new_wave) {
                    if (val > 1.0) val = 1.0;
                    if (val < -1.0) val = -1.0;
                    
                    int16_t sample = static_cast<int16_t>(val * 32767.0);
                    file.write(reinterpret_cast<const char*>(&sample), sizeof(int16_t));
                }
                break;
            }
            case 24: {
                for (double val : new_wave) {
                    if (val > 1.0) val = 1.0;
                    if (val < -1.0) val = -1.0;
                    
                    int32_t sample = static_cast<int32_t>(val * 8388607.0);
                    
                    // リトルエンディアンの仕様
                    char buf[3];
                    buf[0] = sample & 0xFF;         
                    buf[1] = (sample >> 8) & 0xFF;  
                    buf[2] = (sample >> 16) & 0xFF; 
                    file.write(buf, 3);
                }
                break;
            }
            default:
                std::cerr << "エラー: 未対応の量子化ビット数です (" << bitsPerSample << "bit)\n";
                return false;
        }
    } 
    else if (audioFormat == 3) {
        if (bitsPerSample == 32) {
            for (double val : new_wave) {
                // double型から元の32bit float型にキャストして書き込む
                float sample = static_cast<float>(val);
                file.write(reinterpret_cast<const char*>(&sample), sizeof(float));
            }
        } else {
            std::cerr << "エラー: float形式は32bitのみ対応しています\n";
            return false;
        }
    }
    else {
        std::cerr << "エラー: 未対応のオーディオフォーマットです\n";
        return false;
    }

    return true;
}

int function::find_loop_length(const std::vector<double>& wave, uint32_t sample_rate, uint16_t channels) {
    int N_total = wave.size();
    int template_len = sample_rate * channels * 5;
    int fft_size = next_power_of_2(N_total + template_len - 1); //高速フーリエ変換用に2のべき乗に調整，テンプレート分も加味する

    std::vector<Complex> wave_c(fft_size, 0.0);
    for (int i = 0; i < N_total; ++i) {
        wave_c[i] = wave[i];
    }
    std::cout << "メイン波形の順変換を実行中... " << std::flush;
    fft(wave_c);
    std::cout << " ⇒ 完了" << std::endl;

    std::vector<double> test_points = {0.2, 0.4, 0.6};  //ループ部分の位置に依存しないように，3か所から調べる
    double absolute_max_score = -1.0;
    int best_peak1 = -1;
    int best_peak2 = -1;
    int best_loop_length = 0;
    int num_tests = test_points.size();

    for (int t = 0; t < num_tests; ++t) {
        double tp = test_points[t];
        std::cout << "\rパターンマッチング進行度: [" 
                  << (t + 1) << "/" << num_tests << "] ポイントを解析中..." << std::flush;
        int start_idx = N_total * tp;
        start_idx = (start_idx / channels) * channels;  //channelの倍数に丸める

        std::vector<Complex> temp_c(fft_size, 0.0);
        for (int i = 0; i < template_len; ++i) {
            temp_c[i] = wave[start_idx + i];
        }
        fft(temp_c);

        std::vector<Complex> cross_corr(fft_size);
        for (int i = 0; i < fft_size; ++i) {
            cross_corr[i] = wave_c[i] * std::conj(temp_c[i]);   //相互相関
        }
        ifft(cross_corr);

        int peak1 = start_idx;  //理論上
        int peak2 = -1;
        double current_max_score = -1.0;

        for (int i = 0; i < N_total - template_len; ++i) {
            if (i >= peak1 - template_len && i <= peak1 + template_len) {
                continue;
            }

            double raw_score = cross_corr[i].real();
            if (raw_score > current_max_score) {
                current_max_score = raw_score;
                peak2 = i;
            }
        }

        /*std::cout << "Test Point (" << tp * 100 << "%): Peak 1 = " << peak1 
                    << ", Peak 2 = " << peak2 << ", Score = " << current_max_score << std::endl;*/

        if (current_max_score > absolute_max_score) {
            absolute_max_score = current_max_score;
            best_peak1 = peak1;
            best_peak2 = peak2;
            best_loop_length = std::abs(peak2 - peak1);
        }
    }
    std::cout << "\n => 解析完了\n";

    if (best_loop_length % channels != 0) {
        best_loop_length = (best_loop_length / channels) * channels;
    }

    /*std::cout << "\n=== 最終結果 ===" << std::endl;
    std::cout << "採用された Peak 1: " << best_peak1 << ", Peak 2: " << best_peak2 << std::endl;
    std::cout << "修正後ループ長 (L/R整合済み): " << best_loop_length << std::endl;*/
            
    return best_loop_length;
}

std::vector<int> function::find_loop_start(const std::vector<double>& wave, int loop_length, uint32_t sample_rate, uint16_t channels) {
    std::vector<int> start_points{};
    int match_required = (sample_rate * channels) * 5;
    match_required = (match_required / channels) * channels;

    double current_diff_sum = 0.0;
    double global_min_diff = 1e9;
    double temp_diff_sum = 0.0;

    //下準備
    for (int i = 0; i < match_required; ++i) {
        temp_diff_sum += std::abs(wave[i] - wave[i + loop_length]);
    }

    for (int i = 0; i + loop_length + match_required < wave.size(); i += channels) {
        if (temp_diff_sum < global_min_diff) {
            global_min_diff = temp_diff_sum;
        }

        for (int c = 0; c < channels; ++c) {
            temp_diff_sum -= std::abs(wave[i + c] - wave[i + c + loop_length]);
            temp_diff_sum += std::abs(wave[i + c + match_required] - wave[i + c + match_required + loop_length]);
        }
    }
    std::cout << "ファイル内の最小誤差" << global_min_diff << std::endl;

    double threshold = std::max(0.0, global_min_diff) + 0.1;
        
    for (int i = 0; i < match_required; ++i) {
        current_diff_sum += std::abs(wave[i] - wave[i + loop_length]);
    }

    for (int i = 0; i + loop_length + match_required < wave.size(); i += channels) {
        if (current_diff_sum <= threshold) { 
            start_points.push_back(i);

            int skip_amount = (loop_length / channels) * channels;

            int next_i = i + skip_amount;

            if (next_i + loop_length + match_required >= wave.size()) {
                break;
            }

            current_diff_sum = 0.0;
            for (int j = 0; j < match_required; ++j) {
                current_diff_sum += std::abs(wave[next_i + j] - wave[next_i + j + loop_length]);
            }
            i = next_i - channels;  //帳尻合わせ

            continue;
        }

        for (int c = 0; c < channels; ++c) {
            current_diff_sum -= std::abs(wave[i + c] - wave[i + c + loop_length]);
            current_diff_sum += std::abs(wave[i + c + match_required] - wave[i + c + match_required +loop_length]);
        }
    }

    return start_points;
}

std::vector<double> function::create_extended_bgm(const std::vector<double>& wave, const std::vector<int>& loop_begins, int loop_length, int goal_time, uint32_t sample_rate, uint16_t channels) {
    std::vector<double> new_wave;
    if (loop_begins.empty()) {
        return new_wave;
    }


    int intro_length = loop_begins.front();

    int last_loop_start = loop_begins.back();

    int outro_start = last_loop_start + loop_length;
    int outro_length = 0;

    if (outro_start < wave.size()) {
        outro_length = wave.size() - outro_start;
    }

    if (goal_time == 0) {
        if (intro_length > 0) {
            new_wave.insert(new_wave.end(), wave.begin(), wave.begin() + intro_length);
        } else {
            std::cout << "このデータからイントロが検出されませんでした．ファイルは出力されません．\n";
        }
        return new_wave;
    }

    if (goal_time == 8888) {
        new_wave.insert(new_wave.end(), wave.begin() + last_loop_start, wave.begin() + last_loop_start + loop_length);
        return new_wave;
    }

    /*if (goal_time == 9999) {
        if (outro_length > 0) {
            new_wave.insert(new_wave.end(), wave.begin() + outro_start, wave.end());
        } else {
            std::cout << "このデータからアウトロが検出されませんでした．ファイルは出力されません．\n";
        }
        return new_wave;
    }*/

    int fixed_samples = intro_length + outro_length;
    double target_samples = static_cast<double>(goal_time) * 60.0 * sample_rate * channels;

    int target_loops = 1;

    if (target_samples > fixed_samples) {
        double needed_loop_samples = target_samples - fixed_samples;    //ループ部分だけで稼がないといけないサンプル数
        target_loops = std::ceil(needed_loop_samples / loop_length);
    }

    new_wave.insert(new_wave.end(), wave.begin(), wave.begin() + intro_length);

    for (int i = 0; i < target_loops; ++i) {
        new_wave.insert(new_wave.end(), wave.begin() + last_loop_start, wave.begin() + last_loop_start + loop_length);
    }

    if (outro_length > 0) {
        new_wave.insert(new_wave.end(), wave.begin() + outro_start, wave.end());
    }

    return new_wave;
}



void function::check_fade(std::vector<double>& new_wave, uint32_t sample_rate, uint16_t channels) {
    int fade_sec = 10;
    int fade_frames = fade_sec * sample_rate;
    int fade_samples = fade_frames * channels;

    if (fade_samples <= new_wave.size()) {
        int fade_start_idx = new_wave.size() - fade_samples;

        int check_window = sample_rate * channels * 1;
        double start_vol_sum = 0.0;
        double end_vol_sum = 0.0;

        for (int i = 0; i < check_window; ++i) {
            start_vol_sum += std::abs(new_wave[fade_start_idx + i]);
            end_vol_sum += std::abs(new_wave[new_wave.size() - check_window + i]);
        }

        double start_avg = start_vol_sum / check_window;
        double end_avg = end_vol_sum / check_window;

        if (start_avg > 0.0001 && end_avg < start_avg * 0.25) { //fade検知
            return;
        } else {
            int fade = 0;
            while(true) {
                std::cout << "末尾のフェードアウト処理を実装しますか(10秒) Yes...1, No...0 : ";
                std::cin >> fade;
                if (fade != 0 && fade != 1) {
                    std::cout << "入力に誤りがあります．\n";
                    continue;
                } else {
                    break;
                }
            }

            if (fade == 1) {
                for (int i = 0; i < fade_samples; ++i) {
                    int current_frame = i / channels;
                    double multiplier = 1.0 - (static_cast<double>(current_frame) / fade_frames);
                    new_wave[fade_start_idx + i] *= multiplier;
                }
            }
        }
    }
}
