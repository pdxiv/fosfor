#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct decoded_msgpack
{
    uint8_t positive_fixint[2];
    int32_t int_32[6];
    char *fixstr;
};

uint8_t input_bytes = 39;

char input_byte[] = {
    0x96,
    0x63,
    0xd2,
    0x00,
    0x00,
    0x00,
    0x08,
    0xd2,
    0x00,
    0x00,
    0x00,
    0x09,
    0xd2,
    0x00,
    0x00,
    0x00,
    0x0a,
    0xd2,
    0x00,
    0x00,
    0x00,
    0x0b,
    0xb0,
    0x68,
    0x65,
    0x6a,
    0x20,
    0x70,
    0xc3,
    0xa5,
    0x20,
    0x64,
    0x69,
    0x67,
    0x20,
    0x64,
    0x69,
    0x6e,
    0x6e};

bool decodething(uint8_t input_bytes, char *input_byte, struct decoded_msgpack *msgpack_data)
{
    uint8_t positive_fixint_location = 0;
    uint8_t int_32_location = 0;
    uint8_t byte_location = 0;
    uint8_t array_entries = input_byte[byte_location] & 0x0f;
    byte_location++;
    while (byte_location < input_bytes)
    {
        // int 32
        if ((input_byte[byte_location] & 0xff) == 0xd2)
        {
            byte_location++;
            int32_t int32_value = input_byte[byte_location + 3];
            msgpack_data->int_32[int_32_location] = int32_value;
            int_32_location++;
            byte_location += 4;
        }
        // fixint value
        else if ((input_byte[byte_location] & 0x80) == 0)
        {
            uint8_t fixint_value = input_byte[byte_location] & 0x7f;
            msgpack_data->positive_fixint[positive_fixint_location] = fixint_value;
            positive_fixint_location++;
            byte_location++;
        }
        // fixstr
        else if ((input_byte[byte_location] & 0b11100000) == 0xa0)
        {
            uint8_t string_characters = (input_byte[byte_location] & 0b00011111);
            byte_location++;
            msgpack_data->fixstr = input_byte + byte_location;
            byte_location += string_characters;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void main()
{

    struct decoded_msgpack msgpack_data;

    bool success = decodething(input_bytes, input_byte, &msgpack_data);

    if (success)
    {
        switch (msgpack_data.positive_fixint[0])
        {
        case 'c':
            printf("command: %c\n", msgpack_data.positive_fixint[0]);
            printf("priceTickOffset: %d\n", msgpack_data.int_32[0]);
            printf("priceTicks: %d\n", msgpack_data.int_32[1]);
            printf("orderBufferAllocationSize: %d\n", msgpack_data.int_32[2]);
            printf("orderBookId: %d\n", msgpack_data.int_32[3]);
            printf("name: %.16s\n", msgpack_data.fixstr);
            break;
        case 'a':
            printf("command: %c\n", msgpack_data.positive_fixint[0]);
            printf("userId: %d\n", msgpack_data.int_32[0]);
            printf("userReference: %d\n", msgpack_data.int_32[1]);
            printf("orderBookId: %d\n", msgpack_data.int_32[2]);
            printf("side: %c\n", msgpack_data.positive_fixint[0]);
            printf("price: %d\n", msgpack_data.int_32[0]);
            printf("volume: %d\n", msgpack_data.int_32[1]);
            printf("timeToLive: %d\n", msgpack_data.int_32[2]);
            break;
        case 'd':
            printf("command: %c\n", msgpack_data.positive_fixint[0]);
            printf("orderBookId: %d\n", msgpack_data.int_32[0]);
            break;
        case 'm':
            printf("command: %c\n", msgpack_data.positive_fixint[0]);
            printf("userId: %d\n", msgpack_data.int_32[0]);
            printf("userReference: %d\n", msgpack_data.int_32[0]);
            printf("orderBookId: %d\n", msgpack_data.int_32[0]);
            printf("side: %c\n", msgpack_data.positive_fixint[0]);
            printf("volume: %d\n", msgpack_data.int_32[0]);
            printf("timeToLive: %d\n", msgpack_data.int_32[0]);
            break;
        }
    }
}
