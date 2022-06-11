#include "../json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char **argv)
{
      if (argc <= 1) {
            printf("./json_test file_path\n");
            return (1);
      }
      FILE *fp = fopen(*(argv + 1), "r");
      if (!fp) {
            printf("File not found\n");
            return (1);
      }
      fseek(fp, 0, SEEK_END);
      long len = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      char *json = calloc(len, sizeof(char));
      fread(json, 1, len, fp);
      fclose(fp);
      printf("%s\n", json);
      struct json_obj *obj = from_json(json, len);
      if (obj) {
            // json_obj_free(obj);
            free(json);
            printf("Passed\n");
      }
      else {
            free(json);
            printf("Not Passed\n");
            return (1);
      }
      return (0);
}