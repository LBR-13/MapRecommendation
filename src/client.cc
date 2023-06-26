#include "seal/seal.h"
#include "predefines.h"
#include "pir_client.hpp"

#include "bloomfilter.h"
#include "cmdline.h"
#include "examples.h" // print_parameter
#include "util.h"

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// #include "threadpool.hpp"
// #include "multi_thread/thread_pool.hpp"
// #include "thread_pool.hpp"
// #include <src/boost/bind.hpp>
// #include <boost/thread_pool.hpp>

// #include "boost/threadpool.hpp"
// #include <boost/bind.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#define TEST_TIME
#define THREAD_NUM 2

using namespace std;
using namespace seal;

void proximity_test(uint64_t i, uint64_t xa, uint64_t ya, uint64_t radius, uint32_t ip, uint16_t port, int domain,
                    size_t poly_modulus_degree, size_t plain_modulus, vector<uint64_t> &result_index,
                    vector<string> &result_name, vector<double> &result_longitude, vector<double> &result_latitude)
{

    // for ipv4 currently
    in_addr tmp{ip};
    string merchant_ip = inet_ntoa(tmp);

    // Connecting............
    int sockfd_server = connect_to_server(merchant_ip, port, domain);
    if (sockfd_server < 0) // fail
        return;

    pppt_printf("Connected to the merchant %" PRIu64 ", proximity test start...\n", i);
    pppt_printf("Client's coordinates:\t(%" PRIu64 ", %" PRIu64 ")\n", xa, ya);
    pppt_printf("Radius:\t\t\t\t%" PRIu64 "\n", radius);

    // send radius
    stringstream radius_stream;
    radius_stream << radius << "\0";
    auto bytes_num = send_by_stream(sockfd_server, radius_stream);
    pppt_printf("Send radius to the merchant, bytes: %zd \n", ssize_t(bytes_num));

    // set the parms
    EncryptionParameters parms(scheme_type::bfv);
    // size_t poly_modulus_degree = 1ull << poly_modulus_degree_bits;
    // size_t plain_modulus = 1ull << plain_modulus_bits;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    parms.set_plain_modulus(plain_modulus); // sq
    // set the context
    SEALContext context(parms);

    // send the parms
    stringstream stream_parms;
    parms.save(stream_parms);
    auto bytes = send(sockfd_server, stream_parms.str().c_str(),
                      stream_parms.str().length(), 0);
    pppt_printf("Send parms(context), bytes: %zu \n", size_t(bytes));

    if (flag_log)
        print_parameters(context);
    pppt_printf("Parameter validation: %s\n", context.parameter_error_message());

    // generate sk and pk
    KeyGenerator keygen(context);
    SecretKey sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);

    // encrypt the data
    Encryptor encryptor(context, pk);
    Ciphertext c1, c2, c3; // x^2+y^2, 2x, 2y
    uint64_t u = xa * xa + ya * ya;
    encryptor.encrypt(Plaintext(uint64_to_hex_string(u)), c1);
    encryptor.encrypt(Plaintext(uint64_to_hex_string(xa << 1)), c2);
    encryptor.encrypt(Plaintext(uint64_to_hex_string(ya << 1)), c3);

    // send the ciphertext
    vector<Ciphertext> lst_cipher{c1, c2, c3};
    for (size_t id_cipher = 0; id_cipher < 3; id_cipher++)
    {
        stringstream stream_cipher;
        lst_cipher[id_cipher].save(stream_cipher);
        bytes = send_by_stream(sockfd_server, stream_cipher);
        pppt_printf("Send the ciphertext %zu, bytes: %zu\n", id_cipher,
                    size_t(bytes));
    }

    // receive the bloom filter (w || BF)
    bytes = bytes_to_receive(sockfd_server);
    //
    uint8_t *bf_buf = (uint8_t *)malloc(bytes);
    ssize_t remain_bytes = bytes;
    for (uint8_t *ptr = bf_buf; remain_bytes != 0;)
    {
        ssize_t cur_bytes = recv(sockfd_server, ptr, remain_bytes, 0);
        ptr += cur_bytes;
        remain_bytes -= cur_bytes;
    }
    uint64_t w = *(uint64_t *)bf_buf;
    bloom_filter bf(bf_buf + sizeof(uint64_t));
    pppt_printf("Recv the BF and hash key, bytes: %zu\n", size_t(bytes));
    //
    free(bf_buf);

    // receive the encrypted blind distance
    stringstream stream_cipher;
    bytes = recv_by_stream(sockfd_server, stream_cipher);
    Ciphertext cipher_blind_distance;
    cipher_blind_distance.load(context, stream_cipher);
    pppt_printf("Recv the encrypted blind distance, bytes: %zu\n", size_t(bytes));

    // decrypt the result to get the blind distance
    Decryptor decryptor(context, sk);
    Plaintext plain_blind_distance;
    decryptor.decrypt(cipher_blind_distance, plain_blind_distance);

    uint64_t blind_distance =
        hex_string_to_uint(plain_blind_distance.to_string());

    pppt_printf("blind_distance: %" PRIu64 "\n", blind_distance);

    bool isNear = bf.contains((blind_distance << get_bitlen(w)) | w);

    std::cout << "Result of proximity test: " << (isNear ? "near" : "far") << endl;

    // receive name
    string name;
    stringstream merchant_info;
    bytes = recv_by_stream(sockfd_server, merchant_info);
    merchant_info >> name;

    // receive longitude
    double longitude;
    stringstream merchant_longitude;
    bytes = recv_by_stream(sockfd_server, merchant_longitude);
    merchant_longitude >> longitude;

    // receive latitude
    double latitude;
    stringstream merchant_latitude;
    bytes = recv_by_stream(sockfd_server, merchant_latitude);
    merchant_latitude >> latitude;

    // std::cout << name << " " << longitude << " " << latitude << endl;

    boost::mutex lock;
    lock.lock();

    if (isNear)
    {
        result_index.push_back(i);
        result_name.push_back(name);
        result_longitude.push_back(longitude);
        result_latitude.push_back(latitude);
    }
    lock.unlock();
    std::cout << "--------------------------------------------------------" << endl;
    close(sockfd_server);

    // boost::this_thread::sleep(boost::posix_time::seconds(1));
}

int main(int argc, char *argv[])
{
    // command line args
    cmdline::parser cmd_parser;
    cmd_parser.add<string>("server_platform_host", 'l', "ip of platform", false, "127.0.0.1");
    cmd_parser.add<uint16_t>("server_platform_port", 't', "port of platform", false, 51111,
                             cmdline::range(1, 65535));
    cmd_parser.add<size_t>("variety_selected", 'v', "variety of merchants", false, 1,
                           cmdline::range(0, 100));
    // cmd_parser.add<string>("merchant_ip", 'm', "ip of merchant", false, "127.0.0.1");
    // cmd_parser.add<uint16_t>("merchant_port", 'p', "port of merchant", false, 51022,
    // 	cmdline::range(1, 65535));
    cmd_parser.add<uint64_t>("xa", 'x', "coordinate1 of client", false, 123456789,
                             cmdline::range(0ul, 1ul << 27)); // 134217728
    cmd_parser.add<uint64_t>("ya", 'y', "coordinate2 of client", false, 132456888,
                             cmdline::range(0ul, 1ul << 27)); // 134217728
    cmd_parser.add<size_t>("plain_modulus_bits", 'b',
                           "bit length of plain modulus", false, 56, cmdline::range(1, 56));
    cmd_parser.add<uint64_t>("radius", 'r', "radius/thershold", false, 128,
                             cmdline::range(1, 8192));
    cmd_parser.add<size_t>("poly_modulus_degree", 'd',
                           "set degree of polynomial(2^d)", false, 13,
                           cmdline::range(12, 15));
    // cmd_parser.add("ipv4", '4', "ipv4");
    cmd_parser.add("ipv6", '6', "ipv6", 0, 0);
    cmd_parser.parse_check(argc, argv);

    // server platform
    string server_platform_host = cmd_parser.get<string>("server_platform_host");
    uint16_t server_platform_port = cmd_parser.get<uint16_t>("server_platform_port");
    size_t variety_selected = cmd_parser.get<size_t>("variety_selected");

    // merchant
    // string merchant_ip = cmd_parser.get<string>("merchant_ip");
    // uint16_t merchant_port = cmd_parser.get<uint16_t>("merchant_port");

    // ip protocol
    int domain = cmd_parser.exist("ipv6") ? AF_INET6 : AF_INET;
    // std::cout << boolalpha << cmd_parser.exist("ipv6") << endl;

    // coordinate and other params
    uint64_t xa = cmd_parser.get<uint64_t>("xa");
    uint64_t ya = cmd_parser.get<uint64_t>("ya");
    uint64_t radius = cmd_parser.get<uint64_t>("radius");
    // uint64_t sq_radius = radius * radius;

    uint64_t u = xa * xa + ya * ya;

    size_t poly_modulus_degree_bits =
        cmd_parser.get<size_t>("poly_modulus_degree");
    size_t plain_modulus_bits = cmd_parser.get<size_t>("plain_modulus_bits");

    size_t poly_modulus_degree = 1ull << poly_modulus_degree_bits;
    size_t plain_modulus = 1ull << plain_modulus_bits;

    // send bfv params to server platform for pir
    EncryptionParameters enc_params(scheme_type::bfv);
    PirParams pir_params;

    gen_encryption_params(N, logt, enc_params);
    verify_encryption_params(enc_params);
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
                   use_symmetric, use_batching, use_recursive_mod_switching);

    // connect to server platform
    std::cout << "Connect to server platform..." << endl;
    int sockfd_platform = connect_to_server(server_platform_host, server_platform_port, domain);

    // use params to initialize
    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();

    // we have to send enc_params and galois_keys, and pir_params can be got by negotiation
    stringstream enc_params_stream, pir_params_stream, galois_keys_stream;
    enc_params.save(enc_params_stream);
    galois_keys.save(galois_keys_stream);
    auto bytes_num = send_by_stream(sockfd_platform, enc_params_stream);
    pppt_printf("Send enc params to the platform, bytes: %zd \n", ssize_t(bytes_num));
    bytes_num = send_by_stream(sockfd_platform, galois_keys_stream);
    pppt_printf("Send galoise keys to the platform, bytes: %zd \n", ssize_t(bytes_num));

    // send query to platform server
    // index of FV plaintext -> according to index and plaintext element
    uint64_t index = client.get_fv_index(variety_selected);
    // offset in FV plaintext -> the coeff of the final plaintext we get
    uint64_t offset = client.get_fv_offset(variety_selected);
    // two dim database -> (Enc(x^i),Enc(x^i))
    PirQuery query = client.generate_query(index);
    // stream to be sent
    stringstream query_stream;
    // index -> query_stream
    int query_size = client.generate_serialized_query(index, query_stream);
    bytes_num = send_by_stream(sockfd_platform, query_stream);
    pppt_printf("Send query to the platform, bytes: %zd \n", ssize_t(bytes_num));

    // receive reply from server platform
    stringstream reply_stream, reply_size_stream;
    size_t reply_size;
    auto sbytes_num = recv_by_stream(sockfd_platform, reply_stream);
    pppt_printf("Recv reply from the platform, bytes: %zd \n", ssize_t(sbytes_num));

    sbytes_num = recv_by_stream(sockfd_platform, reply_size_stream);
    pppt_printf("Recv reply size from the platform, bytes: %zd \n", ssize_t(sbytes_num));
    reply_size_stream >> reply_size;
    PirReply reply = client.deserialize_reply(reply_size, reply_stream);

    // get data from server platform (pir Result)
    vector<uint8_t> elems = client.decode_reply(reply, offset);
    assert(elems.size() == size_per_item);

    vector<uint32_t> ip_vec;
    vector<uint16_t> port_vec;
    std::cout << "--------------------------------------------------------" << endl;
    std::cout << "These merchants we have to communicate with(pir result): " << endl;
    for (uint64_t i = 0; i < elems.size(); i += 6)
    {
        uint32_t ip = 0;
        uint64_t ipPos = i;
        for (uint64_t j = 0; j < 4; j++)
        {
            ip = (ip << 8) | elems[ipPos];
            ++ipPos;
        }
        if (ip == 0) // invalid
            break;
        uint16_t port = 0;
        port = (port << 8) | elems[ipPos];
        port = (port << 8) | elems[++ipPos];
        ip_vec.push_back(ip);
        port_vec.push_back(port);
        std::cout << "IP: " << ip << ", Port: " << port << endl;
    }
    std::cout << "--------------------------------------------------------" << endl;
    close(sockfd_platform);

    vector<uint64_t> result_index;
    vector<string> result_name;
    vector<double> result_longitude;
    vector<double> result_latitude;
    std::cout << "Starting communication..." << endl;

// test time cost
#ifdef TEST_TIME
    auto begin = chrono::high_resolution_clock::now();
#endif

    // pool pool(THREAD_NUM);
    // pool.wait();
    // for (uint64_t i = 0; i < ip_vec.size(); i++)
    // {
    // 	pool.schedule(boost::bind(proximity_test, i, xa, ya, radius, ip_vec[i], port_vec[i], domain, ref(result_index), ref(result_name), ref(result_longitude), ref(result_latitude), plain_modulus_bits, poly_modulus_degree_bits));
    // }
    // // 等待线程池中的任务全部完成
    // pool.wait();

    // boost::asio::thread_pool pool(THREAD_NUM);
    // for (uint64_t i = 0; i < ip_vec.size(); i++)
    // {
    //     boost::asio::dispatch(pool,
    //                           bind(proximity_test, i, xa, ya, radius, ip_vec[i], port_vec[i], domain, poly_modulus_degree,
    //                                plain_modulus, ref(result_index), ref(result_name), ref(result_longitude),
    //                                ref(result_latitude)));
    // }
    // pool.join();

    // boost::thread t[ip_vec.size()];

    // for (int i = 0; i < ip_vec.size(); ++i)
    //     t[i] = boost::thread(bind(proximity_test, i, xa, ya, radius, ip_vec[i], port_vec[i], domain, poly_modulus_degree,
    //                                plain_modulus, ref(result_index), ref(result_name), ref(result_longitude),
    //                                ref(result_latitude)));

    // for (int i = 0; i < ip_vec.size(); ++i)
    //     t[i].join();

    // 多线程调用 proximity_test
    // vector<thread> threads;
    // for (uint64_t i = 0; i < ip_vec.size(); i++)
    // {
    // 	threads.push_back(thread(proximity_test, i, xa, ya, radius, ip_vec[i], port_vec[i], domain, plain_modulus_bits, poly_modulus_degree_bits, ref(result_index), ref(result_name), ref(result_longitude), ref(result_latitude)));
    // }
    // for (auto &th : threads)
    // 	th.join();

    // connect to each merchant
    // for (uint64_t i = 0; i < ip_vec.size(); i++)
    // {
    // 	std::cout << "--------------------------------------------------------" << endl;
    // 	// for ipv4 currently
    // 	in_addr tmp{ip_vec[i]};
    // 	string merchant_ip = inet_ntoa(tmp);
    // 	uint16_t merchant_port = port_vec[i];

    // 	// Connecting............
    // 	int sockfd_server = connect_to_server(merchant_ip, merchant_port, domain);
    // 	if (sockfd_server < 0) // fail
    // 		return 1;

    // 	pppt_printf("Connected to the merchant %" PRIu64 ", proximity test start...\n", i);
    // 	pppt_printf("Client's coordinates:\t(%" PRIu64 ", %" PRIu64 ")\n", xa, ya);
    // 	pppt_printf("Radius:\t\t\t\t%" PRIu64 "\n", radius);

    // 	// auto begin = chrono::high_resolution_clock::now();

    // 	// send radius
    // 	stringstream radius_stream;
    // 	radius_stream << radius << "\0";
    // 	bytes_num = send_by_stream(sockfd_server, radius_stream);
    // 	pppt_printf("Send radius to the merchant, bytes: %zd \n", ssize_t(bytes_num));

    // 	// set the parms
    // 	EncryptionParameters parms(scheme_type::bfv);
    // 	size_t poly_modulus_degree = 1ull << poly_modulus_degree_bits;
    // 	size_t plain_modulus = 1ull << plain_modulus_bits;
    // 	parms.set_poly_modulus_degree(poly_modulus_degree);
    // 	parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    // 	parms.set_plain_modulus(plain_modulus); // sq
    // 	// set the context
    // 	SEALContext context(parms);

    // 	// send the parms
    // 	stringstream stream_parms;
    // 	parms.save(stream_parms);
    // 	auto bytes = send(sockfd_server, stream_parms.str().c_str(),
    // 					  stream_parms.str().length(), 0);
    // 	pppt_printf("Send parms(context), bytes: %zu \n", size_t(bytes));

    // 	if (flag_log)
    // 		print_parameters(context);
    // 	pppt_printf("Parameter validation: %s\n", context.parameter_error_message());

    // 	// generate sk and pk
    // 	KeyGenerator keygen(context);
    // 	SecretKey sk = keygen.secret_key();
    // 	PublicKey pk;
    // 	keygen.create_public_key(pk);

    // 	// encrypt the data
    // 	Encryptor encryptor(context, pk);
    // 	Ciphertext c1, c2, c3; // x^2+y^2, 2x, 2y
    // 	encryptor.encrypt(Plaintext(uint64_to_hex_string(u)), c1);
    // 	encryptor.encrypt(Plaintext(uint64_to_hex_string(xa << 1)), c2);
    // 	encryptor.encrypt(Plaintext(uint64_to_hex_string(ya << 1)), c3);

    // 	// send the ciphertext
    // 	vector<Ciphertext> lst_cipher{c1, c2, c3};
    // 	for (size_t id_cipher = 0; id_cipher < 3; id_cipher++)
    // 	{
    // 		stringstream stream_cipher;
    // 		lst_cipher[id_cipher].save(stream_cipher);
    // 		bytes = send_by_stream(sockfd_server, stream_cipher);
    // 		pppt_printf("Send the ciphertext %zu, bytes: %zu\n", id_cipher,
    // 					size_t(bytes));
    // 	}

    // 	// receive the bloom filter (w || BF)
    // 	bytes = bytes_to_receive(sockfd_server);
    // 	//
    // 	uint8_t *bf_buf = (uint8_t *)malloc(bytes);
    // 	ssize_t remain_bytes = bytes;
    // 	for (uint8_t *ptr = bf_buf; remain_bytes != 0;)
    // 	{
    // 		ssize_t cur_bytes = recv(sockfd_server, ptr, remain_bytes, 0);
    // 		ptr += cur_bytes;
    // 		remain_bytes -= cur_bytes;
    // 	}
    // 	uint64_t w = *(uint64_t *)bf_buf;
    // 	bloom_filter bf(bf_buf + sizeof(uint64_t));
    // 	pppt_printf("Recv the BF and hash key, bytes: %zu\n", size_t(bytes));
    // 	//
    // 	free(bf_buf);

    // 	// receive the encrypted blind distance
    // 	stringstream stream_cipher;
    // 	bytes = recv_by_stream(sockfd_server, stream_cipher);
    // 	Ciphertext cipher_blind_distance;
    // 	cipher_blind_distance.load(context, stream_cipher);
    // 	pppt_printf("Recv the encrypted blind distance, bytes: %zu\n", size_t(bytes));

    // 	// decrypt the result to get the blind distance
    // 	Decryptor decryptor(context, sk);
    // 	Plaintext plain_blind_distance;
    // 	decryptor.decrypt(cipher_blind_distance, plain_blind_distance);

    // 	uint64_t blind_distance =
    // 		hex_string_to_uint(plain_blind_distance.to_string());

    // 	pppt_printf("blind_distance: %" PRIu64 "\n", blind_distance);

    // 	bool isNear = bf.contains((blind_distance << get_bitlen(w)) | w);
    // 	// auto end = chrono::high_resolution_clock::now();
    // 	// auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end - begin);

    // 	if (isNear)
    // 		result_index.push_back(i);
    // 	std::cout << "Result of proximity test: " << (isNear ? "near" : "far") << endl;
    // 	// printf("Time measured: %.3f seconds\n", elapsed.count() * 1e-9);

    // 	// receive name
    // 	string name;
    // 	stringstream merchant_info;
    // 	bytes = recv_by_stream(sockfd_server, merchant_info);
    // 	merchant_info >> name;

    // 	// receive longitude
    // 	double longitude;
    // 	stringstream merchant_longitude;
    // 	bytes = recv_by_stream(sockfd_server, merchant_longitude);
    // 	merchant_longitude >> longitude;

    // 	// receive latitude
    // 	double latitude;
    // 	stringstream merchant_latitude;
    // 	bytes = recv_by_stream(sockfd_server, merchant_latitude);
    // 	merchant_latitude >> latitude;

    // 	// std::cout << name << " " << longitude << " " << latitude << endl;
    // 	if (isNear)
    // 	{
    // 		result_name.push_back(name);
    // 		result_longitude.push_back(longitude);
    // 		result_latitude.push_back(latitude);
    // 	}

    // 	std::cout << "--------------------------------------------------------" << endl;

    // 	close(sockfd_server);
    // }

    std::cout << "类别：" << Variety[variety_selected] << "，半径：" << radius << "米" << endl;
    std::cout << "以下商家在您附近：" << endl;
    for (int i = 0; i < result_index.size(); i++)
    {
        std::cout << result_name[i] << " " << result_longitude[i] << " " << result_latitude[i] << endl;
        // if (i % 3 == 0 && i != 0)
        // 	std::cout << endl;
    }
    std::cout << endl;

// test time cost
#ifdef TEST_TIME
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end - begin);
    printf("Time measured: %.3f seconds\n", elapsed.count() * 1e-9);
#endif

    return 0;
}