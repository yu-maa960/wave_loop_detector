#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include "function.h"

namespace fs = std::filesystem;

int main() {
    SetConsoleOutputCP(CP_UTF8);

    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc < 2) {
        std::cout << "【使い方】このexeファイルにWAVファイルをドラッグ＆ドロップしてください。\n";
        system("pause");
        
        if (argvW) {
            LocalFree(argvW);
        }

        return 1;
    }

    fs::path input_path(argvW[1]);
    fs::path exe_dir = fs::path(argvW[0]).parent_path();
    LocalFree(argvW);

    
    std::string base_name = input_path.stem().string();
    std::vector<double> waveForm{};
    char header[44]{};

    if (!function::readFile_for_wave(input_path, waveForm, header)) {
        std::cerr << "エラー: ファイルが開けません．名前や場所を確認してください．" << std::endl;
        return 1;
    } else {
        std::cout << "読み込み完了" << std::endl;
    }


    uint16_t channels = *reinterpret_cast<uint16_t*>(&header[22]);
    uint32_t sample_rate = *reinterpret_cast<uint32_t*>(&header[24]);
    int loop_length = function::find_loop_length(waveForm, sample_rate, channels);

    if (loop_length == 0) {
        std::cout << "ループ部分が見つかりませんでした．終了します．\n";
        system("pause");
        return 1;
    }

    std::vector<int> loop_begins(function::find_loop_start(waveForm, loop_length, sample_rate, channels));

    if (loop_begins.empty()) {
        std::cout << "ループ開始点が見つかりませんでした．終了します．\n";
        system("pause");
        return 1;
    }

    
    /*std::cout << "ループの長さ:" << loop_length << '\n';
    std::cout << "ループ開始点: ";

    for (size_t i = 0; i < loop_begins.size(); ++i) {
        std::cout << loop_begins[i];
            
        if (i != loop_begins.size() - 1) {
            std::cout << ", ";
        }
    }*/

    while (true) {
        int goal_time = 1;    //全体として何分まで拡張したいか．
        std::cout << '\n';
        std::cout << "拡張したい時間(単位は分)を入力してください\n";
        std::cout << " -1 : 素材抽出モード\n";
        std::cout << " -2 : プログラムを終了する\n";
        std::cout << '\n' << std::endl;

        while (true) {
            std::cout << "入力: ";
            std::cin >> goal_time;

            if (std::cin.fail() || goal_time < -2) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "エラー: 無効な入力です．もう一度入力してください" << std::endl;
                continue;
            }

            break; 
        }
        if (goal_time == -2) {
            std::cout << "プログラムを終了します\n";
            break;
        }

        if (goal_time == -1) {
            int option{};
            std::cout << '\n';
            std::cout << "抽出したい素材の番号を選択してください\n";
            std::cout << " 1 : イントロ部分のみ抽出\n";
            std::cout << " 2 : ループ部分(無限ループ用)のみ抽出\n";
            std::cout << " 3 : アウトロ部分のみ抽出\n";
            std::cout << " 9 : 前のメニューに戻る\n";
            std::cout << std::endl;

            while (true) {
                std::cout << "番号入力: ";
                std::cin >> option;

                if (option == 1) {
                    goal_time = 0;
                } else if (option == 2) {
                    goal_time = 8888;
                } else if (option == 3) {
                    goal_time = 9999;
                } else if (option == 9) {
                    goal_time = -9;
                } else {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "エラー: 無効な入力です．もう一度入力してください\n\n";
                    continue;
                }

                break; 
            }
            if (goal_time == -9) {
            continue; 
            }
        }


        fs::path out_path{};
        switch(goal_time) {
            case 0: {
                out_path = exe_dir / "music_output" / (base_name + "_intro.wav");
                break;
            }
            case 8888: {
                out_path = exe_dir / "music_output" / (base_name + "_loop.wav");
                break;
            }
            case 9999: {
                out_path = exe_dir / "music_output" / (base_name + "_outro.wav");
                break;
            }
            default: {
                out_path = exe_dir / "music_output" / (base_name + "_extended.wav");
                break;
            } 
        }


        std::vector<double> new_waveForm(function::create_extended_bgm(waveForm, loop_begins, loop_length, goal_time, sample_rate, channels));
        
        if (new_waveForm.empty()) {
            std::cout << "エラー: ループ部分が存在しません\n";
            break; 
        }

        if (goal_time != 0 && goal_time != 8888 && goal_time != 9999) {
            function::check_fade(new_waveForm, sample_rate, channels);
        }
        uint16_t audioFormat{*reinterpret_cast<uint16_t*>(&header[20])};
        uint16_t bitsPerSample{*reinterpret_cast<uint16_t*>(&header[34])};

        if (!function::writeFile_for_wave(out_path, new_waveForm, sample_rate, channels, audioFormat, bitsPerSample)) {
            std::cerr << "エラー: 出力ファイルが作成できません" << std::endl;
        } else {
            std::cout << "\n出力完了: " << out_path.string() << "\n";
        }
    }

    system("pause");
    return 0;
}