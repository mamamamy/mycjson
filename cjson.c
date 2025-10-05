#include "cjson.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

static const char hex_chars[] = "0123456789ABCDEF";

typedef struct buffer buffer;

struct buffer {
  char *data;
  uint64_t cap;
  uint64_t len;
};

static void buffer_init(buffer *buf) {
  buf->cap = 64;
  buf->data = cj_malloc(buf->cap);
  buf->len = 0;
}

static void buffer_clean(buffer *buf) {
  cj_free(buf->data);
}

static void buffer_write_byte(buffer *buf, char byte) {
  if (buf->len == buf->cap) {
    buf->cap <<= 1;
    buf->data = cj_realloc(buf->data, buf->cap);
  }
  buf->data[buf->len] = byte;
  ++buf->len;
}

static void buffer_write_string(buffer *buf, const char *string, uint64_t len) {
  if (buf->len + len >= buf->cap) {
    do {
      buf->cap <<= 1;
    } while(buf->len + len >= buf->cap);
    buf->data = cj_realloc(buf->data, buf->cap);
  }
  memcpy(buf->data + buf->len, string, len);
  buf->len += len;
}

static cj_value *parse_value(const char **pp);

static cj_value *create_cj_value(int type) {
  cj_value *value = cj_malloc(sizeof(cj_value));
  memset(value, 0, sizeof(cj_value));
  value->type = type;
  return value;
}

static void skip_whitespace(const char **pp) {
  const char *p = *pp;
  while (
    *p == ' ' ||
    *p == '\t' ||
    *p == '\n' ||
    *p == '\r'
  ) {
    ++p;
  }
  *pp = p;
}

static double parse_number_raw(const char **pp, bool *ok) {
  const char *p = *pp;
  double result;
  int sign = 1;
  uint64_t intg = 0;
  double frac = 0;
  int64_t exp = 0;
  if (*p == '-') {
    sign = -1;
    ++p; // '-'
  }
  if (*p < '0' || *p > '9') {
    goto label_error;
  }
  if (*p == '0') {
    ++p; // '0'
  } else { // '1' - '9'
    for (;*p >= '0' && *p <= '9';) {
      int x = *p - '0';
      intg *= 10;
      intg += x;
      ++p;
    }
  }
  if (*p == '.') {
    ++p; // '.'
    if (*p < '0' || *p > '9') {
      goto label_error;
    }
    double frac_intg = 0;
    uint64_t frac_len = 0;
    for (;*p >= '0' && *p <= '9';) {
      int x = *p - '0';
      frac_intg *= 10;
      frac_intg += x;
      ++p;
      ++frac_len;
    }
    frac = frac_intg / pow(10.0, (double)frac_len);
  }
  if (*p == 'e' || *p == 'E') {
    ++p; // 'e' 'E'
    int exp_sign = 1;
    if (*p == '+') {
      ++p; // '+'
    } else if (*p == '-') {
      ++p; // '-'
      exp_sign = -1;
    }
    if (*p < '0' || *p > '9') {
      goto label_error;
    }
    for (;*p >= '0' && *p <= '9';) {
      int x = *p - '0';
      exp *= 10;
      exp += x;
      ++p;
    }
    exp *= exp_sign;
  }
  result = (double)intg + frac;
  result *= pow(10.0, (double)exp);
  result *= sign;
  *ok = true;
  goto label_return;
label_error:
  *ok = false;
label_return:
  *pp = p;
  return result;
}

static cj_string *parse_string_raw(const char **pp) {
  const char *p = *pp;
  cj_string *result = NULL;
  buffer buf;
  buffer_init(&buf);
  if (*p != '"') {
    goto label_error;
  }
  ++p; // '"'
  for (;;) {
    if (*p == '"') {
      break;
    }
    if (*p >= 0 && *p <= 0x1F) {
      goto label_error;
    } else if (*p == '\\') { // escape
      ++p; // '\'
      if (
        *p == '"' ||
        *p == '\\' ||
        *p == '/'
      ) {
        buffer_write_byte(&buf, *p);
        ++p; // '"'   '\'   '/'
      } else if (*p == 'b') {
        buffer_write_byte(&buf, '\b');
        ++p; // 'b'
      } else if (*p == 'f') {
        buffer_write_byte(&buf, '\f');
        ++p; // 'f'
      } else if (*p == 'n') {
        buffer_write_byte(&buf, '\n');
        ++p; // 'n'
      } else if (*p == 'r') {
        buffer_write_byte(&buf, '\r');
        ++p; // 'r'
      } else if (*p == 't') {
        buffer_write_byte(&buf, '\t');
        ++p; // 't'
      } else if (*p == 'u') {
        ++p; // 'u'
        uint16_t code = 0;
        for (int i = 0; i < 4; ++i) {
          uint8_t val;
          if (*p >= '0' && *p <= '9') {
            val = *p - '0';
          } else if (*p >= 'a' && *p <= 'f') {
            val = *p - 'a' + 10;
          } else if (*p >= 'A' && *p <= 'F') {
            val = *p - 'A' + 10;
          } else {
            goto label_error;
          }
          ++p;
          code = (code << 4) | val;
        }
        if (
          code >= 0xD800 &&
          code <= 0xDBFF &&
          p[0] == '\\' &&
          p[1] == 'u' &&
          isxdigit(p[2]) &&
          isxdigit(p[3]) &&
          isxdigit(p[4]) &&
          isxdigit(p[5])
        ) {
          uint16_t low_code = 0;
          for (int i = 0; i < 4; ++i) {
            uint8_t val;
            char c = p[i+2];
            if (c >= '0' && c <= '9') {
              val = c - '0';
            } else if (c >= 'a' && c <= 'f') {
              val = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
              val = c - 'A' + 10;
            } else {
              goto label_unicode_continue;
            }
            low_code = (low_code << 4) | val;
          }
          if (low_code < 0xDC00 || low_code > 0xDFFF) {
            goto label_unicode_continue;
          }
          uint32_t full_code = 0x10000 + ((code - 0xD800) << 10) + (low_code - 0xDC00);
          buffer_write_byte(&buf, 0xF0 | ((full_code >> 18) & 0x07)); // 11110xxx
          buffer_write_byte(&buf, 0x80 | ((full_code >> 12) & 0x3F)); // 10xxxxxx
          buffer_write_byte(&buf, 0x80 | ((full_code >> 6) & 0x3F)); // 10xxxxxx
          buffer_write_byte(&buf, 0x80 | (full_code & 0x3F)); // 10xxxxxx
          p += 6;
          continue;
        }
label_unicode_continue:
        if (code <= 0x7F) {
          buffer_write_byte(&buf, code); // 0xxxxxxx
        } else if (code <= 0x7FF) {
          buffer_write_byte(&buf, 0xC0 | (code >> 6)); // 110xxxxx
          buffer_write_byte(&buf, 0x80 | (code & 0x3F)); // 10xxxxxx
        } else { // <= FFFF
          buffer_write_byte(&buf, 0xE0 | (code >> 12)); // 1110xxxx
          buffer_write_byte(&buf, 0x80 | ((code >> 6) & 0x3F)); // 10xxxxxx
          buffer_write_byte(&buf, 0x80 | (code & 0x3F)); // 10xxxxxx
        }
      } else {
        goto label_error;
      }
    } else { // unescaped
      uint8_t code = *p;
      if (code <= 0x7F) {
        buffer_write_byte(&buf, code); // 0xxxxxxx
        ++p;
      } else if ((code & 0xE0) == 0xC0) {
        buffer_write_byte(&buf, code); // 110xxxxx
        ++p;
        code = *p;
        if ((code & 0xC0) != 0x80) {
          goto label_error;
        }
        buffer_write_byte(&buf, code); // 10xxxxxx
        ++p;
      } else if ((code & 0xF0) == 0xE0) {
        buffer_write_byte(&buf, code); // 1110xxxx
        ++p;
        for (int i = 0; i < 2; ++i) {
          code = *p;
          if ((code & 0xC0) != 0x80) {
            goto label_error;
          }
          buffer_write_byte(&buf, code); // 10xxxxxx
          ++p;
        }
      } else if ((code & 0xF8) == 0xF0) {
        buffer_write_byte(&buf, code); // 11110xxx
        ++p;
        for (int i = 0; i < 3; ++i) {
          code = *p;
          if ((code & 0xC0) != 0x80) {
            goto label_error;
          }
          buffer_write_byte(&buf, code); // 10xxxxxx
          ++p;
        }
      } else {
        goto label_error;
      }
    }
  }
  if (*p != '"') {
    goto label_error;
  }
  ++p; // '"'
  result = cj_malloc(sizeof(cj_string) + buf.len + 1);
  result->len = buf.len;
  memcpy(result->data, buf.data, buf.len);
  result->data[buf.len] = '\0';
  goto label_return;
label_error:
  result = NULL;
label_return:
  buffer_clean(&buf);
  *pp = p;
  return result;
}

static cj_value *parse_object(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  if (*p != '{') {
    goto label_error;
  }
  ++p; // '{'
  result = create_cj_value(CJ_TYPE_OBJECT);
  cj_value *prev = NULL;
  skip_whitespace(&p); // ws
  if (*p == '}') {
    ++p; // '}'
    goto label_return;
  }
  for (;;) {
    cj_string *name = parse_string_raw(&p); // string
    if (name == NULL) {
      goto label_error;
    }
    skip_whitespace(&p); // ws
    if (*p != ':') {
      cj_free(name);
      goto label_error;
    }
    ++p; // ':'
    skip_whitespace(&p); // ws
    cj_value *member = parse_value(&p); // value
    if (member == NULL) {
      cj_free(name);
      goto label_error;
    }
    member->name = name;
    if (prev != NULL) {
      prev->next = member;
    } else {
      result->value.members = member;
    }
    prev = member;
    skip_whitespace(&p); // ws
    if (*p != ',') {
      break;
    }
    ++p; // ','
    skip_whitespace(&p); // ws
  }
  if (*p != '}') {
    goto label_error;
  }
  ++p; // '}'
  goto label_return;
label_error:
  cj_clean(result);
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_array(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  if (*p != '[') {
    goto label_error;
  }
  ++p; // '['
  result = create_cj_value(CJ_TYPE_ARRAY);
  cj_value *prev = NULL;
  skip_whitespace(&p); // ws
  if (*p == ']') {
    ++p; // ']'
    goto label_return;
  }
  for (;;) {
    cj_value *element = parse_value(&p); // value
    if (element == NULL) {
      goto label_error;
    }
    if (prev != NULL) {
      prev->next = element;
    } else {
      result->value.elements = element;
    }
    prev = element;
    skip_whitespace(&p); // ws
    if (*p != ',') {
      break;
    }
    ++p; // ','
    skip_whitespace(&p); // ws
  }
  if (*p != ']') {
    goto label_error;
  }
  ++p; // ']'
  goto label_return;
label_error:
  cj_clean(result);
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_number(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  bool ok;
  double raw = parse_number_raw(&p, &ok);
  if (!ok) {
    goto label_error;
  }
  result = create_cj_value(CJ_TYPE_NUMBER);
  result->value.number = raw;
  goto label_return;
label_error:
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_string(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  cj_string *raw = parse_string_raw(&p);
  if (raw == NULL) {
    goto label_error;
  }
  result = create_cj_value(CJ_TYPE_STRING);
  result->value.string = raw;
  goto label_return;
label_error:
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static bool check_literal(const char **pp, const char *pattern, uint64_t len) {
  const char *p = *pp;
  bool result = true;
  for (uint64_t i = 0; i < len; ++i) {
    if (*p != pattern[i]) {
      goto label_error;
    }
    ++p;
  }
  goto label_return;
label_error:
  result = false;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_true(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  bool ok = check_literal(&p, "true", 4);
  if (!ok) {
    goto label_error;
  }
  result = create_cj_value(CJ_TYPE_TRUE);
  goto label_return;
label_error:
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_false(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  bool ok = check_literal(&p, "false", 5);
  if (!ok) {
    goto label_error;
  }
  result = create_cj_value(CJ_TYPE_FALSE);
  goto label_return;
label_error:
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_null(const char **pp) {
  const char *p = *pp;
  cj_value *result = NULL;
  bool ok = check_literal(&p, "null", 4);
  if (!ok) {
    goto label_error;
  }
  result = create_cj_value(CJ_TYPE_NULL);
  goto label_return;
label_error:
  result = NULL;
label_return:
  *pp = p;
  return result;
}

static cj_value *parse_value(const char **pp) {
  const char *p = *pp;
  cj_value *value = NULL;
  if (*p == '{') {
    value = parse_object(&p);
  } else if (*p == '[') {
    value = parse_array(&p);
  } else if (*p == '"') {
    value = parse_string(&p);
  } else if (*p == 't') {
    value = parse_true(&p);
  } else if (*p == 'f') {
    value = parse_false(&p);
  } else if (*p == 'n') {
    value = parse_null(&p);
  } else if ((*p >= '0' && *p <= '9') || *p == '-') {
    value = parse_number(&p);
  }
  *pp = p;
  return value;
}

cj_value *cj_parse(const char *text, char **end) {
  const char *p = text;
  skip_whitespace(&p); // ws
  cj_value *value = parse_value(&p); // value
  if (value == NULL) {
    goto label_error;
  }
  skip_whitespace(&p); // ws
  if (*p != '\0') {
    goto label_error;
  }
  goto label_return;
label_error:
  cj_clean(value);
  value = NULL;
label_return:
  if (end != NULL) {
    *end = (char *)p;
  }
  return value;
}

void cj_clean(cj_value *value) {
  if (value == NULL) {
    return;
  }
  cj_value *next = NULL;
  cj_value *p = value;
  for (; p != NULL; p = next) {
    next = p->next;
    if (p->name != NULL) {
      cj_free(p->name);
    }
    if (p->type == CJ_TYPE_OBJECT) {
      cj_clean(p->value.members);
    } else if (p->type == CJ_TYPE_ARRAY) {
      cj_clean(p->value.elements);
    } else if (p->type == CJ_TYPE_STRING) {
      cj_free(p->value.string);
    }
    cj_free(p);
  }
}

static void stringify_string(cj_string *string, buffer *buf) {
  buffer_write_byte(buf, '"');
  uint64_t len = string->len;
  char *data = string->data;
  for (uint64_t i = 0; i < len; ++i) {
    char c = data[i];
    if (c == '"') {
      buffer_write_string(buf, "\\\"", 2);
    } else if (c == '\\') {
      buffer_write_string(buf, "\\\\", 2);
    } else if (c == '/') {
      buffer_write_byte(buf, '/');
    } else if (c == '\b') {
      buffer_write_string(buf, "\\b", 2);
    } else if (c == '\f') {
      buffer_write_string(buf, "\\f", 2);
    } else if (c == '\n') {
      buffer_write_string(buf, "\\n", 2);
    } else if (c == '\r') {
      buffer_write_string(buf, "\\r", 2);
    } else if (c == '\t') {
      buffer_write_string(buf, "\\t", 2);
    } else if (c >= 0x00 && c <= 0x1F) {
      buffer_write_string(buf, "\\u00", 4);
      buffer_write_byte(buf, hex_chars[(uint8_t)c >> 4]);
      buffer_write_byte(buf, hex_chars[(uint8_t)c & 0xF]);
    } else {
      buffer_write_byte(buf, c);
    }
  }
  buffer_write_byte(buf, '"');
}

static void stringify_value(cj_value *value, buffer *buf) {
  if (value->type == CJ_TYPE_OBJECT) {
    buffer_write_byte(buf, '{');
    if (value->value.members != NULL) {
      cj_value *p = value->value.members;
      for (; p != NULL; p = p->next) {
        stringify_string(p->name, buf);
        buffer_write_byte(buf, ':');
        stringify_value(p, buf);
        if (p->next != NULL) {
          buffer_write_byte(buf, ',');
        }
      }
    }
    buffer_write_byte(buf, '}');
  } else if (value->type == CJ_TYPE_ARRAY) {
    buffer_write_byte(buf, '[');
    if (value->value.elements != NULL) {
      cj_value *p = value->value.elements;
      for (; p != NULL; p = p->next) {
        stringify_value(p, buf);
        if (p->next != NULL) {
          buffer_write_byte(buf, ',');
        }
      }
    }
    buffer_write_byte(buf, ']');
  } else if (value->type == CJ_TYPE_STRING) {
    stringify_string(value->value.string, buf);
  } else if (value->type == CJ_TYPE_NUMBER) {
    if (isnan(value->value.number) || isinf(value->value.number)) {
      buffer_write_string(buf, "null", 4);
    } else {
      char num_buf[32];
      int n = snprintf(num_buf, 32, "%g", value->value.number);
      for (int i = 0; i < n; ++i) {
        buffer_write_byte(buf, num_buf[i]);
      }
    }
  } else if (value->type == CJ_TYPE_TRUE) {
    buffer_write_string(buf, "true", 4);
  } else if (value->type == CJ_TYPE_FALSE) {
    buffer_write_string(buf, "false", 5);
  } else if (value->type == CJ_TYPE_NULL) {
    buffer_write_string(buf, "null", 4);
  }
}

char *cj_stringify(cj_value *value, uint64_t *len) {
  buffer buf;
  buffer_init(&buf);
  stringify_value(value, &buf);
  char *result = cj_malloc(buf.len + 1);
  memcpy(result, buf.data, buf.len);
  result[buf.len] = '\0';
  if (len != NULL) {
    *len = buf.len;
  }
  buffer_clean(&buf);
  return result;
}
