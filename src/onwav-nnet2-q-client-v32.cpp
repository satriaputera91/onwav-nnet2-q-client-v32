/*
 ============================================================================
 Name        : client-nnet2.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#define PERIOD_SIZE 800
#define BUF_SIZE (PERIOD_SIZE * 2)

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <zmq.h>
#include <iostream>
#include <json-c/json.h>
#include <uuid/uuid.h>

#define REQ_CONNECT	1
#define REP_ACCEPT	1

#define RECV_BUF_LEN 3200
#define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))

int main(int argc, char *argv[]) {

	int seed;
	int rc;

	char worker_identity[37];
	char recv_buff[RECV_BUF_LEN];
	clock_t start = clock();
	int i = 0;

	void *context;
	void *stream_frontend;
	uuid_t uuid;

	while (i < 2000) {

		context = zmq_ctx_new();
		stream_frontend = zmq_socket(context, ZMQ_DEALER);

		/*create identity worker identification*/
//		seed = time(NULL);
//		srand(seed * getpid());
//		sprintf(worker_identity, "%04X-%04X", randof(0x10000), randof(0x10000));
//		zmq_setsockopt(stream_frontend, ZMQ_IDENTITY, worker_identity,
//				strlen(worker_identity));

// generate
		memset(worker_identity, 0, strlen(worker_identity));
		uuid_generate_random(uuid);
		uuid_unparse(uuid, worker_identity);
		worker_identity[16] = '\0';
		zmq_setsockopt(stream_frontend, ZMQ_IDENTITY, worker_identity,
				strlen(worker_identity));

		std::cout << "uuid :" << worker_identity << std::endl;
		/* connection to engine identification*/
		rc = zmq_connect(stream_frontend, "tcp://localhost:1903");
		assert(rc == 0);
		int j = 0;

		zmq_send(stream_frontend, "READY", 5, 0);

		zmq_pollitem_t items[] = { { stream_frontend, 0, ZMQ_POLLIN, 0 } };

		// poll every 10 milliseconds
		rc = zmq_poll(items, 1, 10);
		if (rc == -1) {
			break;              //  Interrupted
		}

		if (items[0].revents & ZMQ_POLLIN) {
			rc = zmq_recv(stream_frontend, recv_buff, RECV_BUF_LEN, 0);
			recv_buff[rc] = '\0';

			if (strncmp(recv_buff, "ACCEPTED", 8) != 0) {
				zmq_close(stream_frontend);
				zmq_ctx_destroy(context);
				continue;
			}
		} else {
			zmq_close(stream_frontend);
			zmq_ctx_destroy(context);
			continue;

		}

		std::cout << "READY" << std::endl;

		while (j < 10) {

			strcpy(recv_buff, worker_identity);
			//printf("REQ : %s\n", recv_buff);
			//std::cout << "REQ :  "<< recv_buff << std::endl;

			zmq_send(stream_frontend, recv_buff, RECV_BUF_LEN, 0);

			zmq_pollitem_t items[] = { { stream_frontend, 0, ZMQ_POLLIN, 0 } };

			// poll every 10 milliseconds
			rc = zmq_poll(items, 1, 10);
			if (rc == -1) {
				break;              //  Interrupted
			}

			if (items[0].revents & ZMQ_POLLIN) {
				rc = zmq_recv(stream_frontend, recv_buff, RECV_BUF_LEN, 0);
				recv_buff[rc] = '\0';
				//std::cout << "RESP :  "<< recv_buff << std::endl;
			}
			j++;
		}
		//zmq_disconnect(stream_frontend, worker_identity);

		zmq_close(stream_frontend);
		zmq_ctx_destroy(context);
		i++;
	}

	std::cout << "time process : " << i << " data, time : "
			<< (double) (clock() - start) / CLOCKS_PER_SEC << " sec"
			<< std::endl;

	return 0;
}
