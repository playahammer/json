# json
This project is a very tiny JSON parser and generator for C.

## APIs
### Base
#### Serialize
```c
struct json_obj* from_json(char *json_content);
```
Loading JSON content, and then parsing into a JSON tree for querying, modifying or deserializing.

Return **NULL** if the input is invalid.
#### Deserialize
```c
int to_json(struct json_obj *root, char *json_content, uint32_t size);
```
Deserialize a JSON tree into JSON text.

Return **0** if dealing successfully, otherwise failed.
#### Operations
##### Query
```c
struct json_obj *json_query(char *command, struct json_obj *obj);
```
Query the value by the key in command given. The grammer rule of command is similar to access object value in JavaScript. 

For example, supposed a JSON file has ```{"1":[1,2,3],"2": {"1":123,"2":false,"3",null}}``` text. 
You want to visit first value of key ```1```, in our rule just type ```1.0``` to access then get its value ```1``` while the JavaScript developer should use ```[]``` to access the value in array. Remember that array is a special object with index key.

Return **NULL** if not found or some error occured like invalid command.
```c
double json_get_double(struct json_obj *obj, char *err_msg);
long json_get_long(struct json_obj *obj, char *err_msg);
bool json_get_bool(struct json_obj *obj, char *err_msg);
char *json_get_string(struct json_obj *obj, char *err_msg);
int json_get_null(struct json_obj *obj, char *err_msg);
```
Above five functions to get value in the appointed type. Error will pass through ```err_msg``` if ```err_msg``` is not NULL when error occurs.
##### Update
```c
int json_update(char *command, struct json_obj *obj, struct json_obj *value);
```
Update the ```value``` of ```command``` in ```obj``` tree. if the key is not found, return ```1``` with error printing.

##### Add
```c
int json_add(char *command, struct json_obj *obj, struct json_obj *value);
```
Add new key and its ```value```. Let's see an example: this is a JSON text  ```{"1":{}}```, and the command is ```1.1``` with its value ```[1,2,3]```. It will work out such result ```{"1":{"1":{"1":[1,2,3]}}}``` since  there is no specific key symbol to differ object and array in command parser, they both use ```.``` operator to finish every operation. By default, we create an object instead of array when meets digital key.
##### Delete
```c
int json_delete(char *command, struct json_obj *obj);
```
Delete the key from ```obj``` including its value. In array, deleting a key will remap remainig keys, but don't do that in object, even all the key are digital.
### Utils
```c
int json_utils_beautify(char *input, char *output, int tab_size);
```
Make your JSON more beautiful and human-readable, ```tab_size``` points indent.
```c
int json_utils_minify(char *input, char *output);
```
Compress JSON file, remove unnecessary spaces and console characters.
```
Beautfy:
{
  "1": "helloworld",
  "3": [
    1,
    2,
    3
  ],
  "5": [
    {
      "1": 2
    }
  ]
}
Minify:
{"1":"helloworld","3":[1,2,3],"5":[{"1":2}]}
```
### Wrappers
```c
LOAD_OBJECT(...)
LOAD_ARRAY(...)
LOAD_NULL
LOAD_TRUE
LOAD_FALSE 
LOAD_STRING(str)
LOAD_NUMBER(num)
LOAD_END
```
The micro definitions will help build a JSON tree more easily. Notice that every array and obejct must be ended with ```LOAD_END``` which tells the function when read stoped. More usage details please see example or source code.

## TODO

* [ ] Fully UTF-8 support
* [x] Add ```add``` and ```delete``` apis in JSON tree
* [ ] Make it more faster?
* [ ] Introduce other JSON extensions
* [ ] More effcient error tips

## Example
```c

#include "json.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
      char json_content[102400] = {0};
      FILE *fp;

      fp = fopen("json_test.json", "r");
      fread(json_content, 102400, 1, fp);
      struct json_obj* obj = from_json(json_content);
      if (!obj) goto exit;
      char json_content1[102400] = {0};
      int size = to_json(obj, json_content1, 102400);
      printf("len:%d\n%s\n",size, json_content1);
      /* Query value */
      struct json_obj* result = json_query("3.index", obj);
      if (result) {
            printf("Query result:%s, %ld\n", result->data, json_get_long(result, NULL));
      }
      /* Test beautify and minify */
      char json_content2[1024000] = {0};

      if (!json_utils_beautify(json_content1, json_content2, -1)) {
            printf("Beautfy:\n%s\n", json_content2);
      }
      
      char json_content3[1024000] = {0};

      if (!json_utils_minify(json_content2, json_content3)) {
            printf("Minify:\n%s\n", json_content3);
      }

      /* Test simple deserialization wrapper */
      struct json_obj *wrapper = LOAD_OBJECT(
                                          LOAD_STRING("123"), LOAD_ARRAY(LOAD_NUMBER(123.123), LOAD_NULL, LOAD_TRUE, LOAD_END), 
                                          LOAD_STRING("345"), LOAD_FALSE,
                                          LOAD_STRING("hello"), LOAD_OBJECT(
                                                LOAD_STRING("index"), LOAD_NUMBER(1),
                                                LOAD_END
                                          ),
                                          LOAD_END);
      char json_content4[1024] = {0};
      int to = to_json(wrapper, json_content4, 1024);
      if (to > 0)
            printf("len:%d\n%s\n", to, json_content4);
      
      json_obj_free(wrapper);

      exit:
            fclose(fp);
            return (0);
}
```
## License
[See license file](./LICENSE)
```
MIT License

Copyright (c) 2021 Play a hammer and magbone

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```
