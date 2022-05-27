#include <stdio.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <memory>

enum class AESKeyLength { AES_128, AES_192, AES_256 };

template <AESKeyLength KeyLength = AESKeyLength::AES_256>
class AES {
 private:
    constexpr static int Nb = 4;

    constexpr static int Nk = []() constexpr {
        switch (KeyLength) {
            case AESKeyLength::AES_128:
                return 4;
            case AESKeyLength::AES_192:
                return 6;
            case AESKeyLength::AES_256:
                return 8;
        }
    }
    ();

    constexpr static int Nr = []() constexpr {
        switch (KeyLength) {
            case AESKeyLength::AES_128:
                return 10;
            case AESKeyLength::AES_192:
                return 12;
            case AESKeyLength::AES_256:
                return 14;
        }
    }
    ();

    void
    sub_bytes(uint8_t* state);

    void
    shift_row(uint8_t* row, int n);

    void
    shift_rows(uint8_t* state);

    void
    mix_columns(uint8_t* state);

    void
    add_round_key(uint8_t* state, const uint8_t* key);

    void
    sub_word(uint8_t* a);

    void
    rot_word(uint8_t* a);

    void
    inv_sub_bytes(uint8_t* state);

    void
    inv_mix_columns(uint8_t* state);

    void
    inv_shift_rows(uint8_t* state);

    void
    check_length(size_t len);

    void
    key_expansion(const uint8_t* key, uint8_t* w);

    void
    encrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key);

    void
    decrypt_block(const uint8_t* in, uint8_t* out, const uint8_t* key);

    void
    xor_blocks(uint8_t* a, const uint8_t* b, size_t len);

 public:
    constexpr static int block_size = 4 * Nb * sizeof(uint8_t);

    explicit AES() noexcept = default;

    std::unique_ptr<uint8_t[]>
    encrypt_cbc(const uint8_t* in, size_t len, const uint8_t* key, const uint8_t* iv = nullptr);

    std::unique_ptr<uint8_t[]>
    decrypt_cbc(const uint8_t* in, size_t len, const uint8_t* key, const uint8_t* iv = nullptr);
};