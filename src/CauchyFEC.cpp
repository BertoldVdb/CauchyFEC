/*
 * Copyright (c) 2018, Bertold Van den Bergh (vandenbergh@bertold.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CauchyFEC.h"
#include "Matrix.h"
#include "GF256Number.h"


void CauchyFEC::getGeneratorRow(Matrix<RSGF256Number>& target, unsigned int row, unsigned int sourcePackets) {
    /* Identity part */
    if(row < sourcePackets) {
        for(unsigned int col = 0; col < sourcePackets; col++) {
            target(0, col) = (row == col)? 1 : 0;
        }
        return;
    }

    /* Line of ones (allows easy XOR decoding) */
    if(row == sourcePackets) {
        for(unsigned int col = 0; col < sourcePackets; col++) {
            target(0, col) = 1;
        }
        return;
    }

    /* Cauchy elements */
    for(unsigned int col = 0; col < sourcePackets; col++) {
        /* row starts at sourcePackets + 1 */
        RSGF256Number x = 255 - row;
        /* 255 - sourcePackets is not used, as this 'slot' was used by the row of ones */
        RSGF256Number y = 255 - sourcePackets + col + 1;

        target(0, col) = RSGF256Number(1)/(x+y);
    }
}