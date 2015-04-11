#include <string.h>
#include <assert.h>
#include "encoding.h"

#define TEST_STRING "hello world!你好世界켐ㅇㄹ貴方元気？☺"

int main()
{
    xcb_compound_text_init();

    size_t len;
    char* result = xcb_utf8_to_compound_text(TEST_STRING, strlen(TEST_STRING), &len);

    char* utf8_result = xcb_compound_text_to_utf8(result, len, NULL);

    assert(strcmp(utf8_result, TEST_STRING) == 0);

    free(result);
    free(utf8_result);

    return 0;
}
