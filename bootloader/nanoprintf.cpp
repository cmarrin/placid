/*-------------------------------------------------------------------------
This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "Print.h"

using namespace placid;

static int strHex(Print::Printer& printer, uint32_t ad, int len, bool fill)
{
    int i, j=0;
    int c;
    char s[Print::MaxStringSize];
    int st;

    st = 0;
    for(i=0; i<8; i++) {
        if ((c = ((ad>>28) & 0x0F))>0) {
            c = (c<10) ? (c+'0') : (c+'a'-10);
            s[j++] = (char) c;
            st = 1;
        } else if (st || i==7) {
            s[j++] = '0';
        }
        ad = ad << 4;
    }
    s[j] = 0;
    for (i = 0; i < len - j; i++) {
        printer.outChar((fill) ? '0' : ' ');
    }
    
    for (char* p = s; *p != '\0'; ++p) {
        printer.outChar(*p);
    }
    
    return j+i;
}

static int strNum(Print::Printer& printer, uint32_t ui, int len, int fill)
{
    unsigned int cmp = 1;
    int i, j;
    int d;
    int l;
    char c;
    char s[Print::MaxStringSize];

    cmp=1;
    l=1;
    //    printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
    while(cmp*10<=ui && l<10) {
        //2^32 = 4.29 * 10^9 -> Max digits =10 for 32 bit
        cmp *=10;
        l++;
        //        printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
    }

    j = 0;
    //    printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
    while(j<l) {
        //        d = u32_div(ui, cmp);
        d = ui/cmp;
        c = (char) (d+'0');
        s[j++] = c;
        ui = ui - d*cmp;
        //cmp = u32_div(cmp, 10);
        cmp = cmp/10;
        //        Printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
    }
    s[j]=0;

    for (i = 0; i < len - l; i++) {
        printer.outChar((fill) ? '0' : ' ');
    }
    
    for (char* p = s; *p != '\0'; ++p) {
        printer.outChar(*p);
    }
    
    return j+i;
}

int32_t placid::Print::vsnprintCore(Printer& printer, const char *format, va_list va)
{
    int si;
    unsigned int ui;
    char *s;
    
    int32_t i = 0;
    int32_t pf = 0;
    int32_t len = 0;
    bool fill = false;
    
    char c;
    while((c = *format++) != 0) {
        if (pf==0) {
            // after regular character
            if (c=='%') {
                pf=1;
                len = 0;
                fill = 0;
            } else {
                printer.outChar(c);
            }
        } else if (pf>0) {
            // previous character was '%'
            if (c=='x' || c=='X') {
                ui = va_arg(va, unsigned);
                i += strHex(printer, (unsigned) ui, len, fill);
                pf=0;
            } else if (c=='u' || c=='U' || c=='i' || c=='I') {
                ui = va_arg(va, unsigned);
                i += strNum(printer, (unsigned) ui, len, fill);
                pf=0;
            } else if (c=='d' || c=='D') {
                si = va_arg(va, int);
                if (si<0) {
                    ui = -si;
                    printer.outChar('-');
                } else {
                    ui = si;
                }
                i += strNum(printer, (unsigned) ui, len, fill);
                pf=0;
            } else if (c=='s' || c=='S') {
                s = va_arg(va, char *);
                for (char* p = s; *p; ++p) {
                    printer.outChar(*p);
                }
                pf=0;
            } else if ('0'<=c && c<='9') {
                if (pf==1 && c=='0') {
                    fill = 1;
                } else {
                    len = len*10+(c-'0');
                }
                pf=2;
            } else {
                // this shouldn't happen
                printer.outChar(c);
                pf=0;
            }
        }
    }
    printer.outChar('\0');
    return (i>0) ? i : -1;
}
