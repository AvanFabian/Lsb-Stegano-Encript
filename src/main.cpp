#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <array>
#include "argparse/argparse.hpp"
#include "aes.hpp"
#include "sha256.hpp"
#include "crc32.hpp"
#include "random.hpp"
#include "image.hpp"
#include "utils.hpp"
#include "gui_main.cpp"

// Definisikan versi format file
#define VERSION 1
// Definisikan jumlah round untuk PBKDF2
#define KEY_ROUNDS 20000
// Definisikan tingkat encoding default
#define LEVEL Image::EncodingLevel::Low

// Buat alias untuk namespace std::filesystem menjadi fs
namespace fs = std::filesystem;

// Struktur Header 64 byte untuk menyimpan metadata file yang disematkan
struct Header {
    std::uint8_t  sig[4];    // Tanda tangan file (HIDE)
    std::uint16_t version;   // Versi format
    std::uint8_t  level;     // Tingkat encoding
    std::uint8_t  flags;     // Bendera (misalnya, untuk opsi tambahan)
    std::uint32_t offset;    // Offset ke data yang disematkan dalam gambar
    std::uint32_t size;      // Ukuran data yang disematkan
    std::uint32_t hash;      // Hash CRC32 dari data asli
    std::uint8_t  name[32];  // Nama file asli, ruang yang tidak digunakan diisi dengan nol
    std::uint8_t  reserved[12]; // Harus diisi dengan nol untuk kompatibilitas di masa mendatang
};
// Pastikan ukuran Header adalah 64 byte
static_assert(sizeof(Header) == 64);

// Array string untuk mengonversi tingkat encoding menjadi representasi string
const char *level_to_str[3] = {
    "Low (Default)", // Representasi string untuk tingkat encoding rendah
    "Medium",        // Representasi string untuk tingkat encoding menengah
    "High"           // Representasi string untuk tingkat encoding tinggi
};

/*
 * * Encode

 * 1. Inisialisasi:
 * - Membaca seluruh file yang akan disisipkan ke dalam memori dan menghitung ukurannya setelah ditambah 'padding' agar sesuai dengan blok AES.
 * - Memastikan ukuran data yang sudah di-'padding' tidak melebihi kapasitas gambar.

 * * 2. Membuat Kunci Enkripsi:
 * - Menghasilkan Salt dan Initialization Vector (IV) acak untuk enkripsi.
 * - Membuat kunci enkripsi 256-bit yang kuat dari password pengguna menggunakan PBKDF2-HMAC-SHA-256.

 * * 3. Membuat Payload:
 * - Menghitung 'checksum' CRC32 dari data asli untuk verifikasi integritas.
 * - Membentuk sebuah 'struct Header' yang berisi metadata seperti tanda tangan, versi, level encoding, offset data, ukuran, checksum, dan nama file.

 * * 4. Enkripsi:
 * - Mengenkripsi Header dan data yang sudah di-'padding' secara terpisah menggunakan AES-256-CBC dengan kunci dan IV yang sudah dibuat.

 * * 5. Penyisipan message yg ingin di-embed kedalam file:
 * - Menyisipkan Salt, IV, Header terenkripsi, dan data terenkripsi ke dalam piksel gambar menggunakan encoding LSB.
 * - Menggunakan offset acak untuk menyisipkan blok data utama guna meningkatkan keamanan.
 * - Menyimpan gambar yang telah dimodifikasi ke lokasi output yang ditentukan.
 */
int encode(Image &image, const std::array<std::uint8_t, 32> &password, const std::string &input, const std::string &output, Image::EncodingLevel level) {
    // Buka file data
    std::ifstream file(input, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERROR: Unable to open file '" << input << "'" << std::endl;
        return -1;
    }

    std::cout << "* Ukuran gambar: " << image.w() << "x" << image.h() << " piksel" << std::endl;
    std::cout << "* Tingkat encoding: " << level_to_str[static_cast<int>(level)] << std::endl;

    // Beri padding untuk memastikan ukuran image awal di kelipatan 16 byte 
    // Temukan ukuran data dan ukuran data dengan padding
    std::size_t size = file.tellg();
    std::size_t padded_size = size + 1; // Setidaknya satu byte padding
    
    if (padded_size % 16)
        padded_size = (size / 16 + 1) * 16;

    // Temukan ukuran maksimum yang mungkin untuk file
    unsigned int max_size = image.w()*image.h()*4/Image::encoded_size(1, level) - Image::encoded_size(sizeof(Header)+32, Image::EncodingLevel::Low); // FIXME

    std::cout << "* Ukuran sematan maks: " << data_size(max_size) << std::endl;
    std::cout << "* Ukuran sematan: " << data_size(size) << std::endl;
    std::cout << "* Ukuran sematan terenkripsi: " << data_size(padded_size) << std::endl;

    // Pastikan itu tidak terlalu besar
    if (padded_size > max_size) {
        std::cerr << "ERROR: File data terlalu besar, ukuran maksimum yang mungkin: " << (max_size / 1024) << " KiB" << std::endl;
        return -1;
    }

    // Baca data
    auto padded_data = std::make_unique<uint8_t[]>(padded_size);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(padded_data.get()), size);
    file.close();

    // Beri padding pada data (#PKCS7)
    std::uint8_t left = padded_size - size;
    std::fill_n(padded_data.get() + size, left, left);

    // Pilih offset acak di dalam gambar untuk menyimpan data
    std::uint32_t offset;
    Random random;
    if (!random.get(&offset, sizeof(offset)))
    {
        std::cerr << "Tidak dapat membuat angka acak" << std::endl;
        return -1;
    }

    offset = (offset + Image::encoded_size(sizeof(Header) + 32, Image::EncodingLevel::Low)) % (Image::encoded_size(max_size - padded_size, level));

    // Hitung hash dari data
    CRC32 crc;
    crc.update(padded_data.get(), size);

    std::cout << "* Checksum CRC32 berhasil dibuat" << std::endl;

    // Salin informasi header
    Header header;
    header.sig[0] = 'H'; header.sig[1] = 'I'; header.sig[2] = 'D'; header.sig[3] = 'E';
    header.version = VERSION;
    header.level  = static_cast<std::uint8_t>(level);
    header.flags  = 0;
    header.offset = offset;
    header.size   = padded_size;
    header.hash   = crc.get_hash();

    // Salin nama file ke header
    auto name = fs::path(input).filename().string();
    if (name.size() > sizeof(header.name)) {
        std::cerr << "ERROR: Nama file '" << name << "' lebih dari 32 karakter" << std::endl;
        return -1;
    }
    std::copy_n(name.data(), name.size(), header.name);
    std::fill_n(&header.name[name.size()], sizeof(header.name) - name.size(), 0x00);
    std::fill_n(header.reserved, sizeof(header.reserved), 0x00);

    // Buat Salt dan IV
    std::uint8_t salt[16], iv[16];
    if (!random.get(salt, sizeof salt) || !random.get(iv, sizeof iv))
    {
        std::cerr << "ERROR: Tidak dapat membuat angka acak" << std::endl;
        return -1;
    }

    // Buat Kunci
    std::uint8_t key[32];
    pbkdf2_hmac_sha256(password.data(), password.size(), salt, sizeof(salt), key, sizeof(key), KEY_ROUNDS);

    std::cout << "* Kunci enkripsi berhasil dibuat dengan PBKDF2-HMAC-SHA-256 (" << KEY_ROUNDS << " putaran)" << std::endl;

    // Enkripsi header
    AES aes(key, iv);
    auto encrypted_header = std::make_unique<uint8_t[]>(sizeof header);
    aes.cbc_encrypt(&header, sizeof(header), encrypted_header.get());

    // Enkripsi data
    auto encrypted_data = std::make_unique<uint8_t[]>(padded_size);
    aes.cbc_encrypt(padded_data.get(), padded_size, encrypted_data.get());

    std::cout << "* Sematan terenkripsi dengan AES-256-CBC" << std::endl;

    // Encode data
    image.encode(salt, 16, level);
    image.encode(iv, 16, level, Image::encoded_size(16, Image::EncodingLevel::Low));
    image.encode(encrypted_header.get(), sizeof(Header), level, Image::encoded_size(32, Image::EncodingLevel::Low));
    image.encode(encrypted_data.get(), padded_size, level, offset);

    std::cout << "* Berhasil menyematkan " << name << " ke dalam gambar" << std::endl;

    // Simpan gambar yang telah di-encode
    if (!image.save(output)) {
        std::cout << "Tidak dapat menyimpan gambar!" << std::endl;
        return false;
    }

    std::cout << "* Berhasil menulis ke " << output << std::endl;    

    return true;
}

/*
 * * Decode
 * 1. Ekstraksi Awal:
 * - Mengekstrak Salt dan Initialization Vector (IV) dari posisi tetap di dalam gambar.
 
 * * 2. Pembuatan Ulang Kunci:
 * - Membuat ulang kunci dekripsi dari Salt yang diekstrak dan password pengguna menggunakan PBKDF2-HMAC-SHA-256.
 
 * * 3. Dekripsi & Validasi Header:
 * - Mengekstrak dan mendekripsi Header terenkripsi dari gambar.
 * - Memvalidasi Header dengan memeriksa tanda tangan file ('HIDE'), nomor versi, dan field yang dicadangkan.
 
 * * 4. Ekstraksi & Dekripsi Data:
 * - Menggunakan metadata dari Header (ukuran, offset, level encoding) untuk mengekstrak blok data terenkripsi.
 * - Mendekripsi blok data tersebut menggunakan AES-256-CBC.
 
 * * 5. Verifikasi Akhir:
 * - Menghitung 'checksum' CRC32 dari data yang telah didekripsi dan membandingkannya dengan 'checksum' di dalam Header.
 * - Jika valid, menulis data yang telah didekripsi ke file output yang ditentukan.
 */
int decode(Image &image, const std::array<std::uint8_t, 32> &password, std::string output) {
    std::cout << "* Ukuran gambar: " << image.w() << "x" << image.h() << " piksel" << std::endl;

    // Ekstrak Salt dan IV
    auto salt = image.decode(16, Image::EncodingLevel::Low);
    auto iv   = image.decode(16, Image::EncodingLevel::Low, Image::encoded_size(16, Image::EncodingLevel::Low));

    // Buat kunci
    std::uint8_t key[32];
    pbkdf2_hmac_sha256(password.data(), password.size(), salt.get(), 16, key, sizeof(key), KEY_ROUNDS);

    std::cout << "* Kunci dekripsi berhasil dibuat dengan PBKDF2-HMAC-SHA-256 (" << KEY_ROUNDS << " putaran)" << std::endl;

    // Ekstrak header
    auto encrypted_header = image.decode(sizeof(Header), Image::EncodingLevel::Low, Image::encoded_size(32, Image::EncodingLevel::Low));

    // Dekripsi header
    AES aes(key, iv.get());
    Header header;
    aes.cbc_decrypt(encrypted_header.get(), sizeof(Header), &header);
    auto level = static_cast<Image::EncodingLevel>(header.level);

    // Pastikan tanda tangan file cocok, yaitu dekripsi berhasil
    if (header.sig[0] != 'H' || header.sig[1] != 'I' || header.sig[2] != 'D' || header.sig[3] != 'E') {
        std::cerr << "ERROR: Dekripsi gagal, kunci tidak valid atau file rusak" << std::endl;
        return -1;
    }

    // Pastikan versi sudah benar
    if (header.version != VERSION) {
        std::cerr << "ERROR: Versi file tidak didukung " << header.version << std::endl;
        return -1;
    }

    // Pastikan semua data yang dicadangkan adalah nol
    for (auto r : header.reserved) {
        if (r) {
            std::cerr << "ERROR: Dekripsi gagal, kunci tidak valid atau file rusak" << std::endl;
            return -1;
        }
    }

    std::cout << "* Header berhasil didekripsi" << std::endl;
    std::cout << "* Tanda tangan file cocok" << std::endl;

    // Salin nama, dengan mempertimbangkan bahwa mungkin tidak ada null-terminator
    std::string name;
    if (header.name[sizeof(header.name)-1])
        name = std::string(reinterpret_cast<char*>(header.name), sizeof(header.name));
    else
        name = std::string(reinterpret_cast<char*>(header.name));

    std::cout << "* Terdeteksi sematan " << name << std::endl;
    std::cout << "* Tingkat encoding: " << level_to_str[header.level] << std::endl;

    // Dekode data
    auto encrypted_data = image.decode(header.size, level, header.offset);

    std::cout << "* Ukuran sematan terenkripsi: " << data_size(header.size) << std::endl;

    // Dekripsi data
    auto padded_data = new std::uint8_t[header.size];
    aes.cbc_decrypt(encrypted_data.get(), header.size, padded_data);

    std::cout << "* Sematan berhasil didekripsi" << std::endl;

    // Temukan berapa banyak padding yang harus dilepaskan
    std::uint8_t left = padded_data[header.size - 1];
    std::size_t size  = header.size - left;

    std::cout << "* Ukuran sematan yang didekripsi: " << data_size(size) << std::endl;

    // Hitung hash CRC32
    CRC32 crc;
    crc.update(padded_data, size);

    // Pastikan data cocok
    if (crc.get_hash() != header.hash) {
        std::cerr << "ERROR: File rusak!" << std::endl;
        return -1;
    }

    std::cout << "* Checksum CRC32 cocok" << std::endl;

    // Jika jalur output kosong, gunakan saja nama file yang disematkan
    if (output.empty())
        output = name;

    // Buka file output
    std::ofstream file(output, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Tidak dapat menyimpan file '" << output << "'" << std::endl;
        return -1;
    }

    // Tulis data
    file.write(reinterpret_cast<char*>(padded_data), size);
    file.close();

    delete[] padded_data;

    std::cout << "* Berhasil menulis ke " << output << std::endl;

    return 0;
}

int main(int argc, char **argv) {
    return run_gui();
}
