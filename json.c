
#include "json.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *bool_str[] = {"false", "true"};

static const char *json_obj_type_strs[] = {
    "Object", "Array", "String", "Boolean", "Number", "Null", NULL};

static char expect(JSONReader *reader, const char word) {
  if (match(reader, word) == MATCH_FAILED) return 0;
  return reader->json[++reader->cursor];
}

static char expect_digit(JSONReader *reader) {
  if (!match_digit(reader)) {
    return 0;
  }
  return reader->json[++reader->cursor];
}

static char expect_alpha(JSONReader *reader) {
  if (match_alpha(reader) == MATCH_FAILED) return 0;
  return reader->json[++reader->cursor];
}

static int match(JSONReader *reader, const char word) {
  if (!reader || reader->cursor + 1 == reader->length) return MATCH_FAILED;
  if (reader->json[reader->cursor + 1] != word) return MATCH_FAILED;
  return MATCH_SUCCESS;
}

static void skip_space(JSONReader *reader) {
  while (match(reader, ' ') == MATCH_SUCCESS ||
         match(reader, '\n') == MATCH_SUCCESS ||
         match(reader, '\t') == MATCH_SUCCESS) {
    reader->cursor++;
  }
}

static JSONObj *json_parser(JSONReader *reader) {
#ifdef DEBUG
  printf("=> Parsing\n");
  printf("%s\n", reader->json);
#endif  // DEBUG
  JSONObj *root = NULL;
  skip_space(reader);
  if (expect(reader, '{')) {
#ifdef DEBUG
    printf("{");
#endif  // DEBUG
    root = json_parse_object(reader);
  } else if (expect(reader, '[')) {
#ifdef DEBUG
    printf("[");
#endif  // DEBUG
    root = json_parse_array(reader);
  } else if (match(reader, '+') == MATCH_SUCCESS ||
             match(reader, '-') == MATCH_SUCCESS || match_digit(reader)) {
    root = json_parse_number(reader);
  } else if (match(reader, 't') == MATCH_SUCCESS ||
             match(reader, 'f') == MATCH_SUCCESS) {
    root = json_parse_boolean(reader);
  } else if (match(reader, 'n') == MATCH_SUCCESS) {
    root = json_parse_null(reader);
  } else if (expect(reader, '"')) {
#ifdef DEBUG
    printf("\"");
#endif  // DEBUG
    root = json_parse_string(reader);
  } else {
    unexpected(reader);
  }

  skip_space(reader);
  if (root && reader->cursor + 1 != reader->length) {
    printf("JSON file ended but still has content\n");
    return NULL;
  }
#ifdef DEBUG
  printf("\n");
#endif  // DEBUG
  return root;
}

static JSONObj *json_parse_string(JSONReader *reader) {
  uint32_t begin = ++reader->cursor;
  for (; reader->cursor < reader->length; reader->cursor++) {
    /* Unescaped control char */
    char c = reader->json[reader->cursor];
#ifdef DEBUG
    printf("%c", c);
#endif  // DEBUG

    if (c != 0x7f && iscntrl(c)) {
      return NULL;
    } else if (c == '"' && reader->json[reader->cursor - 1] != '\\') {
      JSONObj *obj = calloc(1, sizeof(JSONObj));
      obj->data = calloc(reader->cursor - begin, sizeof(char));
      strncpy(obj->data, reader->json + begin, reader->cursor - begin);
      if (!json_string_validator(obj->data, reader->cursor - begin)) {
        free(obj->data);
        free(obj);
        return NULL;
      }
      return obj;
    }
  }

  return NULL;
}

static JSONObj *json_parse_number(JSONReader *reader) {
  uint32_t begin = reader->cursor + 1;
  JSONObj *obj = NULL;
  int c = 0;
  /* Decimal number */
  expect(reader, '-');
  /* Integer part */
  if (expect(reader, '0') && ++c && expect_digit(reader)) {
    return NULL;
  }
  /* Number at least has one digit */
  while (expect_digit(reader)) c++;
  if (!c) return NULL;
  /* Fractional part */
  if (expect(reader, '.')) {
    /* Fractional part must have one digit */
    if (!expect_digit(reader)) {
      return NULL;
    }
    while (expect_digit(reader))
      ;
  }
  /* Exponent part of scientific nation */
  if (expect(reader, 'E') || expect(reader, 'e')) {
    expect(reader, '+') || expect(reader, '-');
    /* Must have one digital */
    if (!expect_digit(reader)) {
      return NULL;
    }
    while (expect_digit(reader))
      ;
  }

number_exit:
  obj = calloc(1, sizeof(JSONObj));
  obj->key_value = false;
  obj->data = calloc(reader->cursor + 1 - begin, sizeof(char));
  strncpy(obj->data, reader->json + begin, reader->cursor + 1 - begin);
  printf("%s", obj->data);
  return obj;
}

static JSONObj *json_parse_boolean(JSONReader *reader) {
  if (expect(reader, 't') && expect(reader, 'r') && expect(reader, 'u') &&
      expect(reader, 'e')) {
    JSONObj *value = calloc(1, sizeof(JSONObj));
    value->key_value = false;
    value->data = calloc(4, sizeof(char));
    strncpy(value->data, "true", 4);
    return value;
  } else if (expect(reader, 'f') && expect(reader, 'a') &&
             expect(reader, 'l') && expect(reader, 's') &&
             expect(reader, 'e')) {
    JSONObj *value = calloc(1, sizeof(JSONObj));
    value->key_value = false;
    value->data = calloc(4, sizeof(char));
    strncpy(value->data, "false", 5);
    return value;
  }
  unexpected(reader);
  return NULL;
}
static JSONObj *json_parse_null(JSONReader *reader) {
  if (expect(reader, 'n') && expect(reader, 'u') && expect(reader, 'l') &&
      expect(reader, 'l')) {
    JSONObj *value = calloc(1, sizeof(JSONObj));
    value->key_value = false;
    value->data = calloc(4, sizeof(char));
    strncpy(value->data, "null", 4);
    return value;
  }
  unexpected(reader);
  return NULL;
}

static JSONObj *json_parse_array(JSONReader *reader) {
  JSONObj *head = (JSONObj *)calloc(1, sizeof(JSONObj));
  head->key_value = true;
  JSONObj *node = head;
  uint32_t index = 0;
  for (;;) {
    JSONObj *key = (JSONObj *)calloc(1, sizeof(JSONObj));
    key->prev_key = node;
    node->next_key = key;
    key->key_value = true;
    key->data = json_utils_long2str(index);
    JSONObj *value = NULL;
    // expect string number boolean object array null
    skip_space(reader);
    if (expect(reader, '{')) {
#ifdef DEBUG
      printf("{");
#endif  // DEBUG
      value = json_parse_object(reader);
    } else if (expect(reader, '[')) {
#ifdef DEBUG
      printf("[");
#endif  // DEBUG
      value = json_parse_array(reader);
    } else if (match(reader, '+') == MATCH_SUCCESS ||
               match(reader, '-') == MATCH_SUCCESS || match_digit(reader)) {
      value = json_parse_number(reader);
    } else if (match(reader, 't') == MATCH_SUCCESS ||
               match(reader, 'f') == MATCH_SUCCESS) {
      value = json_parse_boolean(reader);
    } else if (match(reader, 'n') == MATCH_SUCCESS) {
      value = json_parse_null(reader);
    } else if (expect(reader, '"')) {
#ifdef DEBUG
      printf("\"");
#endif  // DEBUG
      value = json_parse_string(reader);
    } else if (index == 0 && expect(reader, ']')) {
#ifdef DEBUG
      printf("]");
#endif  // DEBUG
      break;
    } else {
      unexpected(reader);
    }
    if (!value) return (NULL);
    value->key_value = false;
    key->value = value;
    skip_space(reader);
    // expect , or }
    if (match(reader, ',') == MATCH_SUCCESS) {
      expect(reader, ',');
#ifdef DEBUG
      printf(",");
#endif  // DEBUG
    } else if (match(reader, ']') == MATCH_SUCCESS) {
      expect(reader, ']');
#ifdef DEBUG
      printf("]");
#endif  // DEBUG
      break;
    } else {
      unexpected(reader);
      return (NULL);
    }
    node = key;
    index++;
  }
  return head;
}

static JSONObj *json_parse_object(JSONReader *reader) {
  JSONObj *head = (JSONObj *)calloc(1, sizeof(JSONObj));
  head->value_type = false;
  JSONObj *node = head;
  uint32_t c = 0;
  for (;;) {
    // expect string
    skip_space(reader);
    // Empty object
    if (c == 0 && expect(reader, '}')) {
#ifdef DEBUG
      printf("}");
#endif  // DEBUG
      break;
    }
    if (!expect(reader, '"')) {
      return NULL;
    }
#ifdef DEBUG
    printf("\"");
#endif  // DEBUG
    JSONObj *key = json_parse_string(reader);
    if (!key) return (NULL);
    key->prev_key = node;
    key->key_value = true;
    node->next_key = key;
    // expect :
    skip_space(reader);
    if (!expect(reader, ':')) {
      return (NULL);
    }
#ifdef DEBUG
    printf(":");
#endif  // DEBUG
    JSONObj *value = NULL;
    // expect string number boolean object array null
    skip_space(reader);
    if (expect(reader, '{')) {
#ifdef DEBUG
      printf("{");
#endif  // DEBUG
      value = json_parse_object(reader);
    } else if (expect(reader, '[')) {
#ifdef DEBUG
      printf("[");
#endif  // DEBUG
      value = json_parse_array(reader);
    } else if (match(reader, '+') == MATCH_SUCCESS ||
               match(reader, '-') == MATCH_SUCCESS || match_digit(reader)) {
      value = json_parse_number(reader);
    } else if (match(reader, 't') == MATCH_SUCCESS ||
               match(reader, 'f') == MATCH_SUCCESS) {
      value = json_parse_boolean(reader);
    } else if (match(reader, 'n') == MATCH_SUCCESS) {
#ifdef DEBUG
      printf("n");
#endif  // DEBUG
      value = json_parse_null(reader);
    } else if (expect(reader, '"')) {
#ifdef DEBUG
      printf("\"");
#endif  // DEBUG
      value = json_parse_string(reader);
    } else {
      // Unexpect
      return (NULL);
    }
    if (!value) return (NULL);
    value->key_value = false;
    key->value = value;
    skip_space(reader);
    // expect , or }
    if (match(reader, ',') == MATCH_SUCCESS) {
      expect(reader, ',');
#ifdef DEBUG
      printf(",");
#endif  // DEBUG
    } else if (match(reader, '}') == MATCH_SUCCESS) {
      expect(reader, '}');
#ifdef DEBUG
      printf("}");
#endif  // DEBUG
      break;
    } else {
      // unexpect
    }
    node = key;
    c++;
  }
  return head;
}

static bool json_string_validator(char *s, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    char c = s[i];
    if (c == '\\') {
      c = s[++i];
      if (c == 'x' || c == 'X') {
        if (i + 2 > len - 1) {
          printf("\\x must follow 2 hexadecimal number\n");
          return false;
        }
        char v = 0;
        for (int j = 0; j < 2; j++) {
          if (!ishexnumber(c)) {
            printf("Invalid hexadecimal number %c(%d)\n", c, c);
            return false;
          }
          c = s[++i] - '0';
          v += (c << (1 - j));
        }
        if (iscntrl(v)) {
          printf("Control character is not allowed %c(%d)\n", v, v);
          return false;
        }
      } else if (c == 'u') {
        if (i + 4 > len - 1) {
          printf("\\u must follow 4 hexadecimal number\n");
          return false;
        }
        for (int j = 0; j < 4; j++) {
          c = s[++i];
          if (!ishexnumber(c)) {
            printf("Unknown escaped word %c(%d)\n", c, c);
            return false;
          }
        }
      } else if (c != '"' && c != '\\' && c != '/' && c != 'b' && c != 'f' &&
                 c != 'n' && c != 'r' && c != 't') {
        printf("Unknown escaped word %c(%d)\n", c, c);
        return false;
      }
    }
  }
  return true;
}
static struct json_query_command *json_query_command_parser(
    char *json_query_command) {
  if (!*json_query_command) {
    return (NULL);
  }

  struct json_query_command *query_command =
                                calloc(1, sizeof(struct json_query_command)),
                            *p;
  p = query_command;
  int i = 0, j = 0;
  const size_t len = strlen(json_query_command);

  while (i <= strlen(json_query_command)) {
    char w = json_query_command[i];
    /* Space is invalid */
    if (isspace(w)) {
      fprintf(stdout, "[JSON Query Command Error] Invalid character: '%c'\n",
              w);
      json_query_command_free(query_command);
      return (NULL);
    }
    /* Digit, alpha and '_' is valid */
    else if (isalpha(w) || isdigit(w) || w == '_') {
      i++;
    }
    /* Dot is command splitter */
    else if (i == strlen(json_query_command) || w == '.') {
      p->next_cmd = calloc(1, sizeof(struct json_query_command));
      p->next_cmd->len = i - j;
      p->next_cmd->command_words = calloc(p->next_cmd->len + 1, sizeof(char));
      strncpy(p->next_cmd->command_words, json_query_command + j,
              p->next_cmd->len);
      p = p->next_cmd;
      j = ++i;
    } else {
      fprintf(stdout, "[JSON Query Command Error] Invalid character: '%c'\n",
              w);
      json_query_command_free(query_command);
      return (NULL);
    }
  }

  return (query_command);
}

static void json_query_command_free(struct json_query_command *commands) {
  if (!commands) return;
  struct json_query_command *c = commands->next_cmd;

  while (c) {
    struct json_query_command *p = c->next_cmd;
    if (c->command_words) {
      free(c->command_words);
    }
    free(c);
    c = p;
  }
  if (commands->command_words) free(commands->command_words);
  free(commands);
}

static char json_next(JSONReader *reader) {
  if (reader == NULL || reader->cursor == reader->length - 1) return 0;
  return reader->json[reader->cursor++];
}
struct json_obj *from_json(const char *json, uint32_t size) {
  JSONReader *reader = (JSONReader *)calloc(1, sizeof(JSONReader));
  if (reader == NULL) {
    return NULL;
  }
  reader->json = json;
  reader->length = size;
  reader->cursor = -1;
  struct json_obj *obj = json_parser(reader);
  free(reader);
  return (obj);
}

void json_obj_free(struct json_obj *obj) {
  if (!obj) {
    return;
  }
  struct json_obj *next = obj->next_key;

  while (next) {
    struct json_obj *p = next->next_key;

    switch (next->value->value_type) {
      case V_OBJECT:
      case V_ARRAY:
        json_obj_free(obj->value);
        break;
      default:
        if (next->value->data) {
          free(next->value->data);
        }
        break;
    }
    free(next->value);
    if (next->data) {
      free(next->data);
    }
    free(next);
    next = p;
  }
  if (obj->data) free(obj->data);
  free(obj);
}

/**
 *  @Description: Firstly, when you set two or more values to a same key in
 * object, we only take the final value as the value of the key. Secondly
 */
struct json_obj *json_query(char *command, struct json_obj *obj) {
  if (!obj) return (NULL);
  struct json_query_command *cmd = json_query_command_parser(command);
  cmd = cmd->next_cmd;
  if (!cmd) return (NULL);

  /* Execute querying */
  while (cmd && obj) {
    /* The obj is object or array of head */
    struct json_obj *o = NULL, *r = NULL;
    for (o = obj->next_key; o; o = o->next_key) {
      if (!strncmp(o->data, cmd->command_words, strlen(cmd->command_words))) {
        goto next;
      }
    }
    /* Not found the key */
    if (!o) {
      fprintf(stderr, "[JSON Query Error] Not found the key: %s\n",
              cmd->command_words);
      json_query_command_free(cmd);
      return (NULL);
    }
  next:
    obj = o->value;
    cmd = cmd->next_cmd;
  }
  json_query_command_free(cmd);

  if (!obj) {
    fprintf(stderr, "[JSON Query Error] Not found the key: %s\n",
            cmd->command_words);
  }
  return (obj);
}

double json_get_double(struct json_obj *obj, char *err_msg) {
  double f = 0;

  if (!obj) {
    if (err_msg) {
      sprintf(err_msg, "Null pointer");
    }
    return (f);
  }
  int sign = 1, exp_sign = 1, i = 0; /* 1 is positive, -1 is negative */
  char integer_part[BUFSIZ], fractional_part[BUFSIZ], exponent_part[BUFSIZ];
  bzero(integer_part, BUFSIZ);
  bzero(fractional_part, BUFSIZ);
  bzero(exponent_part, BUFSIZ);

  /* Hexadecimal number */
  if (obj->data[i] == '0' ||
      (obj->data[i + 1] == 'x' || obj->data[i + 1] == 'X')) {
    return (json_get_long(obj, err_msg));
  }
  /* Decimal number */
  if (obj->data[i] == '+') {
    sign = 1;
    i++;
  } else if (obj->data[i] == '-') {
    sign = -1;
    i++;
  }

  for (size_t j = 0; obj->data[i] != '.' && i < strlen(obj->data); j++, i++) {
    integer_part[j] = obj->data[i];
  }
  /* Like 123 */
  if (i == strlen(obj->data)) {
    goto integer_exit;
  }
  /* Skip dot */
  i++;
  /* Copy */
  for (size_t j = 0;
       obj->data[i] != 'e' && obj->data[i] != 'E' && i < strlen(obj->data);
       j++, i++) {
    fractional_part[j] = obj->data[i];
  }

  /* Like 123.123, but we only get integer part */
  if (i == strlen(obj->data)) {
    goto integer_exit;
  }

  i++;
  if (obj->data[i] == '+') {
    exp_sign = 1;
    i++;
  } else if (obj->data[i] == '-') {
    exp_sign = -1;
    i++;
  }

  for (size_t j = 0; i < strlen(obj->data); i++, j++) {
    exponent_part[j] = obj->data[i];
  }

  long exp = atol(exponent_part);
  /* Like 23.45e+15 */
  if (exp_sign > 0) {
    int j = 0, i = strlen(integer_part);
    for (; j < exp && j < strlen(fractional_part); i++, j++) {
      integer_part[i] = fractional_part[j];
    }

    if (j < exp) {
      for (; j < exp; j++, i++) {
        integer_part[i] = '0';
      }
    } else if (j < strlen(fractional_part)) {
      integer_part[i++] = '.';
      for (; j < strlen(fractional_part); j++, i++) {
        integer_part[i] = fractional_part[j];
      }
    }
    return (atof(integer_part));
  }
  /* Like 1.1e-1 */
  else {
    char new_value[BUFSIZ] = {0};
    int i = 0, j = 0;
    if (exp < strlen(integer_part)) {
      for (; i < strlen(integer_part) - exp; i++, j++) {
        new_value[i] = integer_part[j];
      }
      new_value[i++] = '.';
    } else {
      new_value[i++] = '0';
      new_value[i++] = '.';
      for (; i < exp - strlen(integer_part) + 2; i++) {
        new_value[i] = '0';
      }
    }
    for (; j < strlen(integer_part); j++, i++) {
      new_value[i] = integer_part[j];
    }

    for (j = 0; j < strlen(fractional_part); j++, i++) {
      new_value[i] = fractional_part[j];
    }
    f = atof(new_value);
    return (f);
  }
integer_exit:
  f = atof(integer_part);
  return (f);
}

long json_get_long(struct json_obj *obj, char *err_msg) {
  long l = 0;

  if (!obj) {
    if (err_msg) {
      sprintf(err_msg, "Null pointer");
    }
    return (l);
  }
  int sign = 1, exp_sign = 1, i = 0; /* 1 is positive, -1 is negative */
  char integer_part[BUFSIZ], fractional_part[BUFSIZ], exponent_part[BUFSIZ];
  bzero(integer_part, BUFSIZ);
  bzero(fractional_part, BUFSIZ);
  bzero(exponent_part, BUFSIZ);

  /* Hexadecimal number */
  if (obj->data[i] == '0' ||
      (obj->data[i + 1] == 'x' || obj->data[i + 1] == 'X')) {
    i = i + 2;
    for (; i < strlen(obj->data); i++) {
      if (isdigit(obj->data[i])) {
        l = l * 16 + obj->data[i] - '0';
      } else if (obj->data[i] >= 'A' && obj->data[i] <= 'F') {
        l = l * 16 + obj->data[i] - 'A' + 10;
      } else if (obj->data[i] >= 'a' && obj->data[i] <= 'f') {
        l = l * 16 + obj->data[i] - 'a' + 10;
      } else {
        if (err_msg) {
          sprintf(err_msg, "Unexpected hexadecimal character: %c",
                  obj->data[i]);
        }
        return (0);
      }
    }
    return (l);
  }

  if (obj->data[i] == '+') {
    sign = 1;
    i++;
  } else if (obj->data[i] == '-') {
    sign = -1;
    i++;
  }

  for (size_t j = 0; obj->data[i] != '.' && i < strlen(obj->data); j++, i++) {
    integer_part[j] = obj->data[i];
  }
  /* Like 123 */
  if (i == strlen(obj->data)) {
    goto integer_exit;
  }
  /* Skip dot */
  i++;
  /* Copy */
  for (size_t j = 0;
       obj->data[i] != 'e' && obj->data[i] != 'E' && i < strlen(obj->data);
       j++, i++) {
    fractional_part[j] = obj->data[i];
  }

  /* Like 123.123, but we only get integer part */
  if (i == strlen(obj->data)) {
    goto integer_exit;
  }

  i++;
  if (obj->data[i] == '+') {
    exp_sign = 1;
    i++;
  } else if (obj->data[i] == '-') {
    exp_sign = -1;
    i++;
  }

  for (size_t j = 0; i < strlen(obj->data); i++, j++) {
    exponent_part[j] = obj->data[i];
  }

  long exp = atol(exponent_part);
  l = atol(integer_part);
  /* Like 23.45e+15 */
  if (exp_sign > 0) {
    for (long i = exp; i; i--) {
      l *= 10;
    }
    char *frac = calloc(exp, sizeof(char));
    memset(frac, '0', exp);
    /* The value of exp may bigger than strlen(fractional_part) */
    strncpy(frac, fractional_part,
            exp > strlen(fractional_part) ? strlen(fractional_part) : exp);
    l += atol(frac);
    free(frac);
    return (l);
  }
  /* Like 1.1e-1, its decimalism is 0.11, and part of integer is zero */
  else {
    for (long i = exp; i; i--) {
      l /= 10;
    }
    return (l);
  }
integer_exit:
  l = atol(integer_part);
  return (l);
}

bool json_get_bool(struct json_obj *obj, char *err_msg) {
  if (!obj) return (false);
  if (obj->value_type != V_BOOL) {
    if (err_msg) {
      sprintf(err_msg, "Can not convert %s to Boolean",
              json_obj_type_strs[obj->value_type]);
      return false;
    }
  }

  return (!strncmp(obj->data, "true", strlen("true")));
}

char *json_get_string(struct json_obj *obj, char *err_msg) {
  if (!obj) return (NULL);

  char *data = calloc(strlen(obj->data), sizeof(char));
  strncpy(data, obj->data, strlen(obj->data));
  return (data);
}

int json_get_null(struct json_obj *obj, char *err_msg) {
  if (!obj) return (0);
  if (obj->value_type != V_NULL) {
    if (err_msg) {
      sprintf(err_msg, "Can not convert %s to Null\n",
              json_obj_type_strs[obj->value_type]);
      return (0);
    }
  }
  return (1);
}

/**
 *  Modify the value of corresponding key
 */
int json_update(char *command, struct json_obj *obj, struct json_obj *value) {
  if (!obj) return (1);
  struct json_query_command *cmd = json_query_command_parser(command);
  struct json_query_command *c = cmd->next_cmd;
  if (!c) return (1);

  /* Execute searching */
  while (c && obj) {
    /* The obj is object or array of head */
    struct json_obj *o = NULL, *r = NULL;
    for (o = obj->next_key; o; o = o->next_key) {
      if (!strncmp(o->data, c->command_words, strlen(c->command_words))) {
        goto next;
      }
    }
    /* Not found the key */
    if (!o) {
      fprintf(stderr, "[JSON Update Error] Not found the key: %s\n",
              c->command_words);
      json_query_command_free(cmd);
      return (1);
    }
  next:
    if (c->next_cmd)
      obj = o->value;
    else
      obj = o;
    c = c->next_cmd;
  }

  json_query_command_free(cmd);
  if (obj) {
    json_obj_free(obj->value);
    obj->value = value;
  } else {
    fprintf(stderr, "[JSON Update Error] Not found the key: %s\n",
            c->command_words);
    return (1);
  }
  return (0);
}

/**
 * Add a new key and its value.
 * */
int json_add(char *command, struct json_obj *obj, struct json_obj *value) {
  struct json_query_command *cmd = json_query_command_parser(command);
  cmd = cmd->next_cmd;
  if (!cmd || !obj) return (1);

  /* Execute searching */
  while (cmd) {
    /* The obj is object or array of head */
    struct json_obj *o = NULL;
    for (o = obj->next_key; o; o = o->next_key) {
      if (!strncmp(o->data, cmd->command_words, strlen(cmd->command_words))) {
        goto next;
      }
    }
    /* Not found the key, create it */
    if (!o) {
      o = calloc(1, sizeof(struct json_obj));
      o->data = calloc(cmd->len, sizeof(char));
      strncpy(o->data, cmd->command_words, cmd->len);
      o->value_type = V_STRING;
      o->key_value = true;
      if (!obj->next_key) {
        obj->next_key = o;
        o->prev_key = obj;
      } else {
        struct json_obj *p = obj->next_key;
        obj->next_key = o;
        o->prev_key = obj;
        o->next_key = p;
        p->prev_key = o;
      }

      // Create a new node
      if (cmd->next_cmd) {
        o->value = calloc(1, sizeof(struct json_obj));
        o->value->value_type = V_OBJECT;
        o->value->key_value = true;
      }
    } else {
      if (o->value) json_obj_free(o->value);
      o->value = value;
    }
  next:
    if (cmd->next_cmd)
      obj = o->value;
    else
      obj = o;
    cmd = cmd->next_cmd;
  }
  /* If the value is not NULL, update it but free it fisrt */
  if (obj->value) free(obj->value);
  obj->value = value;
  json_query_command_free(cmd);
  return (0);
}

/**
 *  Delete key and its value in given command. If not found, throw error.
 */
int json_delete(char *command, struct json_obj *obj) {
  struct json_query_command *cmd = json_query_command_parser(command);
  cmd = cmd->next_cmd;
  if (!cmd) return (1);

  /* Execute seaching */
  while (cmd && obj) {
    /* The obj is object or array of head */
    struct json_obj *o = NULL, *r = NULL;
    for (o = obj->next_key; o; o = o->next_key) {
      if (!strncmp(o->data, cmd->command_words, strlen(cmd->command_words))) {
        goto next;
      }
    }
    /* Not found the key */
    if (!o) {
      fprintf(stderr, "[JSON Delete Error] Not found the key: %s\n",
              cmd->command_words);
      json_query_command_free(cmd);
      return (1);
    }
  next:
    if (cmd->next_cmd)
      obj = o->value;
    else
      obj = o;
    cmd = cmd->next_cmd;
  }
  json_query_command_free(cmd);

  if (!obj) {
    fprintf(stderr, "[JSON Delete Error] Not found the key: %s\n",
            cmd->command_words);
    return (1);
  } else {
    if (obj->next_key) obj->next_key->prev_key = obj->prev_key;
    obj->prev_key->next_key = obj->next_key;
    struct json_obj *head = obj->prev_key;
    /* Remap array and its keys */
    /* Look back to the head */
    while (head->prev_key) {
      head = head->prev_key;
    }

    if (head->value_type == V_ARRAY) {
      uint32_t index = atol(obj->data);
      struct json_obj *p = obj->next_key;

      while (p) {
        char tmp[BUFSIZ] = {0};
        sprintf(tmp, "%u", index);
        if (strlen(p->data) < strlen(tmp)) {
          p->data = realloc(p, strlen(tmp));
        }
        strncpy(p->data, tmp, strlen(tmp));
        p = p->next_key;
        index++;
      }
    }
    json_obj_free(obj->value);
    free(obj->data);
    free(obj);
  }
  return (0);
}

int to_json(struct json_obj *root, char *json_content, uint32_t size) {
  if (!json_content) return (-1);
  bzero(json_content, size);
  return (json_build(root, json_content, size, false, -1));
}

static int json_build(struct json_obj *root, char *json_content, uint32_t size,
                      bool beautfy, int tab_size) {
  if (!root) return (-1);
  int built_size = 1;
  /* Build object */
  if (root->value_type == V_OBJECT) {
    *json_content = '{';
    built_size = json_build_object(root->next_key, json_content, size,
                                   built_size, beautfy, tab_size, 1);
  }
  /* Build array */
  else if (root->value_type == V_ARRAY) {
    *json_content = '[';
    built_size = json_build_array(root->next_key, json_content, size,
                                  built_size, beautfy, tab_size, 1);
  }
  /* Structure only */
  else if (root->value_type == V_STRING) {
    built_size = json_build_string(root->data, json_content, size, 0);
  } else if (root->value_type == V_BOOL || root->value_type == V_NUMBER ||
             root->value_type == V_NULL) {
    for (; *(root->data + built_size); built_size++) {
      json_content[built_size] = root->data[built_size];
    }
  }
  /* Other type is invalid */
  else {
    fprintf(stdout,
            "[JSON Build Error] Expected object or array as root in JSON\n");
    return (0);
  }
  return (built_size);
}

static int json_build_object(struct json_obj *obj, char *json_content,
                             uint32_t size, uint32_t built_size, bool beautfy,
                             int tab_size, int layer) {
  /* Empty object */
  if (!obj) {
    *(json_content + built_size++) = '}';
    return (built_size);
  }

  if (beautfy) {
    *(json_content + built_size++) = '\n';
    for (uint32_t i = 0; i < tab_size * layer; i++)
      *(json_content + built_size++) = ' ';
  }
  /* Non-empty object */
  /* Build key */
  if (obj->value_type != V_STRING) {
    fprintf(stdout,
            "[JSON Build Error] JSON's key should be String, not '%s'\n",
            json_obj_type_strs[obj->value_type]);
    return (0);
  }

  built_size = json_build_string(obj->data, json_content, size, built_size);

  /* Colon */
  *(json_content + built_size++) = ':';

  if (beautfy) *(json_content + built_size++) = ' ';

  /* Build value */

  built_size = json_build_dispatch(obj->value, json_content, size, built_size,
                                   beautfy, tab_size, layer);

  /* Next key or end of object */
  if (!obj->next_key) {
    if (beautfy) {
      *(json_content + built_size++) = '\n';
      for (uint32_t i = 0; i < tab_size * (layer - 1); i++)
        *(json_content + built_size++) = ' ';
    }

    *(json_content + built_size++) = '}';
  } else if (obj->next_key->value_type == V_STRING) {
    *(json_content + built_size++) = ',';
    built_size = json_build_object(obj->next_key, json_content, size,
                                   built_size, beautfy, tab_size, layer);
  } else {
    fprintf(stdout,
            "[JSON Build Error] Excepted type of String as object key\n");
    return (0);
  }
  return (built_size);
}

static int json_build_string(char *string, char *json_content, uint32_t size,
                             uint32_t built_size) {
  *(json_content + built_size++) = '\"';

  for (size_t i = 0; i < strlen(string); i++) {
    switch (string[i]) {
      case '\r':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 'r';
        break;
      case '\b':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 'b';
        break;
      case '\n':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 'n';
        break;
      case '\t':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 't';
      case '\f':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 'f';
      case '\v':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = 'v';
        break;
      case '"':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = '"';
        break;
      case '\\':
        *(json_content + built_size++) = '\\';
        *(json_content + built_size++) = '\\';
        break;
      default:
        *(json_content + built_size++) = string[i];
        break;
    }
  }

  *(json_content + built_size++) = '\"';
  return (built_size);
}

static int json_build_array(struct json_obj *obj, char *json_content,
                            uint32_t size, uint32_t built_size, bool beautfy,
                            int tab_size, int layer) {
  /* Empty array */
  if (!obj) {
    *(json_content + built_size++) = ']';
    return (built_size);
  }

  if (beautfy) {
    *(json_content + built_size++) = '\n';
    for (uint32_t i = 0; i < tab_size * layer; i++)
      *(json_content + built_size++) = ' ';
  }
  /* Non-empty array */
  /* Build Value */
  built_size = json_build_dispatch(obj->value, json_content, size, built_size,
                                   beautfy, tab_size, layer);

  /* Next value or end of array */
  if (!obj->next_key) {
    if (beautfy) {
      *(json_content + built_size++) = '\n';
      for (uint32_t i = 0; i < tab_size * (layer - 1); i++)
        *(json_content + built_size++) = ' ';
    }
    *(json_content + built_size++) = ']';
  } else if (obj->next_key->value_type == V_NUMBER) {
    *(json_content + built_size++) = ',';
    built_size = json_build_array(obj->next_key, json_content, size, built_size,
                                  beautfy, tab_size, layer);
  } else {
    fprintf(stdout, "[JSON Build Error] Excepted String value\n");
    return (0);
  }

  return (built_size);
}

static int json_build_dispatch(struct json_obj *obj, char *json_content,
                               uint32_t size, uint32_t built_size, bool beautfy,
                               int tab_size, int layer) {
  if (!obj) return (0);

  switch (obj->value_type) {
    case V_NUMBER:
    case V_BOOL:
    case V_NULL:
      strncpy(json_content + built_size, obj->data, strlen(obj->data));
      built_size += strlen(obj->data);
      break;
    case V_STRING:
      built_size = json_build_string(obj->data, json_content, size, built_size);
      break;
    case V_OBJECT:
      *(json_content + built_size++) = '{';
      built_size = json_build_object(obj->next_key, json_content, size,
                                     built_size, beautfy, tab_size, layer + 1);
      break;
    case V_ARRAY:
      *(json_content + built_size++) = '[';
      built_size = json_build_array(obj->next_key, json_content, size,
                                    built_size, beautfy, tab_size, layer + 1);
      break;
    default:
      fprintf(stdout, "[JSON Build Error] \n");
      return (0);
      break;
  }
  return (built_size);
}

/* Simple deserialization wrapper */
struct json_obj *load_single_value(char *str, int value_type) {
  struct json_obj *obj = calloc(1, sizeof(struct json_obj));
  if (!obj) return (NULL);

  obj->data = calloc(strlen(str) + 1, sizeof(char));
  obj->value_type = value_type;
  strncpy(obj->data, str, strlen(str));

  if (value_type == V_NUMBER) free(str);
  return (obj);
}

struct json_obj *load_object(struct json_obj *args, ...) {
  struct json_obj *head = calloc(1, sizeof(struct json_obj)), *o;
  if (!head) return (NULL);
  head->value_type = V_OBJECT;
  o = head;

  va_list argp;
  int count = 0;
  va_start(argp, args);
  struct json_obj *obj = args, *old = NULL;
  for (;;) {
    if (count) obj = va_arg(argp, struct json_obj *);
    if (!obj) break;
    /* Key */
    if (count % 2 == 0) {
      obj->key_value = true;
      /* Verify if it has a same key */
      /* Look back */
      struct json_obj *prev = o, *target = NULL;
      while (prev) {
        if (prev->data && !strncmp(prev->data, obj->data, strlen(obj->data))) {
          target = prev;
        }
        prev = prev->prev_key;
      }
      if (target) {
        /* Store current key address if find a same key */
        old = o;
        o = target;
      } else {
        /* If old is not NULL, restore it */
        if (old) {
          o = old;
          /* Clear the old */
          old = NULL;
        }
        o->next_key = obj;
        obj->prev_key = o;
        o = o->next_key;
      }
    }
    /* Value */
    else {
      obj->key_value = false;
      o->value = obj;
    }
    count++;
  }
  va_end(argp);
  return (head);
}

struct json_obj *load_array(struct json_obj *args, ...) {
  struct json_obj *head = calloc(1, sizeof(struct json_obj)), *o;
  if (!head) return (NULL);
  head->value_type = V_ARRAY;
  o = head;

  va_list argp;
  int key = 0;
  va_start(argp, args);
  struct json_obj *obj = args, *new_key;

  for (;;) {
    if (key) obj = va_arg(argp, struct json_obj *);
    if (!obj) break;
    /* Create a key */
    new_key = calloc(1, sizeof(struct json_obj));
    new_key->value_type = V_NUMBER;
    new_key->key_value = true;
    char *num_str = json_utils_long2str(key);
    new_key->data = calloc(strlen(num_str) + 1, sizeof(char));
    strncpy(new_key->data, num_str, strlen(num_str));
    free(num_str);

    /* Copy data */
    obj->key_value = false;
    o->next_key = new_key;
    new_key->prev_key = o;
    new_key->value = obj;
    o = new_key;
    key++;
  }
  va_end(argp);
  return (head);
}

/* Utils */
char *json_utils_long2str(long value) {
  char *str = calloc(BUFSIZ, sizeof(char));
  if (!str) return (NULL);

  sprintf(str, "%ld", value);

  return (str);
}

char *json_utils_double2str(double value) {
  char *str = calloc(BUFSIZ, sizeof(char));
  if (!str) return (NULL);

  sprintf(str, "%f", value);

  return (str);
}

int json_utils_beautify(char *input, char *output, int tab_size) {
  struct json_obj *obj = from_json(input, strlen(input));
  if (!obj) return (1);
  tab_size = tab_size <= 0 ? DEFAULT_TAB_SIZE : tab_size;
  return (!(json_build(obj, output, -1, true, tab_size) > 0));
}

int json_utils_minify(char *input, char *output) {
  struct json_obj *obj = from_json(input, strlen(input));
  if (!obj) return (1);
  return (!(json_build(obj, output, -1, false, -1) > 0));
}

static uint32_t json_utils_hex2uint(char *hex) {
  uint32_t v = 0;
  for (int i = 0; i < strlen(hex); i++) {
    v = v * 16 + (isdigit(hex[i]) ? hex[i] - '0'
                                  : isupper(hex[i]) ? hex[i] - 'A' + 10
                                                    : hex[i] - 'a' + 10);
  }
  return (v);
}

static size_t json_utils_wstrlen(char *s, int encoding, size_t slen) {
  size_t len = 0;
  if (encoding == W_UTF8) {
    for (size_t i = 0; i < slen;) {
      if (s[i] >> 7 == 0) {
        len++;
        i++;
      } else if ((s[i] >> 5 & 0xff) == 0xf6) {
        len++;
        i += 2;
      } else if ((s[i] >> 4 & 0xff) == 0xfe) {
        len++;
        i += 3;
      }
    }
  }

  return (len);
}

/**
 *  Note that the width of Chinese fonts is twice bigger than English's.
 */

static size_t json_utils_console_wstrlen(char *s, int encoding, size_t slen) {
  size_t len = 0;
  if (encoding == W_UTF8) {
    for (size_t i = 0; i < slen;) {
      if (s[i] >> 7 == 0) {
        len++;
        i++;
      } else if ((s[i] >> 5 & 0xff) == 0xf6) {
        len += 2;
        i += 2;
      } else if ((s[i] >> 4 & 0xff) == 0xfe) {
        len += 2;
        i += 3;
      } else {
        len += 2;
        i += 4;
      }
    }
  }

  return (len);
}

bool json_iterable(struct json_obj *obj) {
  if (!obj) return (false);
  return (obj->value_type == V_OBJECT || obj->value_type == V_ARRAY);
}

struct json_obj *json_next_key(struct json_obj *obj) {
  // printf("Before:\n");
  if (!obj) return (NULL);
  // printf("key: %s\n", obj->data);
  return (obj->next_key);
}

struct json_obj *json_begin(struct json_obj *obj) {
  if (!json_iterable(obj)) {
    return (NULL);
  }
  return (obj->next_key);
}

struct json_obj *json_get_value(struct json_obj *obj) {
  if (!obj || !obj->key_value) {
    return (obj);
  }
  return (obj->value);
}

char *json_get_key(struct json_obj *obj) {
  if (!obj || !obj->key_value || obj->value_type == V_ARRAY ||
      obj->value_type == V_OBJECT) {
    return (NULL);
  }
  return (obj->data);
}