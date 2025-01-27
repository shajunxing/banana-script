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
    string_buffer_new(str, len, cap);
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

static inline unsigned char _printable(unsigned char ch) {
    return ch >= 0x20 && ch <= 0x7e ? ch : ' ';
}

void print_hex(void *p, size_t len) {
    size_t bytes_per_line = _segs_per_line * _bytes_per_seg;
    unsigned char *base = (unsigned char *)p;
    size_t loffs;
    for (loffs = 0; loffs <= len; loffs += bytes_per_line) { // <= make sure when len=0 also print a line
        int what;
        printf("    %p:  ", (char *)p + loffs);
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

struct string *string_new(const char *p, size_t len) {
    struct string *str = (struct string *)calloc(1, sizeof(struct string));
    assert(str != NULL);
    if (p && len) {
        buffer_alloc(char, str->p, str->cap, len + 1);
        memcpy(str->p, p, len);
        str->len = len;
    } else {
        buffer_alloc(char, str->p, str->cap, 1);
    }
    return str;
}

void string_delete(struct string *str) {
    free(str->p);
    free(str);
}

void string_clear(struct string *str) {
    buffer_clear(char, str->p, str->len, str->cap);
}

void string_append(struct string *str, const char *p, size_t len) {
    size_t newlen = str->len + len;
    buffer_realloc(char, str->p, str->cap, newlen + 1);
    memcpy(str->p + str->len, p, len);
    str->len = newlen;
}

int string_compare(struct string *str_l, struct string *str_r) {
    // DONT use min len, for example, "111" is > "11", if use min len, result == is wrong
    return strncmp(str_l->p, str_r->p, str_l->len > str_r->len ? str_l->len : str_r->len);
}

struct list *list_new(size_t reqcap) {
    struct list *list = (struct list *)calloc(1, sizeof(struct list));
    assert(list != NULL);
    buffer_alloc(void *, list->p, list->cap, reqcap);
    return list;
}

void list_delete(struct list *list) {
    free(list->p);
    free(list);
}

void list_clear(struct list *list) {
    buffer_clear(void *, list->p, list->len, list->cap);
}

void list_push(struct list *list, void *val) {
    size_t newlen = list->len + 1;
    buffer_realloc(void *, list->p, list->cap, newlen);
    list->p[list->len] = val;
    list->len = newlen;
}

void *list_pop(struct list *list) {
    void *ret = NULL;
    if (list->len > 0) {
        list->len--;
        ret = list->p[list->len];
    }
    return ret;
}

// always expand, be careful memory usage
void list_put(struct list *list, size_t idx, void *val) {
    if (val == NULL) { // special treat NULL to prevent useless expand
        if (idx < list->len) {
            list->p[idx] = NULL;
        } // else do nothing
    } else {
        size_t newlen = idx + 1;
        if (newlen > list->len) {
            buffer_realloc(void *, list->p, list->cap, newlen);
            list->len = newlen;
        }
        list->p[idx] = val;
    }
}

void *list_get(struct list *list, size_t idx) {
    if (idx < list->len) {
        return list->p[idx];
    } else {
        return NULL;
    }
}

struct map *map_new(size_t reqcap) {
    struct map *map = (struct map *)calloc(1, sizeof(struct map));
    assert(map != NULL);
    buffer_alloc(struct mapnode, map->p, map->cap, reqcap);
    return map;
}

static inline void _map_p_clear(struct mapnode *p, size_t cap) {
    int i;
    for (i = 0; i < cap; i++) {
        struct mapnode *node = p + i;
        if (node->k) {
            string_delete(node->k);
            node->k = NULL;
        }
        node->v = NULL;
    }
}

void map_delete(struct map *map) {
    _map_p_clear(map->p, map->cap);
    free(map->p);
    free(map);
}

void map_clear(struct map *map) {
    _map_p_clear(map->p, map->cap);
    map->len = 0;
}

static inline size_t _fh(const char *s, size_t slen, size_t mask) {
    size_t hash = 0;
    size_t i;
    for (i = 0; i < slen; i++) {
        hash = (hash + (hash << 4) + s[i]) & mask;
    }
    return hash;
}

static inline size_t _nh(size_t hash, size_t mask) {
    // hash = hash * 5 + 1 can perfectly cover 0-2 pow
    // https://tieba.baidu.com/p/8968552557
    return (hash + (hash << 4) + 1) & mask;
}

static inline void _put_after_expanded(struct mapnode *p, size_t cap, const char *key, size_t klen, void *val) {
    size_t mask = cap - 1;
    size_t hash;
    size_t rep;
    for (rep = 0, hash = _fh(key, klen, mask); rep < cap; rep++, hash = _nh(hash, mask)) {
        struct mapnode *node = p + hash;
        // log("rep=%d hash=%d", rep, hash);
        if (!node->k && !node->v) {
            node->k = string_new(key, klen);
            node->v = val;
            return;
        }
    }
    fatal("Whole loop ended, this shouldn't happen");
}

void map_put(struct map *map, const char *key, size_t klen, void *val) {
    size_t mask = map->cap - 1;
    size_t hash;
    size_t rep;
    for (rep = 0, hash = _fh(key, klen, mask); rep < map->cap; rep++, hash = _nh(hash, mask)) {
        struct mapnode *node = map->p + hash;
        // log("rep=%d hash=%d", rep, hash);
        if (!node->k) {
            if (node->v) {
                fatal("Key is NULL but value not NULL, this shouldn't happen");
            } else {
                if (val) {
                    size_t newlen = map->len + 1;
                    size_t reqcap = newlen << 1;
                    if (map->cap >= reqcap) {
                        node->k = string_new(key, klen);
                        node->v = val;
                        map->len++;
                        return;
                    } else {
                        // rehash
                        struct mapnode *newp = NULL;
                        size_t newlen = 0;
                        size_t newcap = 0;
                        size_t i;
                        buffer_alloc(struct mapnode, newp, newcap, reqcap);
                        // log("rehash, newmap->cap= %lld", newmap->cap);
                        for (i = 0; i < map->cap; i++) {
                            node = map->p + i;
                            if (node->k && node->v) {
                                _put_after_expanded(newp, newcap, node->k->p, node->k->len, node->v);
                                newlen++;
                            }
                        }
                        _put_after_expanded(newp, newcap, key, klen, val);
                        newlen++;
                        _map_p_clear(map->p, map->cap);
                        free(map->p);
                        map->p = newp;
                        map->len = newlen;
                        map->cap = newcap;
                        return;
                    }
                } else {
                    return;
                }
            }
        } else if (node->k->len == klen && memcmp(node->k->p, key, klen) == 0) {
            node->v = val;
            return;
        } else {
            if (node->v) {
                continue;
            } else {
                string_clear(node->k);
                string_append(node->k, key, klen);
                node->v = val;
                return;
            }
        }
    }
    fatal("Whole loop ended, this shouldn't happen");
}

// v can be NULL
void *map_get(struct map *map, const char *key, size_t klen) {
    size_t mask = map->cap - 1;
    size_t hash;
    size_t rep;
    for (rep = 0, hash = _fh(key, klen, mask); rep < map->cap; rep++, hash = _nh(hash, mask)) {
        struct mapnode *node = map->p + hash;
        if (!node->k) {
            return NULL;
        } else if (node->k->len == klen && memcmp(node->k->p, key, klen) == 0) {
            return node->v;
        }
    }
    return NULL;
}

void *map_get_sz(struct map *map, const char *key) {
    return map_get(map, key, strlen(key));
}

struct link *link_new() {
    struct link *link = (struct link *)calloc(1, sizeof(struct link));
    assert(link != NULL);
    return link;
}

void link_push(struct link *link, void *val) {
    struct linknode *node = (struct linknode *)calloc(1, sizeof(struct linknode));
    assert(node != NULL);
    node->v = val;
    node->owner = link;
    node->next = link->p;
    if (link->p) {
        link->p->prev = node;
    }
    link->p = node;
    link->len++;
}

void link_delete_node(struct link *link, struct linknode *node) {
    if (node->owner != link) {
        return;
    }
    if (link->p == node) {
        link->p = node->next;
    } else {
        node->prev->next = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    free(node);
    link->len--;
}

void link_dump(struct link *link) {
    printf("len=%lld\n", link->len);
    link_for_each(link, n, printf("    %p-> {v=%p, next=%p, prev=%p}\n", n, n->v, n->next, n->prev));
}

void link_delete(struct link *link) {
    link_for_each(link, n, link_delete_node(link, n));
    free(link);
}
