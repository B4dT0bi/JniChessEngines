/*
    Protector -- a UCI chess engine

    Copyright (C) 2009-2010 Raimund Heid (Raimund_Heid@yahoo.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include "tools.h"

unsigned long getTimestamp()
{
   struct timeb t_current;

   ftime(&t_current);

   return 1000 * (long) t_current.time + (long) t_current.millitm;
}

long getProcessTimestamp()
{
   clock_t ts = clock();

   while (ts < 0)
   {
      ts = clock();
   }

   return ts / (CLOCKS_PER_SEC / 1000);
}

String getEmptyString()
{
   String s;

   s.bufferSize = 256;
   s.buffer = s.tail = (char *) malloc(s.bufferSize);
   *s.tail = '\0';

   return s;
}

String getString(const char *buffer, const char *lastChar)
{
   String s;

   s.bufferSize = (unsigned int) (lastChar - buffer + 2);
   s.buffer = (char *) malloc(s.bufferSize);
   s.tail = s.buffer + s.bufferSize - 1;
   strncpy(s.buffer, buffer, s.bufferSize - 1);
   *s.tail = '\0';

   return s;
}

static String *appendBufferToString(String * string, const char *buffer)
{
   size_t appendedLength = strlen(buffer);
   size_t newLength = string->tail - string->buffer + appendedLength + 1;

   if (newLength > string->bufferSize)
   {
      size_t delta = string->tail - string->buffer;

      string->bufferSize = (unsigned int) (2 * newLength);
      string->buffer = (char *) realloc(string->buffer, string->bufferSize);
      string->tail = string->buffer + delta;
   }

   strcat(string->tail, buffer);
   string->tail += appendedLength;

   return string;
}

#define BUFFER_SIZE 8192

String *appendToString(String * string, const char *fmt, ...)
{
   va_list args;

   /* int numBytesWritten; */
   char buffer[BUFFER_SIZE];

   va_start(args, fmt);
   /*numBytesWritten = */
   vsprintf(buffer, fmt, args);
   va_end(args);

   /* assert(numBytesWritten < BUFFER_SIZE); */

   return (appendBufferToString(string, buffer));
}

void breakLines(char *buffer, unsigned int maxLineLength)
{
   char *lastChar;

   while (strlen(buffer) > maxLineLength)
   {
      lastChar = buffer + maxLineLength;

      while (isspace((int) *lastChar) == 0 && lastChar > buffer)
      {
         lastChar--;
      }

      if (isspace((int) *lastChar))
      {
         *lastChar = '\n';
         buffer = lastChar + 1;
      }
      else
      {
         while (*buffer != '\0')
         {
            if (isspace((int) *buffer))
            {
               *buffer++ = '\n';

               break;
            }
            else
            {
               buffer++;
            }
         }
      }
   }
}

void trim(char *buffer)
{
   char *p = buffer;
   size_t length = strlen(buffer);

   if (length == 0)
   {
      return;
   }

   while (isspace((int) *p))
   {
      p++;
   }

   if (p > buffer)
   {
      length -= p - buffer;
      memmove(buffer, p, length + 1);
   }

   p = buffer + length - 1;

   while (p >= buffer && isspace((int) *p))
   {
      *(p--) = '\0';
   }
}

char *getToken(const char *token, const char *tokenDelimiters)
{
   unsigned int i = 0;
   char *buffer;

   while (token[i] != '\0' && strchr(tokenDelimiters, token[i]) == NULL)
   {
      i++;
   }

   buffer = (char *) malloc(i + 1);
   strncpy(buffer, token, i);
   buffer[i] = '\0';

   assert(strlen(buffer) == i);

   return buffer;
}

int isPrime(unsigned long candidate)
{
   long limit, i;

   if (candidate == 2 || candidate == 3)
   {
      return 1;
   }

   if (candidate < 2 || candidate % 2 == 0)
   {
      return 0;
   }

   limit = (unsigned long) sqrt((double) candidate) + 1;

   for (i = 3; i <= limit; i += 2)
   {
      if (candidate % i == 0)
      {
         return 0;
      }
   }

   return 1;
}

int logIntValue(const double zeroPoint, const int maxPoint,
                const double maxValue, const int x)
{
   const double xScaleFactor = 1.0 / (zeroPoint);
   const double yScaleFactor = maxValue /
      log(((double) maxPoint) * xScaleFactor);
   const double y = log(((double) x) * xScaleFactor) * yScaleFactor;

   return (int) (floor(y + 0.5));
}

int applyWeight(double value, double weight)
{
   double weightedValue = (value * weight) / 256.0;

   return (int) (floor(weightedValue + 0.5));
}

int getLimitedValue(const int minValue, const int maxValue, const int value)
{
   return min(maxValue, max(minValue, value));
}

unsigned long long getUnsignedLongLongFromHexString(const char *str)
{
   unsigned long long number = strtoull(str, NULL, 16);

   return number;
}

void getHexStringFromUnsignedLongLong(char *buffer, unsigned long long value)
{
   char *fmt = "%I64x";

   sprintf(buffer, fmt, value);
}

int initializeModuleTools()
{
   return 0;
}

static int testStringOperations()
{
   const char *testString = "Pascal";
   String string = getEmptyString();
   String string2;

   appendToString(&string, "test");
   assert(strcmp(string.buffer, "test") == 0);
   assert(string.tail - string.buffer == 4);

   appendToString(&string, " %d", 123);
   assert(strcmp(string.buffer, "test 123") == 0);
   assert(string.tail - string.buffer == 8);

   string2 = getString(testString, strchr(testString, 'c'));
   assert(strcmp(string2.buffer, "Pasc") == 0);
   assert(string2.bufferSize == 5);

   return (string2.bufferSize == 5 ? 0 : -1);
}

static int testLineBreaking()
{
   char ts1[] = "abcd efgh", ts2[] = "abcdefgh";
   char buffer[1024];

   strcpy(buffer, ts1);
   breakLines(buffer, 3);
   assert(strcmp(buffer, "abcd\nefgh") == 0);

   strcpy(buffer, ts1);
   breakLines(buffer, 4);
   assert(strcmp(buffer, "abcd\nefgh") == 0);

   strcpy(buffer, ts1);
   breakLines(buffer, 5);
   assert(strcmp(buffer, "abcd\nefgh") == 0);

   strcpy(buffer, ts2);
   breakLines(buffer, 3);
   assert(strcmp(buffer, "abcdefgh") == 0);

   return 0;
}

static int testTrimming()
{
   char buffer1[] = "   abcd efgh\n  ", buffer2[] = " ";

   trim(buffer1);
   assert(strcmp(buffer1, "abcd efgh") == 0);

   trim(buffer2);
   assert(strcmp(buffer2, "") == 0);

   return 0;
}

static int testTokenizer()
{
   char *result;

   result = getToken("123456", "6");
   assert(strcmp(result, "12345") == 0);
   free(result);

   result = getToken("123456", " ");
   assert(strcmp(result, "123456") == 0);
   free(result);

   return 0;
}

static int testPrimechecker()
{
   assert(isPrime(159) == 0);
   assert(isPrime(221) == 0);
   assert(isPrime(3337) == 0);

   assert(isPrime(101) == 1);
   assert(isPrime(257) == 1);
   assert(isPrime(997) == 1);

   return 0;
}

static int testMiscFunctions()
{
   unsigned long long testValue = 18446744073709551564llu;
   char buffer[256];

   assert(getLimitedValue(100, 200, 50) == 100);
   assert(getLimitedValue(100, 200, 150) == 150);
   assert(getLimitedValue(100, 200, 250) == 200);

   assert(getUnsignedLongLongFromHexString("ffffffffffffffce") ==
          18446744073709551566llu);
   assert(getUnsignedLongLongFromHexString("FFFFFFFFFFFFFFCD") ==
          18446744073709551565llu);
   getHexStringFromUnsignedLongLong(buffer, testValue);
   assert(getUnsignedLongLongFromHexString(buffer) == testValue);

   return 0;
}

int testModuleTools()
{
   int result;

   if ((result = testStringOperations()) != 0)
   {
      return result;
   }

   if ((result = testLineBreaking()) != 0)
   {
      return result;
   }

   if ((result = testTrimming()) != 0)
   {
      return result;
   }

   if ((result = testTokenizer()) != 0)
   {
      return result;
   }

   if ((result = testPrimechecker()) != 0)
   {
      return result;
   }

   if ((result = testMiscFunctions()) != 0)
   {
      return result;
   }

   return 0;
}
