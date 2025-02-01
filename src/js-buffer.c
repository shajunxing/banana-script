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
