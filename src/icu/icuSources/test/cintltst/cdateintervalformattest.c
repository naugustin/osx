// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * Copyright (c) 2011-2016, International Business Machines Corporation
 * and others. All Rights Reserved.
 ********************************************************************/
/* C API TEST FOR DATE INTERVAL FORMAT */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/udateintervalformat.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cmemory.h"

static void TestDateIntervalFormat(void);
static void TestFPos_SkelWithSeconds(void);

void addDateIntervalFormatTest(TestNode** root);

#define TESTCASE(x) addTest(root, &x, "tsformat/cdateintervalformattest/" #x)

void addDateIntervalFormatTest(TestNode** root)
{
    TESTCASE(TestDateIntervalFormat);
    TESTCASE(TestFPos_SkelWithSeconds);
}

static const char tzUSPacific[] = "US/Pacific";
static const char tzAsiaTokyo[] = "Asia/Tokyo";
#define Date201103021030 1299090600000.0 /* 2011-Mar-02 1030 in US/Pacific, 2011-Mar-03 0330 in Asia/Tokyo */
#define Date201009270800 1285599629000.0 /* 2010-Sep-27 0800 in US/Pacific */
#define Date201712300900 1514653200000.0 /* 2017-Dec-30 0900 in US/Pacific */
#define _MINUTE (60.0*1000.0)
#define _HOUR   (60.0*60.0*1000.0)
#define _DAY    (24.0*60.0*60.0*1000.0)

typedef struct {
    const char * locale;
    const char * skeleton;
    const char * tzid;
    UDateIntervalFormatAttributeValue minimizeType;
    const UDate  from;
    const UDate  to;
    const char * resultExpected;
} DateIntervalFormatTestItem;

/* Just a small set of tests for now, the real functionality is tested in the C++ tests */
static const DateIntervalFormatTestItem testItems[] = {
    { "en", "MMMdHHmm", tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201103021030, Date201103021030 + 7.0*_HOUR,  "Mar 2, 10:30\\u2009\\u2013\\u200917:30" },
    { "en", "MMMdHHmm", tzAsiaTokyo, UDTITVFMT_MINIMIZE_NONE,            Date201103021030, Date201103021030 + 7.0*_HOUR,  "Mar 3, 03:30\\u2009\\u2013\\u200910:30" },
    { "en", "yMMMEd",   tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 12.0*_HOUR, "Mon, Sep 27, 2010" },
    { "en", "yMMMEd",   tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 31.0*_DAY,  "Mon, Sep 27\\u2009\\u2013\\u2009Thu, Oct 28, 2010" },
    { "en", "yMMMEd",   tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 410.0*_DAY, "Mon, Sep 27, 2010\\u2009\\u2013\\u2009Fri, Nov 11, 2011" },
    { "de", "Hm",       tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 12.0*_HOUR, "08:00\\u201320:00 Uhr" },
    { "de", "Hm",       tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 31.0*_DAY,  "27.9.2010, 08:00\\u2009\\u2013\\u200928.10.2010, 08:00" },
    { "ja", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 1.0*_DAY,   "9\\u670827\\u65E5\\uFF5E28\\u65E5" },
    { "en", "jm",       tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201103021030, Date201103021030 + 1.0*_HOUR,  "10:30 AM\\u2009\\u2013\\u200911:30 AM" },
    { "en", "jm",       tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201103021030, Date201103021030 + 12.0*_HOUR, "10:30 AM\\u2009\\u2013\\u200910:30 PM" },
    { "it", "yMMMMd",   tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201103021030, Date201103021030 + 15.0*_DAY,  "2\\u201317 marzo 2011" },
    { "en_SA", "MMMd",  tzUSPacific, UDTITVFMT_MINIMIZE_NONE,            Date201009270800, Date201009270800 + 6.0*_DAY,   "18\\u2009\\u2013\\u200924 Shaw." },
    { "en@calendar=islamic-umalqura", "MMMd", tzUSPacific, UDTITVFMT_MINIMIZE_NONE, Date201009270800, Date201009270800 + 6.0*_DAY, "Shaw. 18\\u2009\\u2013\\u200924" },
    // Apple-specific
    { "en", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 6.0*_DAY,   "Sep 27\\u2009\\u2013\\u20093" },
    { "en", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 32.0*_DAY,  "Sep 27\\u2009\\u2013\\u2009Oct 29" },
    { "en", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 6.0*_DAY,   "Dec 30\\u2009\\u2013\\u20095" }, // across year boundary
    { "en", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 32.0*_DAY,  "Dec 30, 2017\\u2009\\u2013\\u2009Jan 31, 2018" }, // across year boundary but > 1 month
    { "fr", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 6.0*_DAY,   "27\\u20133 oct." },
    { "fr", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 32.0*_DAY,  "27 sept.\\u2009\\u2013\\u200929 oct." },
    { "fr", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 6.0*_DAY,   "30\\u20135 janv." }, // across year boundary
    { "fr", "MMMd",     tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 32.0*_DAY,  "30 d\\u00E9c. 2017\\u2009\\u2013\\u200931 janv. 2018" }, // across year boundary but > 1 month

    { "en", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 6.0*_DAY,   "Sep 27\\u2009\\u2013\\u20093, 2010" },
    { "en", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 32.0*_DAY,  "Sep 27\\u2009\\u2013\\u2009Oct 29, 2010" },
    { "en", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 6.0*_DAY,   "Dec 30, 2017\\u2009\\u2013\\u2009Jan 5, 2018" }, // across year boundary
    { "en", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 32.0*_DAY,  "Dec 30, 2017\\u2009\\u2013\\u2009Jan 31, 2018" }, // across year boundary but > 1 month
    { "fr", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 6.0*_DAY,   "27\\u20133 oct. 2010" },
    { "fr", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201009270800, Date201009270800 + 32.0*_DAY,  "27 sept.\\u2009\\u2013\\u200929 oct. 2010" },
    { "fr", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 6.0*_DAY,   "30 d\\u00E9c. 2017\\u2009\\u2013\\u20095 janv. 2018" }, // across year boundary
    { "fr", "yMMMd",    tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_MONTHS, Date201712300900, Date201712300900 + 32.0*_DAY,  "30 d\\u00E9c. 2017\\u2009\\u2013\\u200931 janv. 2018" }, // across year boundary but > 1 month

    { "en", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800, Date201009270800 + 10.0*_HOUR, "Sep 27, 8:00 AM\\u2009\\u2013\\u20096:00 PM" },
    { "en", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800, Date201009270800 + 17.0*_HOUR, "Sep 27, 8:00 AM\\u2009\\u2013\\u2009Sep 28, 1:00 AM" },
    { "en", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 17.0*_HOUR, "Sep 27, 8:00 PM\\u2009\\u2013\\u20091:00 AM" },
    { "en", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 26.0*_HOUR, "Sep 27, 8:00 PM\\u2009\\u2013\\u2009Sep 28, 10:00 AM" },
    { "en", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 35.0*_HOUR, "Sep 27, 8:00 PM\\u2009\\u2013\\u2009Sep 28, 7:00 PM" },
    { "fr", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800, Date201009270800 + 10.0*_HOUR, "27 sept. \\u00E0 08:00\\u2009\\u2013\\u200918:00" },
    { "fr", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800, Date201009270800 + 17.0*_HOUR, "27 sept. \\u00E0 08:00\\u2009\\u2013\\u200928 sept. \\u00E0 01:00" },
    { "fr", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 17.0*_HOUR, "27 sept. \\u00E0 20:00\\u2009\\u2013\\u200901:00" },
    { "fr", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 26.0*_HOUR, "27 sept. \\u00E0 20:00\\u2009\\u2013\\u200928 sept. \\u00E0 10:00" },
    { "fr", "MMMdjmm",  tzUSPacific, UDTITVFMT_MINIMIZE_ADJACENT_DAYS,   Date201009270800 + 12.0*_HOUR, Date201009270800 + 35.0*_HOUR, "27 sept. \\u00E0 20:00\\u2009\\u2013\\u200928 sept. \\u00E0 19:00" },

   { NULL, NULL,       NULL,        UDTITVFMT_MINIMIZE_NONE,            0,                0,                             NULL }
};

enum {
    kSkelBufLen = 32,
    kTZIDBufLen = 96,
    kFormatBufLen = 128
};

static void TestDateIntervalFormat()
{
    const DateIntervalFormatTestItem * testItemPtr;
    UErrorCode status = U_ZERO_ERROR;
    ctest_setTimeZone(NULL, &status);
    log_verbose("\nTesting udtitvfmt_open() and udtitvfmt_format() with various parameters\n");
    for ( testItemPtr = testItems; testItemPtr->locale != NULL; ++testItemPtr ) {
        UDateIntervalFormat* udtitvfmt;
        int32_t tzidLen;
        UChar skelBuf[kSkelBufLen];
        UChar tzidBuf[kTZIDBufLen];
        const char * tzidForLog = (testItemPtr->tzid)? testItemPtr->tzid: "NULL";

        status = U_ZERO_ERROR;
        u_unescape(testItemPtr->skeleton, skelBuf, kSkelBufLen);
        if ( testItemPtr->tzid ) {
            u_unescape(testItemPtr->tzid, tzidBuf, kTZIDBufLen);
            tzidLen = -1;
        } else {
            tzidLen = 0;
        }
        udtitvfmt = udtitvfmt_open(testItemPtr->locale, skelBuf, -1, tzidBuf, tzidLen, &status);
        if ( U_SUCCESS(status) ) {
            UChar result[kFormatBufLen];
            UChar resultExpected[kFormatBufLen];
            udtitvfmt_setAttribute(udtitvfmt, UDTITVFMT_MINIMIZE_TYPE, testItemPtr->minimizeType, &status);
            if ( U_FAILURE(status) ) {
                log_err("FAIL: udtitvfmt_setAttribute for locale %s, skeleton %s, tzid %s, minimizeType %d: %s\n",
                        testItemPtr->locale, testItemPtr->skeleton, tzidForLog, (int)testItemPtr->minimizeType, myErrorName(status) );
            }
            int32_t fmtLen = udtitvfmt_format(udtitvfmt, testItemPtr->from, testItemPtr->to, result, kFormatBufLen, NULL, &status);
            if (fmtLen >= kFormatBufLen) {
                result[kFormatBufLen-1] = 0;
            }
            if ( U_SUCCESS(status) ) {
                u_unescape(testItemPtr->resultExpected, resultExpected, kFormatBufLen);
                if ( u_strcmp(result, resultExpected) != 0 ) {
                    char bcharBuf[kFormatBufLen];
#if 0
                    log_err("ERROR: udtitvfmt_format for locale %s, skeleton %s, tzid %s, minimizeType %d, from %.1f, to %.1f: expect %s, get %s\n",
                             testItemPtr->locale, testItemPtr->skeleton, tzidForLog, (int)testItemPtr->minimizeType,
                             testItemPtr->from, testItemPtr->to, testItemPtr->resultExpected, u_austrcpy(bcharBuf,result) );
#else
                    // Apple-specific version
                    char bexpbuf[kFormatBufLen];
                    u_strToUTF8(bexpbuf, kFormatBufLen, NULL, resultExpected, -1, &status);
                    u_strToUTF8(bcharBuf, kFormatBufLen, NULL, result, fmtLen, &status);
                    log_err("ERROR: udtitvfmt_format for locale %s, skeleton %s, tzid %s, minimizeType %d, from %.1f, to %.1f: expect %s, get %s\n",
                             testItemPtr->locale, testItemPtr->skeleton, tzidForLog, (int)testItemPtr->minimizeType,
                             testItemPtr->from, testItemPtr->to, bexpbuf, bcharBuf );
#endif
                }
            } else {
                log_err("FAIL: udtitvfmt_format for locale %s, skeleton %s, tzid %s, minimizeType %d, from %.1f, to %.1f: %s\n",
                        testItemPtr->locale, testItemPtr->skeleton, tzidForLog, (int)testItemPtr->minimizeType,
                        testItemPtr->from, testItemPtr->to, myErrorName(status) );
            }
            udtitvfmt_close(udtitvfmt);
        } else {
            log_data_err("FAIL: udtitvfmt_open for locale %s, skeleton %s, tzid %s - %s\n",
                    testItemPtr->locale, testItemPtr->skeleton, tzidForLog, myErrorName(status) );
        }
    }
    ctest_resetTimeZone();
}

/********************************************************************
 * TestFPos_SkelWithSeconds and related data
 ********************************************************************
 */

static UChar zoneGMT[] = { 0x47,0x4D,0x54,0 }; // GMT
static const UDate startTime = 1416474000000.0; // 2014 Nov 20 09:00 GMT

static const double deltas[] = {
	0.0, // none
	200.0, // 200 millisec
	20000.0, // 20 sec
	1200000.0, // 20 min
	7200000.0, // 2 hrs
	43200000.0, // 12 hrs
	691200000.0, // 8 days
	1382400000.0, // 16 days,
	8640000000.0, // 100 days
	-1.0
};
enum { kNumDeltas = UPRV_LENGTHOF(deltas) - 1 };

typedef struct {
    int32_t posBegin;
    int32_t posEnd;
    const char * format;
} ExpectPosAndFormat;

static const ExpectPosAndFormat exp_en_HHmm[kNumDeltas] = {
    {  3,  5, "09:00" },
    {  3,  5, "09:00" },
    {  3,  5, "09:00" },
    {  3,  5, "09:00\\u2009\\u2013\\u200909:20" },
    {  3,  5, "09:00\\u2009\\u2013\\u200911:00" },
    {  3,  5, "09:00\\u2009\\u2013\\u200921:00" },
    { 15, 17, "11/20/2014, 09:00\\u2009\\u2013\\u200911/28/2014, 09:00" },
    { 15, 17, "11/20/2014, 09:00\\u2009\\u2013\\u200912/6/2014, 09:00" },
    { 15, 17, "11/20/2014, 09:00\\u2009\\u2013\\u20092/28/2015, 09:00" }
};

static const ExpectPosAndFormat exp_en_HHmmss[kNumDeltas] = {
    {  3,  5, "09:00:00" },
    {  3,  5, "09:00:00" },
    {  3,  5, "09:00:00\\u2009\\u2013\\u200909:00:20" },
    {  3,  5, "09:00:00\\u2009\\u2013\\u200909:20:00" },
    {  3,  5, "09:00:00\\u2009\\u2013\\u200911:00:00" },
    {  3,  5, "09:00:00\\u2009\\u2013\\u200921:00:00" },
    { 15, 17, "11/20/2014, 09:00:00\\u2009\\u2013\\u200911/28/2014, 09:00:00" },
    { 15, 17, "11/20/2014, 09:00:00\\u2009\\u2013\\u200912/6/2014, 09:00:00" },
    { 15, 17, "11/20/2014, 09:00:00\\u2009\\u2013\\u20092/28/2015, 09:00:00" }
};

static const ExpectPosAndFormat exp_en_yyMMdd[kNumDeltas] = {
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14" },
    {  0,  0, "11/20/14\\u2009\\u2013\\u200911/28/14" },
    {  0,  0, "11/20/14\\u2009\\u2013\\u200912/6/14" },
    {  0,  0, "11/20/14\\u2009\\u2013\\u20092/28/15" }
};

static const ExpectPosAndFormat exp_en_yyMMddHHmm[kNumDeltas] = {
    { 13, 15, "11/20/14, 09:00" },
    { 13, 15, "11/20/14, 09:00" },
    { 13, 15, "11/20/14, 09:00" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200909:20" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200911:00" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200921:00" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200911/28/14, 09:00" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200912/06/14, 09:00" },
    { 13, 15, "11/20/14, 09:00\\u2009\\u2013\\u200902/28/15, 09:00" }
};

static const ExpectPosAndFormat exp_en_yyMMddHHmmss[kNumDeltas] = {
    { 13, 15, "11/20/14, 09:00:00" },
    { 13, 15, "11/20/14, 09:00:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200909:00:20" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200909:20:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200911:00:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200921:00:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200911/28/14, 09:00:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200912/06/14, 09:00:00" },
    { 13, 15, "11/20/14, 09:00:00\\u2009\\u2013\\u200902/28/15, 09:00:00" }
};

static const ExpectPosAndFormat exp_en_yMMMdhmmssz[kNumDeltas] = {
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u20099:00:20 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u20099:20:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u200911:00:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u20099:00:00 PM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u2009Nov 28, 2014, 9:00:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u2009Dec 6, 2014, 9:00:00 AM GMT" },
    { 16, 18, "Nov 20, 2014, 9:00:00 AM GMT\\u2009\\u2013\\u2009Feb 28, 2015, 9:00:00 AM GMT" }
};

static const ExpectPosAndFormat exp_ja_yyMMddHHmm[kNumDeltas] = {
    { 11, 13, "14/11/20 9:00" },
    { 11, 13, "14/11/20 9:00" },
    { 11, 13, "14/11/20 9:00" },
    { 11, 13, "14/11/20 9\\u664200\\u5206\\uFF5E9\\u664220\\u5206" },
    { 11, 13, "14/11/20 9\\u664200\\u5206\\uFF5E11\\u664200\\u5206" },
    { 11, 13, "14/11/20 9\\u664200\\u5206\\uFF5E21\\u664200\\u5206" },
    { 11, 13, "14/11/20 9:00\\uFF5E14/11/28 9:00" },
    { 11, 13, "14/11/20 9:00\\uFF5E14/12/06 9:00" },
    { 11, 13, "14/11/20 9:00\\uFF5E15/02/28 9:00" }
};

static const ExpectPosAndFormat exp_ja_yyMMddHHmmss[kNumDeltas] = {
    { 11, 13, "14/11/20 9:00:00" },
    { 11, 13, "14/11/20 9:00:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E9:00:20" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E9:20:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E11:00:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E21:00:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E14/11/28 9:00:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E14/12/06 9:00:00" },
    { 11, 13, "14/11/20 9:00:00\\uFF5E15/02/28 9:00:00" }
};

static const ExpectPosAndFormat exp_ja_yMMMdHHmmss[kNumDeltas] = {
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E9:00:20" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E9:20:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E11:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E21:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E2014\\u5E7411\\u670828\\u65E5 9:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E2014\\u5E7412\\u67086\\u65E5 9:00:00" },
    { 14, 16, "2014\\u5E7411\\u670820\\u65E5 9:00:00\\uFF5E2015\\u5E742\\u670828\\u65E5 9:00:00" }
};

typedef struct {
	const char * locale;
	const char * skeleton;
	UDateFormatField fieldToCheck;
	const ExpectPosAndFormat * expected;
} LocaleAndSkeletonItem;

static const LocaleAndSkeletonItem locSkelItems[] = {
	{ "en",		"HHmm",         UDAT_MINUTE_FIELD, exp_en_HHmm },
	{ "en",		"HHmmss",       UDAT_MINUTE_FIELD, exp_en_HHmmss },
	{ "en",		"yyMMdd",       UDAT_MINUTE_FIELD, exp_en_yyMMdd },
	{ "en",		"yyMMddHHmm",   UDAT_MINUTE_FIELD, exp_en_yyMMddHHmm },
	{ "en",		"yyMMddHHmmss", UDAT_MINUTE_FIELD, exp_en_yyMMddHHmmss },
	{ "en",		"yMMMdhmmssz",  UDAT_MINUTE_FIELD, exp_en_yMMMdhmmssz },
	{ "ja",		"yyMMddHHmm",   UDAT_MINUTE_FIELD, exp_ja_yyMMddHHmm },
	{ "ja",		"yyMMddHHmmss", UDAT_MINUTE_FIELD, exp_ja_yyMMddHHmmss },
	{ "ja",		"yMMMdHHmmss",  UDAT_MINUTE_FIELD, exp_ja_yMMMdHHmmss },
	{ NULL, NULL, (UDateFormatField)0, NULL }
};

enum { kSizeUBuf = 96, kSizeBBuf = 192 };

static void TestFPos_SkelWithSeconds()
{
	const LocaleAndSkeletonItem * locSkelItemPtr;
	for (locSkelItemPtr = locSkelItems; locSkelItemPtr->locale != NULL; locSkelItemPtr++) {
	    UDateIntervalFormat* udifmt;
	    UChar   ubuf[kSizeUBuf];
	    int32_t ulen, uelen;
	    UErrorCode status = U_ZERO_ERROR;
            ulen = u_unescape(locSkelItemPtr->skeleton, ubuf, kSizeUBuf);
	    udifmt = udtitvfmt_open(locSkelItemPtr->locale, ubuf, ulen, zoneGMT, -1, &status);
	    if ( U_FAILURE(status) ) {
           log_data_err("FAIL: udtitvfmt_open for locale %s, skeleton %s: %s\n",
                    locSkelItemPtr->locale, locSkelItemPtr->skeleton, u_errorName(status));
	    } else {
			const double * deltasPtr = deltas;
			const ExpectPosAndFormat * expectedPtr = locSkelItemPtr->expected;
			for (; *deltasPtr >= 0.0; deltasPtr++, expectedPtr++) {
			    UFieldPosition fpos = { locSkelItemPtr->fieldToCheck, 0, 0 };
			    UChar uebuf[kSizeUBuf];
			    char bbuf[kSizeBBuf];
			    char bebuf[kSizeBBuf];
			    status = U_ZERO_ERROR;
			    uelen = u_unescape(expectedPtr->format, uebuf, kSizeUBuf);
			    ulen = udtitvfmt_format(udifmt, startTime, startTime + *deltasPtr, ubuf, kSizeUBuf, &fpos, &status);
			    if ( U_FAILURE(status) ) {
			        log_err("FAIL: udtitvfmt_format for locale %s, skeleton %s, delta %.1f: %s\n",
			             locSkelItemPtr->locale, locSkelItemPtr->skeleton, *deltasPtr, u_errorName(status));
			    } else if ( ulen != uelen || u_strncmp(ubuf,uebuf,uelen) != 0 ||
			                fpos.beginIndex != expectedPtr->posBegin || fpos.endIndex != expectedPtr->posEnd ) {
			        u_strToUTF8(bbuf, kSizeBBuf, NULL, ubuf, ulen, &status);
			        u_strToUTF8(bebuf, kSizeBBuf, NULL, uebuf, uelen, &status); // convert back to get unescaped string
			        log_err("FAIL: udtitvfmt_format for locale %s, skeleton %s, delta %12.1f, expect %d-%d \"%s\", get %d-%d \"%s\"\n",
			             locSkelItemPtr->locale, locSkelItemPtr->skeleton, *deltasPtr,
			             expectedPtr->posBegin, expectedPtr->posEnd, bebuf,
			             fpos.beginIndex, fpos.endIndex, bbuf);
			    }
			}
	        udtitvfmt_close(udifmt);
	    }
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
