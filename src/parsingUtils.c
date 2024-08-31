#include "common.h"
#include "parsingUtils.h"


// Simply checks if the number of arguments [argc] is
// equal to the expected number of arguments [expected].
bool validNumberOfArgs(int argc, int expected) {
    return argc == expected;
}

// Given a string [str], checks if the string is a positive
// integer number. Works only with ascii encoded strings.
bool isPositiveIntegerNumber(const char* str) {
    if (str == NULL || *str == '\0')
        return false;

    // If a '-' sign is in the front of the string
    // we consider it as a negative value
    if(str[0] == '-')
        return false;
    
    for (int i = 0; str[i] != '\0'; i++) {
        // In case a '+' sign is in the front of the string
        if(i == 0 && str[i] == '+')
            continue;
        
        // If the string is 0
        if(i == 0 && str[i] == '0')
            return false;

        // Checks the ascii characters
        if (str[i] < '0' || str[i] > '9')
            return false;
    }

    return true;
}

// Returns the size of the concatenated argv array WITH the null terminator
// and the whitespaces between each argument
size_t concatenated_args_size(char** argv, int n_args) {
    size_t size = 0;
    for (int i = 0; i < n_args; i++) {
        size += strlen(argv[i]);
        
        // For either a whitespace or the null terminator at the end
        size += 1;
    }

    return size;
}

// Concatenates into one string [args_str_dest] all [n_args] number of
// strings of [argv_src] array. The separator between each pair of strings
// is a single whitespace.
// Keep in mind that the memory of [args_str_dest] should already be allocated
// before calling this function.
void concatenate_args(char* args_str_dest, char** argv_src, int n_args) {
    size_t pos = 0;
    for (int i = 0; i < n_args; i++) {
        size_t len = strlen(argv_src[i]);
        strncpy(args_str_dest + pos, argv_src[i], len);
        pos += len;
        if (i < n_args - 1) {
            args_str_dest[pos] = ' ';
            pos++;
        }
    }
    args_str_dest[pos] = '\0';
}

// This function simply converts a given positive number [num]
// to a string and returns this string.
// It is the user's responsibility to free the allocated space for
// the string, once they are done processing with it.
char* itoa(size_t num) {
    int str_len = snprintf(NULL, 0, "%ld", num);
    char* str = malloc(str_len + 1);
    HANDLE_ERROR("malloc", str, NULL);

    snprintf(str, str_len + 1, "%ld", num);
    return str;
}

// Given a string [str], the number of tokens within it (words) [n_tokens] and a [splitter]
// (for example the whitespace character), it tokenizes the string: creates and returns
// an array of pointers to strings which are the tokens of the original string [str].
// It is the user's responsibility to free the allocated space for the array of strings,
// once they are done processing with it.
char** tokenize_str(const char* str, size_t n_tokens, char splitter) {
    char** tokens_arr = malloc(n_tokens * sizeof(char*));
    HANDLE_ERROR("malloc", tokens_arr, NULL);

    const char* ptr = str;
    size_t token_count = 0;
    
    // Iterate over the string and tokenize it
    while (*ptr != '\0' && token_count < n_tokens) {

        const char* end = strchr(ptr, splitter);
        if (end == NULL)
            end = ptr + strlen(ptr);

        // Calculate the length of the token
        size_t token_len = end - ptr;

        // Allocate memory for the token
        tokens_arr[token_count] = malloc((token_len + 1) * sizeof(char));
        HANDLE_ERROR("malloc", tokens_arr[token_count], NULL);

        // Copy the token into the allocated memory
        strncpy(tokens_arr[token_count], ptr, token_len);
        tokens_arr[token_count][token_len] = '\0';

        // Move to the next token
        ptr = end;
        if (*ptr != '\0')
            ptr++;          // Skip the splitter character

        // Get the next token
        token_count++;
    }

    return tokens_arr;
}

// Adds 'NULL' as the last string to the string array that [arr_ptr]
// points to.
void append_null(char*** arr_ptr, size_t arr_size) {
    char** arr = *arr_ptr;

    char** new_arr = realloc(arr, (arr_size + 1) * sizeof(char*));
    HANDLE_ERROR("realloc", new_arr, NULL);

    new_arr[arr_size] = NULL;

    *arr_ptr = new_arr;
}