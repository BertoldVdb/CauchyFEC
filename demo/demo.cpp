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

#include <iostream>
#include <vector>


void makeRandomVector(std::vector<uint8_t>& output, unsigned int length) {
    output.resize(length);
    for(unsigned int i=0; i<length; i++) {
        output[i]=rand();
    }
}

bool testFEC() {
    unsigned int sourcePackets = rand()%256 + 1;
    unsigned int totalPackets = rand()%256 + 1;

    if(totalPackets < sourcePackets) {
        totalPackets = sourcePackets;
    }

    std::cout<<"Source packets: "<<sourcePackets<<" Total: "<<totalPackets<<" ";

    /* Run the fec module as encoder */
    CauchyFEC fec;
    fec.reset(true, sourcePackets);

    /* Generate input */
    std::vector<std::vector<uint8_t>> source(sourcePackets);
    for(unsigned int i=0; i<sourcePackets; i++) {
        makeRandomVector(source[i], rand()%1024+1);
        fec << source[i];
    }

    /* Generate output */
    std::vector<std::vector<uint8_t>> outputPackets;
    for(unsigned int i=0; i<totalPackets; i++) {
        fec >> outputPackets;
    }

    /* Try to decode, select sourcePacket random non-identical packets */
    std::vector<uint8_t> selector(totalPackets);
    for(unsigned int i=0; i<totalPackets; i++) {
        selector[i] = i;
    }

    /* Run the FEC module as decoder */
    fec.reset(false);

    unsigned int sourceRead = 0;
    std::vector<uint8_t> source2;

    /* Check if the packets are valid */
    for(unsigned int i=0; i<sourcePackets; i++) {
        uint8_t selected = rand() % selector.size();
        uint8_t chosenPacket = selector[selected];
        selector.erase(selector.begin() + selected);

        fec << outputPackets[chosenPacket];

        if(fec >> source2) {
            if(source2 != source[sourceRead]) {
                return false;
            }
            sourceRead++;
        }
    }

    for(unsigned int i=sourceRead; i<sourcePackets; i++) {
        std::vector<uint8_t> source2;

        fec >> source2;

        if(source2 != source[i]) {
            return false;
        }
    }


    return true;
}

int main() {
    srand(time(NULL));
    /* Call this before using the module to precalculate some tables */
    CauchyFEC::init();


    for(unsigned int i=0; i<1000000; i++) {
        if(!testFEC()) {
            std::cout<<"Test Failed\n";
            return 1;
        } else {
            std::cout<<"OK\n";
        }

    }

    std::cout<<"Test passed\n";
    return 0;
}

