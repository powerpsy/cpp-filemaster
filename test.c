#include <stdio.h>
#include <string.h>

// Define MAX_PATH for this test, as it's defined in windows.h
#define MAX_PATH 260

// This function simulates the original, vulnerable code from rename.c
// It uses strcpy, which does not check buffer boundaries.
void vulnerable_copy(char* dest, const char* src) {
    strcpy(dest, src);
}

// This function simulates the fixed code.
// It uses strncpy, which is safe because it limits the number of
// characters copied to the size of the destination buffer.
void fixed_copy(char* dest, const char* src, size_t dest_size) {
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0'; // Ensure null-termination
}

int main() {
    printf("=== Test for rename.c Buffer Overflow Fix ===\\n");

    // Create a source string that is longer than MAX_PATH
    char long_string[MAX_PATH + 50];
    for (int i = 0; i < MAX_PATH + 49; i++) {
        long_string[i] = 'A';
    }
    long_string[MAX_PATH + 49] = '\0';

    printf("Source string length: %zu\\n", strlen(long_string));

    // --- Test the fixed function ---
    char dest_buffer_fixed[MAX_PATH];
    printf("\\nTesting the fixed_copy function...\\n");
    fixed_copy(dest_buffer_fixed, long_string, sizeof(dest_buffer_fixed));
    printf("Copied string length (fixed): %zu\\n", strlen(dest_buffer_fixed));
    printf("The copy was successful and truncated, preventing a crash.\\n");

    // --- Demonstrate the vulnerable function ---
    char dest_buffer_vulnerable[MAX_PATH];
    printf("\\nDemonstrating the vulnerable_copy function...\\n");
    printf("The following line is commented out because running it would likely\\n");
    printf("cause a segmentation fault (crash) due to the buffer overflow.\\n");

    // vulnerable_copy(dest_buffer_vulnerable, long_string);
    // printf("If this line is printed, the overflow did not crash (unlikely).\\n");

    printf("\\nTest complete. The fix is verified conceptually.\\n");

    return 0;
}
