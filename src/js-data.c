/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "js-data.h"

#define X(name) #name,
static const char *const _value_type_names[] = {js_value_type_list};
#undef X

void js_map_dump(struct js_kv_pair *base, size_t length, size_t capacity) {
    size_t i;
    printf("length=%zu capacity=%zu\n", length, capacity);
    for (i = 0; i < capacity; i++) {
        struct js_kv_pair *node = base + i;
        printf("    %zu %.*s %s\n", i, (int)node->key.length, node->key.base, _value_type_names[node->value.type]);
    }
}

static size_t _first_hash(const char *string, uint16_t length, size_t mask) {
    size_t hash = 0;
    for (uint16_t i = 0; i < length; i++) {
        hash = (hash + (hash << 4) + string[i]) & mask;
    }
    return hash;
}

static size_t _next_hash(size_t hash, size_t mask) {
    // hash = hash * 5 + 1 can perfectly cover 0-2 pow
    // https://tieba.baidu.com/p/8968552557
    return (hash + (hash << 4) + 1) & mask;
}

static struct js_kv_pair *_find_empty(struct js_kv_pair *base, size_t capacity, const char *key, uint16_t key_length) {
    size_t mask = capacity - 1;
    for (size_t repeat = 0, hash = _first_hash(key, key_length, mask); repeat < capacity; repeat++, hash = _next_hash(hash, mask)) {
        struct js_kv_pair *node = base + hash;
        // js_info("repeat=%d hash=%d", repeat, hash);
        if (!node->key.base && !node->value.type) {
            return node;
        }
    }
    fatal("Whole loop ended, this shouldn't happen");
}

// BUG: Stage 2 should also check rehash
// void js_map_put(struct js_kv_pair **base, size_t *length, size_t *capacity, const char *key, uint16_t key_length, struct js_value value) {
//     // js_map_dump(*base, *length, *capacity);
//     // printf("key=%.*s, value=%s\n", key_length, key, _value_type_names[value.type]);
//     bool next_stage = false;
//     struct js_kv_pair *recorded;
//     if (!*base) { // first allocate
//         buffer_alloc(*base, *length, *capacity, 2);
//     }
//     size_t mask = (*capacity) - 1;
//     for (size_t repeat = 0, hash = _first_hash(key, key_length, mask); repeat < *capacity; repeat++, hash = _next_hash(hash, mask)) {
//         struct js_kv_pair *node = *base + hash;
//         // js_info("repeat=%d hash=%d", repeat, hash);
//         if (node->key.base == NULL) { // k is empty
//             if (node->value.type != 0) { // v not empty
//                 fatal("Key is NULL but value not NULL, this shouldn't happen");
//             } else {
//                 if (value.type != 0) {
//                     if (next_stage) {
//                         goto next_stage_done;
//                     } else {
//                         size_t newlen = *length + 1;
//                         size_t reqcap = newlen << 1;
//                         if (*capacity >= reqcap) {
//                             string_buffer_append(node->key.base, node->key.length, node->key.capacity, key, key_length);
//                             node->value = value;
//                             *length = newlen;
//                             // printf("++ *length=%zu\n", *length);
//                             return;
//                         } else {
//                             // rehash
//                             struct js_kv_pair *newbase = NULL;
//                             size_t newlen = 0;
//                             size_t newcap = 0;
//                             size_t i;
//                             buffer_alloc(newbase, newlen, newcap, reqcap);
//                             for (i = 0; i < *capacity; i++) {
//                                 node = *base + i;
//                                 if (node->key.base && node->value.type) {
//                                     struct js_kv_pair *newnode = _find_empty(newbase, newcap, node->key.base, node->key.length);
//                                     *newnode = *node;
//                                     newlen++;
//                                 }
//                             }
//                             node = _find_empty(newbase, newcap, key, key_length);
//                             string_buffer_append(node->key.base, node->key.length, node->key.capacity, key, key_length);
//                             node->value = value;
//                             newlen++;
//                             free(*base);
//                             *base = newbase;
//                             *length = newlen;
//                             *capacity = newcap;
//                             return;
//                         }
//                     }
//                 } else {
//                     return;
//                 }
//             }
//         } else if (node->key.length == key_length && memcmp(node->key.base, key, key_length) == 0) { // matched
//             if (node->value.type != 0) {
//                 if (value.type == 0) {
//                     (*length)--;
//                     // printf("-- *length=%zu\n", *length);
//                 }
//                 node->value = value;
//             } else {
//                 if (value.type != 0) {
//                     (*length)++;
//                     // printf("++ *length=%zu\n", *length);
//                     node->value = value;
//                 }
//             }
//             return;
//         } else { // k not matched
//             if (next_stage) {
//                 continue;
//             } else if (node->value.type != 0) {
//                 continue;
//             } else {
//                 recorded = node;
//                 next_stage = true;
//                 // string_buffer_clear(node->key.base, node->key.length, node->key.capacity);
//                 // string_buffer_append(node->key.base, node->key.length, node->key.capacity, key, key_length);
//                 // if (value.type != 0) {
//                 //     node->value = value;
//                 //     (*length)++;
//                 //     // printf("++ *length=%zu\n", *length);
//                 // }
//                 // return;
//             }
//         }
//     }
//     if (next_stage) {
//         goto next_stage_done;
//     }
//     fatal("Whole loop ended, this shouldn't happen");
// next_stage_done:
//     string_buffer_clear(recorded->key.base, recorded->key.length, recorded->key.capacity);
//     string_buffer_append(recorded->key.base, recorded->key.length, recorded->key.capacity, key, key_length);
//     recorded->value = value;
//     (*length)++;
//     // printf("++ *length=%zu\n", *length);
// }

// BUGFIX: always check rehash
void js_map_put_internal(struct js_kv_pair **base, size_t *length, size_t *capacity, const char *key, uint16_t key_length, struct js_value value) {
    // js_map_dump(*base, *length, *capacity);
    // printf("key=%.*s, value=%s\n", key_length, key, _value_type_names[value.type]);
    bool next_stage = false;
    struct js_kv_pair *recorded;
    if (!*base) { // first allocate
        buffer_alloc(*base, *length, *capacity, 2);
    }
    size_t mask = (*capacity) - 1;
    struct js_kv_pair *node;
    for (size_t repeat = 0, hash = _first_hash(key, key_length, mask); repeat < *capacity; repeat++, hash = _next_hash(hash, mask)) {
        node = *base + hash;
        // js_info("repeat=%d hash=%d", repeat, hash);
        if (node->key.base == NULL) { // k is empty
            if (node->value.type != 0) { // v not empty
                fatal("Key is NULL but value not NULL, this shouldn't happen");
            } else {
                if (value.type != 0) {
                    if (next_stage) {
                        goto next_stage_done;
                    } else {
                        string_buffer_append(node->key.base, node->key.length, node->key.capacity, key, key_length);
                        node->value = value;
                        (*length)++;
                        // printf("++ *length=%zu\n", *length);
                        goto check_rehash;
                    }
                } else {
                    return;
                }
            }
        } else if (node->key.length == key_length && memcmp(node->key.base, key, key_length) == 0) { // matched
            if (node->value.type != 0) {
                if (value.type == 0) {
                    (*length)--;
                    // printf("-- *length=%zu\n", *length);
                }
                node->value = value;
                return;
            } else {
                if (value.type != 0) {
                    (*length)++;
                    // printf("++ *length=%zu\n", *length);
                    node->value = value;
                }
                goto check_rehash;
            }
        } else { // k not matched
            if (next_stage) {
                continue;
            } else if (node->value.type != 0) {
                continue;
            } else {
                recorded = node;
                next_stage = true;
                // string_buffer_clear(node->key.base, node->key.length, node->key.capacity);
                // string_buffer_append(node->key.base, node->key.length, node->key.capacity, key, key_length);
                // if (value.type != 0) {
                //     node->value = value;
                //     (*length)++;
                //     // printf("++ *length=%zu\n", *length);
                // }
                // return;
            }
        }
    }
    if (next_stage) {
        goto next_stage_done;
    }
    fatal("Whole loop ended, this shouldn't happen");
next_stage_done:
    string_buffer_clear(recorded->key.base, recorded->key.length, recorded->key.capacity);
    string_buffer_append(recorded->key.base, recorded->key.length, recorded->key.capacity, key, key_length);
    recorded->value = value;
    (*length)++;
    // printf("++ *length=%zu\n", *length);
check_rehash:
    size_t reqcap = *length << 1;
    if (*capacity < reqcap) {
        // rehash
        struct js_kv_pair *newbase = NULL;
        size_t newlen = 0;
        size_t newcap = 0;
        size_t i;
        buffer_alloc(newbase, newlen, newcap, reqcap);
        for (i = 0; i < *capacity; i++) {
            node = *base + i;
            if (node->key.base) {
                if (node->value.type) {
                    struct js_kv_pair *newnode = _find_empty(newbase, newcap, node->key.base, node->key.length);
                    *newnode = *node;
                    newlen++;
                } else {
                    free(node->key.base);
                }
            }
        }
        free(*base);
        *base = newbase;
        *length = newlen;
        *capacity = newcap;
    }
}

// void js_map_put_sz(struct js_kv_pair **base, size_t *length, size_t *capacity, const char *key, struct js_value value) {
//     js_map_put(base, length, capacity, key, (uint16_t)strlen(key), value);
// }

// v can be NULL
struct js_value js_map_get(struct js_kv_pair *base, size_t length, size_t capacity, const char *key, uint16_t key_length) {
    size_t mask = capacity - 1;
    size_t hash;
    size_t repeat;
    for (repeat = 0, hash = _first_hash(key, key_length, mask); repeat < capacity; repeat++, hash = _next_hash(hash, mask)) {
        struct js_kv_pair *node = base + hash;
        // printf("key=%.*s hash=%zu\n", key_length, key, hash);
        if (node->key.base == NULL) {
            // printf("k is NULL\n");
            return (struct js_value){0};
        } else if (node->key.length == key_length && memcmp(node->key.base, key, key_length) == 0) {
            // printf("k matched\n");
            return node->value;
        }
    }
    // log_info("Whole loop ended");
    return (struct js_value){0};
}

struct js_value js_map_get_sz(struct js_kv_pair *base, size_t length, size_t capacity, const char *key) {
    return js_map_get(base, length, capacity, key, (uint16_t)strlen(key));
}

// void js_map_free_internal(struct js_kv_pair **base, size_t *length, size_t *capacity) {
//     for (size_t i = 0; i < *capacity; i++) {
//         struct js_kv_pair *node = *base + i;
//         if (node->key.base) {
//             free(node->key.base);
//         }
//     }
//     buffer_free(*base, *length, *capacity);
// }

struct js_value js_null() {
    return (struct js_value){.type = vt_null};
}

struct js_value js_boolean(bool boolean) {
    return (struct js_value){.type = vt_boolean, .boolean = boolean};
}

struct js_value js_number(double number) {
    return (struct js_value){.type = vt_number, .number = number};
}

struct js_value js_scripture(const char *base, uint32_t length) {
    return (struct js_value){.type = vt_scripture, .scripture.base = (char *)base, .scripture.length = length};
}

struct js_value js_scripture_sz(const char *sz) {
    return js_scripture(sz, (uint32_t)strlen(sz));
}

struct js_value js_inscription(uint8_t **base, uint32_t offset, uint32_t length) {
    return (struct js_value){.type = vt_inscription, .inscription.base = base, .inscription.offset = offset, .inscription.length = length};
}

struct js_value js_string(struct js_heap *heap, const char *str, size_t slen) {
    struct js_value ret = {.type = vt_string};
    ret.managed = alloc(struct js_managed_value, 1);
    ret.managed->type = vt_string;
    string_buffer_append(ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity, str, slen);
    buffer_push(heap->base, heap->length, heap->capacity, ret.managed);
    return ret;
}

struct js_value js_string_sz(struct js_heap *heap, const char *str) {
    return js_string(heap, str, strlen(str));
}

struct js_value js_array(struct js_heap *heap) {
    struct js_value ret = {.type = vt_array};
    ret.managed = alloc(struct js_managed_value, 1);
    ret.managed->type = vt_array;
    buffer_push(heap->base, heap->length, heap->capacity, ret.managed);
    return ret;
}

void js_array_push(struct js_value *container, struct js_value element) {
    buffer_push(container->managed->array.base, container->managed->array.length, container->managed->array.capacity, element.type == vt_null ? (struct js_value){0} : element);
}

void js_array_put(struct js_value *container, size_t index, struct js_value element) {
    if (element.type == vt_null) { // special treat to prevent useless expand
        if (index < container->managed->array.length) {
            container->managed->array.base[index].type = 0;
        } // else do nothing
    } else {
        buffer_put(container->managed->array.base, container->managed->array.length, container->managed->array.capacity, index, element);
    }
}

struct js_value js_array_get(struct js_value *container, size_t index) {
    if (index < container->managed->array.length) {
        struct js_value ret = container->managed->array.base[index];
        return ret.type == 0 ? js_null() : ret;
    } else {
        return js_null();
    }
}

struct js_value js_object(struct js_heap *heap) {
    struct js_value ret = {.type = vt_object};
    ret.managed = alloc(struct js_managed_value, 1);
    ret.managed->type = vt_object;
    buffer_push(heap->base, heap->length, heap->capacity, ret.managed);
    return ret;
}

void js_object_put(struct js_value *container, const char *key, uint16_t key_length, struct js_value element) {
    js_map_put(container->managed->object.base, container->managed->object.length, container->managed->object.capacity, key, key_length, element.type == vt_null ? (struct js_value){0} : element);
}

void js_object_put_sz(struct js_value *container, const char *key, struct js_value element) {
    js_object_put(container, key, (uint16_t)strlen(key), element);
}

struct js_value js_object_get(struct js_value *container, const char *key, uint16_t key_length) {
    struct js_value ret;
    ret = js_map_get(container->managed->object.base, container->managed->object.length, container->managed->object.capacity, key, key_length);
    return ret.type == 0 ? js_null() : ret;
}

struct js_value js_object_get_sz(struct js_value *container, const char *key) {
    return js_object_get(container, key, (uint16_t)strlen(key));
}

struct js_value js_function(struct js_heap *heap, uint32_t ingress) {
    struct js_value ret = {.type = vt_function};
    ret.managed = alloc(struct js_managed_value, 1);
    ret.managed->type = vt_function;
    ret.managed->function.ingress = ingress;
    buffer_push(heap->base, heap->length, heap->capacity, ret.managed);
    return ret;
}

struct js_value js_c_function(void *c_function) {
    return (struct js_value){.type = vt_c_function, .c_function = c_function};
}

void js_mark(struct js_value *value) {
    // printf("js_mark: ");
    // js_value_dump(pjs, value);
    // printf("\n");
    switch (value->type) {
    case vt_string:
        value->managed->in_use = 1;
        break;
    case vt_array:
        if (!value->managed->in_use) {
            value->managed->in_use = 1;
            buffer_for_each(value->managed->array.base, value->managed->array.length, _, i, v, {
                // https://stackoverflow.com/questions/1486904/how-do-i-best-silence-a-warning-about-unused-variables
                (void)i;
                js_mark(v);
            });
        }
        break;
    case vt_object:
        if (!value->managed->in_use) {
            value->managed->in_use = 1;
            js_map_for_each(value->managed->object.base, _, value->managed->object.capacity, k, kl, v, {
                (void)k;
                (void)kl;
                js_mark(v);
            });
        }
        break;
    case vt_function:
        if (!value->managed->in_use) {
            value->managed->in_use = 1;
            js_map_for_each(value->managed->function.closure.base, _, value->managed->function.closure.capacity, k, kl, v, {
                (void)k;
                (void)kl;
                js_mark(v);
            });
        }
        break;
    default:
        // puts("skip");
        break;
    }
}

void _free_managed(struct js_managed_value *managed) {
    switch (managed->type) {
    case vt_string:
        buffer_free(managed->string.base, managed->string.length, managed->string.capacity);
        free(managed);
        break;
    case vt_array:
        buffer_free(managed->array.base, managed->array.length, managed->array.capacity);
        free(managed);
        break;
    case vt_object: {
        js_map_free(managed->object.base, managed->object.length, managed->object.capacity);
        free(managed);
        break;
    }
    case vt_function: {
        js_map_free(managed->function.closure.base, managed->function.closure.length, managed->function.closure.capacity);
        free(managed);
        break;
    }
    default:
        fatal("Illegal managed type \"%u\"", managed->type);
        break;
    }
}

void js_sweep(struct js_heap *heap) {
    struct js_managed_value **new_base = NULL;
    size_t new_length = 0;
    size_t new_capacity = 0;
    buffer_for_each(heap->base, heap->length, heap->capacity, i, v, {
        if ((*v)->in_use) {
            (*v)->in_use = 0;
            buffer_push(new_base, new_length, new_capacity, *v);
        } else {
            _free_managed(*v);
        }
    });
    free(heap->base);
    heap->base = new_base;
    heap->length = new_length;
    heap->capacity = new_capacity;
}

void js_managed_value_dump(struct js_managed_value *managed) { // also used by heap dump
    switch (managed->type) {
    case vt_string:
        printf("'%.*s'", (int)managed->string.length, managed->string.base);
        break;
    case vt_array:
        printf("[");
        js_list_for_each(managed->array.base, managed->array.length, _, i, v, {
            printf("%zu:", i);
            js_value_dump(v);
            printf(",");
        });
        printf("]");
        break;
    case vt_object:
        printf("{");
        js_map_for_each(managed->object.base, _, managed->object.capacity, k, kl, v, {
            printf("%.*s:", (int)kl, k);
            js_value_dump(v);
            printf(",");
        });
        printf("}");
        break;
    case vt_function:
        printf("<function %u {", managed->function.ingress);
        js_map_for_each(managed->function.closure.base, _, managed->function.closure.capacity, k, kl, v, {
            printf("%.*s:", (int)kl, k);
            js_value_dump(v);
            printf(",");
        });
        printf("}>");
        break;
    default:
        fatal("Unknown managed value type %d", managed->type);
    }
}

void js_value_dump(struct js_value *value) {
    switch (value->type) {
    case vt_undefined:
        printf("undefined");
        break;
    case vt_null:
        printf("null");
        break;
    case vt_boolean:
        printf(value->boolean ? "true" : "false");
        break;
    case vt_number:
        printf("%lg", value->number);
        break;
    case vt_scripture:
        printf("'''%.*s'''", (int)js_string_length(value), js_string_base(value));
        break;
    case vt_inscription:
        printf("''%.*s''", (int)js_string_length(value), js_string_base(value));
        break;
    case vt_string:
    case vt_array:
    case vt_object:
    case vt_function:
        js_managed_value_dump(value->managed);
        break;
    case vt_c_function:
        printf("<c_function %p>", value->c_function);
        break;
    default:
        fatal("Unknown value type %d", value->type);
    }
}

void js_value_print(struct js_value *value) {
    switch (value->type) {
    case vt_null:
        printf("null");
        break;
    case vt_boolean:
        printf(value->boolean ? "true" : "false");
        break;
    case vt_number:
        printf("%lg", value->number);
        break;
    case vt_scripture:
    case vt_inscription:
    case vt_string:
        printf("%.*s", (int)js_string_length(value), js_string_base(value));
        break;
    case vt_array:
        printf("[");
        js_list_for_each(value->managed->array.base, value->managed->array.length, _, i, v, {
            printf("%zu:", i);
            js_value_print(v);
            printf(",");
        });
        printf("]");
        break;
    case vt_object:
        printf("{");
        js_map_for_each(value->managed->object.base, _, value->managed->object.capacity, k, kl, v, {
            printf("%.*s:", (int)kl, k);
            js_value_print(v);
            printf(",");
        });
        printf("}");
        break;
    case vt_function:
        printf("<function>");
        break;
    case vt_c_function:
        printf("<c_function>");
        break;
    default:
        fatal("Unknown value type %d", value->type);
    }
}

bool js_is_string(struct js_value *value) {
    return value->type == vt_scripture || value->type == vt_inscription || value->type == vt_string;
}

char *js_string_base(struct js_value *value) {
    switch (value->type) {
    case vt_scripture:
        return value->scripture.base;
    case vt_inscription:
        return (char *)(*(value->inscription.base) + value->inscription.offset);
    case vt_string:
        return value->managed->string.base;
    default:
        return NULL;
    }
}

size_t js_string_length(struct js_value *value) {
    switch (value->type) {
    case vt_scripture:
        return value->scripture.length;
    case vt_inscription:
        return value->inscription.length;
    case vt_string:
        return value->managed->string.length;
    default:
        return 0;
    }
}

int js_string_compare(struct js_value *lhs, struct js_value *rhs) {
    char *pl = js_string_base(lhs);
    size_t ll = js_string_length(lhs);
    char *pr = js_string_base(rhs);
    size_t lr = js_string_length(rhs);
    return strncmp(pl, pr, max(ll, lr));
}

struct js_result js_add(struct js_heap *heap, struct js_value *lhs, struct js_value *rhs) {
    struct js_value value;
    if (lhs->type == vt_number && rhs->type == vt_number) {
        return (struct js_result){.success = true, .value = js_number(lhs->number + rhs->number)};
    } else if (js_is_string(lhs) && js_is_string(rhs)) {
        value = js_string(heap, js_string_base(lhs), js_string_length(lhs));
        string_buffer_append(value.managed->string.base, value.managed->string.length, value.managed->string.capacity, js_string_base(rhs), js_string_length(rhs));
        return (struct js_result){.success = true, .value = value};
    } else {
        return (struct js_result){.success = false, .value = js_scripture_sz("Add operand must be number or string")};
    }
}

#ifndef NOTEST

int test_data_structure_size(int argc, char *argv[]) {
    print_result(sizeof(struct js_value), "%zu");
    print_result(sizeof(struct js_kv_pair), "%zu");
    print_result(sizeof(struct js_variable_map), "%zu");
    print_result(sizeof(struct js_managed_value), "%zu");
    print_result(sizeof(struct js_heap), "%zu");
    print_result(sizeof(struct js_result), "%zu");
    return EXIT_SUCCESS;
}

int test_js_map(int argc, char *argv[]) {
    struct js_kv_pair *p = NULL;
    size_t len = 0;
    size_t cap = 0;
    for (int i = 0; i < 10; i++) {
        char *key = random_sz_static(NULL);
        struct js_value val, ret;
        val.type = vt_number;
        val.number = random_double();
        js_map_put_sz(p, len, cap, key, val);
        ret = js_map_get_sz(p, len, cap, key);
        enforce(ret.type == vt_number);
        enforce(ret.number == val.number);
        printf("len=%zu cap=%zu\n", len, cap);
    }
    js_map_for_each(p, _, cap, key, klen, val, {
        printf("%.*s %g\n", (int)klen, key, val->number);
    });
    js_map_free(p, len, cap);
    return EXIT_SUCCESS;
}

int test_js_map_loop(int argc, char *argv[]) {
    struct js_kv_pair *p = NULL;
    size_t len = 0;
    size_t cap = 0;
    for (;;) {
        for (int i = 0; i < 100000; i++) {
            char *key = random_sz_static(NULL);
            struct js_value val = js_number(random_double());
            js_map_put_sz(p, len, cap, key, val);
            struct js_value ret = js_map_get_sz(p, len, cap, key);
            enforce(ret.type == vt_number);
            enforce(ret.number == val.number);
            // random delete, to check stage 2
            if (rand() % 2 == 0) {
                js_map_put_sz(p, len, cap, key, (struct js_value){0});
            }
        }
        printf("len=%zu cap=%zu\n", len, cap);
        js_map_free(p, len, cap);
        // js_map_free_internal(&p, &len, &cap);
    }
    return EXIT_SUCCESS;
}

static enum js_value_type _random_js_value_type() {
    return rand() % (countof(_value_type_names) - 1) + 1;
}

struct tablet {
    char foo;
    short bar;
    const char s[256];
    int baz;
    long qux;
};

static struct js_value _random_js_value(struct js_heap *heap, enum js_value_type type, size_t depth) {
    // https://www.biblestudytools.com/topical-verses/the-25-most-read-bible-verses/
    static const char *scriptures[] = {
        "1. John 3:16 - For God so loved the world that he gave his one and only Son, that whoever believes in him shall not perish but have eternal life.",
        "2. Jeremiah 29:11 - For I know the plans I have for you,” declares the LORD, “plans to prosper you and not to harm you, plans to give you hope and a future.",
        "3. Romans 8:28 - And we know that in all things God works for the good of those who love him, who have been called according to his purpose.",
        "4. Philippians 4:13 - I can do everything through him who gives me strength.",
        "5. Genesis 1:1 - In the beginning God created the heavens and the earth.",
        "6. Proverbs 3:5 - Trust in the LORD with all your heart and lean not on your own understanding.",
        "7. Proverbs 3:6 - In all your ways acknowledge him, and he will make your paths straight.",
        "8. Romans 12:2 - Do not conform any longer to the pattern of this world, but be transformed by the renewing of your mind. Then you will be able to test and approve what God's will is—his good, pleasing and perfect will.",
        "9. Philippians 4:6 - Do not be anxious about anything, but in everything, by prayer and petition, with thanksgiving, present your requests to God.",
        "10. Matthew 28:19 - Therefore go and make disciples of all nations, baptizing them in the name of the Father and of the Son and of the Holy Spirit.",
    };
    static struct tablet tablet = {
        .s = "A Sample Inscription",
    };
    static struct tablet *ptablet = &tablet;
    struct js_value ret;
    int i;
    if (depth >= 10) {
        return js_null();
    }
    switch (type) {
    case vt_null:
        return js_null();
    case vt_boolean:
        return js_boolean(rand() % 2 == 1);
    case vt_number:
        return js_number(random_double());
    case vt_scripture:
        return js_scripture_sz(scriptures[rand() % countof(scriptures)]);
    case vt_inscription:
        return js_inscription((uint8_t **)&ptablet, (uint32_t)offsetof(struct tablet, s), (uint32_t)strlen(tablet.s));
    case vt_string:
        return js_string_sz(heap, random_sz_static(NULL));
    case vt_array:
        ret = js_array(heap);
        for (i = 0; i < rand() % 10; i++) {
            struct js_value v = _random_js_value(heap, _random_js_value_type(), depth + 1);
            js_array_put(&ret, rand() % 100, v);
        }
        return ret;
    case vt_object:
        ret = js_object(heap);
        for (i = 0; i < rand() % 10; i++) {
            struct js_value v = _random_js_value(heap, _random_js_value_type(), depth + 1);
            js_object_put_sz(&ret, random_sz_static(NULL), v);
        }
        return ret;
    case vt_function:
        ret = js_function(heap, rand() % UINT32_MAX);
        for (i = 0; i < rand() % 10; i++) {
            struct js_value v = _random_js_value(heap, _random_js_value_type(), depth + 1);
            size_t len = ret.managed->function.closure.length; // uint16_t -> size_t
            size_t cap = ret.managed->function.closure.capacity;
            js_map_put_sz(ret.managed->function.closure.base, len, cap, random_sz_static(NULL), v);
            ret.managed->function.closure.length = (uint16_t)len; // size_t -> uint16_t
            ret.managed->function.closure.capacity = (uint16_t)cap;
        }
        return ret;
    case vt_c_function:
        return js_c_function((void *)(intptr_t)rand());
    default:
        fatal("Unknown value type %u", type);
    }
}

int test_js_value(int argc, char *argv[]) {
    struct js_heap heap = {0};
    for (enum js_value_type type = 1; type < countof(_value_type_names); type++) {
        struct js_value val = _random_js_value(&heap, type, 0);
        printf("%s: ", _value_type_names[type]);
        js_value_dump(&val);
        printf("\n");
    }
    js_sweep(&heap);
    buffer_free(heap.base, heap.length, heap.capacity);
    return EXIT_SUCCESS;
}

int test_js_value_loop(int argc, char *argv[]) {
    struct js_heap heap = {0};
    for (;;) {
        for (int i = 0; i < 10000; i++) {
            struct js_value val = _random_js_value(&heap, _random_js_value_type(), 0);
            if (rand() % 10 == 0) { // mark 1/10 of them
                js_mark(&val);
            }
        }
        js_sweep(&heap);
        js_sweep(&heap); // second round sweep remained all
        buffer_free(heap.base, heap.length, heap.capacity);
        putchar('.');
    }
}

int test_js_value_bug(int argc, char *argv[]) {
    // REPRODUCE BUG:
    struct js_heap heap = {0};
    const char *bug_keys[] = {"b",
        "CVUcQn5KYZkjKSa1eJAsg0nUQsnZBdSNquxXsYnwIoNTEAtZBOt",
        "Swy4fCH8h03lkwQwW9BxW7O3dH9EReeng80wiI37Jwid6RXMwQ0cgiPn",
        "EFvi653FKJKm04nqvfux6YzKZhmukC7biyUhulH9eLPxZUX"};
    for (int i = 0; i < countof(bug_keys); i++) {
        const char *k = bug_keys[i];
        size_t fh = _first_hash(k, (uint16_t)strlen(k), 0b01);
        size_t nh = _next_hash(fh, 0b01);
        printf("%d. %s %zu %zu\n", i, k, fh, nh);
    }
    struct js_value obj = js_object(&heap);
    js_object_put_sz(&obj, bug_keys[0], js_boolean(true));
    js_object_put_sz(&obj, bug_keys[0], js_null());
    js_object_put_sz(&obj, bug_keys[2], js_boolean(true));
    js_object_put_sz(&obj, bug_keys[1], js_boolean(true));
    js_object_put_sz(&obj, bug_keys[3], js_boolean(true));
    return EXIT_SUCCESS;
}

int test_js_string_family(int argc, char *argv[]) {
    struct js_heap heap = {0};
    for (;;) {
        for (uint8_t vt = vt_scripture; vt <= vt_string; vt++) {
            struct js_value str = _random_js_value(&heap, vt, 0);
            printf("%s %s %.*s\n", js_is_string(&str) ? "true" : "false", _value_type_names[vt], (int)js_string_length(&str), js_string_base(&str));
            js_sweep(&heap);
        }
    }
}

#endif