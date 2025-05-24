#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "MQTTClient.h"
#include "spongent.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "SpongentPublisher"
#define TOPIC       "iot/spongent/hash"
#define QOS         1
#define TIMEOUT     10000L

int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connect(client, &conn_opts);

    const char *message = "Integrity Check via SPONGENT";
    size_t msg_len = strlen(message);

    uint8_t hash[SPONGENT_HASH_SIZE];

    uint64_t start_time = get_time_microseconds();
    spongent((uchar *)message, hash);
    uint64_t end_time = get_time_microseconds();

    double exec_time_sec = (end_time - start_time) / 1e6;
    double throughput = msg_len / exec_time_sec;

    printf("\n===== SPONGENT HASH METRICS =====\n");
    printf("Message          : %s\n", message);
    printf("Message Size     : %zu bytes\n", msg_len);
    printf("Hash Size        : %d bytes\n", SPONGENT_HASH_SIZE);
    printf("Execution Time   : %.6f seconds\n", exec_time_sec);
    printf("Throughput       : %.2f bytes/sec\n", throughput);
    printf("RAM Usage        : %lu bytes\n", (size_t)(SPONGENT_STATE_SIZE + SPONGENT_STATE_SIZE));
    print_hex("Hash", hash, SPONGENT_HASH_SIZE);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = hash;
    pubmsg.payloadlen = SPONGENT_HASH_SIZE;
    pubmsg.qos = QOS;

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, NULL);
    printf("Published SPONGENT hash.\n");

    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);

    return 0;
}
