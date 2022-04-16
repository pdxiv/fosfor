#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct decoded_msgpack
{
    int32_t int_32[8];
    char *str;
    uint8_t strLength;
};

/*
uint8_t input_bytes = 39;

char input_byte[] = {
    0x96, // fixarray
    0x63, // positive fixint
    0xd2, // int 32
    0x00, 0x00, 0x00, 0x08,
    0xd2, // int 32
    0x00, 0x00, 0x00, 0x09,
    0xd2, // int 32
    0x00, 0x00, 0x00, 0x0a,
    0xd2, // int 32
    0x00, 0x00, 0x00, 0x0b,
    0xb0, // fixstr
    0x68, 0x65, 0x6a, 0x20, 0x70, 0xc3, 0xa5, 0x20, 0x64, 0x69, 0x67, 0x20, 0x64, 0x69, 0x6e, 0x6f};

*/

uint8_t input_bytes = 27;

char input_byte[] = {
    0xdd, // array 32
    0x00, 0x00, 0x00, 0x06,
    0x63, // positive fixint
    0x08,
    0x09,
    0x0a,
    0x0b,
    0xb0, // fixstr
    0x68, 0x65, 0x6a, 0x20, 0x70, 0xc3, 0xa5, 0x20, 0x64, 0x69, 0x67, 0x20, 0x64, 0x69, 0x6e, 0x6e};

bool decodething(uint8_t input_bytes, char *input_byte, struct decoded_msgpack *msgpack_data)
{
    uint8_t int_32_location = 0;
    uint8_t byte_location = 0;
    uint8_t array_entries;
    uint8_t array_counter = 0;

    // Because we don't know how MessagePack encoders will encode arrays,
    // we'll need to support all the possible array formats.

    // fixarray (0b1001xxxx).
    if ((input_byte[byte_location] & 0b11110000) == 0b10010000)
    {
        array_entries = input_byte[byte_location] & 0x0f;
        byte_location++;
    }
    // array 16 (0xdc)
    else if ((input_byte[byte_location] & 0xff) == 0xdc)
    {
        byte_location++;
        uint16_t uint16_value = input_byte[byte_location + 1];
        array_entries = (uint8_t)uint16_value;
        byte_location += 2;
    }
    // array 32 (0xdd)
    else if ((input_byte[byte_location] & 0xff) == 0xdd)
    {
        byte_location++;
        uint32_t uint32_value = input_byte[byte_location + 3];
        array_entries = (uint8_t)uint32_value;
        byte_location += 4;
    }
    else
    {
        return false;
    }

    while (byte_location < input_bytes)
    {
        // Because we don't know how MessagePack encoders will encode numbers,
        // we should support all int types that could be used to represent "int 32"
        // All non-string formats should be typecasted into "int 32" to simplify things.

        // positive fixint (0b0xxxxxxx)
        if ((input_byte[byte_location] & 0x80) == 0)
        {
            uint8_t fixint_value = input_byte[byte_location] & 0x7f;
            msgpack_data->int_32[int_32_location] = (uint32_t)fixint_value;
            int_32_location++;
            byte_location++;
        }
        // negative fixint (0b111xxxxx)
        else if ((input_byte[byte_location] & 0xe0) == 0xe0)
        {
            uint8_t fixint_value = input_byte[byte_location] & 0x1f;
            msgpack_data->int_32[int_32_location] = (uint32_t)(fixint_value - 32);
            int_32_location++;
            byte_location++;
        }
        // uint 8 (0xcc)
        else if ((input_byte[byte_location] & 0xff) == 0xcc)
        {
            byte_location++;
            uint8_t uint8_value = input_byte[byte_location];
            msgpack_data->int_32[int_32_location] = (int32_t)uint8_value;
            int_32_location++;
            byte_location += 1;
        }
        // int 8 (0xd0)
        else if ((input_byte[byte_location] & 0xff) == 0xd0)
        {
            byte_location++;
            int8_t int8_value = input_byte[byte_location];
            msgpack_data->int_32[int_32_location] = (int32_t)int8_value;
            int_32_location++;
            byte_location += 1;
        }
        // uint 16 (0xcd)
        else if ((input_byte[byte_location] & 0xff) == 0xcd)
        {
            byte_location++;
            uint16_t uint16_value = input_byte[byte_location + 1];
            msgpack_data->int_32[int_32_location] = (int32_t)uint16_value;
            int_32_location++;
            byte_location += 2;
        }
        // int 16 (0xd1)
        else if ((input_byte[byte_location] & 0xff) == 0xd1)
        {
            byte_location++;
            int16_t int16_value = input_byte[byte_location + 1];
            msgpack_data->int_32[int_32_location] = (int32_t)int16_value;
            int_32_location++;
            byte_location += 2;
        }
        // uint 32 (0xce)
        else if ((input_byte[byte_location] & 0xff) == 0xce)
        {
            byte_location++;
            uint32_t uint32_value = input_byte[byte_location + 3];
            msgpack_data->int_32[int_32_location] = (int32_t)uint32_value;
            int_32_location++;
            byte_location += 4;
        }
        // int 32 (0xd2)
        else if ((input_byte[byte_location] & 0xff) == 0xd2)
        {
            byte_location++;
            int32_t int32_value = input_byte[byte_location + 3];
            msgpack_data->int_32[int_32_location] = int32_value;
            int_32_location++;
            byte_location += 4;
        }

        // Because we don't know how MessagePack encoders will encode strings,
        // we should support all string types cable of representing a 16 entry string.

        // fixstr (0b101xxxxx)
        else if ((input_byte[byte_location] & 0b11100000) == 0b10100000)
        {
            uint8_t string_characters = (input_byte[byte_location] & 0b00011111);
            byte_location++;
            msgpack_data->str = input_byte + byte_location;
            msgpack_data->strLength = string_characters;
            byte_location += string_characters;
        }
        // str 8 (0xd9)
        else if ((input_byte[byte_location] & 0xff) == 0xd9)
        {
            byte_location++;
            uint8_t string_characters = input_byte[byte_location];
            byte_location += 1;
            msgpack_data->str = input_byte + byte_location;
            msgpack_data->strLength = string_characters;
            byte_location += string_characters;
        }
        // str 16 (0xda)
        else if ((input_byte[byte_location] & 0xff) == 0xda)
        {
            byte_location++;
            uint16_t string_characters = input_byte[byte_location + 1];
            byte_location += 2;
            msgpack_data->str = input_byte + byte_location;
            msgpack_data->strLength = (uint8_t)string_characters;
            byte_location += (uint8_t)string_characters;
        }
        // str 32 (0xdb)
        else if ((input_byte[byte_location] & 0xff) == 0xdb)
        {
            byte_location++;
            uint32_t string_characters = input_byte[byte_location + 3];
            byte_location += 4;
            msgpack_data->str = input_byte + byte_location;
            msgpack_data->strLength = (uint8_t)string_characters;
            byte_location += (uint8_t)string_characters;
        }
        else
        {
            return false;
        }
        array_counter++;
    }
    if (array_counter != array_entries)
    {
        return false;
    }

    return true;
}

int main()
{
    struct decoded_msgpack msgpack_data;

    bool success = decodething(input_bytes, input_byte, &msgpack_data);

    if (success)
    {
        switch ((uint8_t)msgpack_data.int_32[0])
        {
        case 'c':
            printf("command: %c\n", msgpack_data.int_32[0]);
            printf("priceTickOffset: %d\n", msgpack_data.int_32[1]);
            printf("priceTicks: %d\n", msgpack_data.int_32[2]);
            printf("orderBufferAllocationSize: %d\n", msgpack_data.int_32[3]);
            printf("orderBookId: %d\n", msgpack_data.int_32[4]);
            printf("name: %.16s\n", msgpack_data.str);
            break;
        case 'a':
            printf("command: %c\n", msgpack_data.int_32[0]);
            printf("userId: %d\n", msgpack_data.int_32[1]);
            printf("userReference: %d\n", msgpack_data.int_32[2]);
            printf("orderBookId: %d\n", msgpack_data.int_32[3]);
            printf("price: %d\n", msgpack_data.int_32[5]);
            printf("volume: %d\n", msgpack_data.int_32[6]);
            printf("timeToLive: %d\n", msgpack_data.int_32[7]);
            break;
        case 'd':
            printf("command: %c\n", msgpack_data.int_32[0]);
            printf("orderBookId: %d\n", msgpack_data.int_32[1]);
            break;
        case 'm':
            printf("command: %c\n", msgpack_data.int_32[0]);
            printf("userId: %d\n", msgpack_data.int_32[1]);
            printf("userReference: %d\n", msgpack_data.int_32[2]);
            printf("orderBookId: %d\n", msgpack_data.int_32[3]);
            printf("volume: %d\n", msgpack_data.int_32[5]);
            printf("timeToLive: %d\n", msgpack_data.int_32[6]);
            break;
        }
    }
}
