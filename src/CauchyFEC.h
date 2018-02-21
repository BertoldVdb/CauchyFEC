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

#include <vector>
#include <cstdint>
#include "Matrix.h"
#include "GF256Number.h"

#ifndef CAUCHYFEC_H_
#define CAUCHYFEC_H_

using RSGF256Number = GF256Number<>;


class CauchyFEC {
public:
    static void init() {
        RSGF256Number::init();
    }

    CauchyFEC() {
        reset(false, 0);
    }

    inline void reset(bool encode, unsigned int numberOfSourcePackets = 0) {
        isEncoder_ = encode;
        if(isEncoder_) {
            encoderReset(numberOfSourcePackets);
        } else {
            decoderReset();
        }
    }

    inline void operator<<(const std::vector<uint8_t>& sourcePacket) {
        if(isEncoder_) {
            encoderOperatorLL(sourcePacket);
        } else {
            decoderOperatorLL(sourcePacket);
        }
    }

    inline void operator<<(const std::vector<std::vector<uint8_t>>& sourcePackets) {
        if(isEncoder_) {
            encoderOperatorLL(sourcePackets);
        } else {
            decoderOperatorLL(sourcePackets);
        }
    }

    inline unsigned int requestPackets(std::vector<std::vector<uint8_t>>& outputPackets, unsigned int numPackets = 1) {
        if(isEncoder_) {
            return encoderRequestPackets(outputPackets, numPackets);
        } else {
            return decoderRequestPackets(outputPackets, numPackets);
        }
    }

    inline bool operator>>(std::vector<uint8_t>& outputPackets) {
        std::vector<std::vector<uint8_t>> tmp;

        if(requestPackets(tmp, 1)) {
            outputPackets = std::move(tmp[0]);
            return true;
        }

        return false;
    }

    inline bool operator>>(std::vector<std::vector<uint8_t>>& outputPackets) {
        return requestPackets(outputPackets, 1) > 0;
    }

private:

    /* Shared */
    void getGeneratorRow(Matrix<RSGF256Number>& target, unsigned int row, unsigned int sourcePackets);

    unsigned int numSourcePackets_;
    bool isEncoder_;

    /* Encoder part */
    void encoderReset(unsigned int numSourcePackets);
    void encoderOperatorLL(const std::vector<uint8_t>& sourcePacket);
    void encoderOperatorLL(const std::vector<std::vector<uint8_t>>& sourcePackets);
    void encoderBuildMessageMatrix();
    void encoderIncrementGenerator();
    unsigned int encoderRequestPackets(std::vector<std::vector<uint8_t>>& packets, unsigned int numPackets);

    std::vector<std::vector<uint8_t>> encoderSourcePackets_;
    unsigned int encoderLongestSourcePacket_;
    bool encoderReadingSourcePackets_;
    unsigned int encoderGeneratorRowIndex_;
    Matrix<RSGF256Number> encoderMessageMatrix_;

    /* Decoder part */
    void decoderReset();
    void decoderOperatorLL(const std::vector<uint8_t>& inputPacket);
    void decoderOperatorLL(const std::vector<std::vector<uint8_t>>& inputPacket);
    bool decoderMatrixInverse(Matrix<RSGF256Number>& matrix);
    unsigned int decoderRequestPackets(std::vector<std::vector<uint8_t>>& packets, unsigned int numPackets);
    bool decoderRun();

    bool decoderWaitingFirstPacket_;
    bool decoderStuck_;
    unsigned int decoderOriginalPacketsReceived_;
    unsigned int decoderPacketsReturned_;
    std::vector<std::vector<uint8_t>> decoderPacketBuffer_;

};

#endif /* CAUCHYFEC_H_ */
