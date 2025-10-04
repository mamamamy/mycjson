#include "cjson.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include <math.h>

int main() {
  char *out;
  uint64_t len;
  char *end;
  cj_value *value;

  // test parse

  value = cj_parse("{}", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_OBJECT);
  cj_clean(value);

  value = cj_parse(" \t { \n } \r", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("{}}", &end);
  assert(value == NULL);
  assert(*end == '}');
  cj_clean(value);

  value = cj_parse("{{}", &end);
  assert(value == NULL);
  assert(*end == '{');
  cj_clean(value);

  value = cj_parse("{", &end);
  assert(value == NULL);
  cj_clean(value);

  value = cj_parse("{ \"name\" : \"value\" }", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_OBJECT);
  assert(value->value.members != NULL);
  assert(value->value.members->name != NULL);
  assert(strcmp(value->value.members->name->data, "name") == 0);
  assert(value->value.members->value.string->data != NULL);
  assert(strcmp(value->value.members->value.string->data, "value") == 0);
  cj_clean(value);

  value = cj_parse("{ \"name\" : \"value\" , }", &end);
  assert(value == NULL);
  assert(*end == '}');
  cj_clean(value);

  value = cj_parse("{ \"name : \"value\" , }", &end);
  assert(value == NULL);
  assert(*end == 'v');
  cj_clean(value);

  value = cj_parse("{ \"name\" : \"value\" , \"hello\": \"world\" }", &end);
  assert(value != NULL);
  assert(value->value.members != NULL);
  assert(value->value.members->next != NULL);
  assert(value->value.members->next->name != NULL);
  assert(strcmp(value->value.members->next->name->data, "hello") == 0);
  assert(value->value.members->next->value.string != NULL);
  assert(value->value.members->next->type == CJ_TYPE_STRING);
  assert(strcmp(value->value.members->next->value.string->data, "world") == 0);
  cj_clean(value);

  value = cj_parse("{ name : \"value\" }", &end);
  assert(value == NULL);
  assert(*end == 'n');
  cj_clean(value);

  value = cj_parse("{ \"object\" : { } , \"array\" : [ ] , \"string\" : \"string\" , \"number\" : 3.14 , \"true\" : true , \"false\" : false , \"null\" : null }", &end);
  assert(value != NULL);
  assert(value->value.members != NULL);
  assert(value->value.members->name != NULL);
  assert(strcmp(value->value.members->name->data, "object") == 0);
  assert(value->value.members->type == CJ_TYPE_OBJECT);
  assert(value->value.members->value.members == NULL);
  assert(value->value.members->next != NULL);
  assert(value->value.members->next->name != NULL);
  assert(strcmp(value->value.members->next->name->data, "array") == 0);
  assert(value->value.members->next->type == CJ_TYPE_ARRAY);
  assert(value->value.members->next->value.members == NULL);
  assert(value->value.members->next->next != NULL);
  assert(value->value.members->next->next->name != NULL);
  assert(strcmp(value->value.members->next->next->name->data, "string") == 0);
  assert(value->value.members->next->next->type == CJ_TYPE_STRING);
  assert(value->value.members->next->next->value.string != NULL);
  assert(strcmp(value->value.members->next->next->value.string->data, "string") == 0);
  assert(value->value.members->next->next->next != NULL);
  assert(value->value.members->next->next->next->name != NULL);
  assert(strcmp(value->value.members->next->next->next->name->data, "number") == 0);
  assert(value->value.members->next->next->next->type == CJ_TYPE_NUMBER);
  assert(value->value.members->next->next->next->value.number == 3.14);
  assert(value->value.members->next->next->next->next != NULL);
  assert(value->value.members->next->next->next->next->name != NULL);
  assert(strcmp(value->value.members->next->next->next->next->name->data, "true") == 0);
  assert(value->value.members->next->next->next->next->type == CJ_TYPE_TRUE);
  assert(value->value.members->next->next->next->next->next != NULL);
  assert(value->value.members->next->next->next->next->next->name != NULL);
  assert(strcmp(value->value.members->next->next->next->next->next->name->data, "false") == 0);
  assert(value->value.members->next->next->next->next->next->type == CJ_TYPE_FALSE);
  assert(value->value.members->next->next->next->next->next->next != NULL);
  assert(value->value.members->next->next->next->next->next->next->name != NULL);
  assert(strcmp(value->value.members->next->next->next->next->next->next->name->data, "null") == 0);
  assert(value->value.members->next->next->next->next->next->next->type == CJ_TYPE_NULL);
  cj_clean(value);

  value = cj_parse("[]", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_ARRAY);
  cj_clean(value);

  value = cj_parse(" [ ] ", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse(" [  ", &end);
  assert(value == NULL);
  cj_clean(value);

  value = cj_parse(" [ [ ] ", &end);
  assert(value == NULL);
  cj_clean(value);

  value = cj_parse(" [ 123 , ] ", &end);
  assert(value == NULL);
  assert(*end == ']');
  cj_clean(value);

  value = cj_parse(" [ ] ] ", &end);
  assert(value == NULL);
  assert(*end == ']');
  cj_clean(value);

  value = cj_parse(" [ [ ] ] ", &end);
  assert(value != NULL);
  assert(value->value.elements != NULL);
  assert(value->value.elements->type == CJ_TYPE_ARRAY);
  cj_clean(value);

  value = cj_parse(" [ {}, [] , \"string\" , 3.14 , true , false , null ] ", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse(" \"\" ", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_STRING);
  assert(value->value.string != NULL);
  assert(value->value.string->len == 0);
  cj_clean(value);

  value = cj_parse(" \"中文测试：你好，世界\" ", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_STRING);
  assert(value->value.string->len == sizeof("中文测试：你好，世界") - 1);
  assert(strcmp(value->value.string->data, "中文测试：你好，世界") == 0);
  cj_clean(value);

  value = cj_parse(" \"😊🎉🌟（笑脸、庆祝、星星）\" ", &end);
  assert(value != NULL);
  assert(strcmp(value->value.string->data, "😊🎉🌟（笑脸、庆祝、星星）") == 0);
  cj_clean(value);

  value = cj_parse(" \"\\u4F60\\u597D\" ", &end);
  assert(value != NULL);
  assert(strcmp(value->value.string->data, "你好") == 0);
  cj_clean(value);

  value = cj_parse(" \"\\ud83d\\ude0a\" ", &end);
  assert(value != NULL);
  assert(strcmp(value->value.string->data, "😊") == 0);
  cj_clean(value);

  value = cj_parse(" \"\\u00A9\\u0026\" ", &end);
  assert(value != NULL);
  assert(strcmp(value->value.string->data, "©&") == 0);
  cj_clean(value);

  value = cj_parse(" \"包含\x01控制字符（未转义）\" ", &end);
  assert(value == NULL);
  cj_clean(value);

  value = cj_parse(" \"不完整的unicode转义\\u123\" ", &end);
  assert(value == NULL);
  cj_clean(value);

  value = cj_parse(" \"\\ud83d\\ude0a\" ", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("0", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_NUMBER);
  cj_clean(value);

  value = cj_parse("123", &end);
  assert(value != NULL);
  assert(value->value.number == 123);
  cj_clean(value);

  value = cj_parse("-456", &end);
  assert(value != NULL);
  assert(value->value.number == -456);
  cj_clean(value);

  value = cj_parse("9876543210", &end);
  assert(value != NULL);
  assert(value->value.number == 9876543210);
  cj_clean(value);

  value = cj_parse("123.45", &end);
  assert(value != NULL);
  assert(value->value.number == 123.45);
  cj_clean(value);

  value = cj_parse("-67.89", &end);
  assert(value != NULL);
  assert(value->value.number == -67.89);
  cj_clean(value);

  value = cj_parse("0.123", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("0.12300", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("123e4", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("456E-7", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("-789e+10", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("0.123e5", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("123.45e-6", &end);
  assert(value != NULL);
  cj_clean(value);

  value = cj_parse("123.", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse(".123", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("1e", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("0123", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("-0123", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("00", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("-00", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("-0.", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("123..456", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("123e", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("456E+", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("-789e-", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("0.123e+", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("123Ea45", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("123 45", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("3.141592653589793238462643383279", &end);
  assert(value != NULL);
  assert(value->value.number == 3.141592653589793238462643383279);
  cj_clean(value);
  
  value = cj_parse("true", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_TRUE);
  cj_clean(value);
  
  value = cj_parse("tarue", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("false", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_FALSE);
  cj_clean(value);
  
  value = cj_parse("faalse", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse("null", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_NULL);
  cj_clean(value);
  
  value = cj_parse("naull", &end);
  assert(value == NULL);
  cj_clean(value);
  
  value = cj_parse(
"{"
"  \"meta\": {"
"    \"version\": \"3.2.1-rc1§\","
"    \"timestamp\": 1730001234567.890123,"
"    \"is_active\": true,"
"    \"description\": \"JSON包含：\\n1. 换行符\\n2. 双引号\\\"示例\\\"\\n3. 反斜杠：\\\\\\n4. Unicode符号：✓🚀\""
"  },"
"  \"users\": ["
"    {"
"      \"id\": 10001,"
"      \"name\": \"O'Neil Doe\","
"      \"profile\": {"
"        \"bio\": \"Line1\\r\\nLine2带制表符\\t\\t缩进\","
"        \"avatar\": null,"
"        \"hobbies\": ["
"          \"coding\","
"          \"hiking\","
"          42,"
"          true"
"        ]"
"      },"
"      \"credentials\": {"
"        \"password\": \"$2b$12$abcdefghijklmnopqrstuvwxyz\","
"        \"last_login\": \"2024-09-01T12:34:56Z\""
"      }"
"    },"
"    {"
"      \"id\": 10002,"
"      \"name\": \"李四\","
"      \"profile\": {},"
"      \"credentials\": {"
"        \"password\": \"\","
"        \"last_login\": null"
"      }"
"    }"
"  ],"
"  \"config\": {"
"    \"limits\": {"
"      \"max_size\": 1073741824,"
"      \"timeout\": 3600.5,"
"      \"nested\": {"
"        \"level1\": {"
"          \"level2\": {"
"            \"value\": \"深层嵌套值📌\""
"          }"
"        }"
"      }"
"    },"
"    \"patterns\": ["
"      \"^[0-9]{3}-[0-9]{2}-[0-9]{4}$\","
"      \"^user@domain\\\\.com$\""
"    ]"
"  },"
"  \"data\": ["
"    ["
"      1,"
"      2.71828,"
"      -345,"
"      null,"
"      false"
"    ],"
"    ["
"      \"a\","
"      \"b\\\\c\","
"      \"d\\\"e\","
"      \"\""
"    ],"
"    ["
"      {"
"        \"key\": \"value\""
"      },"
"      []"
"    ]"
"  ],"
"  \"numbers\": ["
"    0,"
"    -0,"
"    9876543210123456789,"
"    3.141592653589793,"
"    1e+30,"
"    -2.5E-40"
"  ]"
"}"
  , &end);
  assert(value != NULL);
  cj_clean(value);
  
  value = cj_parse("\"这是一段很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长很长的字符串\"", &end);
  assert(value != NULL);
  assert(value->type == CJ_TYPE_STRING);
  assert(value->value.string != NULL);
  assert(value->value.string->len == 420);
  cj_clean(value);

  // stringify

  value = cj_parse("{}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":\"value\"}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":\"value\"}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":true}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":true}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":null}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":null}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":{}}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":{}}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":[[[[[[[[[[[[[[{}]]]]]]]]]]]]]]}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":[[[[[[[[[[[[[[{}]]]]]]]]]]]]]]}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"name\":1234567890}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"name\":1.23457e+09}") == 0);
  cj_clean(value);
  value = cj_parse(out, NULL);
  cj_free(out);
  out = cj_stringify(value, &len);
  assert(strcmp(out, "{\"name\":1.23457e+09}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"object\":{\"嵌套\":\"嵌套\"},\"array\":[],\"string\":\"string\",\"number\":123,\"true\":true,\"false\":false,\"null\":null}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"object\":{\"嵌套\":\"嵌套\"},\"array\":[],\"string\":\"string\",\"number\":123,\"true\":true,\"false\":false,\"null\":null}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("{\"object\":{\"嵌套\":\"嵌套\"},\"array\":[{\"嵌套\":\"嵌套\"},{\"嵌套\":\"嵌套\"},{\"嵌套\":\"嵌套\"}],\"string\":\"string\",\"number\":123,\"true\":true,\"false\":false,\"null\":null}", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "{\"object\":{\"嵌套\":\"嵌套\"},\"array\":[{\"嵌套\":\"嵌套\"},{\"嵌套\":\"嵌套\"},{\"嵌套\":\"嵌套\"}],\"string\":\"string\",\"number\":123,\"true\":true,\"false\":false,\"null\":null}") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"https://example.com\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"https://example.com\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"https:\\/\\/example.com\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"https://example.com\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"C:\\\\test\\\\test.txt\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"C:\\\\test\\\\test.txt\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"\\b\\f\\n\\r\\t\\\"\\\\\\u0000\\u0001\\u001F\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"\\b\\f\\n\\r\\t\\\"\\\\\\u0000\\u0001\\u001F\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"中文测试\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"中文测试\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("\"emoji🌚😀\"", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "\"emoji🌚😀\"") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("true", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "true") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("false", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "false") == 0);
  cj_free(out);
  cj_clean(value);

  value = cj_parse("null", NULL);
  out = cj_stringify(value, &len);
  assert(out != NULL);
  assert(strcmp(out, "null") == 0);
  cj_free(out);
  cj_clean(value);

  return 0;
}
