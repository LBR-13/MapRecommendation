#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "ipport.h"
#include "cmdline.h"
#include "util.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>
#include <sys/socket.h>

using namespace std::chrono;
using namespace std;
using namespace seal;

int main(int argc, char** argv) {
	cmdline::parser cmd_parser;
	cmd_parser.add<string>("host", 'h', "listening ip of serverplatform", false, "127.0.0.1");
	cmd_parser.add<uint16_t>("port", 'p', "listening port of serverplatform", false, 51111,
		cmdline::range(1, 65535));

	// cmd_parser.add("ipv4", '4', "ipv4");
	cmd_parser.add("ipv6", '6', "ipv6", 0, 0);
	cmd_parser.parse_check(argc, argv);

	// get cmd from input
	string ip = cmd_parser.get<string>("host");
	uint16_t port = cmd_parser.get<uint16_t>("port");
	int domain = cmd_parser.exist("ipv6") ? AF_INET6 : AF_INET;

	// connet with client...
	// listening on the ip + port and waitting for the client
	int sockfd_client = connect_to_client(ip, port, domain);
	if (sockfd_client < 0)
		return -1;

	// receive BFV paras, galois_keys and query
	EncryptionParameters enc_params;
	stringstream enc_params_stream;
	auto bytes_num = recv_by_stream(sockfd_client, enc_params_stream);
	enc_params.load(enc_params_stream);
	pppt_printf("Recv the enc parms, bytes: %zd \n", ssize_t(bytes_num)); 

	stringstream galois_keys_stream; 
	bytes_num = recv_by_stream(sockfd_client, galois_keys_stream);
	pppt_printf("Recv the galois keys stream, bytes: %zd \n", ssize_t(bytes_num)); 

	stringstream query_stream; 
	bytes_num = recv_by_stream(sockfd_client, query_stream);
	pppt_printf("Recv the query stream, bytes: %zd \n", ssize_t(bytes_num)); 
	
	verify_encryption_params(enc_params);

	// PIR scheme params are negotiated
	// get pir params
	PirParams pir_params;
	gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params,
		use_symmetric, use_batching, use_recursive_mod_switching);

	// use paras to initialize
	PIRServer server(enc_params, pir_params);
	server.set_galois_keys_from_stream(0, galois_keys_stream); // id of the client

	// database initialize
	auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
	uint64_t ind = 0;
	for (uint64_t i = 0; i < VARIETY_NUM; i++) {
		// below is one elem
		for (uint64_t j = 0; j < MAX_NUM_PER_VARIETY; j++) {
			db.get()[ind] = (IP[i][j] & 0xff000000) >> 24;
			db.get()[ind + 1] = (IP[i][j] & 0x00ff0000) >> 16;
			db.get()[ind + 2] = (IP[i][j] & 0x0000ff00) >> 8;
			db.get()[ind + 3] = IP[i][j] & 0x000000ff;
			db.get()[ind + 4] = (Port[i][j] & 0xff00) >> 8;
			db.get()[ind + 5] = Port[i][j] & 0x00ff;
			ind += 6;
		}
	}
	server.set_database(move(db), number_of_items, size_per_item);
	server.preprocess_database();

	// generate reply stream and send to client
	cout << "Send all data to the client..." << endl;
	stringstream reply_stream, reply_size_stream;
	PirQuery client_query = server.deserialize_query(query_stream); // server get the stream
	PirReply reply = server.generate_reply(client_query, 0); // reply to client 0
	int reply_size = server.serialize_reply(reply, reply_stream); // server reply to stream
	auto sbytes_num = send_by_stream(sockfd_client, reply_stream);
	pppt_printf("Send reply to the client, bytes: %zd \n", ssize_t(sbytes_num));
	
	reply_size_stream << reply.size() << "\0";
	sbytes_num = send_by_stream(sockfd_client, reply_size_stream);
	pppt_printf("Send reply size to the client, bytes: %zd \n", ssize_t(sbytes_num));

	close(sockfd_client);
	cout << "finished." << endl;
	return 0;
}