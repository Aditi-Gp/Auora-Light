#include "aurora_light.h"
#include "MQTTClient.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include<time.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "AuroraPublisher"
#define TOPIC       "aurora/encrypted"
#define QOS         1
#define TIMEOUT     10000L


void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    // Key and nonce (128-bit each)
    size_t msg_len;         // Total length of ciphertext
    double elapsed_time;    // Time measurement

    uint8_t key[16] = {0};
    uint8_t nonce[16] = {0};
    for (int i = 0; i < 16; i++) {
        key[i] = i;
        nonce[i] = 15 - i;
    }

    const char *plaintext = "Hello from Aurora!";
    
    size_t pt_len = strlen(plaintext);
    uint8_t ciphertext[128] = {0};
    uint8_t tag[AURORA_TAG_BYTES] = {0};
    clock_t start = clock();
    aurora_ctx_t ctx;
    aurora_init(&ctx, key, nonce);
    aurora_encrypt(&ctx, NULL, 0, (const uint8_t *)plaintext, pt_len, ciphertext, tag);
    clock_t end = clock();
    // Combine ciphertext + tag
    msg_len = pt_len;
    uint8_t msg[256] = {0};
    memcpy(msg, ciphertext, pt_len);
    memcpy(msg + pt_len, tag, AURORA_TAG_BYTES);
    size_t total_len = pt_len + AURORA_TAG_BYTES;

    // MQTT setup
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connect(client, &conn_opts);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = msg;
    pubmsg.payloadlen = (int)total_len;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Published encrypted message and tag to MQTT.\n");
    printf("\n===== SENDER METRICS =====\n");
    printf("Algorithm        : Aurora AEAD\n");
    printf("Message Size     : %zu bytes\n", msg_len);
    printf("Execution Time   : %.6f seconds\n", elapsed_time);
    printf("Throughput       : %.2f bytes/sec\n", msg_len / elapsed_time);
    printf("Static RAM Usage : %lu bytes\n", sizeof(key) + sizeof(nonce) + sizeof(ciphertext) + sizeof(tag));

    print_hex("Ciphertext", ciphertext, msg_len);
    print_hex("Tag", tag, AURORA_TAG_BYTES);

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, NULL);
    printf("Published encrypted message.\n");


    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    return 0;
}
