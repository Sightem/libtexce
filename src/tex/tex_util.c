#include "tex_util.h"

int tex_util_is_escape_char(char c) { return (c == '\\' || c == '$' || c == '{' || c == '}'); }

// copy with unescape into dest (caller ensures dest has space for unescaped_len + 1)
void tex_util_copy_unescaped(char* dst, const char* s, int raw_len)
{
	int di = 0;
	for (int i = 0; i < raw_len;)
	{
		if (s[i] == '\\' && i + 1 < raw_len && tex_util_is_escape_char(s[i + 1]))
		{
			dst[di++] = s[i + 1];
			i += 2;
		}
		else
		{
			dst[di++] = s[i++];
		}
	}
	dst[di] = '\0';
}

// compute de-escaped length of a segment
int tex_util_unescaped_len(const char* s, int raw_len)
{
	int out = 0;
	for (int i = 0; i < raw_len;)
	{
		if (s[i] == '\\' && i + 1 < raw_len && tex_util_is_escape_char(s[i + 1]))
		{
			++out;
			i += 2;
		}
		else
		{
			++out;
			++i;
		}
	}
	return out;
}
