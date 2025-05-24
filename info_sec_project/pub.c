#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "MQTTClient.h"
#include "quark.h" 

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "QuarkPublisher"
#define TOPIC       "iot/sensor"
#define QOS         1
#define TIMEOUT     10000L

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02X", data[i]);
    printf("\n");
}

int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connect(client, &conn_opts);

    const char *message = "Secure IoT Message";
    size_t msg_len = strlen(message);

    uint8_t ciphertext[64] = {0}, tag[TAG_SIZE] = {0};
    uint8_t key[RATE/2], nonce[RATE/2];
    for (int i = 0; i < RATE/2; i++) {
        key[i] = i;
        nonce[i] = i + 0x10;
    }

    clock_t start = clock();

    quark_aead_encrypt(key, nonce, (const uint8_t*)message, msg_len, ciphertext, tag);

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    // Combine ciphertext + tag
    uint8_t payload[128];
    memcpy(payload, ciphertext, msg_len);
    memcpy(payload + msg_len, tag, TAG_SIZE);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = msg_len + TAG_SIZE;
    pubmsg.qos = QOS;

    printf("\n===== QUARK ENCRYPTION METRICS =====\n");
    printf("Algorithm        : Quark AEAD\n");
    printf("Message Size     : %zu bytes\n", msg_len);
    printf("Execution Time   : %.6f seconds\n", elapsed_time);
    printf("Throughput       : %.2f bytes/sec\n", msg_len / elapsed_time);
    printf("Static RAM Usage : %lu bytes\n", sizeof(key) + sizeof(nonce) + sizeof(ciphertext) + sizeof(tag));
    print_hex("Ciphertext", ciphertext, msg_len);
    print_hex("Tag", tag, TAG_SIZE);

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, NULL);
    printf("Published encrypted message.\n");

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return 0;
}
