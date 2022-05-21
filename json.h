#ifndef _JSON_H_
#define _JSON_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct json_token {
      /* Length of token */
      uint32_t token_len; 
      /* Token words */
      char *token_words;
      /* Token words' position in JSON */
      uint32_t row;
      uint32_t col;
      #define LEFT_BUCKET     0x00 // {
      #define RIGHT_BUCKET    0x01 // }
      #define STRING          0x02 // "abc"
      #define BOOLEAN         0x03 // true, false
      #define NUMBER          0x04 // 123, -123, 1.234, 1e+10
      #define NULL_OBJ        0x05 // null
      #define LEFT_S_BUCKET   0x06 // [
      #define RIGHT_S_BUCKET  0x07 // ]
      #define COLON           0x08 // :
      #define COMMA           0x09 // ,
      #define COMMENT         0x10 // /
      uint8_t token_type;
      struct json_token *next_token;
};

/* The minimum length of a JSON file is 2, a pair of bucket like '{}' or '[]' */
#define MIN_JSON_LEN 2

typedef struct json_obj {
      /* It's used to look back keys, if has same key in the object, 
       * replace its data instead of creating a new same key.
       */
      struct json_obj *prev_key;
      /* Next key behind current key, we define the index of array as key,
       * if its end of key or index, next_key should be NULL.
      */
      struct json_obj *next_key;
      /* The value field may be a string, boolean, number, null 
       * or the array(object) value of first index(key).
      */
      struct json_obj *value; 
      /* Key or value */
      char *data; 
      /* Six kinds data type */
      #define V_OBJECT  0x00
      #define V_ARRAY   0x01
      #define V_STRING  0x02
      #define V_BOOL    0x03
      #define V_NUMBER  0x04
      #define V_NULL    0x05
      uint8_t value_type;
      /* The flag tells us the data type, true is key and false is value */
      bool key_value;
}JSONObj;

/* Query command */
struct json_query_command{
      /* Length of command words */
      uint32_t len;
      /* The command words which the command line is splited to */
      char *command_words;
      /* Next command */
      struct json_query_command *next_cmd;
};

typedef struct {
      const char *json;
      uint32_t length;
      uint32_t cursor;
}JSONReader;

/* Serialize */
/* Export JSON APIs for user */
struct json_obj* from_json(const char *json_content, uint32_t size);
void json_obj_free(struct json_obj *obj);
/* Internal functions */
static int json_comments_skip(uint32_t *col, uint32_t *row, char *json_content, uint32_t *i, uint32_t len);
static JSONObj *json_parser(JSONReader *reader);
#define MATCH_SUCCESS 0
#define MATCH_FAILED  1
static int match(JSONReader *reader, const char word);
#define match_digit(reader)   match(reader, '0') || match(reader, '1') || match(reader, '2') || match(reader, '3') || match(reader, '4') || \
                              match(reader, '5') || match(reader, '6') || match(reader, '7') || match(reader, '8') || match(reader, '9')
#define match_alpha(reader)   match(reader, 'a') || match(reader, 'b') || match(reader, 'c') || match(reader, 'd') || match(reader, 'e') || \
                              match(reader, 'f') || match(reader, 'g') || match(reader, 'h') || match(reader, 'i') || match(reader, 'j') || \
                              match(reader, 'k') || match(reader, 'l') || match(reader, 'm') || match(reader, 'n') || match(reader, 'o') || \
                              match(reader, 'p') || match(reader, 'q') || match(reader, 'r') || match(reader, 's') || match(reader, 't') || \
                              match(reader, 'u') || match(reader, 'v') || match(reader, 'w') || match(reader, 'x') || match(reader, 'y') || \
                              match(reader, 'z') || match(reader, 'A') || match(reader, 'B') || match(reader, 'C') || match(reader, 'D') || \
                              match(reader, 'E') || match(reader, 'F') || match(reader, 'G') || match(reader, 'H') || match(reader, 'I') || \
                              match(reader, 'J') || match(reader, 'K') || match(reader, 'L') || match(reader, 'M') || match(reader, 'N') || \
                              match(reader, 'O') || match(reader, 'P') || match(reader, 'Q') || match(reader, 'R') || match(reader, 'S') || \
                              match(reader, 'T') || match(reader, 'U') || match(reader, 'V') || match(reader, 'W') || match(reader, 'X') || \
                              match(reader, 'Y') || match(reader, 'Z')
static char expect(JSONReader *reader, const char word);
static char expect_digit(JSONReader *reader);
static char expect_alpha(JSONReader *reader);
static void skip_space(JSONReader *reader);
#define unexpected(reader) do {\
            if (reader->cursor + 1 == reader->length) \
                  fprintf(stderr, "Unexpect end of content");\
            else   \
            fprintf(stderr, "Unexpect word: %c(%d)\n", reader->json[reader->cursor + 1], reader->json[reader->cursor + 1]);\
      }while (0);
static JSONObj *json_parse_object(JSONReader *reader);
static JSONObj *json_parse_array(JSONReader *reader);
static JSONObj *json_parse_string(JSONReader *reader);
static JSONObj *json_parse_number(JSONReader *reader);
static JSONObj *json_parse_boolean(JSONReader *reader);
static JSONObj *json_parse_null(JSONReader *reader);
/* Operations */
/* Query */
struct json_obj *json_query(char *command, struct json_obj *obj);
static struct json_query_command *json_query_command_parser(char *json_query_command);
static void json_query_command_free(struct json_query_command *commands);
double json_get_double(struct json_obj *obj, char *err_msg);
long json_get_long(struct json_obj *obj, char *err_msg);
bool json_get_bool(struct json_obj *obj, char *err_msg);
char *json_get_string(struct json_obj *obj, char *err_msg);
#define IS_NULL 1
int json_get_null(struct json_obj *obj, char *err_msg);
/* Modify */
int json_update(char *command, struct json_obj *obj, struct json_obj *value);
/* Add */
int json_add(char *command, struct json_obj *obj, struct json_obj *value);
/* Delete */
int json_delete(char *command, struct json_obj *obj);
/* Deserialize */
int to_json(struct json_obj *root, char *json_content, uint32_t size);
static int json_build(struct json_obj *obj, char *json_content, uint32_t size, bool beautfy, int tab_size);
static int json_build_object(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer);
static int json_build_array(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer);
static int json_build_string(char *string, char *json_content, uint32_t size, uint32_t built_size);
static int json_build_dispatch(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer);
/* Utils */
#define DEFAULT_TAB_SIZE 2
char *json_utils_long2str(long value);
char *json_utils_double2str(double value);
int json_utils_beautify(char *input, char *output, int tab_size);
int json_utils_minify(char *input, char *output);
static uint32_t json_utils_hex2uint(char *hex);
#define W_UTF8 1
static size_t json_utils_wstrlen(char *s, int encoding, size_t slen);
static size_t json_utils_console_wstrlen(char *s, int encoding, size_t slen);
/* Simple deserialization wrapper 
 * Example: LOAD_OBJECT(LOAD_STRING("123"), LOAD_ARRAY(LOAD_NUMBER(123), LOAD_NULL, LOAD_TRUE, END), LOAD_STRING("234"), LOAD_OBJECT(END), END)
 * Ouput: {"123":[123, null, true], 234: {}}
 */
/* Wrapper functions */
struct json_obj *load_single_value(char *str, int value_type);
struct json_obj *load_object(struct json_obj* args, ...);
struct json_obj *load_array(struct json_obj* args, ...);
/* Multivalue wrapper */
#define LOAD_OBJECT(...) load_object(__VA_ARGS__)
#define LOAD_ARRAY(...) load_array(__VA_ARGS__)
/* Single value wrapper */
#define LOAD_NULL             load_single_value("null", V_NULL)
#define LOAD_TRUE             load_single_value("true", V_BOOL)
#define LOAD_FALSE            load_single_value("false", V_BOOL)
#define LOAD_STRING(str)      load_single_value(str, V_STRING)
#define LOAD_NUMBER(num)      load_single_value((long long)num == num ? json_utils_long2str(num) : json_utils_double2str(num), V_NUMBER)
/* Terminator of array and object */
#define LOAD_END NULL

bool json_iterable(struct json_obj *obj);
struct json_obj *json_next_key(struct json_obj *obj);
struct json_obj *json_begin(struct json_obj *obj);
#define json_end(obj)  (obj == NULL)
struct json_obj *json_get_value(struct json_obj *obj);
char *json_get_key(struct json_obj *obj);
#endif // !_JSON_H_