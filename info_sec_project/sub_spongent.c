#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "MQTTClient.h"
#include "spongent.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "SpongentSubscriber"
#define TOPIC       "iot/spongent/hash"
#define QOS         1
#define TIMEOUT     10000L

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("\nHash received (%d bytes):\n", message->payloadlen);

    uint8_t *hash = (uint8_t *)message->payload;
    size_t hash_len = message->payloadlen;

    uint64_t start_time = get_time_microseconds();
    usleep(100);  // Simulate processing
    uint64_t end_time = get_time_microseconds();

    double exec_time_sec = (end_time - start_time) / 1e6;
    double throughput = hash_len / exec_time_sec;

    printf("\n===== RECEIVER METRICS =====\n");
    printf("Hash Size        : %zu bytes\n", hash_len);
    printf("Execution Time   : %.6f seconds\n", exec_time_sec);
    printf("Throughput       : %.2f bytes/sec\n", throughput);
    printf("RAM Usage        : %lu bytes (approx)\n", sizeof(hash));

    print_hex("Received Hash", hash, hash_len);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);

    MQTTClient_connect(client, &conn_opts);
    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("Waiting for SPONGENT hashes...\n");

    while (1) sleep(1);

    return 0;
}
