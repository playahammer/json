
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define FALSE_LEN strlen("false")
#define TRUE_LEN strlen("true")

static const char *bool_str[] = {
      "false",
      "true"
};

static const char *token_type_strs[] = {
      "Left Bucket",
      "Right Bucket",
      "String",
      "Boolean",
      "Number",
      "Null",
      "Left Square Bucket",
      "Right Square Bucket",
      "Colon",
      "Comma",
      NULL
};

static const char *json_obj_type_strs[] = {
      "Object",
      "Array",
      "String",
      "Boolean",
      "Number",
      "Null",
      NULL
};

static struct json_tracker* 
json_tracker_init(char *json_content)
{
      struct json_tracker *tracker = calloc(1, sizeof(struct json_tracker)), *p;
      tracker->row = 0;
      p = tracker;
      size_t tracker_begin = 0, line = 1;

      if (json_content[strlen(json_content) - 1] != '\n') {
            json_content[strlen(json_content)] = '\n';
      }
      
      for (size_t i = 0; i < strlen(json_content); i++) {
            if (json_content[i] == '\n') {
                  struct json_tracker *q = calloc(1, sizeof(struct json_tracker));
                  q->row = line++;
                  q->line = calloc(i - tracker_begin + 1, sizeof(char));
                  strncpy(q->line, json_content + tracker_begin, i - tracker_begin);
                  p->next = q;
                  p = q;
                  tracker_begin = i + 1;
            }
      }
      return (tracker);
}

static void 
json_tracker_print(struct json_tracker *tracker, uint32_t line, uint32_t col)
{
      if (line <= 0) return;

      struct json_tracker *t = tracker->next;

      /* Traceback up four rows of JSON content from the given line */
      while (t && (long)line - 4 >= t->row) t = t->next;
      
      for (; t && line >= t->row; t = t->next) {
            printf("%d%s\n", t->row, t->line);
      }

      /* Print space */
      while (line) {
            printf(" ");
            line /= 10;
      }
      for (int i = 0; i < col; i++) {
            printf(" "); 
      } 
      /* Print ^ */
      printf("^");

      /* Free tracker */
      json_tracker_free(tracker);
}

static void 
json_tracker_free(struct json_tracker *tracker)
{
      if (!tracker) {
            return;
      } 

      struct json_tracker *t = tracker->next;

      while (t) {
            struct json_tracker *p = t->next;
            if (t->line) free(t->line);
            free(t);
            t = p; 
      }
      free(tracker);
}

/**
 *  Turn json content of input into tokens.
*/
struct json_token*
json_lexer(char *json_content, struct json_tracker *tracker)
{
      struct json_token *head = (struct json_token *)calloc(1, sizeof(struct json_token));
      struct json_token *p = head, *q;
      int i = 0, j = 0;
      uint32_t row = 1, col = 0;
      const uint32_t json_len = strlen(json_content);

      while (i < json_len) {
            bool token_bool = false;
            switch (json_content[i]) {
                  case '{':
                        q = calloc(1, sizeof(struct json_token)); 
                        q->token_len = 1;
                        q->token_type = LEFT_BUCKET;
                        q->token_words = (char *)calloc(2, sizeof(char));
                        q->col = col;
                        q->row = row;
                        q->token_words[0] = '{';
                        // printf("Type: BUCKET_LEFT \ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case '}':
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = 1;
                        q->token_type = RIGHT_BUCKET;
                        q->token_words = calloc(2, sizeof(char));
                        q->token_words[0] = '}';
                        q->col = col;
                        q->row = row;
                        // printf("Type: BUCKET_RIGHT \ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case '[':
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = 1;
                        q->token_type = LEFT_S_BUCKET;
                        q->token_words = calloc(2, sizeof(char));
                        q->token_words[0] = '[';
                        q->col = col;
                        q->row = row;
                        // printf("Type: S_BUCKET_LEFT \ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case ']':
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = 1;
                        q->token_type = RIGHT_S_BUCKET;
                        q->token_words = calloc(2, sizeof(char));
                        q->token_words[0] = ']';
                        q->col = col;
                        q->row = row;
                        // printf("Type: S_BUCKET_RIGHT \ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case ':':
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = 1;
                        q->token_type = COLON;
                        q->token_words = calloc(2, sizeof(char));
                        q->token_words[0] = ':';
                        q->col = col;
                        q->row = row;
                        // printf("Type: COLON \t\ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case ',':
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = 1;
                        q->token_type = COMMA;
                        q->token_words = calloc(2, sizeof(char));
                        q->token_words[0] = ',';
                        q->col = col;
                        q->row = row;
                        // printf("Type: COMMA \t\ttoken: %c\n", q->token_words[0]);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case ' ': /* Space should be skipped */
                        break;
                  case '"': /* String */
                        /* TODO: Support utf-8 encoding */
                        p = json_string_lexer(p, &col, row, json_content, (uint32_t *)&i, json_len, tracker);
                        if (!p) {
                              json_token_free(head);
                              return (NULL);
                        }
                        break;
                  case '\n': /* Move to new line */
                        row ++;
                        /* Reset col when turn into the new line */
                        col = -1; 
                        break;
                  case 'f': /* Boolean: false and true */
                  case 't':
                        j = col;
                        if(!strncmp("false", json_content + i, FALSE_LEN)) {
                              token_bool = false;
                              i += FALSE_LEN - 1;
                              col += FALSE_LEN - 1;
                        }
                        else if (!strncmp("true", json_content + i, TRUE_LEN)) {
                              token_bool = true;
                              i += TRUE_LEN - 1;
                              col += TRUE_LEN - 1;
                        }
                        else {
                              json_tracker_print(tracker, row, col);
                              fprintf(stdout, "[JSON Lexer Error] Unknown token: %c(%d)\n", json_content[i], json_content[i]);
                              json_token_free(head);
                              return (NULL);
                        }

                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = strlen(bool_str[token_bool]);
                        q->token_type = BOOLEAN;
                        q->token_words = calloc(strlen(bool_str[token_bool]) + 1, sizeof(char));
                        q->col = j;
                        q->row = row;
                        strncpy(q->token_words, bool_str[token_bool], strlen(bool_str[token_bool]));
                        // printf("Type: BOOLEAN \t\ttoken: %s\n", q->token_words);
                        p->next_token = q;
                        p = p->next_token;
                        break;
                  case 'n': /* null */
                        if (strncmp("null", json_content + i, strlen("null"))) {
                              json_tracker_print(tracker, row, col);
                              fprintf(stdout, "[JSON Lexer Error] Unknown token: %c(%d)\n", json_content[i], json_content[i]);
                              json_token_free(head);
                              return (NULL);
                        }
                        q = calloc(1, sizeof(struct json_token));
                        q->token_len = strlen("null");
                        q->token_type = NULL_OBJ;
                        q->token_words = calloc(strlen("null") + 1, sizeof(char));
                        q->col = col;
                        q->row = row;
                        strncpy(q->token_words, "null", strlen("null"));
                        // printf("Tyep: NULL_OBJ \t\ttoken: %s\n", q->token_words);
                        p->next_token = q;
                        p = p->next_token;
                        i += strlen("null") - 1;
                        col += strlen("null") - 1; 
                        break;
                  case '+':
                  case '-':
                  case '0':
                  case '1':
                  case '2':
                  case '3':
                  case '4':
                  case '5':
                  case '6':
                  case '7':
                  case '8':
                  case '9': /* Number including integer and float */
                        p = json_number_lexer(p, &col, row, json_content, (uint32_t *)&i, json_len);
                        if (!p) {
                              json_tracker_print(tracker, row, col);
                              fprintf(stdout, "[JSON Lexer Error] Invalid number");
                              json_token_free(head);
                              return (NULL);
                        }
                        i --;
                        col --;
                        break;
                  default:
                        if (isprint(json_content[i])) {
                              json_tracker_print(tracker, row, col);
                              fprintf(stdout, " [JSON Lexer Error] Unknown token %c(%d)\n", 
                                          json_content[i], json_content[i]);
                              json_token_free(head);
                              return (NULL);
                        }
                        break;
            }
            i++;
            col++;

      }

      return (head);
}

/**
 * Number := DecimalNumber | HexNumber
 * DecimalNumber := DecimalInteger [FractionalPart] [ExponentPart]
 * DecimalInteger := [+|-] DecimalDigits
 * DecimalDigits := [0-9]+
 * FractionalPart := "." DecimalDigits
 * ExponentPart := (E|e) DecimalInteger
 * HexNumber := ("0x"|"0X") DecimalDigits
 * 
*/
static struct json_token *
json_number_lexer(struct json_token *token, uint32_t *col, uint32_t row, char *json_content, uint32_t *i, uint32_t len)
{
      struct json_token *number_token = NULL;
      /* Store col value */
      int num_col = *col;
      uint32_t j = *i;
      /* Hexadecimal number */
      if (json_content[*i] == '0' && (json_content[*i + 1] == 'x' || json_content[*i + 1] == 'X')) {
            for((*i) += 2, (*col) += 2; *i < len && ishexnumber(json_content[*i]); (*i)++, (*col)++);
            /* At least it must have one decimal digit */
            if (j + 2 == *i) return (number_token);
            goto number_exit;
      }

      /* Decimal number */
      if (json_content[*i] == '+' || json_content[*i] == '-') (*i)++, (*col)++;
      /* Integer part */
      /* Number at least has one digit */
      if (!isdigit(json_content[*i])) {
            return (number_token);
      }
      for (; *i < len && isdigit(json_content[*i]); (*i)++, (*col)++);
      /* Fractional part */
      if (json_content[*i] == '.') {
            /* Fractional part must have one digit */
            if (!isdigit(json_content[*i + 1])) {
                  return (number_token);
            }
            for ((*i)++, (*col)++; *i < len && isdigit(json_content[*i]); (*i)++, (*col)++);

      } 
      /* Exponent part of scientific nation */
      if (json_content[*i] == 'E' || json_content[*i] == 'e') {
            (*i)++, (*col)++;
            if (json_content[*i] == '+' || json_content[*i] == '-') (*i)++, (*col)++;
            /* Must have one digital */
            if (!isdigit(json_content[*i])) {
                  return (number_token);
            }
            for (; *i < len && isdigit(json_content[*i]); (*i)++, (*col)++);
      }
      
      number_exit:
            number_token = calloc(1, sizeof(struct json_token));
            number_token->token_len = *i - j;
            number_token->token_type = NUMBER;
            number_token->token_words = calloc(*i - j + 1, sizeof(char));
            number_token->col = num_col;
            number_token->row = row;
            strncpy(number_token->token_words, json_content + j, *i - j);
            token->next_token = number_token;
            return (number_token);
}

static struct json_token *
json_string_lexer(struct json_token *token, uint32_t *col, uint32_t row, char *json_content, uint32_t *i, uint32_t len, struct json_tracker *tracker)
{
      struct json_token *new_token = NULL;
      uint32_t j;
      for (j = ++(*i); *i < len && json_content[*i] != '"'; (*i)++) {
            if (json_content[(*i) - 1] != '\\' &&  json_content[*i] == '\\' && json_content[(*i) + 1] == '"') (*i)++;
      }
      if ((*i) == len) {
            json_tracker_print(tracker, row, *col);
            fprintf(stdout, "[JSON Lexer Error] Expected a comma as string terminator\n");
            return (new_token);
      }
      new_token = calloc(1, sizeof(struct json_token));
      new_token->token_len = *i - j;
      new_token->token_type = STRING;
      new_token->token_words = calloc(new_token->token_len + 1, sizeof(char));
      new_token->col = *col;
      new_token->row = row;
      strncpy(new_token->token_words, json_content + j, *i - j);

      char *tmp = calloc(new_token->token_len + 1, sizeof(char));
      strncpy(tmp, new_token->token_words, strlen(new_token->token_words));
                        
      /* Process escape */
      size_t k = 0;
      for (size_t l = 0; l < new_token->token_len; l++, k++) {
            if (new_token->token_words[l] == '\\') {
                  switch (new_token->token_words[l + 1]) {
                        case 'r':
                              new_token->token_words[++l] = '\r';
                              break;
                        case 'b':
                              new_token->token_words[++l] = '\b';
                              break;
                        case 'n':
                              new_token->token_words[++l] = '\n';
                              break;
                        case 't':
                              new_token->token_words[++l] = '\t';
                              break;
                        case '\\':
                              case '"':
                              l++;
                              break;
                        case 'u':
                              /* Dealing with unicode */
                              l += 2;
                              char unicode[5] = {0};
                              for (size_t i = l, j = 0; l < i + 4  && l < new_token->token_len; l++) {
                                    if (!ishexnumber(new_token->token_words[l])) {
                                          json_tracker_print(tracker, new_token->row, new_token->col + json_utils_console_wstrlen(tmp, W_UTF8, l + 1));
                                          fprintf(stdout, "[JSON Lexer Error] Unknown unicode character: %c\n", tmp[l]);
                                          free(tmp);
                                          free(new_token->token_words);
                                          free(new_token);
                                          return (NULL);
                                    }
                                    unicode[j++] = new_token->token_words[l];
                              }
                              /* Invalid unicode */
                              if (strlen(unicode) < 4) {
                                          json_tracker_print(tracker, new_token->row, new_token->col + json_utils_console_wstrlen(tmp, W_UTF8, l + 1));
                                          fprintf(stdout, "[JSON Lexer Error] Unknown unicode: \\u%s\n", unicode);
                                          free(tmp);
                                          free(new_token->token_words);
                                          free(new_token);
                                          return (NULL);
                              }

                              uint32_t u = json_utils_hex2uint(unicode);
                                                
                              /* U+ 0000 ~ U+007F */
                              if (u <= json_utils_hex2uint("007F")) {
                                    new_token->token_words[k] = u & 0x7f;
                              }
                              /* U+ 0080 ~ U+07FF */
                              else if (u <= json_utils_hex2uint("07FF")) {
                                    new_token->token_words[k++] = (0x6 << 5) + (u >> 6);
                                    new_token->token_words[k] = (1 << 7) + (u & 0x3f);
                              }
                              /* U+ 0800 ~ U+FFFF */
                              else if (u <= json_utils_hex2uint("FFFF")){
                                    new_token->token_words[k++] = (0xe << 4) + (u >> 12);
                                    new_token->token_words[k++] = (1 << 7) + ((u >> 6) & 0x3f);
                                    new_token->token_words[k] = (1 << 7) + (u & 0x3f);
                              }
                              /*  U+10000 ~ U+10FFFF */
                              else {

                              }
                              l--;
                              continue; 
                        default:
                              json_tracker_print(tracker, new_token->row, new_token->col + json_utils_console_wstrlen(tmp, W_UTF8, l + 1));
                              fprintf(stdout, "[JSON Lexer Error] Unknown escape words %c%c\n", tmp[l], tmp[l + 1]);
                              free(tmp);
                              free(new_token->token_words);
                              free(new_token);
                              return(NULL);
                  }
            }
            new_token->token_words[k] = new_token->token_words[l];
      }
      new_token->token_words[k] = '\0';
      new_token->token_len = strlen(new_token->token_words);
      *col = *col + json_utils_console_wstrlen(tmp, W_UTF8, strlen(tmp)) + 1;
      free(tmp);
      // printf("Type: STRING \t\ttoken: %s\n", q->token_words);
      token->next_token = new_token;
      return (new_token);
}

static void 
json_token_free(struct json_token *token)
{
      if (!token) {
            return;
      }

      struct json_token *next = token->next_token;

      while (next) {
            struct json_token *p = next->next_token;
            if (next->token_words) {
                  free(token->token_words);
            }
            free(next);
            next = p;
      }
      free(token);
}

static struct json_obj* 
json_parser(struct json_token *head, struct json_tracker *tracker)
{
      #ifdef DEBUG
      printf("=> Parsing\n");
      #endif // DEBUG
      struct json_obj *root = (struct json_obj *)calloc(1, sizeof(struct json_obj));

      head = head->next_token;
      if (!head) {
            fprintf(stdout, "[JSON Parser Error] Empty JSON content\n");
            return (NULL);
      }

      /* Create a json error packet instance */
      struct json_error_pkt *e_pkt = calloc(1, sizeof(struct json_error_pkt));
      struct json_token *remain = NULL;

      /* When meet one left (square) bucket, create a new object(array) root node */
      if (head->token_type == LEFT_BUCKET) {
            #ifdef DEBUG
            printf("%s", head->token_words);
            #endif // DEBUG
            root->value_type = V_OBJECT;
            remain = json_parse_object(head, root, true, e_pkt);
            
      }
      else if (head->token_type == LEFT_S_BUCKET) {
            #ifdef DEBUG
            printf("%s", head->token_words);
            #endif // DEBUG
            root->value_type = V_ARRAY;
            remain = json_parse_array(head, root, true, e_pkt);
      }
      else {
            json_tracker_print(tracker, head->row, head->col);
            fprintf(stdout, "[JSON Parser Error] JSON shuold begin with left (square) bucket, but got %s\n",
                  token_type_strs[head->token_type]);
            return (NULL);
      }

      #ifdef DEBUG
      printf("\n");
      #endif // DEBUG

      if (!remain) {
            json_tracker_print(tracker, e_pkt->row, e_pkt->col);
            fprintf(stdout, "[JSON Parser Error] %s\n", e_pkt->err_msg);
            return (NULL);
      } 
      else if (remain->next_token) {
            json_tracker_print(tracker, remain->row, remain->col);
            fprintf(stdout, "[JSON Parser Error] Meet the whole JSON terminator but still have other words\n");
            return (NULL);
      }

      #ifdef DEBUG
      printf("=> Parsing end\n");
      #endif // DEBUG

      return (root);
}

/**
 *  @param token, struct json_token
 *  @param obj, struct json_obj
 *  @param can_end, bool. The flag to mark empty obejct.
 *  @param e_pkt, struct json_error_pkt. Record the error message.
 *  @return Current token, return null if error occured or end.
*/

static struct json_token * 
json_parse_object(struct json_token *token, struct json_obj *obj, bool can_end, struct json_error_pkt *e_pkt)
{
      /* Error occured */
      if (!token) 
            return (NULL);

      /* Move to the next token when the next token is not null */
      if (!token->next_token) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Object need a right bucket(})");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      token = token->next_token;

      /* Empty object */
      if (can_end && token->token_type == RIGHT_BUCKET) {
            #ifdef DEBUG
            printf("%s", token->token_words);
            #endif // DEBUG
            return (token);
      }

      /* Non-Empty Object */
      /* Key */
      if (token->token_type != STRING) {
            /* Key should be string type */
            sprintf(e_pkt->err_msg, "JSON's key should be string, but got %s", 
                  token_type_strs[token->token_type]);
            e_pkt->row = token->row;
            e_pkt->col = token->col;
            e_pkt->error_occur = true;
            return (NULL);
      }

      /* Firstly, look over the created keys weather it has a same key. */
      struct json_obj *prev = obj, *target = NULL;
      while (prev) {
            if (prev->data && !strncmp(prev->data, token->token_words, strlen(token->token_words))) {
                  target = prev;
            }
            prev = prev->prev_key;
      }
      /* If it dose not exsit same key, create a new key and copy the token words to the data field. */
      if (!target) {
            obj->next_key = calloc(1, sizeof(struct json_obj));
            obj->next_key->value_type = V_STRING;
            obj->next_key->data = calloc(strlen(token->token_words) + 1, sizeof(char));
            strncpy(obj->next_key->data, token->token_words, strlen(token->token_words));
            obj->key_value = true;
            obj->next_key->prev_key = obj;
            obj = obj->next_key;
      }

      #ifdef DEBUG
      printf("\"%s\"", token->token_words);
      #endif // DEBUG
      
      /* Colon */
      if (!token->next_token) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Expected a colon to separate key and value");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      token = token->next_token;

      if (token->token_type != COLON) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Expected a colon to separate key and value");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }

      #ifdef DEBUG
      printf("%s", token->token_words);
      #endif // DEBUG
      
      /* Value */
      /* If key is in the created keys, move to the target position. */
      if (target)
            token = json_parse_dispatch(token, target, e_pkt);
      /* If it's a new key, go on. */
      else
            token = json_parse_dispatch(token, obj, e_pkt);
      if (!token) return (NULL);  
      
      if (!token->next_token) {
            if (e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Missing object terminator");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      token = token->next_token;
      
      #ifdef DEBUG
      printf("%s", token->token_words);
      #endif // DEBUG
      /* Comma, Other keys and values behind current k-v */
      if (token->token_type == COMMA) {
            token = json_parse_object(token, obj, false, e_pkt);
      }
      /* Right bucket, end of object */
      else if (token->token_type != RIGHT_BUCKET) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Missing object terminator");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      return (token);
}

static struct json_token* 
json_parse_array(struct json_token *token, struct json_obj *obj, bool can_end, struct json_error_pkt *e_pkt)
{
      /* Error occured */
      if (!token) return (NULL);
      char tmp[BUFSIZ] = {0};
      
      /* Move to the next token when next token is not null */
      if (!token->next_token) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "Array need a right square bucket");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      
      /* Empty array */
      if (can_end && token->next_token->token_type == RIGHT_S_BUCKET) {
            #ifdef DEBUG
            printf("%s", token->next_token->token_words);
            #endif // DEBUG
            return (token->next_token);
      }
      
      /* Create a key as index for array */
      obj->next_key = calloc(1, sizeof(struct json_obj));
      obj->next_key->key_value = true;
      /* The data is key and number type, it must be index of array. */
      obj->next_key->value_type = V_NUMBER; 

      /* Head node of array */
      if (obj->value_type == V_ARRAY) {
            obj->next_key->data = calloc(2, sizeof(char));
            obj->next_key->data[0] = '0';
      }
      else {
            /* Compute index of next value */
            uint32_t index = atol(obj->data) + 1;
            sprintf(tmp, "%u", index);
            obj->next_key->data = calloc(strlen(tmp) + 1, sizeof(char));
            strncpy(obj->next_key->data, tmp, strlen(tmp));
      }
      obj = obj->next_key;

      /* Non-empty array */
      /* Array value */
      token = json_parse_dispatch(token, obj, e_pkt);
      if (!token) return (NULL);
      
      if (!token->next_token) {
            if (!e_pkt->error_occur) {
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
                  sprintf(e_pkt->err_msg, "Array need a comma to sperate value");
            }
            return (NULL);
      }

      token = token->next_token;
      
      #ifdef DEBUG
      printf("%s", token->token_words);
      #endif // DEBUG
      /* Other array value */
      if (token->token_type == COMMA) {
            token = json_parse_array(token, obj, false, e_pkt);
      }
      /* Right square bucket, end of array */
      else if (token->token_type == RIGHT_S_BUCKET) {
            token = token;
      }
      else {
            e_pkt->row = token->row;
            e_pkt->col = token->col;
            e_pkt->error_occur = true;
            sprintf(e_pkt->err_msg, "Missing array terminator");
            return (NULL);
      }

      return (token);
}

static struct json_token* 
json_parse_dispatch(struct json_token *token, struct json_obj *obj, struct json_error_pkt *e_pkt)
{
      if (!token) return (NULL);

      if (!token->next_token) {
            if (!e_pkt->error_occur) {
                  sprintf(e_pkt->err_msg, "JSON object need value");
                  e_pkt->row = token->row;
                  e_pkt->col = token->col;
                  e_pkt->error_occur = true;
            }
            return (NULL);
      }
      token = token->next_token;

      // Value
      switch (token->token_type)
      {
      case NUMBER:  
            /* Number */
      case BOOLEAN:
            /* Boolean */
      case NULL_OBJ:
            /* Null */
      case STRING:
            /* String */ 
            #ifdef DEBUG
            if (token->token_type == STRING)
                  printf("\"%s\"", token->token_words);
            else printf("%s", token->token_words);
            #endif // DEBUG
      
            /* General value */
            obj->value = calloc(1, sizeof(struct json_obj));
            obj->value->data = calloc(strlen(token->token_words) + 1, sizeof(char));
            obj->value->key_value = false;
            obj->value->value_type = token->token_type;
            strncpy(obj->value->data, token->token_words, strlen(token->token_words));
            // token = token->next_token;
            break;
      case LEFT_S_BUCKET:
            /* Array */
            #ifdef DEBUG
            printf("%s", token->token_words);
            #endif // DEBUG
            /* New Array, create a new array node */
            obj->value = calloc(1, sizeof(struct json_obj));
            obj->value->value_type = V_ARRAY;
            token = json_parse_array(token, obj->value, true, e_pkt);
            break;
      case LEFT_BUCKET:
            /* Object */
            /* New Object, create a new object node */
            #ifdef DEBUG
            printf("%s", token->token_words);
            #endif // DEBUG
            
            obj->value = calloc(1, sizeof(struct json_obj));
            obj->value->value_type = V_OBJECT;
            token = json_parse_object(token, obj->value, true, e_pkt);
            break;
      default:
            e_pkt->row = token->row;
            e_pkt->col = token->col;
            e_pkt->error_occur = true;
            sprintf(e_pkt->err_msg, "Unexpected token type: %s",
                        token_type_strs[token->token_type]);
            return (NULL);
      }
      return (token);
}

static struct json_query_command* 
json_query_command_parser(char *json_query_command)
{
      if (!*json_query_command) {
            return (NULL);
      }

      struct json_query_command *query_command = calloc(1, sizeof(struct json_query_command)), *p;
      p = query_command;
      int i = 0, j = 0;
      const size_t len = strlen(json_query_command);
      
      while (i <= strlen(json_query_command)) {
            char w = json_query_command[i];
            /* Space is invalid */
            if (isspace(w)) {
                  fprintf(stdout, "[JSON Query Command Error] Invalid character: '%c'\n", w);
                  json_query_command_free(query_command);
                  return (NULL);
            }
            /* Digit, alpha and '_' is valid */
            else if (isalpha(w) || isdigit(w) || w == '_') {
                i++;  
            }
            /* Dot is command splitter */
            else if (i == strlen(json_query_command) || w =='.') {
                  p->next_cmd = calloc(1, sizeof(struct json_query_command));
                  p->next_cmd->len = i - j;
                  p->next_cmd->command_words = calloc(p->next_cmd->len + 1, sizeof(char));
                  strncpy(p->next_cmd->command_words, json_query_command + j, p->next_cmd->len);
                  p = p->next_cmd;
                  j = ++i;
            }
            else {
                  fprintf(stdout, "[JSON Query Command Error] Invalid character: '%c'\n", w);
                  json_query_command_free(query_command);
                  return (NULL);
            }
      }

      return (query_command);
}

static void 
json_query_command_free(struct json_query_command *commands)
{
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
struct json_obj* 
from_json(char *json_content)
{
      struct json_tracker *tracker = json_tracker_init(json_content);
      struct json_token *token = json_lexer(json_content, tracker);
      if (!token) return (NULL);
      struct json_obj *obj = json_parser(token, tracker);
      if (!obj) goto exit;
      /* Free */
      json_tracker_free(tracker);
      exit:
            json_token_free(token);
            return (obj);
}

void 
json_obj_free(struct json_obj *obj)
{
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
 *  @Description: Firstly, when you set two or more values to a same key in object, we only take the final value as
 *  the value of the key. Secondly
*/
struct json_obj* 
json_query(char *command, struct json_obj *obj)
{
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
                  fprintf(stderr, "[JSON Query Error] Not found the key: %s\n", cmd->command_words);
                  json_query_command_free(cmd);
                  return (NULL);
            }
            next:
                  obj = o->value;
                  cmd = cmd->next_cmd;
      }
      json_query_command_free(cmd);

      if (!obj) {
            fprintf(stderr, "[JSON Query Error] Not found the key: %s\n", cmd->command_words);
      }
      return (obj);
}

double 
json_get_double(struct json_obj *obj, char *err_msg)
{
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
      if (obj->data[i] == '0' || (obj->data[i + 1] == 'x' || obj->data[i + 1] == 'X')) {
            return (json_get_long(obj, err_msg));
      }
      /* Decimal number */
      if (obj->data[i] == '+') {
            sign = 1;
            i++;
      }
      else if (obj->data[i] == '-') {
            sign = -1;
            i++;
      }

      for (size_t j = 0; obj->data[i] != '.' && i < strlen(obj->data); j++, i++) {
            integer_part[j] = obj->data[i];
      }
      /* Like 123 */
      if (i ==  strlen(obj->data)) {
            goto integer_exit;
      }
      /* Skip dot */
      i++;
      /* Copy */
      for (size_t j = 0; obj->data[i] != 'e' && obj->data[i] != 'E' && i < strlen(obj->data); j++, i++) {
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
      }
      else if (obj->data[i] == '-') {
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
            }
            else if (j < strlen(fractional_part)) {
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
            }
            else {
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

long 
json_get_long(struct json_obj *obj, char *err_msg)
{
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
      if (obj->data[i] == '0' || (obj->data[i + 1] == 'x' || obj->data[i + 1] == 'X')) {
            i = i + 2;
            for (; i < strlen(obj->data); i++) {
                  if (isdigit(obj->data[i])) {
                        l = l * 16 + obj->data[i] - '0';
                  }
                  else if (obj->data[i] >= 'A' && obj->data[i] <= 'F') {
                        l = l * 16 + obj->data[i] - 'A' + 10;
                  }
                  else if (obj->data[i] >= 'a' && obj->data[i] <= 'f') {
                        l = l * 16 + obj->data[i] - 'a' + 10;
                  }
                  else {
                        if (err_msg) {
                              sprintf(err_msg, "Unexpected hexadecimal character: %c", obj->data[i]);
                        }
                        return (0);
                  }
            }
            return (l);
      }

      if (obj->data[i] == '+') {
            sign = 1;
            i++;
      }
      else if (obj->data[i] == '-') {
            sign = -1;
            i++;
      }

      for (size_t j = 0; obj->data[i] != '.' && i < strlen(obj->data); j++, i++) {
            integer_part[j] = obj->data[i];
      }
      /* Like 123 */
      if (i ==  strlen(obj->data)) {
            goto integer_exit;
      }
      /* Skip dot */
      i++;
      /* Copy */
      for (size_t j = 0; obj->data[i] != 'e' && obj->data[i] != 'E' && i < strlen(obj->data); j++, i++) {
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
      }
      else if (obj->data[i] == '-') {
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
            strncpy(frac, fractional_part, exp > strlen(fractional_part) ? strlen(fractional_part) : exp);
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

bool 
json_get_bool(struct json_obj *obj, char *err_msg)
{
      if (!obj) return (false);
      if (obj->value_type != V_BOOL) {
            if (err_msg) {
                  sprintf(err_msg, "Can not convert %s to Boolean", json_obj_type_strs[obj->value_type]);
                  return false;
            }
      }

      return (!strncmp(obj->data, "true", strlen("true")));
}

char *
json_get_string(struct json_obj *obj, char *err_msg)
{
      if (!obj) return (NULL);

      char *data = calloc(strlen(obj->data), sizeof(char));
      strncpy(data, obj->data, strlen(obj->data));
      return (data);
}

int json_get_null(struct json_obj *obj, char *err_msg)
{
      if (!obj) return (0);
      if (obj->value_type != V_NULL) {
            if (err_msg) {
                  sprintf(err_msg, "Can not convert %s to Null\n", json_obj_type_strs[obj->value_type]);
                  return (0);
            }
      }
      return (1);
}

/** 
 *  Modify the value of corresponding key
*/
int 
json_update(char *command, struct json_obj *obj, struct json_obj *value)
{
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
                  fprintf(stderr, "[JSON Update Error] Not found the key: %s\n", c->command_words);
                  json_query_command_free(cmd);
                  return (1);
            }
            next:
                  if (c->next_cmd)
                        obj = o->value;
                  else obj = o;
                  c = c->next_cmd;
      }

      json_query_command_free(cmd);
      if (obj) {
            json_obj_free(obj->value);
            obj->value = value;
      }
      else {
            fprintf(stderr, "[JSON Update Error] Not found the key: %s\n", c->command_words);
            return (1);
      }
      return (0);
}

/**
 * Add a new key and its value.
 * */
int 
json_add(char *command, struct json_obj *obj, struct json_obj *value)
{
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
                  }
                  else {
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
            }
            else {
                  if (o->value)
                        json_obj_free(o->value);
                  o->value = value;
            }
            next:
                  if (cmd->next_cmd)
                        obj = o->value;
                  else obj = o;
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
int 
json_delete(char *command, struct json_obj *obj)
{
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
                  fprintf(stderr, "[JSON Delete Error] Not found the key: %s\n", cmd->command_words);
                  json_query_command_free(cmd);
                  return (1);
            }
            next:
                  if (cmd->next_cmd)
                        obj = o->value;
                  else obj = o;
                  cmd = cmd->next_cmd;
      }
      json_query_command_free(cmd);

      if (!obj) {
            fprintf(stderr, "[JSON Delete Error] Not found the key: %s\n", cmd->command_words);
            return (1);
      }else {
            if (obj->next_key) 
                  obj->next_key->prev_key = obj->prev_key;
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

int 
to_json(struct json_obj *root, char *json_content, uint32_t size)
{
      
      if (!json_content || size < MIN_JSON_LEN) return (-1);
      bzero(json_content, size);
      return (json_build(root, json_content, size, false, -1));
}

static int 
json_build(struct json_obj *root, char *json_content, uint32_t size, bool beautfy, int tab_size)
{
      if (!root) return (-1);
      int built_size = 1;
      /* Build object */
      if (root->value_type == V_OBJECT) {
            *json_content = '{';
            built_size = json_build_object(root->next_key, json_content, size, built_size, beautfy, tab_size, 1);
      }
      /* Build array */
      else if (root->value_type == V_ARRAY) {
            *json_content = '[';
            built_size = json_build_array(root->next_key, json_content, size, built_size, beautfy, tab_size, 1);
      }
      /* Other type is invalid */
      else {
            fprintf(stdout, "[JSON Build Error] Expected object or array as root in JSON\n");
            return (0);
      }
      return (built_size);
}

static int 
json_build_object(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer)
{

      /* Empty object */
      if (!obj) {
            *(json_content + built_size++) = '}';  
            return (built_size);
      }

      if (beautfy) {
            *(json_content + built_size++) = '\n';
            for (uint32_t i = 0; i < tab_size * layer; i++) *(json_content + built_size++) = ' ';
      }
      /* Non-empty object */
      /* Build key */
      if (obj->value_type != V_STRING) {
            fprintf(stdout, "[JSON Build Error] JSON's key should be String, not '%s'\n", 
                  json_obj_type_strs[obj->value_type]);
            return (0);
      }

      built_size = json_build_string(obj->data, json_content, size, built_size);
      
      /* Colon */
      *(json_content + built_size++) = ':';

      if (beautfy) *(json_content + built_size++) = ' ';

      /* Build value */
      
      built_size = json_build_dispatch(obj->value, json_content, size, built_size, beautfy, tab_size, layer);

      /* Next key or end of object */
      if (!obj->next_key) {
            if (beautfy) {
                  *(json_content + built_size++) = '\n';
                  for (uint32_t i = 0; i < tab_size * (layer - 1); i++) *(json_content + built_size++) = ' ';
            }

            *(json_content + built_size++) = '}';
      }
      else if (obj->next_key->value_type == V_STRING) {
            *(json_content + built_size++) = ',';
            built_size = json_build_object(obj->next_key, json_content, size, built_size, beautfy, tab_size, layer);
      } 
      else {
            fprintf(stdout, "[JSON Build Error] Excepted type of String as object key\n");
            return (0);
      }
      return (built_size);
} 

static int 
json_build_string(char *string, char *json_content, uint32_t size, uint32_t built_size)
{
      *(json_content + built_size++) = '\"';

      for (size_t i = 0; i < strlen(string); i++) {
            switch (string[i])
            {
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

static int 
json_build_array(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer)
{
      /* Empty array */
      if (!obj) {
            *(json_content + built_size++) = ']';
            return (built_size);
      }

      if (beautfy) {
            *(json_content + built_size++) = '\n';
            for (uint32_t i = 0; i < tab_size * layer; i++) *(json_content + built_size++) = ' ';
      }
      /* Non-empty array */
      /* Build Value */
      built_size = json_build_dispatch(obj->value, json_content, size, built_size, beautfy, tab_size, layer);

      /* Next value or end of array */
      if (!obj->next_key) {
            if (beautfy) {
                  *(json_content + built_size++) = '\n';
                   for (uint32_t i = 0; i < tab_size * (layer - 1); i++) *(json_content + built_size++) = ' ';
            }
            *(json_content + built_size++) = ']';
      }
      else if (obj->next_key->value_type == V_NUMBER) {
            *(json_content + built_size++) = ',';
            built_size = json_build_array(obj->next_key, json_content, size, built_size, beautfy, tab_size, layer);
      }
      else {
            fprintf(stdout, "[JSON Build Error] Excepted String value\n");
            return (0);
      }

      return (built_size);
}

static int 
json_build_dispatch(struct json_obj *obj, char *json_content, uint32_t size, uint32_t built_size, bool beautfy, int tab_size, int layer)
{
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
                  built_size = json_build_object(obj->next_key, json_content, size, built_size, beautfy, tab_size, layer + 1);
                  break;
            case V_ARRAY:
                  *(json_content + built_size++) = '[';
                  built_size = json_build_array(obj->next_key, json_content, size, built_size, beautfy, tab_size, layer + 1);
                  break;
            default:
                  fprintf(stdout, "[JSON Build Error] \n");
                  return (0);
                  break;
      }
      return (built_size);
}

/* Simple deserialization wrapper */
struct json_obj* 
load_single_value(char *str, int value_type)
{
      struct json_obj *obj = calloc(1, sizeof(struct json_obj));
      if (!obj) return (NULL);

      obj->data = calloc(strlen(str) + 1, sizeof(char));
      obj->value_type = value_type;
      strncpy(obj->data, str, strlen(str));

      if (value_type == NUMBER) free(str);
      return (obj);
}

struct json_obj* 
load_object(struct json_obj* args, ...)
{
      struct json_obj *head = calloc(1, sizeof(struct json_obj)), *o;
      if (!head) return (NULL);
      head->value_type = V_OBJECT;
      o = head;

      va_list argp;
      int count = 0;
      va_start(argp, args);
      struct json_obj *obj = args, *old = NULL;
      for (;;) {
            if (count)
                  obj = va_arg(argp, struct json_obj *);
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
                  }
                  else {
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

struct json_obj* 
load_array(struct json_obj* args, ...)
{
      struct json_obj *head = calloc(1, sizeof(struct json_obj)), *o;
      if (!head) return (NULL);
      head->value_type = V_ARRAY;
      o = head;

      va_list argp;
      int key = 0;
      va_start(argp, args);
      struct json_obj *obj = args, *new_key;

      for (;;) {
            if (key) 
                  obj = va_arg(argp, struct json_obj *);
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
char * 
json_utils_long2str(long value)
{
      char *str = calloc(BUFSIZ, sizeof(char));
      if (!str) return (NULL);

      sprintf(str, "%ld", value);

      return (str);
}

char *
json_utils_double2str(double value)
{
      char *str = calloc(BUFSIZ, sizeof(char));
      if (!str) return (NULL);

      sprintf(str, "%f", value);

      return (str);
}

int 
json_utils_beautify(char *input, char *output, int tab_size)
{
      struct json_obj *obj = from_json(input);
      if (!obj) return (1);
      tab_size = tab_size <= 0 ? DEFAULT_TAB_SIZE : tab_size; 
      return (!(json_build(obj, output, -1, true, tab_size) > 0));
}

int 
json_utils_minify(char *input, char *output)
{
      struct json_obj *obj = from_json(input);
      if (!obj) return (1);
      return (!(json_build(obj, output, -1, false, -1) > 0));
}

static uint32_t 
json_utils_hex2uint(char *hex)
{
      uint32_t v = 0;
      for (int i = 0; i < strlen(hex); i++) {
            v = v * 16 + (isdigit(hex[i]) ? hex[i] - '0' : isupper(hex[i]) ? hex[i] - 'A' + 10 : hex[i] - 'a' + 10); 
      }
      return (v);
}

static size_t 
json_utils_wstrlen(char *s, int encoding, size_t slen)
{
      size_t len = 0;
      if (encoding == W_UTF8) {
            for (size_t i = 0; i < slen; ) {
                  
                  if (s[i] >> 7 == 0) {
                        len++;
                        i++;
                  } 
                  else if ((s[i] >> 5 & 0xff) == 0xf6) {
                        len++;
                        i += 2;
                  }
                  else if ((s[i] >> 4 & 0xff) == 0xfe) {
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

static size_t 
json_utils_console_wstrlen(char *s, int encoding, size_t slen)
{
      size_t len = 0;
      if (encoding == W_UTF8) {
            for (size_t i = 0; i < slen; ) {
                  
                  if (s[i] >> 7 == 0) {
                        len++;
                        i++;
                  } 
                  else if ((s[i] >> 5 & 0xff) == 0xf6) {
                        len += 2;
                        i += 2;
                  }
                  else if ((s[i] >> 4 & 0xff) == 0xfe) {
                        len += 2;
                        i += 3;
                  }
                  else {
                        len += 2;
                        i += 4;
                  }
                  
            }
            
      }

      return (len);
}