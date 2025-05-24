#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "MQTTClient.h"
#include "photon_beetle.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "PhotonPublisher"
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
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connect(client, &conn_opts);

    const char *message = "Photon-Beetle Secure IoT!";
    size_t msg_len = strlen(message);

    photon_beetle_ctx ctx;
    uint8_t key[KEY_SIZE], nonce[NONCE_SIZE];
    for (int i = 0; i < KEY_SIZE; i++) key[i] = i;
    for (int i = 0; i < NONCE_SIZE; i++) nonce[i] = i + 0xA0;

    uint8_t ciphertext[64] = {0}, tag[TAG_SIZE] = {0};

    clock_t start = clock();

    // Encryption Procedure
    memset(&ctx, 0, sizeof(ctx));
    memcpy(ctx.state, nonce, NONCE_SIZE);
    memcpy(ctx.state + NONCE_SIZE, key, KEY_SIZE);
    permute(ctx.state, 12);
    absorb(&ctx, NULL, 0, 0x02);  // No associated data
    encrypt(&ctx, (const uint8_t*)message, ciphertext, msg_len);
    generate_tag(&ctx, tag);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    // Compose payload (ciphertext + tag)
    uint8_t payload[128];
    memcpy(payload, ciphertext, msg_len);
    memcpy(payload + msg_len, tag, TAG_SIZE);

    // MQTT Publish
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = msg_len + TAG_SIZE;
    pubmsg.qos = QOS;

    printf("\n===== PHOTON-BEETLE ENCRYPTION METRICS =====\n");
    printf("Message Size     : %zu bytes\n", msg_len);
    printf("Execution Time   : %.6f seconds\n", elapsed);
    printf("Throughput       : %.2f bytes/sec\n", msg_len / elapsed);
    printf("Static RAM Usage : %lu bytes\n", sizeof(ctx) + sizeof(key) + sizeof(nonce) + sizeof(tag) + sizeof(ciphertext));

    print_hex("Ciphertext", ciphertext, msg_len);
    print_hex("Tag", tag, TAG_SIZE);

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, NULL);
    printf("Published encrypted Photon-Beetle message.\n");

    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return 0;
}
