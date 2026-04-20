#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_FILE ".pes/index"

// ---------------- LOAD ----------------
int index_load(Index *idx) {
    idx->count = 0;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;

    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (idx->count >= MAX_INDEX_ENTRIES) break;

        IndexEntry *e = &idx->entries[idx->count];
        char hash_hex[HASH_HEX_SIZE + 1];

        if (sscanf(line, "%o %64s %255[^\n]", &e->mode, hash_hex, e->path) != 3)
            continue;

        hex_to_hash(hash_hex, &e->hash);
        idx->count++;
    }

    fclose(f);
    return 0;
}

// ---------------- SAVE ----------------
int index_save(const Index *idx) {
    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];

    for (int i = 0; i < idx->count; i++) {
        const IndexEntry *e = &idx->entries[i];

        hash_to_hex(&e->hash, hex);
        fprintf(f, "%o %s %s\n", e->mode, hex, e->path);
    }

    fclose(f);
    return 0;
}

// ---------------- ADD ----------------
int index_add(Index *idx, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    void *data = malloc(size);
    if (!data) {
        fclose(f);
        return -1;
    }

    fread(data, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // update if exists
    for (int i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, path) == 0) {
            idx->entries[i].hash = id;
            idx->entries[i].mode = 0100644;
            return index_save(idx);   // ✅ SAVE HERE
        }
    }

    // add new
    if (idx->count >= MAX_INDEX_ENTRIES) return -1;

    IndexEntry *e = &idx->entries[idx->count++];
    strcpy(e->path, path);
    e->hash = id;
    e->mode = 0100644;

    return index_save(idx);   // ✅ SAVE HERE
}

// ---------------- STATUS ----------------
int index_status(const Index *idx) {
    if (idx->count == 0) {
        printf("Index is empty\n");
        return 0;
    }

    printf("Staged files:\n");

    for (int i = 0; i < idx->count; i++) {
        printf("%s\n", idx->entries[i].path);
    }

    return 0;
}
