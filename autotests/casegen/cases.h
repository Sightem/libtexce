#ifndef AUTOTEST_CASES_H
#define AUTOTEST_CASES_H

#include <stddef.h>

typedef struct TestCase
{
	const char* name; /* case name */
	const char* prog_name; /* optional 1-8 char program name, auto generated if NULL */
	const char* expr; /* TeX expression */
	const char* expected_crc; /* expected CRC32 (hex, uppercase), NULL to placeholder */
	int x;
	int y;
} TestCase;

typedef struct TestSuite
{
	const char* name;
	const TestCase* cases;
	size_t case_count;
} TestSuite;

extern const TestSuite g_test_suites[];
extern const size_t g_test_suite_count;

#endif
