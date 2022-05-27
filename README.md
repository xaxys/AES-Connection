# AES Connection

![Build Status](https://github.com/xaxys/AES-Connection/actions/workflows/aes-ci.yml/badge.svg?branch=master)

Modern C++ AES (Advanced Encryption Standard) implementation

2022 SSDUT 系统安全理论 田大师上机

- 实现AES对称分组加密解密算法
- 实现CBC的密文链模式算法
- 基于上述算法实现对TCP的通信保密服务

## Links

- [Wiki](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)
- [NIST](https://www.nist.gov/publications/advanced-encryption-standard-aes)

## Quick Start

1. `git clone https://github.com/xaxys/AES-Connection.git`
1. `docker-compose build`
1. `docker-compose up -d`
1. `make`
1. Check `lib/libaes.a` and `lib/libaes_debug.a` for aes library
1. Check `bin/speedtest` for benchmark

## Usage

### AES Encrypt/Decrypt Library

```cpp
...
uint8_t plain[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }; // plaintext example
uint8_t key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f }; // key example
size_t len = 16 * sizeof(unsigned char);  // bytes in plaintext

AES<AESKeyLength::AES_128> aes;  // 128 - key length, can be 128, 192 or 256
std::unique_ptr<uint8_t[]> c = aes.encrypt_cbc(plain, len, key);
// now variable c contains len bytes - ciphertext
std::unique_ptr<uint8_t[]> d = aes.decrypt_cbc(c.get(), len, key);
// now variable d contains len bytes - decrypted ciphertext
...
```

## Benchmark

Use `make speed_test` to run benchmark

```shell
docker-compose exec aes bin/speedtest
[AES benchmark] test scale: 100MB
>>> start raw encrypt speedtest
raw encrypt speed 43.37 MB/s
>>> start raw decrypt speedtest
raw decrypt speed 51.19 MB/s
>>> start tcp aes connection speedtest
sent 104857600 bytes
connection closed
received 104857600 bytes
tcp average speed 22.48 MB/s (transmission time included)
```

Test scale can be defined by first argument

e.g.

```shell
docker-compose exec aes bin/speedtest 512
[AES benchmark] test scale: 512MB
>>> start raw encrypt speedtest
raw encrypt speed 43.43 MB/s
>>> start raw decrypt speedtest
raw decrypt speed 51.26 MB/s
>>> start tcp aes connection speedtest
sent 536870912 bytes
connection closed
received 536870912 bytes
tcp average speed 22.51 MB/s (transmission time included)
```

## Introduction

### src/aes.h

```cpp
template <AESKeyLength KeyLength> class AES;
```

AES class is the encoder/decoder, which accept 3 key length: 128, 192 or 256

```cpp
// aes.h
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
    
    // all these are standard implemetation
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
```

- `Nb, Nk, Nr, block_size` is calculated on compilation
- `shift_rows, sub_bytes ...` are standard implementation
- `(inv_)mix_columns, (inv_)sub_bytes` use a lookup table. Reference: [https://crypto.stackexchange.com/questions/71204/how-are-these-aes-mixcolumn-multiplication-tables-calculated](https://crypto.stackexchange.com/questions/71204/how-are-these-aes-mixcolumn-multiplication-tables-calculated)

### test/main.cpp

use `gtest` framework to validity the aes algorithm

### speedtest/main.cpp

use `boost::asio` to manage `tcp` connection asynchronously. (C++20 coroutine included)

100MB plain text as default

- setup a server and a client
- start timing when started encryption
- end timing when data decrtypted

### cmd/main.cpp

I'm trying to create a REPL as chatting client/server, but not finished yet

## Development

1. `git clone https://github.com/xaxys/AES-Connection.git`
1. `docker-compose build`
1. `docker-compose up -d`
1. use make commands

There are four executables in `bin` folder:  

- `test` - run tests
- `debug` - version for debugging (main code will be taken from cmd/main.cpp)  
- `profile` - version for profiling with gprof (main code will be taken from cmd/main.cpp)  
- `speedtest` - performance speed test (main code will be taken from speedtest/main.cpp)
- `release` - version with optimization (main code will be taken from cmd/main.cpp)  

Build commands:

- `make build_all` - build all targets
- `make build_test` - build `test` target
- `make build_debug` - build `debug` target
- `make build_profile` - build `profile` target
- `make build_speed_test` - build `speedtest` target
- `make build_release` - build `release` target
- `make style_fix` - fix code style
- `make test` - run tests
- `make debug` - run debug version
- `make profile` - run profile version
- `make speed_test` - run performance speed test
- `make release` - run `release` version
- `make clean` - clean `bin` directory
