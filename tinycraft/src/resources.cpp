char *global_resources_dir = NULL;
char *global_output_dir = NULL;

std::string get_resource_path(std::string relpath) {
    char *resources_dir;
    if (global_resources_dir == NULL) {
        resources_dir = getenv("RESOURCES_DIR");
    } else {
        resources_dir = global_resources_dir;
    }
    fassert(resources_dir != NULL);
    return std::string(resources_dir) + "/" + relpath;
}

std::string get_output_path(std::string relpath) {
    char *output_dir;
    if (global_output_dir == NULL) {
        output_dir = getenv("OUTPUT_DIR");
    } else {
        output_dir = global_output_dir;
    }
    fassert(output_dir != NULL);
    return std::string(output_dir) + "/" + relpath;
}