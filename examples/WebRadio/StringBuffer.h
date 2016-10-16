
#define NUL '\0'
#define CR '\r'
#define LF '\n'
#define SPACE ' '
#define QUOTE '\"'

// StringBuffer is a helper class for building long texts by using an fixed allocated memory region.

class StringBuffer {
  public:
    /// setup a StringBuffer by passing a local char[] variable and it's size.
    StringBuffer(char *buffer, unsigned int bufferSize)
    {
      _buf = buffer;
      _size = bufferSize;
      clear();
    };

    /// clear the buffer.
    void clear() {
      *_buf = '\0';
      _len = 1; // The ending NUL character has to be part of the buffer;
    };

    char *getBuffer() {
      return (_buf);
    };

    uint16_t getLength()  {
      return(_len);
    };

    void append(char c)
    {
      char *t = _buf + _len - 1;
      if (_len < _size) {
        *t++ = c;
        _len++;
      } // if
      *t = NUL;
    }; // append()

    void append(const char *txt)
    {
      char *t = _buf + _len - 1;
      char *s = (char *)txt;
      while (_len < _size) {
        if (! *s) break;
        *t++ = *s++;
        _len++;
      }
      *t = NUL;
    }; // append()


    void append(const __FlashStringHelper *txt)
    {
      char *t = _buf + _len - 1;
      PGM_P s = reinterpret_cast<PGM_P>(txt);
      while (_len < _size) {
        unsigned char c = pgm_read_byte(s++);
        if (! c) break;
        *t++ = c;
        _len++;
      }
      *t = NUL;
    }; // append()


    /// Append a number.
    void append(int num)
    {
      char buf[1 + 3 * sizeof(int)];
      itoa(num, buf, 10);
      append(buf);
    } // append()


    void append_without_itoa(int num)
    {
      char buf[1 + 3 * sizeof(int)];
      int  n = 3 * sizeof(int);

      buf[n--] = NUL; // terminating the result string

      // allow negative numbers
      if (num < 0) {
        append('-');
        num = - num;
      } // if

      do {
        buf[n--] = '0' + (num % 10);
        num = num / 10;
      } while (num);
      append(&buf[n+1]);
    } // append()



    /// Append a number.
    void append(uint32_t num)
    {
      char buf[1 + 3 * sizeof(uint32_t)];
      ultoa(num, buf, 10);
      append(buf);
    } // append()


    /// Append a string with enclosing quotes at the given buffer.
    void appendQuoted(const char *txt)
    {
      append(QUOTE);
      append(txt);
      append(QUOTE);
    } // appendQuoted()


    /// Append an object with value in the JSON notation to the buffer.
    void appendJSON(const char *name, char *value) {
      appendQuoted(name);
      append(':');
      appendQuoted(value);
    } // appendJSON()


    /// Append an object with value in the JSON notation to the buffer.
    void appendJSON(const char *name, int value) {
      appendQuoted(name);
      append(':');
      append(value);
    } // appendJSON()

  private:
    char* _buf; ///< The allocated buffer
    unsigned int _size; ///< The size of the buffer.
    unsigned int _len; ///< The actual used len of the buffer.

};

