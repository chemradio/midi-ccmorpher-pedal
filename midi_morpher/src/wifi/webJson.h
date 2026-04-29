#pragma once
// JSON parsing primitives — always included from webServer.h after its includes.

// ── JSON helpers ───────────────────────────────────────────────────────────────
inline int jsonInt(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return -9999;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return -9999;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  bool neg = false;
  if(pos < (int)body.length() && body[pos] == '-') {
    neg = true;
    pos++;
  }
  String num;
  while(pos < (int)body.length() && isDigit(body[pos]))
    num += body[pos++];
  if(num.isEmpty())
    return -9999;
  int v = num.toInt();
  return neg ? -v : v;
}

// Parse an unsigned 32-bit int. Returns 0xFFFFFFFF if the key is absent or
// unparseable. Needed for rampUpMs/rampDownMs which may have bit 31 set
// (CLOCK_SYNC_FLAG) and therefore overflow a plain signed int.
inline uint32_t jsonUint(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return 0xFFFFFFFFUL;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return 0xFFFFFFFFUL;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  uint32_t val = 0;
  bool gotDigits = false;
  while(pos < (int)body.length() && isDigit(body[pos])) {
    val = val * 10 + (uint32_t)(body[pos] - '0');
    pos++;
    gotDigits = true;
  }
  return gotDigits ? val : 0xFFFFFFFFUL;
}

inline bool jsonBool(const String &body, const char *key, bool &out) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return false;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return false;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  if(body.substring(pos, pos + 4) == "true") {
    out = true;
    return true;
  }
  if(body.substring(pos, pos + 5) == "false") {
    out = false;
    return true;
  }
  return false;
}

// Find matching closing brace/bracket for the open character at pos.
inline int skipToMatch(const String &s, int pos) {
  char open = s[pos], close = (open == '{') ? '}' : ']';
  int depth = 0;
  bool inStr = false;
  for(int i = pos; i < (int)s.length(); i++) {
    char c = s[i];
    if(inStr) {
      if(c == '\\')
        i++;
      else if(c == '"')
        inStr = false;
      continue;
    }
    if(c == '"') {
      inStr = true;
      continue;
    }
    if(c == open)
      depth++;
    if(c == close) {
      if(--depth == 0)
        return i;
    }
  }
  return -1;
}

inline float jsonFloat(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return -1.0f;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return -1.0f;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  String num;
  while(pos < (int)body.length() && (isDigit(body[pos]) || body[pos] == '.'))
    num += body[pos++];
  if(num.isEmpty())
    return -1.0f;
  return num.toFloat();
}
