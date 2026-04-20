int commit_create(const char *message, ObjectID *out_commit) {
    Index index;

    // Load index (staged files)
    if (index_load(&index) != 0) {
        printf("error: failed to load index\n");
        return -1;
    }

    // Create tree from index
    ObjectID tree_id;
    if (tree_from_index(&tree_id) != 0) {
        printf("error: failed to create tree\n");
        return -1;
    }

    // Read parent commit (if exists)
    ObjectID parent_id;
    int has_parent = (head_read(&parent_id) == 0);

    // Get author
    const char *author = pes_author();
    long timestamp = time(NULL);

    // Prepare commit struct
    Commit commit;
    memset(&commit, 0, sizeof(commit));

    commit.tree = tree_id;
    if (has_parent) {
        commit.parent = parent_id;
        commit.has_parent = 1;
    } else {
        commit.has_parent = 0;
    }

    strncpy(commit.author, author, sizeof(commit.author) - 1);
    strncpy(commit.message, message, sizeof(commit.message) - 1);
    commit.timestamp = timestamp;

    // Serialize commit
    void *data;
    size_t len;

    if (commit_serialize(&commit, &data, &len) != 0) {
        printf("error: serialize failed\n");
        return -1;
    }

    // Write commit object
    if (object_write(OBJ_COMMIT, data, len, out_commit) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // Update HEAD
    if (head_update(out_commit) != 0) {
        printf("error: failed to update HEAD\n");
        return -1;
    }

    return 0;
}
