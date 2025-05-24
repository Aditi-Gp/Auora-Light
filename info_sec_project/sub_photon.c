#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "MQTTClient.h"
#include "photon_beetle.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "PhotonSubscriber"
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

    size_t ct_len = payload_len - TAG_SIZE;
    const uint8_t *ciphertext = payload;
    const uint8_t *tag = payload + ct_len;

    clock_t start = clock();
    usleep(100);  // Simulate processing
    clock_t end = clock();

    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n===== RECEIVER METRICS (NO DECRYPTION) =====\n");
    printf("Ciphertext Size  : %zu bytes\n", ct_len);
    printf("Execution Time   : %.6f seconds\n", elapsed_time);
    printf("Throughput       : %.2f bytes/sec\n", ct_len / elapsed_time);

    print_hex("Ciphertext", ciphertext, ct_len);
    print_hex("Tag", tag, TAG_SIZE);

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
    printf("Waiting for Photon-Beetle encrypted messages...\n");

    while (1) {
        sleep(1);
    }

    return 0;
}
