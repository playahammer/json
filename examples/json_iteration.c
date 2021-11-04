#include "../json.h"
#include <string.h>

int main() 
{
      // JSON text
      char *json = "{\"array\":[1,2,3,4], \"id\": 1, \"descriptions\":\"helloword\"}";
      printf("%s\n", json);
      struct json_obj *obj = from_json(json, strlen(json));
      if (!obj) {
            return (1);
      }
      // Iterate an object or array
      for (struct json_obj *o = json_begin(obj); o; o = json_next_key(o)) {
            char *key = json_get_key(o);
            if (!strncmp(key, "array", strlen("array"))) {
                  for (struct json_obj* arr = json_begin(json_get_value(o)); !json_end(arr); arr = json_next_key(arr)) {
                        printf("array[%s]: ", json_get_key(arr));
                        struct json_obj *value = json_get_value(arr);
                        if (value->value_type == V_NUMBER) {
                              printf("%ld\n", json_get_long(value, NULL));
                        }
                  }
                  
            }
            else {
                  printf("%s\n", json_get_key(o));
            }
      }

      json_obj_free(obj);
      return (0);
}