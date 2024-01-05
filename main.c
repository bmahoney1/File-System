#include <stdio.h>
#include <stdlib.h>
#include "fs.h"

int main() {
    // Example usage of the file system functions

    // Create a file system on a virtual disk named "my_disk"
    if (make_fs("my_disk") == -1) {
        fprintf(stderr, "Error creating file system.\n");
        return EXIT_FAILURE;
    }

    // Mount the file system
    if (mount_fs("my_disk") == -1) {
        fprintf(stderr, "Error mounting file system.\n");
        return EXIT_FAILURE;
    }

    // Example file operations (you can modify this part based on your needs)

    // Create a file named "example.txt"
    if (fs_create("example.txt") == -1) {
        fprintf(stderr, "Error creating file.\n");
        return EXIT_FAILURE;
    }

    // Open the file for writing
    int file_descriptor = fs_open("example.txt");
    if (file_descriptor == -1) {
        fprintf(stderr, "Error opening file.\n");
        return EXIT_FAILURE;
    }

    // Write data to the file
    char data[] = "Hello, this is an example!";
    if (fs_write(file_descriptor, data, sizeof(data)) == -1) {
        fprintf(stderr, "Error writing to file.\n");
        return EXIT_FAILURE;
    }
	if (fs_read(file_descriptor, 0, sizeof(data)) == -1) {
        fprintf(stderr, "Error writing to file.\n");
        return EXIT_FAILURE;
    }
    // Close the file
    if (fs_close(file_descriptor) == -1) {
        fprintf(stderr, "Error closing file.\n");
        return EXIT_FAILURE;
    }

    // Unmount the file system
    if (umount_fs("my_disk") == -1) {
        fprintf(stderr, "Error unmounting file system.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

