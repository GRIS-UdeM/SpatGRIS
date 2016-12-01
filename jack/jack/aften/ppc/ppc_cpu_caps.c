/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006, David Conrad
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include "cpu_caps.h"

int ppc_cpu_caps_altivec = 0;

// Altivec detection from
// http://developer.apple.com/hardwaredrivers/ve/g3_compatibility.html
#ifdef SYS_DARWIN
#include <sys/sysctl.h>
#else
#include <signal.h>
#include <setjmp.h>

volatile int g_is_altivec_present = -1L;

static sigjmp_buf g_env;

void sig_ill_handler(int sig)
{
    //Set our flag to 0 to indicate AltiVec is illegal
    g_is_altivec_present = 0;

    //long jump back to safety
    siglongjmp(g_env, 0);
}
#endif

#ifdef SYS_DARWIN
void cpu_caps_detect(void)
{
    int sels[2] = { CTL_HW, HW_VECTORUNIT };
    int v_type = 0; //0 == scalar only
    size_t len = sizeof(v_type);
    int err = sysctl(sels, 2, &v_type, &len, NULL, 0);

    ppc_cpu_caps_altivec = err == 0 && v_type != 0;
}
#else
void cpu_caps_detect(void)
{
    if (g_is_altivec_present == -1L)
    {
        sig_t oldhandler;
        sigset_t signame;
        struct sigaction sa_new, sa_old;

        //Set AltiVec to ON
        g_is_altivec_present = 1;

        //Set up the signal mask
        sigemptyset(&signame);
        sigaddset(&signame, SIGILL);

        //Set up the signal handler
        sa_new.sa_handler = sig_ill_handler;
        sa_new.sa_mask = signame;
        sa_new.sa_flags = 0;

        //Install the signal handler
        sigaction(SIGILL, &sa_new, &sa_old);

        //Attempt to use AltiVec
        if(!sigsetjmp(g_env, 0))
        {
            asm volatile ( "vor v0, v0, v0" );
        }

        //Restore the old signal handler
        sigaction(SIGILL, &sa_old, &sa_new);
    }

    ppc_cpu_caps_altivec = g_is_altivec_present;
}
#endif

void apply_simd_restrictions(AftenSimdInstructions *simd_instructions)
{
    ppc_cpu_caps_altivec &= simd_instructions->altivec;
}
