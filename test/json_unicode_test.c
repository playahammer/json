#include "../json.h"
#include <string.h>
#include <wchar.h>
#include <locale.h>

int main(){
      setlocale(LC_ALL, "");
      wchar_t t[] = {0x6211, 0x622f, 0x732a, 0x1F600};
      wprintf(L"%ls\n", t);
      char *json = "[\"\\u6211\\u662f\\u732a\\uD83D\\uDE2D\\ud83d\\udc4b\\ud83c\\udffe\\u12345\\uffff\"]";
      struct json_obj *obj = from_json(json, strlen(json));
      if (!obj) {
            return (1);
      }

      char s_json[1024] = {0};
      if (to_json(obj, s_json, 1024)) {
            printf("%s\n", s_json);
            json_obj_free(obj);
      }
      else {
            json_obj_free(obj);
            return (1);
      }
      
      return (0);
}