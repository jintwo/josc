#include "janet.h"

/* Some code is stolen from https://github.com/mhroth/tinyosc
 * Check their repo for more information
 */

static Janet is_bundle(int32_t argc, Janet *argv) {
        return janet_wrap_false();
}

static Janet parse_bundle(int32_t argc, Janet *argv) {return janet_wrap_true();}

void read_buffer_values(JanetArray *data, const char *format, const char *buffer, uint32_t buffer_len) {
        printf("!rbv\n");
        int ptr = 0;
        uint8_t n_args = strlen(format);
        printf("rbv-format-len: %d\n", n_args);
        for (int i = 0; i < n_args; i++) {
                printf("reading(%d): %c\n", ptr, format[i]);
                char *bptr = buffer + ptr;
                switch (format[i]) {
                case 'b': {
                        int b_len = (int) ntohl(*((uint32_t *) bptr));
                        // check end of buffer
                        if (bptr + 4 + b_len <= buffer + buffer_len) {
                                JanetBuffer *v = janet_buffer(b_len);
                                janet_buffer_push_bytes(v, bptr + 4, b_len);
                                janet_array_push(data, janet_wrap_buffer(v));
                                b_len = (b_len + 7) & ~0x3;
                                ptr += b_len;
                        } else {
                                janet_array_push(data, janet_wrap_nil());
                        }
                        break;
                }
                case 'f': {
                        const uint32_t v = ntohl(*((uint32_t *) bptr));
                        float f = *((float *) (&v));
                        janet_array_push(data, janet_wrap_number(f));
                        ptr += 4;
                        break;
                }
                case 'd': {
                        const uint64_t v = ntohll(*((uint64_t *) bptr));
                        double d = *((double *) (&v));
                        janet_array_push(data, janet_wrap_number(d));
                        ptr += 8;
                        break;
                }
                case 'i': {
                        const int32_t v = (int32_t) ntohl(*((uint32_t *) bptr));
                        janet_array_push(data, janet_wrap_integer(v));
                        ptr += 4;
                        break;
                }
                case 'm': {
                        janet_array_push(data, janet_wrap_string(janet_cstring(bptr)));
                        ptr += 4;
                        break;
                }
                /* case 't': */
                /* case 'h': { */
                /*         const uint64_t k = janet_getinteger64(argv, arg_n); */
                /*         janet_buffer_push_u64(buffer, htonll(k)); */
                /*         break; */
                /* } */
                case 's': {
                        janet_array_push(data, janet_wrap_string(janet_cstring(bptr)));
                        int s_len = strlen(bptr);
                        s_len = (s_len + 4) & ~0x3;
                        ptr += s_len;
                        break;
                }
                case 'T': {
                        janet_array_push(data, janet_wrap_true());
                        break;
                }
                case 'F': {
                        janet_array_push(data, janet_wrap_false());
                        break;
                }
                case 'N': {
                        janet_array_push(data, janet_wrap_nil());
                        break;
                }
                case 'I': // infinitum
                        break;
                default: {
                        continue;
                }
                }
        }
}

// TODO: fix unnecessary data copying
static Janet parse_message(int32_t argc, Janet *argv) {
        JanetBuffer *buffer = janet_getbuffer(argv, 0);

        int i = 0;

        while (buffer->data[i] != '\0') ++i;

        int address_len = i;
        char *address = (char *)malloc(address_len + 1);
        memset(address, 0, address_len + 1);
        strncpy(address, buffer->data, address_len + 1);

        printf("address(%d): %s\n", address_len, address);

        while (buffer->data[i] != ',') ++i;
        // format is empty
        if (i >= buffer->count) {
                return janet_wrap_nil();
        }

        int format_start_idx = i + 1; // there is comma

        while (i < buffer->count && buffer->data[i] != '\0') ++i;

        // format non zero-terminated
        if (i == buffer->count) {
                return janet_wrap_nil();
        }

        int format_len = i - format_start_idx;
        printf("format-len: %d\n", format_len);

        char *format = (char *)malloc(format_len + 1);
        memset(format, 0, format_len + 1);
        strncpy(format, buffer->data + format_start_idx, format_len + 1);

        printf("i = %d\n", i);

        JanetKV *result = janet_struct_begin(3);

        janet_struct_put(result,
                         janet_wrap_keyword(janet_cstring("address")),
                         janet_wrap_string(janet_cstring(address)));
        janet_struct_put(result,
                         janet_wrap_keyword(janet_cstring("format")),
                         janet_wrap_string(janet_cstring(format)));



        i = (i + 4) & ~0x3;

        printf("i = %d\n", i);

        uint32_t payload_length = buffer->count - i;

        char *payload_buffer = buffer->data + i;

        printf("payload-len: %d / %d\n", strlen(payload_buffer), payload_length);

        JanetArray *payload = janet_array(format_len);
        read_buffer_values(payload, format, payload_buffer, payload_length);

        janet_struct_put(result,
                         janet_wrap_keyword(janet_cstring("data")),
                         janet_wrap_array(payload));

        return janet_wrap_struct(result);

        /* i = (i + 4) & ~0x3; */
}

static Janet write_bundle(int32_t argc, Janet *argv) {return janet_wrap_true();}


uint32_t push_padding_bytes(JanetBuffer *buffer, uint32_t entry_len) {
        // TODO: looks too complicated!
        uint8_t pad_len = (((entry_len & ~0x3) + 4) - entry_len) % 4;
        printf("pad-len: %d\n", pad_len);
        char pad[4];
        memset(pad, 0, 4);
        janet_buffer_push_bytes(buffer, pad, pad_len);
}

void write_buffer_values(JanetBuffer *buffer, const char *format, Janet *argv, uint8_t first_arg) {
        uint8_t n_args = strlen(format);
        for (int i = 0; i < n_args; i++) {
                uint8_t arg_n = i + first_arg;
                switch (format[i]) {
                case 'b': {
                        // TODO: fixme! invalid blob serialization
                        const char *blob = janet_getcstring(argv, arg_n);
                        uint32_t b_len = strlen(blob);
                        printf("blob-len: %d\n", b_len);
                        janet_buffer_push_u32(buffer, htonl(b_len));
                        janet_buffer_push_cstring(buffer, blob);
                        push_padding_bytes(buffer, b_len + 4);
                        break;
                }
                case 'f': {
                        float f = (float) janet_getnumber(argv, arg_n);
                        janet_buffer_push_u32(buffer, htonl(*((uint32_t *) &f)));
                        break;
                }
                case 'd': {
                        double f = janet_getnumber(argv, arg_n);
                        janet_buffer_push_u64(buffer, htonll(*((uint64_t *) &f)));
                        break;
                }
                case 'i': {
                        uint32_t k = janet_getinteger(argv, arg_n);
                        printf("arg<i> = %d\n", k);
                        janet_buffer_push_u32(buffer, htonl(k));
                        break;
                }
                case 'm': {
                        const char *k = janet_getcstring(argv, arg_n);
                        janet_buffer_push_bytes(buffer, k, 4);
                        break;
                }
                case 't':
                case 'h': {
                        const uint64_t k = janet_getinteger64(argv, arg_n);
                        janet_buffer_push_u64(buffer, htonll(k));
                        break;
                }
                case 's': {
                        const char *str = janet_getcstring(argv, arg_n);
                        janet_buffer_push_cstring(buffer, str);
                        push_padding_bytes(buffer, strlen(str));
                        break;
                }
                case 'T': // true
                case 'F': // false
                case 'N': // nil
                case 'I': // infinitum
                        break;
                default: {
                        printf("[!] unknown type\n");
                        return; // unknown type
                }
                }
        }
        /* push_padding_bytes(buffer); */
        printf("buf-len: %d\n", buffer->count);
}

static Janet write_message(int32_t argc, Janet *argv) {
        JanetBuffer *buffer = janet_buffer(2048);

        // write address
        const char *address = janet_getcstring(argv, 0);
        janet_buffer_push_cstring(buffer, address);
        janet_buffer_push_u8(buffer, 0);
        push_padding_bytes(buffer, strlen(address) + 1);

        // write format
        const char *format = janet_getcstring(argv, 1);
        janet_buffer_push_cstring(buffer, ",");
        janet_buffer_push_cstring(buffer, format);
        janet_buffer_push_u8(buffer, 0);
        push_padding_bytes(buffer, strlen(format) + 2);

        write_buffer_values(buffer, format, argv, 2);
        return janet_wrap_buffer(buffer);
}

static const JanetReg cfuns[] = {
    {"is-bundle", is_bundle, "Returns true if the buffer refers to a bundle of OSC messages. False otherwise."},
    {"parse-bundle", parse_bundle, "Reads a buffer containing a bundle of OSC messages."},
    {"parse-message", parse_message, "Reads a buffer containing a OSC message."},
    {"write-bundle", write_bundle, "write_bundle"},
    {"write-message", write_message, "Writes an OSC packet to a buffer. Returns the total number of bytes written."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "josc", cfuns);
}
