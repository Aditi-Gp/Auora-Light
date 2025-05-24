#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "MQTTClient.h"
#include "quark.h"  // for TAG_SIZE, RATE

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "QuarkSubscriber"
#define TOPIC       "iot/sensor"
#define QOS         1
#define TIMEOUT     10000L

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02X", data[i]);
    printf("\n");
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("\nEncrypted payload received (%d bytes):\n", message->payloadlen);

    uint8_t *payload = (uint8_t *)message->payload;
    size_t payload_len = message->payloadlen;

    clock_t start = clock();

    // Simulate work (e.g. logging, verifying, etc.)
    usleep(100);  // tiny delay to simulate processing

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n===== RECEIVER METRICS (NO DECRYPTION) =====\n");
    printf("Message Size     : %zu bytes\n", payload_len);
    printf("Execution Time   : %.6f seconds\n", elapsed_time);
    printf("Throughput       : %.2f bytes/sec\n", payload_len / elapsed_time);
    printf("Static RAM Usage : %lu bytes\n", sizeof(payload));  // optional; not real RAM usage

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

    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);
    MQTTClient_connect(client, &conn_opts);

    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("Waiting for encrypted messages...\n");

    while (1) {
        sleep(1);
    }

    return 0;
}
