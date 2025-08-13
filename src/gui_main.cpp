#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <array>
#include <filesystem>
#include <windows.h>
#include "sha256.hpp"
#include "image.hpp"

// Forward declarations of encode and decode from main.cpp
int encode(Image &image, const std::array<std::uint8_t, 32> &password, const std::string &input, const std::string &output, Image::EncodingLevel level);
int decode(Image &image, const std::array<std::uint8_t, 32> &password, std::string output);

// Deklarasi fungsi yang menghasilkan hash kata sandi
static std::array<std::uint8_t, 32> generate_password_hash(const std::string &password_str);

// Deklarasi fungsi yang menampilkan dialog pemilihan file
static void show_file_dialog(std::string &path, const char *dialog_title);

// Fungsi utama untuk menjalankan antarmuka pengguna grafis (GUI)
int run_gui() {
    // Inisialisasi GLFW
    if (!glfwInit()) {
        std::cerr << "Gagal menginisialisasi GLFW" << std::endl;
        return -1;
    }

    // Atur konfig window untuk OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Buat jendela GLFW
    GLFWwindow* window = glfwCreateWindow(800, 600, "LSB Steganography Tool", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Gagal membuat jendela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Siapkan objek ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Siapkan gaya Dear ImGui (misalnya, tema gelap)
    ImGui::StyleColorsDark();

    // Siapkan backend Platform/Renderer
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Inisialisasi ImGui untuk GLFW
    ImGui_ImplOpenGL3_Init("#version 330 core"); // Inisialisasi ImGui untuk OpenGL3

    // Variabel untuk status UI
    std::string encode_input_image_path; // Jalur gambar input untuk encoding
    std::string encode_embed_file_path;  // Jalur file yang akan disematkan

    char encode_password[128] = {0}; // Buffer untuk kata sandi encoding
    std::string encode_status;       // Status operasi encoding

    std::string decode_input_image_path; // Jalur gambar input untuk decoding
    std::string decode_output_path;      // Jalur output untuk file yang didekode
    char decode_password[128] = {0};     // Buffer untuk kata sandi decoding
    std::string decode_status;           // Status operasi decoding

    bool show_encode_tab = true;  // Bendera untuk menampilkan tab Encode
    bool show_decode_tab = false; // Bendera untuk menampilkan tab Decode

    // Loop utama jendela
    while (!glfwWindowShouldClose(window)) {
        // Proses semua peristiwa GLFW yang tertunda
        glfwPollEvents();

        // Mulai frame ImGui baru
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Mulai jendela ImGui utama
        ImGui::Begin("LSB Steganography Tool");

        // Buat bilah tab untuk Encode dan Decode
        if (ImGui::BeginTabBar("MainTabs")) {
            // Tab Encode
            if (ImGui::BeginTabItem("Encode")) {
                // Tombol untuk memilih gambar input
                if (ImGui::Button("Pilih Gambar Input")) {
                    show_file_dialog(encode_input_image_path, "Pilih Gambar Input");
                }
                // Tampilkan jalur gambar input yang dipilih
                ImGui::TextWrapped("%s", encode_input_image_path.c_str());

                // Tombol untuk memilih file yang akan disematkan
                if (ImGui::Button("Pilih File untuk Disematkan")) {
                    show_file_dialog(encode_embed_file_path, "Pilih File untuk Disematkan");
                }
                // Tampilkan jalur file yang akan disematkan
                ImGui::TextWrapped("%s", encode_embed_file_path.c_str());

                // Input kata sandi (disembunyikan)
                ImGui::InputText("Kata Sandi", encode_password, sizeof(encode_password), ImGuiInputTextFlags_Password);

                // Tombol Encode
                if (ImGui::Button("Encode Gambar")) {
                    // Periksa apakah jalur input dan sematan tidak kosong
                    if (!encode_input_image_path.empty() && !encode_embed_file_path.empty()) {
                        std::filesystem::path input_path(encode_input_image_path);
                        // Buat jalur output untuk gambar yang disematkan
                        std::string output_path_str = input_path.parent_path().string() + "/" + input_path.stem().string() + "_embedded.png";

                        Image image;
                        // Muat gambar input
                        if (!image.load(encode_input_image_path)) {
                            encode_status = "Error: Gagal memuat gambar input.";
                        } else {
                            // Hasilkan hash kata sandi
                            auto password_hash = generate_password_hash(std::string(encode_password));
                            // Panggil fungsi encode
                            int result = encode(image, password_hash, encode_embed_file_path, output_path_str, Image::EncodingLevel::Low);
                            // Atur status berdasarkan hasil encoding
                            encode_status = (result >= 0) ? "Berhasil!" : "Error: Encoding gagal.";
                        }
                    } else {
                        encode_status = "Error: Harap pilih gambar input dan file untuk disematkan.";
                    }
                }

                // Tampilkan status encoding
                ImGui::TextWrapped("Status: %s", encode_status.c_str());

                ImGui::EndTabItem(); // Akhiri tab Encode
            }

            // Tab Decode
            if (ImGui::BeginTabItem("Decode")) {
                // Tombol untuk memilih gambar yang akan didekode
                if (ImGui::Button("Pilih Gambar untuk Dekode")) {
                    show_file_dialog(decode_input_image_path, "Pilih Gambar untuk Dekode");
                }
                // Tampilkan jalur gambar input yang dipilih
                ImGui::TextWrapped("%s", decode_input_image_path.c_str());

                // Input kata sandi (disembunyikan)
                ImGui::InputText("Kata Sandi", decode_password, sizeof(decode_password), ImGuiInputTextFlags_Password);

                // Tombol Decode
                if (ImGui::Button("Dekode Gambar")) {
                    // Periksa apakah jalur gambar input tidak kosong
                    if (!decode_input_image_path.empty()) {
                        std::filesystem::path input_path(decode_input_image_path);
                        // Buat jalur output default untuk file yang didekode
                        std::string output_path_str = input_path.parent_path().string() + "/" + input_path.stem().string() + "_decoded.zip";

                        Image image;
                        // Muat gambar input
                        if (!image.load(decode_input_image_path)) {
                            decode_status = "Error: Gagal memuat gambar.";
                        } else {
                            // Hasilkan hash kata sandi
                            auto password_hash = generate_password_hash(std::string(decode_password));
                            // Panggil fungsi decode
                            int result = decode(image, password_hash, output_path_str);
                            // Atur status berdasarkan hasil decoding
                            decode_status = (result >= 0) ? "Berhasil! Output disimpan ke " + output_path_str : "Error: Decoding gagal.";
                        }
                    } else {
                        decode_status = "Error: Harap pilih gambar untuk didekode.";
                    }
                }

                // Tampilkan status decoding
                ImGui::TextWrapped("Status: %s", decode_status.c_str());

                ImGui::EndTabItem(); // Akhiri tab Decode
            }

            ImGui::EndTabBar(); // Akhiri bilah tab
        }

        ImGui::End(); // Akhiri jendela ImGui utama

        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        // Dapatkan ukuran framebuffer jendela
        glfwGetFramebufferSize(window, &display_w, &display_h);
        // Atur viewport OpenGL
        glViewport(0, 0, display_w, display_h);
        // Atur warna latar belakang
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // Hapus buffer warna
        glClear(GL_COLOR_BUFFER_BIT);
        // Render data gambar ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Tukar buffer depan dan belakang
        glfwSwapBuffers(window);
    }

    // Pembersihan
    // Matikan ImGui OpenGL3 backend
    ImGui_ImplOpenGL3_Shutdown();
    // Matikan ImGui GLFW backend
    ImGui_ImplGlfw_Shutdown();
    // Hancurkan konteks ImGui
    ImGui::DestroyContext();
    // Hancurkan jendela GLFW
    glfwDestroyWindow(window);
    // Hentikan GLFW
    glfwTerminate();

    return 0;
}

// Fungsi untuk menghasilkan hash SHA256 dari string kata sandi
static std::array<std::uint8_t, 32> generate_password_hash(const std::string &password_str) {
    std::array<std::uint8_t, 32> hash{}; // Inisialisasi array hash
    // Gunakan SHA256 untuk menghash string kata sandi
    SHA256 sha; // Buat objek SHA256
    sha.update(password_str.data(), password_str.size()); // Perbarui hash dengan data kata sandi
    sha.finish(); // Selesaikan perhitungan hash
    sha.get_hash(hash.data()); // Dapatkan hash yang dihasilkan
    return hash; // Kembalikan hash
}

// Fungsi untuk menampilkan dialog pemilihan file (khusus Windows)
static void show_file_dialog(std::string &path, const char *dialog_title) {
    OPENFILENAMEA ofn; // Struktur untuk parameter dialog file
    char szFile[260] = { 0 }; // Buffer untuk nama file yang dipilih

    ZeroMemory(&ofn, sizeof(ofn)); // Inisialisasi struktur dengan nol
    ofn.lStructSize = sizeof(ofn); // Ukuran struktur
    ofn.hwndOwner = NULL; // Handle jendela pemilik (NULL untuk desktop)
    ofn.lpstrFile = szFile; // Buffer untuk nama file
    ofn.nMaxFile = sizeof(szFile); // Ukuran buffer
    ofn.lpstrFilter = "Semua File\0*.*\0"; // Filter file
    ofn.nFilterIndex = 1; // Indeks filter default
    ofn.lpstrFileTitle = NULL; // Tidak perlu judul file terpisah
    ofn.nMaxFileTitle = 0; // Ukuran judul file
    ofn.lpstrInitialDir = NULL; // Direktori awal
    ofn.lpstrTitle = dialog_title; // Judul dialog
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // Bendera: jalur dan file harus ada

    // Tampilkan dialog "Buka File"
    if (GetOpenFileNameA(&ofn)) {
        path = ofn.lpstrFile; // Jika file dipilih, simpan jalurnya
    }
}