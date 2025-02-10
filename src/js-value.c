/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "js.h"

// plen can be NULL
char *read_file(const char *filename, size_t *plen) {
    // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
    char *str = NULL;
    long len_allocated = 0;
    size_t len_read = 0;
    FILE *fp = fopen(filename, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len_allocated = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        str = (char *)calloc(len_allocated + 1, 1);
        if (str) {
            len_read = fread(str, 1, len_allocated, fp);
        }
        fclose(fp);
    }
    if (str && plen) {
        *plen = len_read;
    }
    // "r" mode will strip cr lf characters, so bytes read may be shorter than file size
    // if pass len_allocated to lexer, will cause "Invalid characters" because will meet lot's of '\0' in the end
    log("\"%s\", allocated %ld bytes, read %llu bytes", filename, len_allocated, len_read);
    return str;
}

// plen can be NULL
char *read_line(FILE *fp, size_t *plen) {
    char *str = NULL;
    size_t len = 0;
    size_t cap = 0;
    int ch;
    for (;;) {
        ch = fgetc(fp);
        if (ch == EOF || ch == '\n') {
            break;
        }
        string_buffer_append_ch(str, len, cap, ch);
    }
    if (len == 0 && (ferror(fp) || feof(fp))) {
        // when eof/error, read again, return NULL instead of empty string to indicate finished
        free(str);
        str = NULL;
    }
    if (plen) {
        *plen = len;
    }
    return str;
}

// for debug print characters
const char *ascii_abbreviation(int ch) {
    static const char *const repr[] = {
        "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
        "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US",
        "SP", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
        "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
        "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
        "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "DEL"};
    if (ch == EOF) {
        return "EOF";
    } else if (ch >= 0 && ch < countof(repr)) {
        return repr[ch];
    } else {
        return "";
    }
}

static const size_t _segs_per_line = 2;
static const size_t _bytes_per_seg = 8;

static unsigned char _printable(unsigned char ch) {
    return ch >= 0x20 && ch <= 0x7e ? ch : ' ';
}

void print_hex(void *p, size_t len) {
    size_t bytes_per_line = _segs_per_line * _bytes_per_seg;
    unsigned char *base = (unsigned char *)p;
    size_t loffs;
    for (loffs = 0; loffs <= len; loffs += bytes_per_line) { // <= make sure when len=0 also print a line
        int what;
        printf("%8llu:  ", loffs);
        for (what = 0; what <= 1; what++) { // 0 hex, 1 ascii
            size_t seg;
            for (seg = 0; seg < _segs_per_line; seg++) {
                size_t boffs;
                for (boffs = 0; boffs < _bytes_per_seg; boffs++) {
                    size_t offs = loffs + seg * _bytes_per_seg + boffs;
                    if (offs < len) {
                        // const unsigned char *fmt = what == 0 ? "%02x " : "%c";
                        // unsigned char ch = what == 0 ? base[offs] : _printable(base[offs]);
                        // printf(fmt, ch);
                        if (what == 0) {
                            unsigned char ch = base[offs];
                            if (ch == 0) {
                                printf(".. ");
                            } else {
                                printf("%02x ", ch);
                            }
                        } else {
                            printf("%c", _printable(base[offs]));
                        }
                    } else {
                        printf(what == 0 ? "   " : " ");
                    }
                }
                printf(what == 0 ? " " : "");
            }
            printf(" ");
        }
        printf("\n");
    }
}

void link_push(struct link_head *ptr, size_t *len, struct link_head *val) {
    val->owner = ptr;
    val->next = ptr->next;
    val->prev = ptr;
    if (ptr->next) {
        ptr->next->prev = val;
    }
    ptr->next = val;
    (*len)++;
}

void link_remove(struct link_head *ptr, size_t *len, struct link_head *val) {
    if (val->owner != ptr) {
        return;
    }
    if (val->next) {
        val->next->prev = val->prev;
    }
    if (val->prev) {
        val->prev->next = val->next;
    }
    (*len)--;
}

void link_dump(struct link_head *ptr, size_t *len) {
    printf("len=%lld\n", *len);
    link_for_each(ptr, val, printf("    %p-> {next=%p, prev=%p}\n", val, val->next, val->prev));
}

static size_t _fh(const char *s, size_t slen, size_t mask) {
    size_t hash = 0;
    size_t i;
    for (i = 0; i < slen; i++) {
        hash = (hash + (hash << 4) + s[i]) & mask;
    }
    return hash;
}

static size_t _nh(size_t hash, size_t mask) {
    // hash = hash * 5 + 1 can perfectly cover 0-2 pow
    // https://tieba.baidu.com/p/8968552557
    return (hash + (hash << 4) + 1) & mask;
}

static struct js_key_value *_find_empty(struct js_key_value *p, size_t cap, const char *key, size_t klen) {
    size_t mask = cap - 1;
    size_t hash;
    size_t rep;
    for (rep = 0, hash = _fh(key, klen, mask); rep < cap; rep++, hash = _nh(hash, mask)) {
        struct js_key_value *node = p + hash;
        // log("rep=%d hash=%d", rep, hash);
        if (!node->k.p && !node->v.type) {
            return node;
        }
    }
    fatal("Whole loop ended, this shouldn't happen");
}

void js_value_map_put(struct js_key_value **p, size_t *len, size_t *cap, const char *key, size_t klen, struct js_value val) {
    size_t mask;
    size_t hash;
    size_t rep;
    bool next_stage = false;
    struct js_key_value *rec_pos;
    if (!*p) { // first allocate
        buffer_alloc(struct js_key_value, *p, *len, *cap, 2);
    }
    mask = (*cap) - 1;
    for (rep = 0, hash = _fh(key, klen, mask); rep < *cap; rep++, hash = _nh(hash, mask)) {
        struct js_key_value *node = *p + hash;
        // log("rep=%d hash=%d", rep, hash);
        if (node->k.p == NULL) { // k is empty
            if (node->v.type != 0) { // v not empty
                fatal("Key is NULL but value not NULL, this shouldn't happen");
            } else {
                if (val.type != 0) {
                    if (next_stage) {
                        goto next_stage_done;
                    } else {
                        size_t newlen = *len + 1;
                        size_t reqcap = newlen << 1;
                        if (*cap >= reqcap) {
                            string_buffer_append(node->k.p, node->k.len, node->k.cap, key, klen);
                            node->v = val;
                            *len = newlen;
                            // printf("++ *len=%llu\n", *len);
                            return;
                        } else {
                            // rehash
                            struct js_key_value *newp = NULL;
                            size_t newlen = 0;
                            size_t newcap = 0;
                            size_t i;
                            buffer_alloc(struct js_key_value, newp, newlen, newcap, reqcap);
                            for (i = 0; i < *cap; i++) {
                                node = *p + i;
                                if (node->k.p && node->v.type) {
                                    struct js_key_value *newnode = _find_empty(newp, newcap, node->k.p, node->k.len);
                                    *newnode = *node;
                                    newlen++;
                                }
                            }
                            node = _find_empty(newp, newcap, key, klen);
                            string_buffer_append(node->k.p, node->k.len, node->k.cap, key, klen);
                            node->v = val;
                            newlen++;
                            free(*p);
                            *p = newp;
                            *len = newlen;
                            *cap = newcap;
                            return;
                        }
                    }
                } else {
                    return;
                }
            }
        } else if (node->k.len == klen && memcmp(node->k.p, key, klen) == 0) { // matched
            if (node->v.type != 0) {
                if (val.type == 0) {
                    (*len)--;
                    // printf("-- *len=%llu\n", *len);
                }
                node->v = val;
            } else {
                if (val.type != 0) {
                    (*len)++;
                    // printf("++ *len=%llu\n", *len);
                    node->v = val;
                }
            }
            return;
        } else { // k not matched
            if (next_stage) {
                continue;
            } else if (node->v.type != 0) {
                continue;
            } else {
                rec_pos = node;
                next_stage = true;
                // string_buffer_clear(node->k.p, node->k.len, node->k.cap);
                // string_buffer_append(node->k.p, node->k.len, node->k.cap, key, klen);
                // if (val.type != 0) {
                //     node->v = val;
                //     (*len)++;
                //     // printf("++ *len=%llu\n", *len);
                // }
                // return;
            }
        }
    }
    if (next_stage) {
        goto next_stage_done;
    }
    js_value_map_dump(*p, *len, *cap);
    fatal("Whole loop ended, this shouldn't happen");
next_stage_done:
    string_buffer_clear(rec_pos->k.p, rec_pos->k.len, rec_pos->k.cap);
    string_buffer_append(rec_pos->k.p, rec_pos->k.len, rec_pos->k.cap, key, klen);
    rec_pos->v = val;
    (*len)++;
    // printf("++ *len=%llu\n", *len);
}

void js_value_map_put_sz(struct js_key_value **p, size_t *len, size_t *cap, const char *key, struct js_value val) {
    js_value_map_put(p, len, cap, key, strlen(key), val);
}

// v can be NULL
struct js_value js_value_map_get(struct js_key_value *p, size_t cap, const char *key, size_t klen) {
    size_t mask = cap - 1;
    size_t hash;
    size_t rep;
    for (rep = 0, hash = _fh(key, klen, mask); rep < cap; rep++, hash = _nh(hash, mask)) {
        struct js_key_value *node = p + hash;
        // printf("key=%.*s hash=%llu\n", klen, key, hash);
        if (node->k.p == NULL) {
            // printf("k is NULL\n");
            return js_undefined();
        } else if (node->k.len == klen && memcmp(node->k.p, key, klen) == 0) {
            // printf("k matched\n");
            return node->v;
        }
    }
    // printf("Whole loop ended\n");
    return js_undefined();
}

struct js_value js_value_map_get_sz(struct js_key_value *p, size_t cap, const char *key) {
    return js_value_map_get(p, cap, key, strlen(key));
}

void js_value_map_free(struct js_key_value **p, size_t *len, size_t *cap) {
    size_t i;
    for (i = 0; i < *cap; i++) {
        struct js_key_value *node = *p + i;
        if (node->k.p) {
            free(node->k.p);
        }
    }
    buffer_free(struct js_key_value, *p, *len, *cap);
}
void js_value_map_dump(struct js_key_value *p, size_t len, size_t cap) {
    size_t i;
    printf("len=%llu cap=%llu\n", len, cap);
    for (i = 0; i < cap; i++) {
        struct js_key_value *node = p + i;
        printf("    %llu %.*s %s\n", i, (int)node->k.len, node->k.p, js_value_type_name(node->v.type));
    }
}

struct js_value js_undefined() {
    struct js_value ret;
    ret.type = vt_undefined;
    return ret;
}

struct js_value js_null() {
    struct js_value ret;
    ret.type = vt_null;
    return ret;
}

struct js_value js_boolean(bool b) {
    struct js_value ret;
    ret.type = vt_boolean;
    ret.value.boolean = b;
    return ret;
}

struct js_value js_number(double d) {
    struct js_value ret;
    ret.type = vt_number;
    ret.value.number = d;
    return ret;
}

struct js_value js_scripture(const char *p, size_t len) {
    struct js_value ret;
    ret.type = vt_scripture;
    ret.value.scripture.p = p;
    ret.value.scripture.len = len;
    return ret;
}

struct js_value js_scripture_sz(const char *sz) {
    return js_scripture(sz, strlen(sz));
}

struct js_value js_inscription(size_t h, size_t len) {
    // append to pjs->tablet
    struct js_value ret;
    ret.type = vt_inscription;
    ret.value.inscription.h = h;
    ret.value.inscription.len = len;
    return ret;
}

struct js_value js_string(struct js *pjs, const char *str, size_t len) {
    struct js_value ret;
    ret.type = vt_string;
    ret.value.string = alloc(struct js_managed_string, 1);
    ret.value.string->h.type = vt_string;
    string_buffer_append(ret.value.string->p, ret.value.string->len, ret.value.string->cap, str, len);
    link_push(&(pjs->heap), &(pjs->heap_len), (struct link_head *)ret.value.string);
    return ret;
}

struct js_value js_string_sz(struct js *pjs, const char *str) {
    return js_string(pjs, str, strlen(str));
}

struct js_value js_array(struct js *pjs) {
    struct js_value ret;
    ret.type = vt_array;
    ret.value.array = alloc(struct js_managed_array, 1);
    ret.value.array->h.type = vt_array;
    link_push(&(pjs->heap), &(pjs->heap_len), (struct link_head *)ret.value.array);
    return ret;
}

void js_array_push(struct js *pjs, struct js_value *arr, struct js_value val) {
    buffer_push(struct js_value, arr->value.array->p, arr->value.array->len, arr->value.array->cap, val.type == vt_null ? js_undefined() : val);
}

void js_array_put(struct js *pjs, struct js_value *arr, size_t idx, struct js_value val) {
    if (val.type == vt_null) { // special treat to prevent useless expand
        if (idx < arr->value.array->len) {
            arr->value.array->p[idx].type = vt_undefined;
        } // else do nothing
    } else {
        buffer_put(struct js_value, arr->value.array->p, arr->value.array->len, arr->value.array->cap, idx, val);
    }
}

struct js_value js_array_get(struct js *pjs, struct js_value *arr, size_t idx) {
    if (idx < arr->value.array->len) {
        struct js_value ret = arr->value.array->p[idx];
        return ret.type == 0 ? js_null() : ret;
    } else {
        return js_null();
    }
}

struct js_value js_object(struct js *pjs) {
    struct js_value ret;
    ret.type = vt_object;
    ret.value.object = alloc(struct js_managed_object, 1);
    ret.value.object->h.type = vt_object;
    link_push(&(pjs->heap), &(pjs->heap_len), (struct link_head *)ret.value.object);
    return ret;
}

void js_object_put(struct js *pjs, struct js_value *obj, const char *key, size_t klen, struct js_value val) {
    js_value_map_put(&(obj->value.object->p), &(obj->value.object->len), &(obj->value.object->cap), key, klen, val.type == vt_null ? js_undefined() : val);
}

void js_object_put_sz(struct js *pjs, struct js_value *obj, const char *key, struct js_value val) {
    js_object_put(pjs, obj, key, strlen(key), val);
}

struct js_value js_object_get(struct js *pjs, struct js_value *obj, const char *key, size_t klen) {
    struct js_value ret;
    ret = js_value_map_get(obj->value.object->p, obj->value.object->cap, key, klen);
    return ret.type == 0 ? js_null() : ret;
}

struct js_value js_object_get_sz(struct js *pjs, struct js_value *obj, const char *key) {
    return js_object_get(pjs, obj, key, strlen(key));
}

struct js_value js_function(struct js *pjs, size_t idx) {
    struct js_value ret;
    ret.type = vt_function;
    ret.value.function = alloc(struct js_managed_function, 1);
    ret.value.function->h.type = vt_function;
    ret.value.function->index = idx;
    link_push(&(pjs->heap), &(pjs->heap_len), (struct link_head *)ret.value.function);
    return ret;
}

struct js_value js_c_function(struct js *pjs, void (*cfunc)(struct js *)) {
    struct js_value ret;
    ret.type = vt_c_function;
    ret.value.c_function = cfunc;
    return ret;
}

const char *js_value_type_name(enum js_value_type type) {
    static const char *const names[] = {"undefined", "null", "boolean", "number", "string", "string", "string", "array", "object", "function", "function"};
    if (type >= 0 && type < countof(names)) {
        return names[type];
    } else {
        return "???";
    }
}

void js_value_dump(struct js *pjs, struct js_value val) {
    switch (val.type) {
    case vt_undefined:
        printf("undefined");
        break;
    case vt_null:
        printf("null");
        break;
    case vt_boolean:
        printf(val.value.boolean ? "true" : "false");
        break;
    case vt_number:
        printf("%lg", val.value.number);
        break;
    case vt_scripture:
        printf("\"\"\"%.*s\"\"\"", (int)val.value.scripture.len, val.value.scripture.p);
        break;
    case vt_inscription:
        printf("\"\"%.*s\"\"", (int)val.value.inscription.len, pjs->tablet + val.value.inscription.h);
        break;
    case vt_string:
        printf("\"%.*s\"", (int)val.value.string->len, val.value.string->p);
        break;
    case vt_array: {
        printf("[");
        js_value_list_for_each(val.value.array->p, val.value.array->len, i, v, {
            printf("%llu:", i);
            js_value_dump(pjs, v);
            printf(",");
        });
        printf("]");
        break;
    }
    case vt_object: {
        printf("{");
        js_value_map_for_each(val.value.object->p, val.value.object->cap, k, kl, v, {
            printf("%.*s:", (int)kl, k);
            js_value_dump(pjs, v);
            printf(",");
        });
        printf("}");
        break;
    }
    case vt_function:
        printf("<function %llu {", val.value.function->index);
        js_value_map_for_each(val.value.function->closure.p, val.value.function->closure.cap, k, kl, v, {
            printf("%.*s:", (int)kl, k);
            js_value_dump(pjs, v);
            printf(",");
        });
        printf("}>");
        break;
    case vt_c_function:
        printf("<c_function>");
        break;
    default:
        fatal("Unknown value type %d", val.type);
    }
}
