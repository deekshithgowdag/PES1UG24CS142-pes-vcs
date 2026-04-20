#include "index.h"
#include "pes.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ---------- LOAD ----------
int index_load(Index *index) {
    FILE *f = fopen(".pes/index", "r");

    // If file doesn't exist → empty index (NOT error)
    if (!f) {
        index->count = 0;
        return 0;
    }

    index->count = 0;

    char hex[65];

    while (fscanf(f, "%o %64s %255[^\n]\n",
                  &index->entries[index->count].mode,
                  hex,
                  index->entries[index->count].path) == 3) {

        hex_to_hash(hex, &index->entries[index->count].hash);
        index->count++;
    }

    fclose(f);
    return 0;
}

// ---------- SAVE ----------
int index_save(const Index *index) {
    FILE *f = fopen(".pes/index", "w");
    if (!f) return -1;

    char hex[65];

    for (int i = 0; i < index->count; i++) {
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(f, "%06o %s %s\n",
                index->entries[i].mode,
                hex,
                index->entries[i].path);
    }

    fclose(f);
    return 0;
}

// ---------- ADD ----------
int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char *data = malloc(st.st_size);
    if (!data) {
        fclose(f);
        return -1;
    }

    fread(data, 1, st.st_size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, st.st_size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // Check if file already exists
    int found = -1;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            found = i;
            break;
        }
    }

    int idx = (found == -1) ? index->count++ : found;

    index->entries[idx].mode = 100644;
    index->entries[idx].hash = id;
    strcpy(index->entries[idx].path, path);

    return 0;
}

// ---------- STATUS ----------
int index_status(const Index *index) {
    printf("Tracked files:\n");

    for (int i = 0; i < index->count; i++) {
        printf("%s\n", index->entries[i].path);
    }

    return 0;
}
