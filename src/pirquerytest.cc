#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>

#define MAX_NUM_PER_VARIETY 10
#define VARIETY_NUM 5

using namespace std::chrono;
using namespace std;
using namespace seal;

static uint32_t IP[VARIETY_NUM][MAX_NUM_PER_VARIETY] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49
};

static uint16_t Port[VARIETY_NUM][MAX_NUM_PER_VARIETY] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49
};

int main(int argc, char** argv) {
    uint64_t number_of_items = VARIETY_NUM * MAX_NUM_PER_VARIETY;
    uint64_t size_per_item = MAX_NUM_PER_VARIETY * 6; // in bytes 
    uint32_t N = 4096;

    // Recommended values: (logt, d) = (20, 2).
    uint32_t logt = 20; // t is coeff modular (plaintext modular)
    uint32_t d = 2; // dimension of the database

    // use symmetric encryption instead of public key (recommended for smaller query)
    bool use_symmetric = false;
    // pack as many elements as possible into a BFV plaintext (recommended)
    bool use_batching = true;
    bool use_recursive_mod_switching = true;

    // BFV paras
    EncryptionParameters enc_params(scheme_type::bfv);
    PirParams pir_params;
    gen_encryption_params(N, logt, enc_params);
    verify_encryption_params(enc_params);
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
        use_symmetric, use_batching, use_recursive_mod_switching);
    cout << "----------------------Parameters------------------------\n";
    print_seal_params(enc_params);
    print_pir_params(pir_params);
    cout << "--------------------Parameters END----------------------\n";

    // use paras to initialize
    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();
    PIRServer server(enc_params, pir_params);
    server.set_galois_key(0, galois_keys); // id of the first client

    // database initialize
    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    uint64_t ind = 0;
    for (uint64_t i = 0; i < VARIETY_NUM; i++) {
        // below is one elem
        for (uint64_t j = 0; j < MAX_NUM_PER_VARIETY; j++) {
            db.get()[ind] = (IP[i][j] & 0xff000000) >> 24;
            db_copy.get()[ind] = (IP[i][j] & 0xff000000) >> 24;
            db.get()[ind + 1] = (IP[i][j] & 0x00ff0000) >> 16;
            db_copy.get()[ind + 1] = (IP[i][j] & 0x00ff0000) >> 16;
            db.get()[ind + 2] = (IP[i][j] & 0x0000ff00) >> 8;
            db_copy.get()[ind + 2] = (IP[i][j] & 0x0000ff00) >> 8;
            db.get()[ind + 3] = IP[i][j] & 0x000000ff;
            db_copy.get()[ind + 3] = IP[i][j] & 0x000000ff;
            db.get()[ind + 4] = (Port[i][j] & 0xff00) >> 8;
            db_copy.get()[ind + 4] = (Port[i][j] & 0xff00) >> 8;
            db.get()[ind + 5] = Port[i][j] & 0x00ff;
            db_copy.get()[ind + 5] = Port[i][j] & 0x00ff;
            ind += 6;
        }
    }
    cout << "==============================" << endl;
    cout << "DB test..." << endl;
    for (uint64_t x = 0;x < number_of_items;x++) {
        uint32_t t1 = 0; // ip test
        for (uint64_t y = 0;y < 4;y++) {
            t1 = (t1 << 8) | db.get()[x * 6 + y];
        }
        cout << t1 << ", ";
        uint16_t t2 = 0; // port test
        t2 = db.get()[x * 6 + 4];
        t2 = (t2 << 8) | db.get()[x * 6 + 5];
        cout << t2 << "\t";
        if ((x + 1) % 5 == 0)
            cout << endl;
    }
    cout << "==============================" << endl;

    // client request
    server.set_database(move(db), number_of_items, size_per_item);
    server.preprocess_database();
    uint64_t ele_index = 1; // the elem to get: variety vector
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext 根据索引和每个明文存储的元素个数判断
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext 最后获得的明文位于明文多项式的哪一个系数
    PirQuery query = client.generate_query(index);     // 针对两维度数据库的(Enc(x^i),Enc(x^i))

    // generate stream and reply
    stringstream client_stream;
    stringstream server_stream;
    int query_size = client.generate_serialized_query(index, client_stream); // index -> client_stream
    PirQuery query2 = server.deserialize_query(client_stream); // server get the stream
    PirReply reply = server.generate_reply(query2, 0); // reply to client 0
    int reply_size = server.serialize_reply(reply, server_stream); // server reply to stream
    PirReply rr = client.deserialize_reply(4, server_stream);

    // client get the result
    vector<uint8_t> elems = client.decode_reply(rr, offset);
    assert(elems.size() == size_per_item);

    // check the result
    bool failed = false;
    for (uint32_t i = 0; i < size_per_item; i++) {
        if (elems[i] != db_copy.get()[(ele_index * size_per_item) + i]) {
            cout << "Main: elems " << (int)elems[i] << ", db "
                << (int)db_copy.get()[(ele_index * size_per_item) + i] << endl;
            cout << "Main: PIR result wrong at " << i << endl;
            failed = true;
        }
    }

    if (failed) {
        cout << "Wrong in the result!" << endl;
    }
    else {
        cout << "We get the true value:" << endl;
        for (int i = 0;i < elems.size();i++) {
            cout << i << "  ";
            if ((i + 1) % 5 == 0)
                cout << endl;
        }
    }

    cout << "-----------------------PIR Result-----------------------\n";
    cout << "PIR Result we get: " << endl;
    for (uint64_t i = 0;i < elems.size();i += 6) {
        uint32_t ip = 0;
        uint64_t ipPos = i;
        for (uint64_t j = 0;j < 4;j++) {
            ip = (ip << 8) | elems[ipPos];
            ++ipPos;
        }
        uint16_t port = 0;
        port = (port << 8) | elems[ipPos];
        port = (port << 8) | elems[++ipPos];
        cout << "Current IP: " << ip << ",  Port: " << port << endl;
    }
    cout << "--------------------PIR Result END---------------------\n";
    return 0;
}




