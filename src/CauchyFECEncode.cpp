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

#include "CauchyFECImpl.h"
#include "Matrix.h"
#include "GF256Number.h"
#include <stdexcept>

void CauchyFEC::impl::encoderReset(unsigned int numSourcePackets) {
    encoderSourcePackets_.clear();
    encoderGeneratorRowIndex_ = 0;
    encoderReadingSourcePackets_ = true;
    numSourcePackets_ = numSourcePackets;
    encoderLongestSourcePacket_ = 0;

    if(!numSourcePackets_) {
        throw std::runtime_error("At least one source packet is needed");
    }
}

void CauchyFEC::impl::encoderOperatorLL(const std::vector<uint8_t>& sourcePacket) {
    if(!sourcePacket.size()) {
        throw std::runtime_error("size() == 0 packets are not supported");
    }

    if(!encoderReadingSourcePackets_) {
        throw std::runtime_error("Reset required");
    }

    if(encoderSourcePackets_.size() >= numSourcePackets_) {
        throw std::runtime_error("Encoder is full");
    }

    if(sourcePacket.size() > encoderLongestSourcePacket_) {
        encoderLongestSourcePacket_ = sourcePacket.size();
    }

    encoderSourcePackets_.push_back(sourcePacket);
}

void CauchyFEC::impl::encoderOperatorLL(const std::vector<std::vector<uint8_t>>& sourcePackets) {
    encoderSourcePackets_.reserve(encoderSourcePackets_.size() + sourcePackets.size());
    for(auto& i: sourcePackets) {
        encoderOperatorLL(i);
    }
}

void CauchyFEC::impl::encoderBuildMessageMatrix() {
    unsigned int index = 0;
    encoderMessageMatrix_ = Matrix<RSGF256Number>(numSourcePackets_, encoderLongestSourcePacket_ + 2);

    for(auto& sourcePacket: encoderSourcePackets_) {
        for(unsigned int i=0; i<sourcePacket.size(); i++) {
            encoderMessageMatrix_(index, i) = sourcePacket[i];
        }

        /* Append with original length */
        encoderMessageMatrix_(index, encoderLongestSourcePacket_) = sourcePacket.size() >> 8;
        encoderMessageMatrix_(index, encoderLongestSourcePacket_ + 1) = sourcePacket.size() & 0xFF;

        index ++;
    }
}

void CauchyFEC::impl::encoderIncrementGenerator() {
    encoderGeneratorRowIndex_++;
    if(encoderGeneratorRowIndex_ > 256) {
        throw std::runtime_error("Can't generate more packets");
    }
}

unsigned int CauchyFEC::impl::encoderRequestPackets(std::vector<std::vector<uint8_t>>& packets, unsigned int numPackets) {
    /* First packets are not encoded */
    unsigned int count = 0;
    for(count = 0; count < numPackets; count++) {
        if(encoderGeneratorRowIndex_ < numSourcePackets_) {

            /* Are enough source packets loaded? */
            if(encoderGeneratorRowIndex_ >= encoderSourcePackets_.size()) {
                return count;
            }

            std::vector<uint8_t>& sourcePacket = encoderSourcePackets_[encoderGeneratorRowIndex_];
            std::vector<uint8_t> outputPacket;
            outputPacket.reserve(2 + sourcePacket.size());

            outputPacket.insert (outputPacket.begin(),
                                 sourcePacket.begin(),sourcePacket.end());

            outputPacket.push_back(encoderGeneratorRowIndex_);
            outputPacket.push_back(numSourcePackets_ - 1);

            packets.push_back(std::move(outputPacket));
            encoderIncrementGenerator();
        } else {
            break;
        }
    }

    if(count == numPackets) {
        return numPackets;
    }

    unsigned int numToGenerate = numPackets - count;

    /* Once we build the message matrix no more source packets can be read */
    if(encoderReadingSourcePackets_) {
        encoderBuildMessageMatrix();
        encoderReadingSourcePackets_ = false;
    }

    unsigned int generatorRowIndexAtStart = encoderGeneratorRowIndex_;

    Matrix<RSGF256Number> generatorSubMatrix(numToGenerate, numSourcePackets_);
    for(unsigned int i=0; i<numToGenerate; i++) {
        Matrix<RSGF256Number> generatorRow = generatorSubMatrix[i];
        getGeneratorRow(generatorRow, encoderGeneratorRowIndex_, numSourcePackets_);
        encoderIncrementGenerator();
    }

    auto encodedMessage = generatorSubMatrix * encoderMessageMatrix_;

    for(unsigned int i=0; i<encodedMessage.rows(); i++) {
        std::vector<uint8_t> parityPacket;
        parityPacket.resize(2 + encodedMessage.columns());

        for(unsigned int j=0; j<encodedMessage.columns(); j++) {
            parityPacket[j] = encodedMessage(i, j);
        }

        parityPacket[encodedMessage.columns()] = generatorRowIndexAtStart + i;
        parityPacket[encodedMessage.columns() + 1] = numSourcePackets_ - 1;

        packets.push_back(std::move(parityPacket));
    }

    return numPackets;

}
