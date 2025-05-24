#include "aurora_light.h"
#include "MQTTClient.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "AuroraSubscriber"
#define TOPIC       "aurora/encrypted"
#define QOS         1

volatile int finished = 0;

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}


int msg_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    size_t payload_len = message->payloadlen;
    uint8_t *payload = (uint8_t *)message->payload;

    clock_t start = clock();
    // (Optionally decrypt/process payload)
    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n===== RECEIVER METRICS (NO DECRYPTION) =====\n");
    printf("Message Size     : %zu bytes\n", payload_len);
    printf("Execution Time   : %.6f seconds\n", elapsed_time);
    printf("Throughput       : %.2f bytes/sec\n", payload_len / elapsed_time);
    printf("Static RAM Usage : %lu bytes\n", sizeof(*payload) * payload_len); // Optional, estimate

    print_hex("Encrypted Payload", payload, payload_len);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}



int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_setCallbacks(client, NULL, NULL, msg_arrived, NULL);

    MQTTClient_connect(client, &conn_opts);
    MQTTClient_subscribe(client, TOPIC, QOS);

    while (!finished)
        ;
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    return 0;
}
